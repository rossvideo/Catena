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
	"strings"
	"testing"

	"github.com/golang-jwt/jwt/v5"
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
			Issuer:            server.URL,
			ValidateSignature: true,
			Http:              server.Client(),
		})
		if err != nil {
			t.Fatalf("newJwtValidator() error = %v", err)
		}
		if validator == nil {
			t.Fatal("newJwtValidator() got nil, want non-nil")
		}
	})

	t.Run("discoverJWKSEndpoint error", func(t *testing.T) {
		_, err := newJwtValidator(t.Context(), JwtValidationOptions{
			Issuer:            "http://[::1",
			ValidateSignature: true,
			Http:              http.DefaultClient,
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
			Issuer:            server.URL,
			ValidateSignature: true,
			Http:              server.Client(),
		})
		if err == nil || !strings.Contains(err.Error(), "create keyfunc") {
			t.Fatalf("newJwtValidator() error = %v, want create keyfunc", err)
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

func TestJwtValidator_validateSignature(t *testing.T) {
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

		token, err := validator.validateSignature(tokenString, nil)
		if err != nil {
			t.Fatalf("validateSignature() error = %v", err)
		}
		claims, ok := token.Claims.(jwt.MapClaims)
		if !ok {
			t.Fatalf("validateSignature() got claims of type %T, want jwt.MapClaims", token.Claims)
		}
		if claims["foo"] != "bar" {
			t.Fatalf("validateSignature() got claims[\"foo\"] = %v, want \"bar\"", claims["foo"])
		}
	})

	t.Run("invalid signature", func(t *testing.T) {
		tokenString, err := jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.MapClaims{
			"foo": "bar",
		}).SignedString([]byte("wrong secret"))
		if err != nil {
			t.Fatalf("SignedString() error = %v", err)
		}

		_, err = validator.validateSignature(tokenString, nil)
		if err == nil || !strings.Contains(err.Error(), "signature is invalid") {
			t.Fatalf("validateSignature() error = %v, want signature is invalid", err)
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
		_, err = v.validateSignature(tokenString, nil)
		if err == nil || !strings.Contains(err.Error(), "keyfunc is not initialized") {
			t.Fatalf("validateSignature() error = %v, want keyfunc is not initialized", err)
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
}
func TestExtractTokenScopes_NonMapClaims(t *testing.T) {
	token := &jwt.Token{
		Claims: &jwt.RegisteredClaims{
			Subject: "test-user",
		},
	}

	scopes, status := extractTokenScopes(token)

	if status.Code != OK {
		t.Fatalf("expected OK status, got %v", status)
	}
	if len(scopes) != 0 {
		t.Errorf("expected no scopes for non-map claims, got %v", scopes)
	}
}

func TestExtractTokenScopes_MissingScopeClaim(t *testing.T) {
	token := &jwt.Token{
		Claims: jwt.MapClaims{
			"sub": "test-user",
		},
	}

	scopes, status := extractTokenScopes(token)

	if status.Code != OK {
		t.Fatalf("expected OK status, got %v", status)
	}
	if len(scopes) != 0 {
		t.Errorf("expected no scopes when scope claim is absent, got %v", scopes)
	}
}
