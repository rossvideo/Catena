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
 * @brief JSON utilities for the Catena REST API.
 * @file json_utils.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package rest

import (
	"bytes"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func TestReadRequestJSON_ValidInt32(t *testing.T) {
	body := `{"int32_value": 42}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json")

	val, err := ReadRequestJSON(req)
	if err.Code != catena.OK {
		t.Fatalf("unexpected error: %v", err)
	}
	if val.GetInt32Value() != 42 {
		t.Errorf("expected int32_value=42, got %d", val.GetInt32Value())
	}
}

func TestReadRequestJSON_ValidString(t *testing.T) {
	body := `{"string_value": "hello"}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json")

	val, err := ReadRequestJSON(req)
	if err.Code != catena.OK {
		t.Fatalf("unexpected error: %v", err)
	}
	if val.GetStringValue() != "hello" {
		t.Errorf("expected string_value='hello', got %q", val.GetStringValue())
	}
}

func TestReadRequestJSON_ValidFloat(t *testing.T) {
	body := `{"float32_value": 3.14}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json")

	val, err := ReadRequestJSON(req)
	if err.Code != catena.OK {
		t.Fatalf("unexpected error: %v", err)
	}
	if val.GetFloat32Value() < 3.13 || val.GetFloat32Value() > 3.15 {
		t.Errorf("expected float32_value~=3.14, got %f", val.GetFloat32Value())
	}
}

func TestReadRequestJSON_ContentTypeWithCharset(t *testing.T) {
	body := `{"int32_value": 100}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json; charset=utf-8")

	val, err := ReadRequestJSON(req)
	if err.Code != catena.OK {
		t.Fatalf("unexpected error: %v", err)
	}
	if val.GetInt32Value() != 100 {
		t.Errorf("expected int32_value=100, got %d", val.GetInt32Value())
	}
}

func TestReadRequestJSON_MissingContentType(t *testing.T) {
	body := `{"int32_value": 42}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	// No Content-Type header

	_, err := ReadRequestJSON(req)
	if err.Code != catena.BAD_REQUEST {
		t.Fatal("expected BAD_REQUEST error for missing Content-Type")
	}
	if err.Error != "missing Content-Type header" {
		t.Errorf("expected 'missing Content-Type header' error, got: %v", err)
	}
}

func TestReadRequestJSON_InvalidContentType(t *testing.T) {
	body := `{"int32_value": 42}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "text/plain")

	_, err := ReadRequestJSON(req)
	if err.Code != catena.INVALID_ARGUMENT {
		t.Fatal("expected error for invalid Content-Type")
	}
	if err.Error != "unsupported content type: text/plain, expected application/json" {
		t.Errorf("expected 'unsupported content type: text/plain, expected application/json' error, got: %v", err)
	}
}

func TestReadRequestJSON_MalformedContentType(t *testing.T) {
	body := `{"int32_value": 42}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "invalid;;;type")

	_, err := ReadRequestJSON(req)
	if err.Code != catena.BAD_REQUEST {
		t.Fatal("expected error for malformed Content-Type")
	}
	if err.Error != "invalid content type: invalid;;;type" {
		t.Errorf("expected 'invalid content type' error, got: %v", err)
	}
}

func TestReadRequestJSON_InvalidJSON(t *testing.T) {
	body := `{not valid json}`
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(body))
	req.Header.Set("Content-Type", "application/json")

	_, err := ReadRequestJSON(req)
	if err.Code != catena.BAD_REQUEST {
		t.Fatal("expected BAD_REQUEST error for invalid JSON")
	}
	if !strings.Contains(err.Error, "failed to unmarshal request body") {
		t.Errorf("expected error to contain 'failed to unmarshal request body', got: %v", err.Error)
	}
}

func TestReadRequestJSON_EmptyBody(t *testing.T) {
	req := httptest.NewRequest(http.MethodPost, "/", strings.NewReader(""))
	req.Header.Set("Content-Type", "application/json")

	_, err := ReadRequestJSON(req)
	if err.Code != catena.BAD_REQUEST {
		t.Fatal("expected BAD_REQUEST error for empty body")
	}
	if !strings.Contains(err.Error, "failed to unmarshal request body") {
		t.Errorf("expected error to contain 'failed to unmarshal request body', got: %v", err.Error)
	}
}

func TestWriteResponseJSON_ValidValue(t *testing.T) {
	w := httptest.NewRecorder()
	val := &protos.Value{
		Kind: &protos.Value_Int32Value{Int32Value: 42},
	}

	err := WriteResponseJSON(w, val, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if w.Code != http.StatusOK {
		t.Errorf("expected status 200, got %d", w.Code)
	}
	if ct := w.Header().Get("Content-Type"); ct != "application/json" {
		t.Errorf("expected Content-Type application/json, got %q", ct)
	}
	if !strings.Contains(w.Body.String(), "int32_value") {
		t.Errorf("expected response to contain 'int32_value', got %q", w.Body.String())
	}
}

func TestWriteResponseJSON_NilValue(t *testing.T) {
	w := httptest.NewRecorder()

	err := WriteResponseJSON(w, nil, http.StatusNoContent)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if w.Code != http.StatusNoContent {
		t.Errorf("expected status 204, got %d", w.Code)
	}
	if w.Body.Len() != 0 {
		t.Errorf("expected empty body, got %q", w.Body.String())
	}
}

func TestWriteResponseJSON_StringValue(t *testing.T) {
	w := httptest.NewRecorder()
	val := &protos.Value{
		Kind: &protos.Value_StringValue{StringValue: "test string"},
	}

	err := WriteResponseJSON(w, val, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if !strings.Contains(w.Body.String(), "test string") {
		t.Errorf("expected response to contain 'test string', got %q", w.Body.String())
	}
}

func TestWriteResponseJSON_Float32Value(t *testing.T) {
	w := httptest.NewRecorder()
	val := &protos.Value{
		Kind: &protos.Value_Float32Value{Float32Value: 3.14159},
	}

	err := WriteResponseJSON(w, val, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if !strings.Contains(w.Body.String(), "float32_value") {
		t.Errorf("expected response to contain 'float32_value', got %q", w.Body.String())
	}
}

func TestWriteResponseJSON_DifferentStatusCodes(t *testing.T) {
	tests := []struct {
		name       string
		statusCode int
	}{
		{"OK", http.StatusOK},
		{"Created", http.StatusCreated},
		{"Accepted", http.StatusAccepted},
		{"BadRequest", http.StatusBadRequest},
		{"NotFound", http.StatusNotFound},
		{"InternalServerError", http.StatusInternalServerError},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			w := httptest.NewRecorder()
			val := &protos.Value{
				Kind: &protos.Value_Int32Value{Int32Value: 1},
			}

			err := WriteResponseJSON(w, val, tt.statusCode)
			if err != nil {
				t.Fatalf("unexpected error: %v", err)
			}

			if w.Code != tt.statusCode {
				t.Errorf("expected status %d, got %d", tt.statusCode, w.Code)
			}
		})
	}
}

func TestWriteResponseJSON_BoolValue(t *testing.T) {
	w := httptest.NewRecorder()
	val := &protos.Value{
		Kind: &protos.Value_Int32Value{Int32Value: 1}, // bool represented as int
	}

	err := WriteResponseJSON(w, val, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if w.Code != http.StatusOK {
		t.Errorf("expected status 200, got %d", w.Code)
	}
}

// errorWriter is a mock writer that always returns an error
type errorWriter struct {
	header http.Header
	code   int
}

func (e *errorWriter) Header() http.Header {
	if e.header == nil {
		e.header = make(http.Header)
	}
	return e.header
}

func (e *errorWriter) Write(p []byte) (int, error) {
	return 0, bytes.ErrTooLarge
}

func (e *errorWriter) WriteHeader(code int) {
	e.code = code
}

func TestWriteResponseJSON_WriteError(t *testing.T) {
	w := &errorWriter{}
	val := &protos.Value{
		Kind: &protos.Value_Int32Value{Int32Value: 42},
	}

	err := WriteResponseJSON(w, val, http.StatusOK)
	if err == nil {
		t.Fatal("expected error when Write fails")
	}
}
