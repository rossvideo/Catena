/*
 * Copyright 2026 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Unit tests for auth.go
 * @file auth_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-22
 */

package catena

import (
	"context"
	"errors"
	"fmt"
	"net"
	"net/http"
	"net/http/httptest"
	"strconv"
	"strings"
	"sync/atomic"
	"testing"
	"time"

	"github.com/cenkalti/backoff/v5"
	"github.com/golang-jwt/jwt/v5"

	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestNewJwtValidator(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		var server *httptest.Server
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			switch r.URL.Path {
			case "/.well-known/openid-configuration":
				w.Header().Set("Content-Type", "application/json")
				_, _ = fmt.Fprintf(w, `{"jwks_uri":"%s/keys"}`, server.URL)
			case "/keys":
				w.Header().Set("Content-Type", "application/json")
				_, _ = fmt.Fprintf(w, `{"keys": []}`)
			default:
				t.Fatalf("unexpected path: %s", r.URL.Path)
			}
		}))
		defer server.Close()

		validator, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 0, //single attempt
		})
		if err != nil {
			t.Fatalf("newJwtValidator() error = %v", err)
		}
		if validator == nil {
			t.Fatal("newJwtValidator() got nil, want non-nil")
		}
	})

	t.Run("empty http", func(t *testing.T) {
		validator, err := newJwtValidator(t.Context(), JwtValidationOptions{
			ValidateSignature:          false,
			StartupRetryMaxElapsedTime: 0, //single attempt
		})
		if err != nil {
			t.Fatalf("newJwtValidator() error = %v", err)
		}
		if validator == nil {
			t.Fatal("newJwtValidator() got nil, want non-nil")
		}
		validatorTyped, ok := validator.(*jwtValidator)
		if !ok {
			t.Fatalf("newJwtValidator() got type %T, want *jwtValidator", validator)
		}
		if validatorTyped.options.Http != http.DefaultClient {
			t.Fatalf("newJwtValidator() got Http = %v, want %v", validatorTyped.options.Http, http.DefaultClient)
		}
	})

	t.Run("discoverJWKSEndpoint error", func(t *testing.T) {
		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     "http://[::1",
			ValidateSignature:          true,
			Http:                       http.DefaultClient,
			StartupRetryMaxElapsedTime: 0, //single attempt
		})
		if err == nil || !strings.Contains(err.Error(), "discover jwks endpoint") {
			t.Fatalf("newJwtValidator() error = %v, want discover jwks endpoint", err)
		}
	})

	t.Run("create keyfunc error", func(t *testing.T) {
		var server *httptest.Server
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path != "/.well-known/openid-configuration" {
				t.Fatalf("unexpected path: %s", r.URL.Path)
				return
			}
			w.Header().Set("Content-Type", "application/json")
			// return a bad url so keyfunc creation fails
			_, _ = fmt.Fprintf(w, `{"jwks_uri":"http://[::1/keys"}`)
		}))
		defer server.Close()
		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 0, //single attempt
		})
		if err == nil || !strings.Contains(err.Error(), "create keyfunc") {
			t.Fatalf("newJwtValidator() error = %v, want create keyfunc", err)
		}
	})
}

func TestNewJwtValidatorRetry_TransientFailures_ErrorStatusCodes(t *testing.T) {

	newTestServer := func(statusCode int, maxFailures int32, discoveryAttempts *atomic.Int32) *httptest.Server {
		t.Helper()

		var server *httptest.Server

		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/.well-known/openid-configuration" {
				attempt := discoveryAttempts.Add(1)

				if attempt <= maxFailures {
					http.Error(w, "Error code", statusCode)
					return
				}

				_, _ = fmt.Fprintf(w, `{"jwks_uri":"%s/keys"}`, server.URL)
				return
			}
			if r.URL.Path == "/keys" {
				_, _ = fmt.Fprint(w, `{"keys":[]}`)
				return
			}
			http.NotFound(w, r)
		}))

		t.Cleanup(server.Close)

		return server
	}
	t.Run("Status 101 Error", func(t *testing.T) {
		var discoveryAttempts atomic.Int32
		var maxFailures int32 = 3

		server := newTestServer(http.StatusSwitchingProtocols, maxFailures, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err != nil {
			t.Fatalf("unexpected newJwtValidator() error: %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != maxFailures+1 {
			t.Fatalf("discovery attempts = %d, want 3", numberOfAttempts)
		}
	})

	t.Run("Status 300 Error", func(t *testing.T) {
		var discoveryAttempts atomic.Int32
		var maxFailures int32 = 3

		server := newTestServer(http.StatusMultipleChoices, maxFailures, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err != nil {
			t.Fatalf("unexpected newJwtValidator() error: %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != maxFailures+1 {
			t.Fatalf("discovery attempts = %d, want 3", numberOfAttempts)
		}
	})

	t.Run("Status 408 Error", func(t *testing.T) {
		var discoveryAttempts atomic.Int32
		var maxFailures int32 = 3

		server := newTestServer(http.StatusRequestTimeout, maxFailures, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err != nil {
			t.Fatalf("unexpected newJwtValidator() error: %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != maxFailures+1 {
			t.Fatalf("discovery attempts = %d, want 3", numberOfAttempts)
		}
	})

	t.Run("Status 429 Error", func(t *testing.T) {
		var discoveryAttempts atomic.Int32
		var maxFailures int32 = 3

		server := newTestServer(http.StatusTooManyRequests, maxFailures, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err != nil {
			t.Fatalf("unexpected newJwtValidator() error: %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != maxFailures+1 {
			t.Fatalf("discovery attempts = %d, want 3", numberOfAttempts)
		}
	})

	t.Run("Status 500 Error", func(t *testing.T) {
		var discoveryAttempts atomic.Int32
		var maxFailures int32 = 3

		server := newTestServer(http.StatusInternalServerError, maxFailures, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err != nil {
			t.Fatalf("unexpected newJwtValidator() error: %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != maxFailures+1 {
			t.Fatalf("discovery attempts = %d, want 3", numberOfAttempts)
		}
	})

	t.Run("Status 429 Error honours Retry-After seconds", func(t *testing.T) {
		var discoveryAttempts atomic.Int32
		var firstAttemptTimeStamp atomic.Int64
		var secondAttemptTimeStamp atomic.Int64

		retryAfter := 1 * time.Second

		var server *httptest.Server
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/.well-known/openid-configuration" {
				attempt := discoveryAttempts.Add(1)

				switch attempt {
				case 1:
					w.Header().Set("Retry-After", strconv.Itoa(int(retryAfter/time.Second)))
					http.Error(w, "too many requests", http.StatusTooManyRequests)
					firstAttemptTimeStamp.Store(time.Now().UnixNano())
				case 2:
					_, _ = fmt.Fprintf(w, `{"jwks_uri":"%s/keys"}`, server.URL)
					secondAttemptTimeStamp.Store(time.Now().UnixNano())
				}

				return
			}
			if r.URL.Path == "/keys" {
				_, _ = fmt.Fprint(w, `{"keys":[]}`)
				return
			}
			http.NotFound(w, r)
		}))
		t.Cleanup(server.Close)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 5 * time.Second,
		})

		if err != nil {
			t.Fatalf("unexpected newJwtValidator() error: %v", err)
		}
		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 2 {
			t.Fatalf("discovery attempts = %d, want 2", numberOfAttempts)
		}

		elapsedTime := time.Duration(secondAttemptTimeStamp.Load() - firstAttemptTimeStamp.Load())
		if elapsedTime < retryAfter { // wait for atleast the retry-after duration
			t.Fatalf("elapsed = %s, want at least %s to honour Retry-After", elapsedTime, retryAfter)
		}
	})
}

func TestNewJwtValidatorRetry_TransientFailures_NetworkError(t *testing.T) {

	var discoveryAttempts atomic.Int32
	var maxFailures int32 = 3

	var server *httptest.Server
	server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/.well-known/openid-configuration" {
			_, _ = fmt.Fprintf(w, `{"jwks_uri":"%s/keys"}`, server.URL)
		}
		if r.URL.Path == "/keys" {
			_, _ = fmt.Fprint(w, `{"keys":[]}`)
		}
		http.NotFound(w, r)
	}))
	t.Cleanup(server.Close)

	transport := server.Client().Transport.(*http.Transport)
	originalDialContext := transport.DialContext
	transport.DialContext = func(ctx context.Context, network, addr string) (net.Conn, error) {
		if discoveryAttempts.Add(1) <= maxFailures {
			return nil, &net.OpError{
				Op:  "dial",
				Net: network,
				Err: errors.New("network error"),
			}
		}
		return originalDialContext(ctx, network, addr)
	}
	client := &http.Client{Transport: transport}

	_, err := newJwtValidator(t.Context(), JwtValidationOptions{
		Issuer:                     server.URL,
		ValidateSignature:          true,
		Http:                       client,
		StartupRetryMaxElapsedTime: 3 * time.Second,
	})

	if err != nil {
		t.Fatalf("unexpected newJwtValidator() error: %v", err)
	}

	if got := discoveryAttempts.Load(); got != maxFailures+1 {
		t.Fatalf("discovery attempts = %d, want %d", got, maxFailures+1)
	}

}

func TestNewJwtValidatorRetry_PermanentFailures(t *testing.T) {

	newTestServer := func(statusCode int, createJSONBody func(url string) string, discoveryAttempts *atomic.Int32) *httptest.Server {
		t.Helper()

		var server *httptest.Server

		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/.well-known/openid-configuration" {
				discoveryAttempts.Add(1)

				w.WriteHeader(statusCode)
				_, _ = fmt.Fprint(w, createJSONBody(server.URL))
				return
			}
			if r.URL.Path == "/keys" {
				_, _ = fmt.Fprint(w, `{"keys":[]}`)
				return
			}
			http.NotFound(w, r)
		}))

		t.Cleanup(server.Close)

		return server
	}

	t.Run("Status 400 error", func(t *testing.T) {
		var discoveryAttempts atomic.Int32

		createJSONBody := func(url string) string {
			return `{"jwks_uri":"` + url + `/keys"}`
		}

		server := newTestServer(http.StatusBadRequest, createJSONBody, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 1 {
			t.Fatalf("discovery attempts = %d, want 1", numberOfAttempts)
		}
	})

	t.Run("Empty Issuer URL", func(t *testing.T) {
		var discoveryAttempts atomic.Int32

		createJSONBody := func(url string) string {
			return `{"jwks_uri":"` + url + `/keys"}`
		}

		server := newTestServer(http.StatusOK, createJSONBody, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     "",
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 0 { // 0 attempts becuase no issuer provided
			t.Fatalf("discovery attempts = %d, want 0", numberOfAttempts)
		}
	})

	t.Run("Nil Context", func(t *testing.T) {
		var discoveryAttempts atomic.Int32

		createJSONBody := func(url string) string {
			return `{"jwks_uri":"` + url + `/keys"}`
		}

		server := newTestServer(http.StatusOK, createJSONBody, &discoveryAttempts)

		_, err := newJwtValidator(nil, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 0 { // 0 attempts because no context
			t.Fatalf("discovery attempts = %d, want 0", numberOfAttempts)
		}
	})

	t.Run("Malformed Discovery JSON", func(t *testing.T) {
		var discoveryAttempts atomic.Int32

		createJSONBody := func(url string) string {
			return `{"jwks_uri" "` + url + `/keys"}` // Not in valid JSON format - missing a colon
		}

		server := newTestServer(http.StatusOK, createJSONBody, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 1 {
			t.Fatalf("discovery attempts = %d, want 1", numberOfAttempts)
		}
	})

	t.Run("Missing JWKS Uri", func(t *testing.T) {
		var discoveryAttempts atomic.Int32

		createJSONBody := func(url string) string {
			return `{"jwks_uri": ""}`
		}

		server := newTestServer(http.StatusOK, createJSONBody, &discoveryAttempts)

		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 1 {
			t.Fatalf("discovery attempts = %d, want 1", numberOfAttempts)
		}
	})

	t.Run("Context Cancelled", func(t *testing.T) {
		var discoveryAttempts atomic.Int32

		createJSONBody := func(url string) string {
			return `{"jwks_uri":"` + url + `/keys"}`
		}

		server := newTestServer(http.StatusInternalServerError, createJSONBody, &discoveryAttempts)

		ctx, cancel := context.WithCancel(t.Context())
		defer cancel()

		// asynchronously cancel context after one second
		var cancelTime = 1 * time.Second
		go func() {
			time.Sleep(cancelTime)
			cancel()
		}()

		_, err := newJwtValidator(ctx, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * cancelTime,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if !errors.Is(err, context.Canceled) {
			t.Fatalf("expected context.Canceled, got %T: %v", err, err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts == 0 { // atleast one retry
			t.Fatalf("discovery attempts = %d, want atleast 1", numberOfAttempts)
		}
	})
}

func TestNewJwtValidatorRetry_RetryBudgetExceeded(t *testing.T) {

	var server *httptest.Server
	var discoveryAttempts atomic.Int32
	server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/.well-known/openid-configuration" {
			discoveryAttempts.Add(1)
			http.Error(w, "Error code", http.StatusInternalServerError)
			return
		}
		if r.URL.Path == "/keys" {
			_, _ = fmt.Fprint(w, `{"keys":[]}`)
			return
		}
		http.NotFound(w, r)
	}))
	defer server.Close()

	_, err := newJwtValidator(t.Context(), JwtValidationOptions{
		Issuer:                     server.URL,
		ValidateSignature:          true,
		Http:                       server.Client(),
		StartupRetryMaxElapsedTime: 2 * time.Second,
	})

	if err == nil {
		t.Fatalf("expected an error")
	}

	if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts == 0 { // atleast one retry
		t.Fatalf("discovery attempts = %d, want atleast 1", numberOfAttempts)
	}
}

func TestNewJwtValidatorRetry_ZeroMaxElapsedTimeIsOneShot(t *testing.T) {
	var discoveryAttempts atomic.Int32
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/.well-known/openid-configuration" {
			discoveryAttempts.Add(1)
			http.Error(w, "Error code", http.StatusInternalServerError)
			return
		}
		http.NotFound(w, r)
	}))
	defer server.Close()

	_, err := newJwtValidator(t.Context(), JwtValidationOptions{
		Issuer:                     server.URL,
		ValidateSignature:          true,
		Http:                       server.Client(),
		StartupRetryMaxElapsedTime: 0,
	})

	if err == nil {
		t.Fatalf("expected an error")
	}

	if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 1 {
		t.Fatalf("discovery attempts = %d, want 1", numberOfAttempts)
	}
}

func TestJwtValidator_validateJwt(t *testing.T) {
	pretendString := "ThisIsNotARealTokenButWeCanPretend"
	t.Run("success", func(t *testing.T) {
		validator := &jwtValidator{
			validateFn: func(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
				if tokenString != pretendString {
					t.Fatalf("validateFn got tokenString = %q, want %q", tokenString, pretendString)
				}
				return &jwt.Token{
					Claims: jwt.MapClaims{
						"foo": "bar",
					},
				}, nil
			},
		}
		token, err := validator.validateJwt(pretendString)
		if err != nil {
			t.Fatalf("validateJwt() error = %v", err)
		}
		claims, ok := token.Claims.(jwt.MapClaims)
		if !ok {
			t.Fatalf("validateJwt() got claims of type %T, want jwt.MapClaims", token.Claims)
		}
		if claims["foo"] != "bar" {
			t.Fatalf("validateJwt() got claims[\"foo\"] = %v, want \"bar\"", claims["foo"])
		}
	})

	t.Run("validation error", func(t *testing.T) {
		validator := &jwtValidator{
			validateFn: func(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
				return nil, errors.New("validation failed")
			},
		}
		_, err := validator.validateJwt(pretendString)
		if err == nil || !strings.Contains(err.Error(), "validation failed") {
			t.Fatalf("validateJwt() error = %v, want validation failed", err)
		}
	})

	t.Run("empty token", func(t *testing.T) {
		validator := &jwtValidator{}
		_, err := validator.validateJwt("")
		if err == nil || !strings.Contains(err.Error(), "jwt is required") {
			t.Fatalf("validateJwt() error = %v, want jwt is required", err)
		}
	})

	t.Run("uninitialized validator", func(t *testing.T) {
		validator := &jwtValidator{}
		_, err := validator.validateJwt(pretendString)
		if err == nil || !strings.Contains(err.Error(), "jwt validator is not properly initialized") {
			t.Fatalf("validateJwt() error = %v, want jwt validator is not properly initialized", err)
		}
	})

	t.Run("with claims", func(t *testing.T) {
		validator := &jwtValidator{
			options: JwtValidationOptions{
				AllowedAlgs: []string{jwt.SigningMethodHS256.Alg()},
				Audience:    "aud-a",
				Issuer:      "issuer-a",
				Leeway:      10,
			},
			validateFn: func(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
				if tokenString != pretendString {
					t.Fatalf("validateFn got tokenString = %q, want %q", tokenString, pretendString)
				}
				if len(parseOptions) != 4 {
					t.Fatalf("validateFn got %d parseOptions, want 3", len(parseOptions))
				}
				return &jwt.Token{
					Claims: jwt.MapClaims{
						"foo": "bar",
						"aud": "aud-a",
					},
				}, nil
			},
		}
		_, err := validator.validateJwt(pretendString)
		if err != nil {
			t.Fatalf("validateJwt() error = %v", err)
		}
	})
}

func TestJwtValidator_validateClaims(t *testing.T) {
	validator := &jwtValidator{}

	t.Run("success", func(t *testing.T) {
		tokenString, err := jwt.NewWithClaims(jwt.SigningMethodNone, jwt.MapClaims{
			"foo": "bar",
		}).SignedString(jwt.UnsafeAllowNoneSignatureType)
		if err != nil {
			t.Fatalf("SignedString() error = %v", err)
		}

		token, err := validator.validateClaims(tokenString, nil)
		if err != nil {
			t.Fatalf("validateClaims() error = %v", err)
		}
		claims, ok := token.Claims.(jwt.MapClaims)
		if !ok {
			t.Fatalf("validateClaims() got claims of type %T, want jwt.MapClaims", token.Claims)
		}
		if claims["foo"] != "bar" {
			t.Fatalf("validateClaims() got claims[\"foo\"] = %v, want \"bar\"", claims["foo"])
		}
	})

	t.Run("invalid token", func(t *testing.T) {
		_, err := validator.validateClaims("not a token", nil)
		if err == nil || !strings.Contains(err.Error(), "token contains an invalid number of segments") {
			t.Fatalf("validateClaims() error = %v, want token contains an invalid number of segments", err)
		}
	})

	t.Run("missing claims", func(t *testing.T) {
		tokenString, err := jwt.NewWithClaims(jwt.SigningMethodNone, jwt.MapClaims{
			"foo": "Bar",
		}).SignedString(jwt.UnsafeAllowNoneSignatureType)
		if err != nil {
			t.Fatalf("SignedString() error = %v", err)
		}
		_, err = validator.validateClaims(tokenString, []jwt.ParserOption{jwt.WithAudience("aud-a")})
		if err == nil || !strings.Contains(err.Error(), "token is missing required claim: aud") {
			t.Fatalf("validateClaims() error = %v, want token is missing required claim: aud", err)
		}
	})
}

func TestJwtValidator_validateSignatureAndClaims(t *testing.T) {
	validator := &jwtValidator{
		keyfunc: func(token *jwt.Token) (any, error) {
			return []byte("secret"), nil
		},
	}

	t.Run("success", func(t *testing.T) {
		tokenString, err := jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
			"foo": "bar",
		}).SignedString([]byte("secret"))
		if err != nil {
			t.Fatalf("SignedString() error = %v", err)
		}

		token, err := validator.validateSignatureAndClaims(tokenString, nil)
		if err != nil {
			t.Fatalf("validateSignatureAndClaims() error = %v", err)
		}
		claims, ok := token.Claims.(jwt.MapClaims)
		if !ok {
			t.Fatalf("validateSignatureAndClaims() got claims of type %T, want jwt.MapClaims", token.Claims)
		}
		if claims["foo"] != "bar" {
			t.Fatalf("validateSignatureAndClaims() got claims[\"foo\"] = %v, want \"bar\"", claims["foo"])
		}
	})

	t.Run("invalid signature", func(t *testing.T) {
		tokenString, err := jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
			"foo": "bar",
		}).SignedString([]byte("wrong secret"))
		if err != nil {
			t.Fatalf("SignedString() error = %v", err)
		}

		_, err = validator.validateSignatureAndClaims(tokenString, nil)
		if err == nil || !strings.Contains(err.Error(), "signature is invalid") {
			t.Fatalf("validateSignatureAndClaims() error = %v, want signature is invalid", err)
		}
	})

	t.Run("keyfunc not initialized", func(t *testing.T) {
		v := &jwtValidator{}
		tokenString, err := jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
			"foo": "bar",
		}).SignedString([]byte("secret"))
		if err != nil {
			t.Fatalf("SignedString() error = %v", err)
		}
		_, err = v.validateSignatureAndClaims(tokenString, nil)
		if err == nil || !strings.Contains(err.Error(), "keyfunc is not initialized") {
			t.Fatalf("validateSignatureAndClaims() error = %v, want keyfunc is not initialized", err)
		}
	})
}

func TestDiscoverJWKSEndpoint(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		var server *httptest.Server
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path != "/.well-known/openid-configuration" {
				t.Fatalf("unexpected path: %s", r.URL.Path)
			}
			w.Header().Set("Content-Type", "application/json")
			_, _ = fmt.Fprintf(w, `{"jwks_uri":"%s/keys"}`, server.URL)
		}))
		defer server.Close()

		jwksURL, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err != nil {
			t.Fatalf("discoverJWKSEndpoint() error = %v", err)
		}

		want := server.URL + "/keys"
		if jwksURL != want {
			t.Fatalf("discoverJWKSEndpoint() got = %q, want = %q", jwksURL, want)
		}
	})

	// A nil client must fall back to http.DefaultClient rather than panic. The
	// httptest server listens on localhost so http.DefaultClient can reach it.
	t.Run("nil client uses default", func(t *testing.T) {
		var server *httptest.Server
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path != "/.well-known/openid-configuration" {
				t.Fatalf("unexpected path: %s", r.URL.Path)
			}
			w.Header().Set("Content-Type", "application/json")
			_, _ = fmt.Fprintf(w, `{"jwks_uri":"%s/keys"}`, server.URL)
		}))
		defer server.Close()

		jwksURL, err := discoverJWKSEndpoint(context.Background(), server.URL, nil)
		if err != nil {
			t.Fatalf("discoverJWKSEndpoint() error = %v", err)
		}

		want := server.URL + "/keys"
		if jwksURL != want {
			t.Fatalf("discoverJWKSEndpoint() got = %q, want = %q", jwksURL, want)
		}
	})

	t.Run("empty issuer", func(t *testing.T) {
		_, err := discoverJWKSEndpoint(context.Background(), "  ", http.DefaultClient)
		if err == nil || !strings.Contains(err.Error(), "issuer is required") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want issuer is required", err)
		}
	})

	t.Run("build request error", func(t *testing.T) {
		_, err := discoverJWKSEndpoint(context.Background(), "http://[::1", http.DefaultClient)
		if err == nil || !strings.Contains(err.Error(), "build request") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want build request", err)
		}
	})

	t.Run("perform request error", func(t *testing.T) {
		transport := &http.Transport{
			DialContext: func(context.Context, string, string) (net.Conn, error) {
				return nil, errors.New("network down")
			},
		}
		t.Cleanup(transport.CloseIdleConnections)

		client := &http.Client{
			Transport: transport,
		}
		_, err := discoverJWKSEndpoint(context.Background(), "https://issuer.example", client)
		if err == nil || !strings.Contains(err.Error(), "perform request") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want perform request", err)
		}
	})

	t.Run("unexpected status", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(http.StatusInternalServerError)
			_, _ = w.Write([]byte("boom"))
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil || !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want unexpected status", err)
		}
	})

	t.Run("decode response error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(http.StatusOK)
			_, _ = w.Write([]byte("{not json"))
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil || !strings.Contains(err.Error(), "decode response") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want decode response", err)
		}
	})

	t.Run("missing jwks_uri", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Content-Type", "application/json")
			_, _ = w.Write([]byte(`{"issuer":"example"}`))
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil || !strings.Contains(err.Error(), "jwks_uri not found") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want jwks_uri not found", err)
		}
	})

	t.Run("response status 429 with retry-after seconds", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Retry-After", "2")
			http.Error(w, "too many requests", http.StatusTooManyRequests)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		var retryAfter *backoff.RetryAfterError
		if !errors.As(err, &retryAfter) {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want RetryAfterError", err)
		}
		if retryAfter.Duration != 2*time.Second {
			t.Fatalf("Retry-After duration = %s, want 2s", retryAfter.Duration)
		}
	})

	t.Run("response status 429 with retry-after HTTP-date", func(t *testing.T) {
		when := time.Now().Add(2 * time.Second).UTC()
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Retry-After", when.Format(http.TimeFormat))
			http.Error(w, "too many requests", http.StatusTooManyRequests)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		var retryAfter *backoff.RetryAfterError
		if !errors.As(err, &retryAfter) {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want RetryAfterError", err)
		}
		if retryAfter.Duration < time.Second || retryAfter.Duration > 2*time.Second {
			t.Fatalf("Retry-After duration = %s, want around 2s", retryAfter.Duration)
		}
	})

	t.Run("response status 429 without retry-after stays retriable", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Error(w, "too many requests", http.StatusTooManyRequests)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil || !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want unexpected status", err)
		}
		var retryAfter *backoff.RetryAfterError
		if errors.As(err, &retryAfter) {
			t.Fatalf("discoverJWKSEndpoint() unexpectedly returned RetryAfterError: %v", err)
		}
		var permanent *backoff.PermanentError
		if errors.As(err, &permanent) {
			t.Fatalf("discoverJWKSEndpoint() unexpectedly returned PermanentError: %v", err)
		}
	})

	t.Run("response status 429 with invalid retry-after stays retriable", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Retry-After", "later")
			http.Error(w, "too many requests", http.StatusTooManyRequests)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		var retryAfter *backoff.RetryAfterError
		if errors.As(err, &retryAfter) {
			t.Fatalf("discoverJWKSEndpoint() unexpectedly returned RetryAfterError: %v", err)
		}
	})
}
func TestExtractTokenScopes_NonMapClaims(t *testing.T) {
	token := &jwt.Token{
		Claims: &jwt.RegisteredClaims{
			Subject: "test-user",
		},
	}

	readScopes, writeScopes := extractTokenScopes(token)

	if len(readScopes) != 0 {
		t.Errorf("expected no read scopes for non-map claims, got %v", readScopes)
	}
	if len(writeScopes) != 0 {
		t.Errorf("expected no write scopes for non-map claims, got %v", writeScopes)
	}
}

func TestExtractTokenScopes_MissingScopeClaim(t *testing.T) {
	token := &jwt.Token{
		Claims: jwt.MapClaims{
			"sub": "test-user",
		},
	}

	readScopes, writeScopes := extractTokenScopes(token)

	if len(readScopes) != 0 {
		t.Errorf("expected no read scopes when scope claim is absent, got %v", readScopes)
	}
	if len(writeScopes) != 0 {
		t.Errorf("expected no write scopes when scope claim is absent, got %v", writeScopes)
	}
}

func TestExtractTokenScopes_NonStringScopeClaim(t *testing.T) {
	token := &jwt.Token{
		Claims: jwt.MapClaims{
			"scope": []string{st2138.ScopeOp},
		},
	}

	readScopes, writeScopes := extractTokenScopes(token)

	if len(readScopes) != 0 {
		t.Errorf("expected no read scopes for non-string scope claim, got %v", readScopes)
	}
	if len(writeScopes) != 0 {
		t.Errorf("expected no write scopes for non-string scope claim, got %v", writeScopes)
	}
}

func TestExtractTokenScopes_SplitsReadAndWriteScopes(t *testing.T) {
	token := &jwt.Token{
		Claims: jwt.MapClaims{
			"scope": "st2138:mon st2138:cfg:w custom:w",
		},
	}

	readScopes, writeScopes := extractTokenScopes(token)

	for _, scopeName := range []string{"st2138:mon", "st2138:cfg", "custom"} {
		if _, ok := readScopes[scopeName]; !ok {
			t.Errorf("expected read scopes to include %s, got %v", scopeName, readScopes)
		}
	}
	for _, scopeName := range []string{"st2138:cfg", "custom"} {
		if _, ok := writeScopes[scopeName]; !ok {
			t.Errorf("expected write scopes to include %s, got %v", scopeName, writeScopes)
		}
	}
	if _, ok := writeScopes["st2138:mon"]; ok {
		t.Errorf("expected read-only scope to be omitted from write scopes, got %v", writeScopes)
	}
	if _, ok := readScopes["st2138:cfg:w"]; ok {
		t.Errorf("expected suffixed write scope to be normalized out of read scopes, got %v", readScopes)
	}
	if _, ok := writeScopes["st2138:cfg:w"]; ok {
		t.Errorf("expected suffixed write scope to be normalized out of write scopes, got %v", writeScopes)
	}
}
