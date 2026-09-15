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
 * @brief Tests for REST CORS middleware.
 * @file cors_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-09-14
 */

package rest

import (
	"context"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/internal/transporttest"
)

func corsHandler(t *testing.T, opts config.RestOptions) (*Transport, http.Handler) {
	t.Helper()
	if opts.Port == 0 {
		opts.Port = 8080
	}
	transport := NewTransport(opts)
	stub := transporttest.MakeStubServerRuntime(t)
	stub.Dev = true
	transport.runtime = stub
	return transport, transport.withCORS(transport.mux)
}

func TestCORS_DisabledByDefault(t *testing.T) {
	_, h := corsHandler(t, config.RestOptions{})
	req := httptest.NewRequest(http.MethodOptions, "/st2138-api/v1/health", nil)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)

	if rec.Code == http.StatusNoContent {
		t.Fatalf("expected OPTIONS not to be short-circuited when CORS is off, got %d", rec.Code)
	}
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	assertHeader(t, rec, "Access-Control-Allow-Origin", "")
	assertHeader(t, rec, "Access-Control-Allow-Credentials", "")
}

func TestCORS_Preflight(t *testing.T) {
	_, h := corsHandler(t, config.RestOptions{
		AllowedOrigins:      []string{"https://example.com"},
		ExtraAllowedHeaders: []string{"X-Tenant-Id"},
		CorsMaxAge:          600 * time.Second,
	})
	req := httptest.NewRequest(http.MethodOptions, "/st2138-api/v1/health", nil)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)

	assertStatus(t, rec, http.StatusNoContent)
	assertHeader(t, rec, "Access-Control-Allow-Origin", "https://example.com")
	assertHeader(t, rec, "Vary", "Origin")
	assertHeader(t, rec, "Access-Control-Max-Age", "600")
	assertHeader(t, rec, "Access-Control-Allow-Credentials", "")
	assertCSVContains(t, rec.Header().Get("Access-Control-Allow-Methods"), "GET", "POST", "PUT", "DELETE", "OPTIONS")
	assertCSVContains(t, rec.Header().Get("Access-Control-Allow-Headers"),
		"Content-Type", "Authorization", "Accept", "Language", "Detail-Level", "X-Tenant-Id")
}

func TestCORS_AllowedOriginEchoedOnGET(t *testing.T) {
	_, h := corsHandler(t, config.RestOptions{AllowedOrigins: []string{"https://example.com"}})
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/health", nil)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)

	assertStatus(t, rec, http.StatusOK)
	assertHeader(t, rec, "Access-Control-Allow-Origin", "https://example.com")
	assertHeader(t, rec, "Vary", "Origin")
	assertHeader(t, rec, "Access-Control-Allow-Credentials", "")
}

func TestCORS_DisallowedOriginOmitted(t *testing.T) {
	_, h := corsHandler(t, config.RestOptions{AllowedOrigins: []string{"https://allowed.example"}})

	cases := []struct {
		name   string
		origin string
	}{
		{"other origin", "https://evil.example"},
		{"null origin", "null"},
		{"substring lookalike", "https://allowed.example.evil"},
		{"prefix lookalike", "https://allowed"},
	}
	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/health", nil)
			req.Header.Set("Origin", tt.origin)
			rec := httptest.NewRecorder()
			h.ServeHTTP(rec, req)
			assertHeader(t, rec, "Access-Control-Allow-Origin", "")
			assertHeader(t, rec, "Vary", "")
		})
	}
}

func TestCORS_StarLiteral(t *testing.T) {
	_, h := corsHandler(t, config.RestOptions{AllowedOrigins: []string{"*"}})
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/health", nil)
	req.Header.Set("Origin", "https://anywhere.example")
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)

	assertStatus(t, rec, http.StatusOK)
	assertHeader(t, rec, "Access-Control-Allow-Origin", "*")
	assertHeader(t, rec, "Vary", "")
}

func TestCORS_StarBeatsSpecificList(t *testing.T) {
	_, h := corsHandler(t, config.RestOptions{
		AllowedOrigins: []string{"https://example.com", "*"},
	})
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/health", nil)
	req.Header.Set("Origin", "https://other.example")
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)

	assertStatus(t, rec, http.StatusOK)
	assertHeader(t, rec, "Access-Control-Allow-Origin", "*")
	assertHeader(t, rec, "Vary", "")
}

func TestCORS_ConnectEchoesAllowedOrigin(t *testing.T) {
	transport, h := corsHandler(t, config.RestOptions{AllowedOrigins: []string{"https://example.com"}})
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	transport.runtime.(*transporttest.StubServerRuntime).WithConnection(transporttest.MakeTestConnection(1))

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()

	go h.ServeHTTP(rec, req)
	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	assertStatus(t, rec, http.StatusOK)
	assertHeader(t, rec, "Access-Control-Allow-Origin", "https://example.com")
	assertHeader(t, rec, "Access-Control-Allow-Credentials", "")
}

func TestUnionCORSTokens(t *testing.T) {
	methods := unionCORSMethods(requiredMethods, []string{"patch", "GET", " "})
	assertCSVContains(t, strings.Join(methods, ", "), "GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH")

	headers := unionCORSHeaders(requiredHeaders, []string{"X-Tenant-Id", "authorization"})
	assertCSVContains(t, strings.Join(headers, ", "), "Content-Type", "Authorization", "X-Tenant-Id")
	// first-seen casing for Authorization (from required), not the extra "authorization"
	if headers[1] != "Authorization" {
		t.Errorf("expected first-seen Authorization casing, got %q", headers[1])
	}
}

func assertCSVContains(t *testing.T, csv string, tokens ...string) {
	t.Helper()
	parts := strings.Split(csv, ",")
	have := map[string]struct{}{}
	for _, p := range parts {
		have[strings.ToLower(strings.TrimSpace(p))] = struct{}{}
	}
	for _, tok := range tokens {
		if _, ok := have[strings.ToLower(tok)]; !ok {
			t.Errorf("expected %q in %q", tok, csv)
		}
	}
}
