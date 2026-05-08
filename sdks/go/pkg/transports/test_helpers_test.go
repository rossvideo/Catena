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
 * @brief Test helpers for the REST server tests.
 * @file test_helpers_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-25
 */

package transports

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"net/http"
	"net/http/httptest"
	"os"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// TestMain sets up test environment for all tests in this package.
func TestMain(m *testing.M) {
	catena.SetEnv(catena.EnvDev)
	code := m.Run()
	os.Exit(code)
}

// --- Mock ResponseWriter types ---

// failWriter is an http.ResponseWriter that fails on Write (used to cover error paths).
type failWriter struct {
	http.ResponseWriter
	failOnWrite bool
}

func (f *failWriter) Write(p []byte) (n int, err error) {
	if f.failOnWrite {
		return 0, errors.New("write failed")
	}
	return f.ResponseWriter.Write(p)
}

// noFlusher wraps ResponseWriter without implementing http.Flusher.
type noFlusher struct {
	http.ResponseWriter
}

// failFlusherWriter implements http.ResponseWriter and http.Flusher
// but fails on Write after a configurable number of successful writes.
type failFlusherWriter struct {
	*httptest.ResponseRecorder
	failAfterN int
	writeCount int
}

func (f *failFlusherWriter) Write(p []byte) (int, error) {
	if f.writeCount >= f.failAfterN {
		return 0, errors.New("write failed")
	}
	f.writeCount++
	return f.ResponseRecorder.Write(p)
}

func (f *failFlusherWriter) Flush() {
	f.ResponseRecorder.Flush()
}

// --- Request helpers ---

// makeRequest creates an HTTP request, serves it through the server mux, and
// returns the response recorder. Non-empty bodies get Content-Type: application/json.
func makeRequest(t *testing.T, transport *RestTransport, method, path, body string) *httptest.ResponseRecorder {
	t.Helper()
	var req *http.Request
	if body != "" {
		req = httptest.NewRequest(method, path, bytes.NewBufferString(body))
		req.Header.Set("Content-Type", "application/json")
	} else {
		req = httptest.NewRequest(method, path, nil)
	}
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
	return rec
}

// makeRequestWithHeaders is like makeRequest but sets explicit headers instead
// of the default Content-Type.
func makeRequestWithHeaders(t *testing.T, transport *RestTransport, method, path, body string, headers map[string]string) *httptest.ResponseRecorder {
	t.Helper()
	var req *http.Request
	if body != "" {
		req = httptest.NewRequest(method, path, bytes.NewBufferString(body))
	} else {
		req = httptest.NewRequest(method, path, nil)
	}
	for k, v := range headers {
		req.Header.Set(k, v)
	}
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
	return rec
}

// --- Assertion helpers ---

func assertStatus(t *testing.T, rec *httptest.ResponseRecorder, expected int) {
	t.Helper()
	if rec.Code != expected {
		t.Errorf("expected status %d, got %d", expected, rec.Code)
	}
}

func assertContentType(t *testing.T, rec *httptest.ResponseRecorder, expected string) {
	t.Helper()
	if ct := rec.Header().Get("Content-Type"); ct != expected {
		t.Errorf("expected Content-Type %q, got %q", expected, ct)
	}
}

func assertHeader(t *testing.T, rec *httptest.ResponseRecorder, key, expected string) {
	t.Helper()
	if got := rec.Header().Get(key); got != expected {
		t.Errorf("expected header %s=%q, got %q", key, expected, got)
	}
}

func parseJSONBody(t *testing.T, rec *httptest.ResponseRecorder) map[string]any {
	t.Helper()
	var response map[string]any
	if err := json.Unmarshal(rec.Body.Bytes(), &response); err != nil {
		t.Fatalf("failed to parse JSON response: %v\nbody: %s", err, rec.Body.String())
	}
	return response
}

// assertHasError verifies the response body has a JSON "error" field and returns its value.
func assertHasError(t *testing.T, rec *httptest.ResponseRecorder) string {
	t.Helper()
	response := parseJSONBody(t, rec)
	errMsg, ok := response["error"].(string)
	if !ok {
		t.Fatal("expected 'error' field in JSON response")
	}
	return errMsg
}

func assertBodyContains(t *testing.T, rec *httptest.ResponseRecorder, substr string) {
	t.Helper()
	if !bytes.Contains(rec.Body.Bytes(), []byte(substr)) {
		t.Errorf("expected body to contain %q, got %q", substr, rec.Body.String())
	}
}

func assertBodyNotContains(t *testing.T, rec *httptest.ResponseRecorder, substr string) {
	t.Helper()
	if bytes.Contains(rec.Body.Bytes(), []byte(substr)) {
		t.Errorf("expected body NOT to contain %q, got %q", substr, rec.Body.String())
	}
}

// --- SSE helpers ---

// setupSSEConnection starts a background SSE connection to /st2138-api/v1/connect
// and waits for the handler to be established. Returns the recorder and a cleanup
// function to tear down the connection.
func setupSSEConnection(t *testing.T, transport *RestTransport) (*httptest.ResponseRecorder, func()) {
	t.Helper()
	ctx, cancel := context.WithCancel(context.Background())
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()
	done := make(chan struct{})
	go func() {
		defer close(done)
		transport.mux.ServeHTTP(rec, req)
	}()
	time.Sleep(150 * time.Millisecond)
	return rec, func() {
		cancel()
		select {
		case <-done:
		case <-time.After(2 * time.Second):
			t.Fatal("SSE handler did not exit after context cancel")
		}
	}
}

// cleanupSSE tears down an SSE connection and waits briefly for cleanup.
func cleanupSSE(cleanup func()) {
	cleanup()
	time.Sleep(50 * time.Millisecond)
}

func makeTestConnection(id int) *catena.Connection {
	return &catena.Connection{
		ID:      id,
		Updates: make(chan *protos.PushUpdates, 10),
		Done:    make(chan struct{}),
	}
}
