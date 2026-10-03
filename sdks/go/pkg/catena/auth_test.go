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
	"bytes"
	"context"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"math"
	"net"
	"net/http"
	"net/http/httptest"
	"net/url"
	"strconv"
	"strings"
	"sync/atomic"
	"testing"
	"time"

	"github.com/MicahParks/jwkset"
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

		validator, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
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
		validator, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
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
		if validatorTyped.log == nil {
			t.Fatal("newJwtValidator() got nil log, want non-nil")
		}
	})

	t.Run("logger with component tag", func(t *testing.T) {
		var buf bytes.Buffer
		log := slog.New(slog.NewTextHandler(&buf, nil))
		validator, err := newJwtValidator(t.Context(), log, JwtValidationOptions{
			ValidateSignature:          false,
			StartupRetryMaxElapsedTime: 0, //single attempt
		})
		if err != nil {
			t.Fatalf("newJwtValidator() error = %v", err)
		}
		validatorTyped, ok := validator.(*jwtValidator)
		if !ok {
			t.Fatalf("newJwtValidator() got type %T, want *jwtValidator", validator)
		}
		validatorTyped.log.Info("probe")
		if !strings.Contains(buf.String(), "component=jwt") {
			t.Fatalf("newJwtValidator() log output = %q, want component=jwt", buf.String())
		}
	})

	t.Run("discoverJWKSEndpoint error", func(t *testing.T) {
		_, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
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
		_, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 0, //single attempt
		})
		if err == nil || !strings.Contains(err.Error(), "create keyfunc") {
			t.Fatalf("newJwtValidator() error = %v, want create keyfunc", err)
		}
	})

	t.Run("nil context is permanent", func(t *testing.T) {

		var server *httptest.Server
		var discoveryAttempts atomic.Int32
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/.well-known/openid-configuration" {
				discoveryAttempts.Add(1)
				http.Error(w, "error", http.StatusInternalServerError)
			}
		}))

		var nilCxt context.Context
		_, err := newJwtValidator(nilCxt, nil, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       http.DefaultClient,
			StartupRetryMaxElapsedTime: -1, // indefinite retry
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if !strings.Contains(err.Error(), "nil context provided") {
			t.Fatalf("unexpected error: %v, want nil context provided", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts >= 1 {
			t.Fatalf("discovery attempts = %d, want 0", numberOfAttempts)
		}

	})
}

func TestNewJwtValidator_Retry(t *testing.T) {

	t.Run("successful retries", func(t *testing.T) {
		t.Parallel()

		var server *httptest.Server
		var discoveryAttempts atomic.Int32
		server = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/.well-known/openid-configuration" {
				attempt := discoveryAttempts.Add(1)

				if attempt <= 2 {
					http.Error(w, "Error code", http.StatusInternalServerError)
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
		defer server.Close()

		_, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 3 * time.Second,
		})

		if err != nil {
			t.Fatalf("newJwtValidator() error = %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 3 {
			t.Fatalf("discovery attempts = %d, want 3", numberOfAttempts)
		}
	})

	t.Run("retry budget exceeded", func(t *testing.T) {

		t.Parallel()

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

		_, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 1 * time.Second,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts <= 1 { // atleast one retry
			t.Fatalf("discovery attempts = %d, want 2 or more attempts", numberOfAttempts)
		}
	})

	t.Run("hung request causes deadline exceeded", func(t *testing.T) {
		t.Parallel()

		var discoveryAttempts atomic.Int32
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			discoveryAttempts.Add(1)
			if r.URL.Path != "/.well-known/openid-configuration" {
				for {
					if r.Context().Err() != nil {
						break
					}
					time.Sleep(1 * time.Millisecond)
				}
				return
			}
			<-r.Context().Done() // keeps the http handler alive to reach the DeadlineExceeded error
		}))
		defer server.Close()

		_, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
			Issuer:                     server.URL,
			ValidateSignature:          true,
			Http:                       server.Client(),
			StartupRetryMaxElapsedTime: 50 * time.Millisecond,
		})

		if err == nil {
			t.Fatalf("expected an error")
		}

		if !errors.Is(err, context.DeadlineExceeded) {
			t.Fatalf("expected context.DeadlineExceeded, got %v", err)
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts != 1 {
			t.Fatalf("discovery attempts = %d, want 1", numberOfAttempts)
		}
	})

	t.Run("zero max elapsed time is a one shot attempt", func(t *testing.T) {
		t.Parallel()

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

		_, err := newJwtValidator(t.Context(), nil, JwtValidationOptions{
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
	})

	t.Run("negative max elapsed time retries until context is cancelled", func(t *testing.T) {
		t.Parallel()

		var discoveryAttempts atomic.Int32
		keptRetrying := make(chan struct{})
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/.well-known/openid-configuration" {
				if discoveryAttempts.Add(1) == 3 {
					close(keptRetrying)
				}
				http.Error(w, "Error code", http.StatusInternalServerError)
				return
			}
			<-r.Context().Done()
		}))
		defer server.Close()

		ctx, cancel := context.WithCancel(t.Context())
		defer cancel()

		errCh := make(chan error, 1)
		// Go routine to call newJwtValidator which will retry indefinitely until the context is cancelled
		go func() {
			_, err := newJwtValidator(ctx, nil, JwtValidationOptions{
				Issuer:                     server.URL,
				ValidateSignature:          true,
				Http:                       server.Client(),
				StartupRetryMaxElapsedTime: -1,
			})
			errCh <- err
		}()

		// Wait for the retry to stop or timeout
		select {
		case <-keptRetrying:
			// Retry stopped because 3 attempts were made - pass
		case err := <-errCh:
			t.Fatalf("retry stopped before 3 attempts: %v (attempts=%d)", err, discoveryAttempts.Load())
		case <-time.After(5 * time.Second):
			t.Fatal("negative retry budget stopped before 3 discovery attempts, attempts = ", discoveryAttempts.Load())
		}

		cancel()

		select {
		case err := <-errCh:
			if !errors.Is(err, context.Canceled) {
				t.Fatalf("expected context.Canceled, got %v", err)
			}
		case <-time.After(2 * time.Second):
			t.Fatal("retry did not stop after context cancellation")
		}

		if numberOfAttempts := discoveryAttempts.Load(); numberOfAttempts < 3 {
			t.Fatalf("discovery attempts = %d, want at least 3", numberOfAttempts)
		}
	})
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

		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want backoff.PermanentError", err)
		}

	})

	t.Run("build request error", func(t *testing.T) {
		_, err := discoverJWKSEndpoint(context.Background(), "http://[::1", http.DefaultClient)
		if err == nil || !strings.Contains(err.Error(), "build request") {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want build request", err)
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want backoff.PermanentError", err)
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

		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want backoff.PermanentError", err)
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

		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want backoff.PermanentError", err)
		}
	})
}

func TestDiscoverJWKSEndpoint_StatusCodeErrors(t *testing.T) {

	t.Run("response status 101 error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Error(w, "switching protocols", http.StatusSwitchingProtocols)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil {
			t.Fatalf("expected an error")
		}

		if !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("unexpected error: %v, want unexpected status", err)
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
		}
	})

	t.Run("response status 300 error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Error(w, "multiple choices", http.StatusMultipleChoices)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil {
			t.Fatalf("expected an error")
		}

		if !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("unexpected error: %v, want unexpected status", err)
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
		}
	})

	t.Run("response status 500 error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Error(w, "internal server error", http.StatusInternalServerError)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil {
			t.Fatalf("expected an error")
		}

		if !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("unexpected error: %v, want unexpected status", err)
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
		}
	})

	t.Run("response status 400 error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Error(w, "bad request", http.StatusBadRequest)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil {
			t.Fatalf("expected an error")
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("discoverJWKSEndpoint() error = %v, want backoff.PermanentError", err)
		}
	})

	t.Run("response status 408 error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			http.Error(w, "request timeout", http.StatusRequestTimeout)
		}))
		defer server.Close()

		_, err := discoverJWKSEndpoint(context.Background(), server.URL, server.Client())
		if err == nil {
			t.Fatalf("expected an error")
		}

		if !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("unexpected error: %v, want unexpected status", err)
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
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

	t.Run("response status 429 with negative retry-after stays retriable", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Retry-After", "-1")
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
	})

	t.Run("response status 429 with past retry-after HTTP-date stays retriable", func(t *testing.T) {
		when := time.Now().Add(-time.Minute).UTC()
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Retry-After", when.Format(http.TimeFormat))
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
	})

	t.Run("response status 429 with overflow retry-after stays retriable", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.Header().Set("Retry-After", strconv.FormatInt(math.MaxInt64, 10))
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
}

func TestClassifyDiscoveryError(t *testing.T) {
	t.Run("nil", func(t *testing.T) {
		if err := classifyDiscoveryError(t.Context(), nil); err != nil {
			t.Fatalf("classifyDiscoveryError(nil) = %v, want nil", err)
		}
	})

	t.Run("network error is transient", func(t *testing.T) {
		networkErr := errors.New("network down")
		err := classifyDiscoveryError(t.Context(), &url.Error{
			Op:  "get",
			URL: "https://issuer.example",
			Err: networkErr,
		})

		if err == nil {
			t.Fatalf("classifyDiscoveryError() error = nil, want error")
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
		}

		if _, ok := errors.AsType[*url.Error](err); !ok {
			t.Fatalf("classifyDiscoveryError() error = %v, want *url.Error", err)
		}

		if !errors.Is(err, networkErr) {
			t.Fatalf("classifyDiscoveryError() error = %v, want network down", err)
		}
	})

	t.Run("context canceled is permanent", func(t *testing.T) {
		ctx, cancel := context.WithCancel(t.Context())
		cancel()

		err := classifyDiscoveryError(ctx, context.Canceled)
		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("classifyDiscoveryError() error = %v, want backoff.PermanentError", err)
		}
		if !errors.Is(err, context.Canceled) {
			t.Fatalf("classifyDiscoveryError() error = %v, want context.Canceled", err)
		}
	})

	t.Run("deadline exceeded is permanent", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(t.Context(), time.Nanosecond)
		defer cancel()

		err := classifyDiscoveryError(ctx, context.DeadlineExceeded)
		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("classifyDiscoveryError() error = %v, want backoff.PermanentError", err)
		}
	})

	t.Run("non network error is permanent", func(t *testing.T) {
		networkErr := errors.New("test-error")
		err := classifyDiscoveryError(t.Context(), networkErr)
		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("classifyDiscoveryError() error = %v, want backoff.PermanentError", err)
		}
		if _, ok := errors.AsType[*url.Error](err); ok {
			t.Fatalf("unexpected *url.Error: %v", err)
		}
		if !errors.Is(err, networkErr) {
			t.Fatalf("unexpected error: %v, want %v", err, networkErr)
		}
	})
}

func TestClassifyKeyfuncError(t *testing.T) {
	t.Run("nil error", func(t *testing.T) {
		if err := classifyKeyfuncError(t.Context(), nil); err != nil {
			t.Fatalf("classifyKeyfuncError(nil) = %v, want nil", err)
		}
	})

	t.Run("context canceled is permanent", func(t *testing.T) {
		ctx, cancel := context.WithCancel(t.Context())
		cancel()

		err := classifyKeyfuncError(ctx, context.Canceled)
		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("classifyKeyfuncError() error = %v, want backoff.PermanentError", err)
		}
		if !errors.Is(err, context.Canceled) {
			t.Fatalf("classifyKeyfuncError() error = %v, want context.Canceled", err)
		}
	})

	t.Run("status code error", func(t *testing.T) {
		statusCodeError := jwkset.ErrInvalidHTTPStatusCode.Error() + ": 500"
		err := classifyKeyfuncError(t.Context(), errors.New(statusCodeError))

		if err == nil {
			t.Fatalf("classifyKeyfuncError() error = nil, want error")
		}

		if !strings.Contains(err.Error(), "unexpected status") {
			t.Fatalf("unexpected error: %v, want unexpected status", err)
		}

		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
		}
	})

	t.Run("extractJWKSResponse failure", func(t *testing.T) {
		statusCodeError := jwkset.ErrInvalidHTTPStatusCode.Error() + ": invaliderror"
		err := classifyKeyfuncError(t.Context(), errors.New(statusCodeError))
		if err == nil {
			t.Fatalf("classifyKeyfuncError() error = nil, want error")
		}
		if !strings.Contains(err.Error(), "invaliderror") {
			t.Fatalf("unexpected error: %v, want invaliderror", err)
		}
	})

	t.Run("unknown error is permanent", func(t *testing.T) {
		err := errors.New("test-error")
		err = classifyKeyfuncError(t.Context(), err)
		if err == nil {
			t.Fatalf("classifyKeyfuncError() error = nil, want error")
		}
		if !strings.Contains(err.Error(), "test-error") {
			t.Fatalf("unexpected error: %v, want test-error", err)
		}
		if _, ok := errors.AsType[*backoff.PermanentError](err); !ok {
			t.Fatalf("classifyKeyfuncError() error = %v, want backoff.PermanentError", err)
		}
	})

	t.Run("EOF is transient", func(t *testing.T) {
		err := io.ErrUnexpectedEOF
		err = classifyKeyfuncError(t.Context(), err)
		if err == nil {
			t.Fatalf("classifyKeyfuncError() error = nil, want error")
		}
		if !strings.Contains(err.Error(), io.ErrUnexpectedEOF.Error()) {
			t.Fatalf("unexpected error: %v, want io.ErrUnexpectedEOF", err)
		}
		if _, ok := errors.AsType[*backoff.PermanentError](err); ok {
			t.Fatalf("unexpected backoff.PermanentError: %v", err)
		}
	})
}

func TestExtractJWKSResponse(t *testing.T) {

	t.Run("success", func(t *testing.T) {
		err := jwkset.ErrInvalidHTTPStatusCode.Error() + ": 500"
		resp, ok := extractJWKSResponse(errors.New(err))
		if !ok {
			t.Fatalf("extractJWKSResponse() = %v, want true", ok)
		}
		if resp.StatusCode != http.StatusInternalServerError {
			t.Fatalf("unexpected status code: %v, want 500", resp.StatusCode)
		}
	})

	t.Run("no status code in error message", func(t *testing.T) {
		err := errors.New("test-error")
		_, ok := extractJWKSResponse(err)
		if ok {
			t.Fatalf("extractJWKSResponse() = %v, want false", ok)
		}
	})

	t.Run("invalid status code in error message", func(t *testing.T) {
		err := jwkset.ErrInvalidHTTPStatusCode.Error() + ": invalid"
		_, ok := extractJWKSResponse(errors.New(err))
		if ok {
			t.Fatalf("extractJWKSResponse() = %v, want false", ok)
		}
	})

	t.Run("strconv.atoi error", func(t *testing.T) {
		err := jwkset.ErrInvalidHTTPStatusCode.Error() + ": 999999999999999999999999999999999999999999999999999999" // force overflow
		_, ok := extractJWKSResponse(errors.New(err))
		if ok {
			t.Fatalf("extractJWKSResponse() = %v, want false", ok)
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
