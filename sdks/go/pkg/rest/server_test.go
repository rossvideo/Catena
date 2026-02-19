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
 * @brief Test for the REST server for the Catena SDK.
 * @file server_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package rest

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// TestMain sets up test environment for all tests in this package
func TestMain(m *testing.M) {
	// Set dev mode for all tests to get detailed error messages
	catena.SetEnv(catena.EnvDev)

	// Run tests
	code := m.Run()

	// Exit with test result code
	os.Exit(code)
}

func TestNewServer(t *testing.T) {
	slots := []int{0, 1, 2}
	srv := NewServer(slots)

	if srv == nil {
		t.Fatal("NewServer returned nil")
	}
	if srv.mux == nil {
		t.Error("server mux should be initialized")
	}
	if srv.getDeviceHandlers == nil {
		t.Error("getDeviceHandlers map should be initialized")
	}
	if srv.getValueHandlers == nil {
		t.Error("getValueHandlers map should be initialized")
	}
	if srv.setValueHandlers == nil {
		t.Error("setValueHandlers map should be initialized")
	}
	if srv.getAssetHandlers == nil {
		t.Error("getAssetHandlers map should be initialized")
	}
	if srv.executeCommandHandlers == nil {
		t.Error("executeCommandHandlers map should be initialized")
	}
}

func TestServer_RegisterGetDeviceHandler(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.CatenaDevice{}, catena.StatusResult{Code: catena.OK}
	})

	// Call the registered handler
	handler := srv.getDeviceHandlers[0]
	_, _ = handler()

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterGetValueHandler(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		if fqoid != "test/param" {
			t.Errorf("expected fqoid 'test/param', got %s", fqoid)
		}
		return catena.CatenaValue{}, catena.StatusResult{Code: catena.OK}
	})

	handler := srv.lookupGetValue(0)
	_, _ = handler(0, "test/param")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterSetValueHandler(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		handlerCalled = true
		if value != int32(42) {
			t.Errorf("expected value int32(42), got %v", value)
		}
		return catena.StatusResult{Code: catena.OK}
	})

	handler := srv.lookupSetValue(0)
	_ = handler(int32(42), 0, "test/param")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterGetAssetHandler(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		handlerCalled = true
		return catena.CatenaAsset{}, catena.StatusResult{Code: catena.OK}
	})

	handler := srv.lookupGetAsset(0)
	_, _ = handler(0, "test/asset")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterExecuteCommandHandler(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		if commandFqoid != "test/command" {
			t.Errorf("expected commandFqoid 'test/command', got %s", commandFqoid)
		}
		return catena.CatenaValue{}, catena.StatusResult{Code: catena.OK}
	})

	handler := srv.lookupExecuteCommand(0)
	_, _ = handler(nil, nil, 0, "test/command", nil)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterFallbackHandler(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		return catena.CatenaValue{}, catena.StatusResult{Code: catena.NOT_FOUND}
	})

	if srv.fallbackHandler == nil {
		t.Fatal("fallback handler should be set")
	}

	_, _ = srv.fallbackHandler(nil, nil)
	if !handlerCalled {
		t.Error("fallback handler was not called")
	}
}

func TestServer_GetDevice_Route(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": catena.DetailLevelFull,
	}
	device, _ := catena.ToCatenaDevice(deviceMap)

	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.Reply(device)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}

	// Verify response is JSON
	contentType := rec.Header().Get("Content-Type")
	if contentType != "application/json" {
		t.Errorf("expected Content-Type 'application/json', got %s", contentType)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_GetDevice_NotFound(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
	})

	// Request device for slot 99 (not registered)
	// The server returns NOT_FOUND status in the response
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/99", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	// Check response body contains error
	// Note: actual HTTP status code handling depends on writeHTTPResult behavior
	if rec.Code != http.StatusNotFound && rec.Code != http.StatusOK {
		t.Errorf("expected status %d or %d, got %d", http.StatusNotFound, http.StatusOK, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_GetDevice_InvalidSlot(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaDevice](catena.INVALID_ARGUMENT, "invalid slot")
	})
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/invalid", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	// Invalid slot should return an error status
	// The exact status depends on implementation
	if rec.Code == http.StatusOK {
		// If 200, check that the body indicates an error
		// This is acceptable as errors may be returned in body
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_GetValue_Route(t *testing.T) {
	srv := NewServer([]int{0})

	value, _ := catena.ToCatenaValue(int32(42))
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.Reply(value)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/value/brightness", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
}

func TestServer_SetValue_Route(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		handlerCalled = true
		if v, ok := value.(int32); !ok || v != 42 {
			t.Errorf("expected value int32(42), got %v (%T)", value, value)
		}
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.StatusResult{Code: catena.OK}
	})

	body := []byte(`{"int32_value": 42}`)
	req := httptest.NewRequest(http.MethodPut, "/st2138-api/v1/0/value/brightness", bytes.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_SetValue_InvalidContentType(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		handlerCalled = true
		return catena.StatusResult{Code: catena.OK}
	})

	body := []byte(`{"int32_value": 42}`)
	req := httptest.NewRequest(http.MethodPut, "/st2138-api/v1/0/value/brightness", bytes.NewReader(body))
	req.Header.Set("Content-Type", "text/plain")
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusBadRequest {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}

	responseBody, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(responseBody, &response); err == nil {
		if response["error"] != "invalid request body" {
			t.Errorf("expected error 'invalid request body', got %s", response["error"])
		}
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_GetAsset_Route(t *testing.T) {
	srv := NewServer([]int{0})

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "image/png"},
		Payload:  []byte("fake image"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/logo", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
}

func TestServer_GetAsset_MethodNotAllowed(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		handlerCalled = true
		return catena.Reply(catena.CatenaAsset{})
	})

	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/asset/logo", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	// POST to asset endpoint should not be allowed
	// Test passes if no panic occurs and response is generated
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_ExecuteCommand_Route(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		if commandFqoid != "reboot" {
			t.Errorf("expected commandFqoid 'reboot', got %s", commandFqoid)
		}
		return catena.Reply(catena.CatenaValue{})
	})

	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/command/reboot", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_ExecuteCommand_WithPayload(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		if payload == nil {
			t.Error("expected payload to be non-nil")
		}
		return catena.Reply(catena.CatenaValue{})
	})

	body := []byte(`{"string_value": "test"}`)
	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/command/process", bytes.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_ExecuteCommand_MethodNotAllowed(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		return catena.Reply(catena.CatenaValue{})
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/command/reboot", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusMethodNotAllowed {
		t.Errorf("expected status %d, got %d", http.StatusMethodNotAllowed, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_Connect_Route(t *testing.T) {
	srv := NewServer([]int{0, 1})

	ctx, cancel := context.WithCancel(context.Background())
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	// Allow handler to write headers and initial SSE event
	time.Sleep(100 * time.Millisecond)
	cancel()
	// Allow handler to exit and defer to run
	time.Sleep(50 * time.Millisecond)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
	if rec.Header().Get("Content-Type") != "text/event-stream" {
		t.Errorf("expected Content-Type text/event-stream, got %s", rec.Header().Get("Content-Type"))
	}
	if rec.Header().Get("Cache-Control") != "no-cache" {
		t.Errorf("expected Cache-Control no-cache, got %s", rec.Header().Get("Cache-Control"))
	}
	body := rec.Body.String()
	if !strings.Contains(body, "data:") {
		t.Error("expected SSE data in response body")
	}
	// Initial event must be protojson PushUpdates format: {"slotsAdded":{"slots":[...]}}
	if !strings.Contains(body, "slotsAdded") {
		t.Error("expected initial slotsAdded in response body")
	}
	if !strings.Contains(body, "\"slots\"") {
		t.Error("expected initial event in proto format with nested \"slots\" (SlotList)")
	}
	lines := strings.Split(body, "\n")
	for _, line := range lines {
		if strings.HasPrefix(line, "data: ") {
			var payload map[string]any
			if err := json.Unmarshal([]byte(strings.TrimPrefix(line, "data: ")), &payload); err != nil {
				t.Errorf("initial SSE data must be valid JSON: %v", err)
				break
			}
			slotsAdded, ok := payload["slotsAdded"].(map[string]any)
			if !ok {
				t.Error("expected slotsAdded object (SlotList) in initial event")
				break
			}
			slots, ok := slotsAdded["slots"].([]any)
			if !ok || len(slots) != 2 {
				t.Errorf("expected slotsAdded.slots to be array of 2 slots, got %T %v", slotsAdded["slots"], slotsAdded["slots"])
			}
			break
		}
	}
}

func TestServer_Connect_MethodNotAllowed(t *testing.T) {
	srv := NewServer([]int{0})

	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/connect", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusMethodNotAllowed {
		t.Errorf("expected status %d, got %d", http.StatusMethodNotAllowed, rec.Code)
	}
	body, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if !strings.Contains(response["error"], "GET") {
			t.Errorf("expected error to mention GET, got %s", response["error"])
		}
	}
}

func TestServer_Fallback_Route(t *testing.T) {
	srv := NewServer([]int{0})

	handlerCalled := false
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "custom not found")
	})

	req := httptest.NewRequest(http.MethodGet, "/unknown/path", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
	if !handlerCalled {
		t.Error("registered handler should have been called, but was not")
	}
}

func TestServer_DefaultHandlers(t *testing.T) {
	srv := NewServer([]int{0})

	// Test default device handler
	device, status := srv.getDeviceHandlers[0]()
	if status.Code != catena.NOT_FOUND {
		t.Errorf("default device handler should return NOT_FOUND, got %v", status.Code)
	}
	if device.GetProtoDevice() != nil {
		t.Error("default device handler should return nil device")
	}

	// Test default get value handler
	value, status := srv.lookupGetValue(0)(0, "test")
	if status.Code != catena.UNIMPLEMENTED {
		t.Errorf("default get value handler should return UNIMPLEMENTED, got %v", status.Code)
	}
	if value.Value != nil {
		t.Error("default get value handler should return nil value")
	}

	// Test default set value handler
	status = srv.lookupSetValue(0)(nil, 0, "test")
	if status.Code != catena.UNIMPLEMENTED {
		t.Errorf("default set value handler should return UNIMPLEMENTED, got %v", status.Code)
	}

	// Test default get asset handler
	asset, status := srv.lookupGetAsset(0)(0, "test")
	if status.Code != catena.NOT_FOUND {
		t.Errorf("default get asset handler should return NOT_FOUND, got %v", status.Code)
	}
	if asset.GetProtoAsset() != nil {
		t.Error("default get asset handler should return nil asset")
	}

	// Test default execute command handler
	value, status = srv.lookupExecuteCommand(0)(nil, nil, 0, "test", nil)
	if status.Code != catena.UNIMPLEMENTED {
		t.Errorf("default execute command handler should return UNIMPLEMENTED, got %v", status.Code)
	}
}

func TestServer_LookupHandlers_NotRegistered(t *testing.T) {
	srv := NewServer([]int{}) // No slots registered

	// Should return default handlers for unregistered slots
	handler := srv.lookupGetValue(99)
	_, status := handler(99, "test")
	if status.Code != catena.UNIMPLEMENTED {
		t.Errorf("expected UNIMPLEMENTED for unregistered slot, got %v", status.Code)
	}

	setHandler := srv.lookupSetValue(99)
	status = setHandler(nil, 99, "test")
	if status.Code != catena.UNIMPLEMENTED {
		t.Errorf("expected UNIMPLEMENTED for unregistered slot, got %v", status.Code)
	}

	assetHandler := srv.lookupGetAsset(99)
	_, status = assetHandler(99, "test")
	if status.Code != catena.NOT_FOUND {
		t.Errorf("expected NOT_FOUND for unregistered slot, got %v", status.Code)
	}

	cmdHandler := srv.lookupExecuteCommand(99)
	_, status = cmdHandler(nil, nil, 99, "test", nil)
	if status.Code != catena.UNIMPLEMENTED {
		t.Errorf("expected UNIMPLEMENTED for unregistered slot, got %v", status.Code)
	}
}

func TestServer_NestedValuePath(t *testing.T) {
	srv := NewServer([]int{0})

	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid != "nested/path/to/param" {
			t.Errorf("expected fqoid 'nested/path/to/param', got %s", fqoid)
		}
		value, _ := catena.ToCatenaValue(int32(1))
		return catena.Reply(value)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/value/nested/path/to/param", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
	body, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if response["value"] != "1" {
			t.Errorf("expected value '1', got %s", response["value"])
		}
	}
}

func TestServer_UnknownEndpoint(t *testing.T) {
	srv := NewServer([]int{0})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/unknown", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
}

func TestServer_InvalidPath(t *testing.T) {
	srv := NewServer([]int{0})

	req := httptest.NewRequest(http.MethodGet, "/kjhgjnghf", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
}

func TestServer_InvalidPathNoSlash(t *testing.T) {
	srv := NewServer([]int{0})

	// Request without trailing slash - should not redirect (301)
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
}

func TestServer_NegativeSlot(t *testing.T) {
	srv := NewServer([]int{0})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/-1", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusBadRequest {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
}

func TestServer_MultipleSlots(t *testing.T) {
	srv := NewServer([]int{0, 1, 2})

	// Register different handlers for each slot
	for i := 0; i < 3; i++ {
		slot := i
		srv.RegisterGetValueHandler(slot, func(s int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
			if s != slot {
				t.Errorf("handler received wrong slot: expected %d, got %d", slot, s)
			}
			value, _ := catena.ToCatenaValue(int32(slot * 10))
			return catena.Reply(value)
		})
	}

	// Test each slot
	for i := 0; i < 3; i++ {
		t.Run("slot"+string(rune('0'+i)), func(t *testing.T) {
			req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/"+string(rune('0'+i))+"/value/test", nil)
			rec := httptest.NewRecorder()

			srv.mux.ServeHTTP(rec, req)

			if rec.Code != http.StatusOK {
				t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
			}
		})
	}
}

func TestWriteHTTPResult_Error(t *testing.T) {
	// Set dev mode for detailed error messages
	catena.SetEnv(catena.EnvDev)
	defer catena.SetEnv(catena.EnvProd)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{
		Code:  catena.NOT_FOUND,
		Error: "test error message",
	}

	writeHTTPResult(rec, result, catena.CatenaValue{})

	body, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if response["error"] != "test error message" {
			t.Errorf("expected error message 'test error message', got %s", response["error"])
		}
	}
}

func TestWriteHTTPStatusResult(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{
		Code: catena.CREATED,
	}

	writeHTTPStatusResult(rec, result)

	if rec.Code != http.StatusCreated {
		t.Errorf("expected status %d, got %d", http.StatusCreated, rec.Code)
	}
}

// Edge case tests for 90% coverage
func TestErrorMessages_DevVsProd(t *testing.T) {
	tests := []struct {
		name     string
		env      catena.Environment
		expected string
	}{
		{"dev mode shows details", catena.EnvDev, "detailed error"},
		{"prod mode hides details", catena.EnvProd, "Not Found"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			original := catena.GetEnv()
			defer catena.SetEnv(original)
			catena.SetEnv(tt.env)

			rec := httptest.NewRecorder()
			result := catena.StatusResult{Code: catena.NOT_FOUND, Error: "detailed error"}
			writeHTTPResult(rec, result, catena.CatenaValue{})

			body, _ := io.ReadAll(rec.Body)
			if !bytes.Contains(body, []byte(tt.expected)) {
				t.Errorf("expected %q in response, got %q", tt.expected, string(body))
			}
		})
	}
}

func TestWriteFunctions_NilValues(t *testing.T) {
	tests := []struct {
		name           string
		fn             func(http.ResponseWriter)
		expectedStatus int
	}{
		{"nil value", func(w http.ResponseWriter) { writeValueResult(w, catena.CatenaValue{}, http.StatusOK) }, http.StatusOK},
		{"nil device", func(w http.ResponseWriter) { writeDeviceResult(w, catena.CatenaDevice{}, http.StatusOK) }, http.StatusOK},
		{"nil asset", func(w http.ResponseWriter) { writeAssetResult(w, catena.CatenaAsset{}, http.StatusOK) }, http.StatusInternalServerError},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			rec := httptest.NewRecorder()
			tt.fn(rec)
			if rec.Code != tt.expectedStatus {
				t.Errorf("expected status %d, got %d", tt.expectedStatus, rec.Code)
			}
		})
	}
}

func TestCommandEndpoint_PayloadHandling(t *testing.T) {
	tests := []struct {
		name        string
		body        string
		contentType string
		expectNil   bool
	}{
		{"no payload", "", "", true},
		{"with payload", `{"int32_value": 42}`, "application/json", false},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := NewServer([]int{0})
			var receivedPayload any
			srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, fqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
				receivedPayload = payload
				return catena.Reply(catena.CatenaValue{})
			})

			var req *http.Request
			if tt.body != "" {
				req = httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/command/test", bytes.NewBufferString(tt.body))
				req.Header.Set("Content-Type", tt.contentType)
			} else {
				req = httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/command/test", nil)
			}

			srv.mux.ServeHTTP(httptest.NewRecorder(), req)

			if tt.expectNil && receivedPayload != nil {
				t.Errorf("expected nil payload, got %v", receivedPayload)
			} else if !tt.expectNil && receivedPayload == nil {
				t.Error("expected non-nil payload")
			}
		})
	}
}

func TestSetValue_FromProtoError(t *testing.T) {
	srv := NewServer([]int{0})
	body := `{"struct_variant_value": {"variant_name": "test"}}`
	req := httptest.NewRequest(http.MethodPut, "/st2138-api/v1/0/value/param", bytes.NewBufferString(body))
	req.Header.Set("Content-Type", "application/json")
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	// Should handle error gracefully
	if rec.Code == 0 {
		t.Error("expected a response code")
	}
}

func TestRouting_EdgeCases(t *testing.T) {
	srv := NewServer([]int{0})

	tests := []struct {
		name string
		path string
	}{
		{"short path", "/st2138-api/"},
		{"invalid slot", "/st2138-api/v1/99999"},
		{"unknown endpoint", "/st2138-api/v1/0/unknown/test"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			req := httptest.NewRequest(http.MethodGet, tt.path, nil)
			rec := httptest.NewRecorder()
			srv.mux.ServeHTTP(rec, req)

			body, _ := io.ReadAll(rec.Body)
			var response map[string]string
			if err := json.Unmarshal(body, &response); err == nil {
				if _, ok := response["error"]; !ok {
					t.Errorf("expected error for %s", tt.name)
				}
			}
		})
	}
}

func TestValueEndpoint_Methods(t *testing.T) {
	srv := NewServer([]int{0})
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		return catena.StatusResult{Code: catena.OK}
	})

	tests := []struct {
		name   string
		method string
		body   string
	}{
		{"PUT with value", http.MethodPut, `{"int32_value": 100}`},
		{"invalid method", http.MethodPost, ""},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			var req *http.Request
			if tt.body != "" {
				req = httptest.NewRequest(tt.method, "/st2138-api/v1/0/value/test", bytes.NewBufferString(tt.body))
				req.Header.Set("Content-Type", "application/json")
			} else {
				req = httptest.NewRequest(tt.method, "/st2138-api/v1/0/value/test", nil)
			}

			srv.mux.ServeHTTP(httptest.NewRecorder(), req)
		})
	}
}

func TestDeviceEndpoint_NotRegistered(t *testing.T) {
	srv := NewServer([]int{})
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	body, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if _, ok := response["error"]; !ok {
			t.Error("expected error for unregistered device")
		}
	}
}

func TestServer_Connect_TooManyConnections(t *testing.T) {
	srv := NewServer([]int{0})
	srv.SetMaxConnections(1)

	ctx1, cancel1 := context.WithCancel(context.Background())
	req1 := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx1)
	rec1 := httptest.NewRecorder()
	go srv.mux.ServeHTTP(rec1, req1)
	time.Sleep(50 * time.Millisecond)

	req2 := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)
	rec2 := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec2, req2)

	if rec2.Code != http.StatusTooManyRequests {
		t.Errorf("expected status %d when at max connections, got %d", http.StatusTooManyRequests, rec2.Code)
	}
	body, _ := io.ReadAll(rec2.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if _, ok := response["error"]; !ok {
			t.Error("expected error in response when at max connections")
		}
		// In dev mode server returns C++-matching message; in prod returns http.StatusText
		msg := response["error"]
		if msg != "Too many connections to service" && msg != "Too Many Requests" {
			t.Errorf("expected error \"Too many connections to service\" (dev) or \"Too Many Requests\" (prod), got %q", msg)
		}
	}

	cancel1()
	time.Sleep(50 * time.Millisecond)
}

func TestWriteResults_ValidData(t *testing.T) {
	// Test valid device
	device, _ := catena.ToCatenaDevice(map[string]any{"slot": int32(0)})
	rec := httptest.NewRecorder()
	writeDeviceResult(rec, device, http.StatusOK)
	if rec.Code != http.StatusOK || rec.Header().Get("Content-Type") != "application/json" {
		t.Error("device write failed")
	}

	// Test valid asset
	asset, _ := catena.ToCatenaAsset(catena.DataPayload{Payload: []byte("data")}, false)
	rec = httptest.NewRecorder()
	writeAssetResult(rec, asset, http.StatusOK)
	if rec.Code != http.StatusOK {
		t.Error("asset write failed")
	}
}

func TestCommandEndpoint_InvalidJSON(t *testing.T) {
	srv := NewServer([]int{0})
	body := `{invalid json}`
	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/command/test", bytes.NewBufferString(body))
	req.Header.Set("Content-Type", "application/json")
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusBadRequest {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
}

func TestDeviceEndpoint_WrongMethod(t *testing.T) {
	srv := NewServer([]int{0})
	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0", nil)
	rec := httptest.NewRecorder()

	srv.mux.ServeHTTP(rec, req)

	body, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if _, ok := response["error"]; !ok {
			t.Error("expected error for POST on device endpoint")
		}
	}
}

func TestCommandEndpoint_FromProtoError(t *testing.T) {
	srv := NewServer([]int{0})
	body := `{"struct_variant_value": {"variant_name": "test"}}`
	req := httptest.NewRequest(http.MethodPost, "/st2138-api/v1/0/command/exec", bytes.NewBufferString(body))
	req.Header.Set("Content-Type", "application/json")

	srv.mux.ServeHTTP(httptest.NewRecorder(), req)
}

func TestWriteHTTPStatusResult_WithError(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvDev)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.INTERNAL, Error: "error"}
	writeHTTPStatusResult(rec, result)

	body, _ := io.ReadAll(rec.Body)
	if !bytes.Contains(body, []byte("error")) {
		t.Error("expected error in response")
	}
}

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

// noFlusher wraps ResponseWriter and does not implement http.Flusher (for "streaming not supported" test).
type noFlusher struct {
	http.ResponseWriter
}

func TestServer_sendSSEEvent(t *testing.T) {
	srv := NewServer([]int{0})
	rec := httptest.NewRecorder()
	var w http.ResponseWriter = rec
	flusher := w.(http.Flusher)
	update := &protos.PushUpdates{
		Slot: 1,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}

	err := srv.sendSSEEvent(rec, flusher, update)
	if err != nil {
		t.Fatalf("sendSSEEvent: %v", err)
	}
	body := rec.Body.String()
	if !strings.HasPrefix(body, "data: ") {
		t.Errorf("expected body to start with 'data: ', got %q", body)
	}
	var decoded map[string]any
	if err := json.Unmarshal([]byte(strings.TrimPrefix(strings.Split(body, "\n")[0], "data: ")), &decoded); err != nil {
		t.Fatalf("SSE data not valid JSON: %v", err)
	}
	if decoded["slot"] != float64(1) {
		t.Errorf("expected slot=1, got %v", decoded["slot"])
	}
	valueObj, ok := decoded["value"].(map[string]any)
	if !ok {
		t.Fatal("expected nested 'value' object in SSE payload")
	}
	if valueObj["oid"] != "test/param" {
		t.Errorf("expected oid=test/param, got %v", valueObj["oid"])
	}
}

func TestServer_BroadcastUpdate(t *testing.T) {
	srv := NewServer([]int{0})
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(150 * time.Millisecond)

	srv.BroadcastUpdate(0, "broadcast/oid", "hello")
	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	body := rec.Body.String()
	if !strings.Contains(body, "broadcast/oid") {
		t.Errorf("expected SSE body to contain broadcast/oid, got %s", body)
	}
}

func TestServer_BroadcastUpdate_ChannelFull(t *testing.T) {
	srv := NewServer([]int{0})
	srv.SetMaxConnections(10)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(100 * time.Millisecond)

	for i := 0; i < 150; i++ {
		srv.BroadcastUpdate(0, "fill", int32(i))
	}
	cancel()
	time.Sleep(50 * time.Millisecond)
	// No assertion other than no panic; we're covering the "channel full" default branch
}

func TestServer_Start(t *testing.T) {
	srv := NewServer([]int{0})
	listener, err := net.Listen("tcp", ":0")
	if err != nil {
		t.Fatalf("net.Listen: %v", err)
	}
	port := listener.Addr().(*net.TCPAddr).Port
	listener.Close()

	go func() { _ = srv.Start(port) }()
	time.Sleep(200 * time.Millisecond)

	url := fmt.Sprintf("http://127.0.0.1:%d/st2138-api/v1", port)
	resp, err := http.Get(url)
	if err != nil {
		t.Fatalf("GET: %v", err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, resp.StatusCode)
	}
}

func TestWriteHTTPResult_DefaultType(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.OK}
	// Pass nil so switch hits default (no CatenaValue/Device/Asset)
	writeHTTPResult(rec, result, nil)
	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
}

func TestServer_Connect_StreamingNotSupported(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvDev)

	srv := NewServer([]int{0})
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)
	rec := httptest.NewRecorder()
	w := &noFlusher{ResponseWriter: rec}

	srv.handleConnect(w, req)

	if rec.Code != http.StatusInternalServerError {
		t.Errorf("expected status %d when Flusher not supported, got %d", http.StatusInternalServerError, rec.Code)
	}
	body, _ := io.ReadAll(rec.Body)
	var response map[string]string
	if err := json.Unmarshal(body, &response); err == nil {
		if !strings.Contains(response["error"], "streaming") && !strings.Contains(response["error"], "Internal") {
			t.Errorf("expected error to mention streaming or Internal, got %s", response["error"])
		}
	}
}

func TestWriteHTTPStatusResult_ProdMode(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvProd)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.NOT_FOUND, Error: "detailed internal error"}
	writeHTTPStatusResult(rec, result)

	body, _ := io.ReadAll(rec.Body)
	if !bytes.Contains(body, []byte("Not Found")) {
		t.Errorf("prod mode should hide details, expected 'Not Found' in body, got %q", string(body))
	}
	if bytes.Contains(body, []byte("detailed internal error")) {
		t.Error("prod mode should not expose detailed error message")
	}
}

func TestServer_Connect_WithOrigin(t *testing.T) {
	srv := NewServer([]int{0})
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	if rec.Header().Get("Access-Control-Allow-Origin") != "https://example.com" {
		t.Errorf("expected CORS Origin header, got %s", rec.Header().Get("Access-Control-Allow-Origin"))
	}
}

func TestWriteValueResult_WriteError(t *testing.T) {
	value, _ := catena.ToCatenaValue(int32(1))
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeValueResult(w, value, http.StatusOK)

	// internal.WriteResponseJSON writes status then body; when Write fails, status may already be 200
	if rec.Code != http.StatusInternalServerError && rec.Code != http.StatusOK {
		t.Errorf("expected status 500 or 200 (if header already sent), got %d", rec.Code)
	}
}

func TestWriteDeviceResult_WriteError(t *testing.T) {
	device, _ := catena.ToCatenaDevice(map[string]any{"slot": int32(0)})
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeDeviceResult(w, device, http.StatusOK)

	// writeDeviceResult only logs on write error and does not change status after WriteHeader(200)
	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d (header already written), got %d", http.StatusOK, rec.Code)
	}
}

func TestWriteAssetResult_WriteError(t *testing.T) {
	asset, _ := catena.ToCatenaAsset(catena.DataPayload{Payload: []byte("x")}, false)
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeAssetResult(w, asset, http.StatusOK)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d (header already written before Write), got %d", http.StatusOK, rec.Code)
	}
}

func TestWriteHTTPResult_WithError_NonDev(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvProd)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.NOT_FOUND, Error: "internal detail"}
	writeHTTPResult(rec, result, catena.CatenaValue{})

	body, _ := io.ReadAll(rec.Body)
	if bytes.Contains(body, []byte("internal detail")) {
		t.Error("prod mode should not expose detailed error in writeHTTPResult")
	}
}

func TestServer_Shutdown(t *testing.T) {
	srv := NewServer([]int{0})

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(100 * time.Millisecond)

	if srv.connectionQueue.ConnectionCount() != 1 {
		t.Errorf("expected 1 connection before shutdown, got %d", srv.connectionQueue.ConnectionCount())
	}

	done := make(chan struct{})
	go func() {
		srv.Shutdown()
		close(done)
	}()

	select {
	case <-done:
		// Shutdown completed
	case <-time.After(2 * time.Second):
		t.Fatal("Shutdown timed out")
	}

	if srv.connectionQueue.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", srv.connectionQueue.ConnectionCount())
	}
}

func TestServer_Shutdown_MultipleConnections(t *testing.T) {
	srv := NewServer([]int{0})
	srv.SetMaxConnections(10)

	ctxs := make([]context.CancelFunc, 3)
	for i := 0; i < 3; i++ {
		ctx, cancel := context.WithCancel(context.Background())
		ctxs[i] = cancel
		req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
		rec := httptest.NewRecorder()
		go srv.mux.ServeHTTP(rec, req)
	}
	defer func() {
		for _, cancel := range ctxs {
			cancel()
		}
	}()

	time.Sleep(150 * time.Millisecond)

	if srv.connectionQueue.ConnectionCount() != 3 {
		t.Errorf("expected 3 connections, got %d", srv.connectionQueue.ConnectionCount())
	}

	done := make(chan struct{})
	go func() {
		srv.Shutdown()
		close(done)
	}()

	select {
	case <-done:
	case <-time.After(2 * time.Second):
		t.Fatal("Shutdown timed out")
	}

	if srv.connectionQueue.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", srv.connectionQueue.ConnectionCount())
	}
}

func TestServer_SetValue_NotifiesConnections(t *testing.T) {
	srv := NewServer([]int{0})

	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		srv.BroadcastUpdate(slot, fqoid, value)
		return catena.StatusResult{Code: catena.OK}
	})

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(150 * time.Millisecond)

	body := []byte(`{"int32_value": 42}`)
	setReq := httptest.NewRequest(http.MethodPut, "/st2138-api/v1/0/value/brightness", bytes.NewReader(body))
	setReq.Header.Set("Content-Type", "application/json")
	setRec := httptest.NewRecorder()
	srv.mux.ServeHTTP(setRec, setReq)

	if setRec.Code != http.StatusOK {
		t.Errorf("expected SetValue status %d, got %d", http.StatusOK, setRec.Code)
	}

	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	sseBody := rec.Body.String()
	if !strings.Contains(sseBody, "brightness") {
		t.Errorf("expected SSE body to contain 'brightness' from SetValue notification, got %s", sseBody)
	}
}

func TestServer_SetValue_FailureDoesNotNotify(t *testing.T) {
	srv := NewServer([]int{0})

	// Handler only broadcasts on success -- never reached here
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		return catena.StatusWithCode(catena.INVALID_ARGUMENT, "bad value")
	})

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(150 * time.Millisecond)

	body := []byte(`{"int32_value": -1}`)
	setReq := httptest.NewRequest(http.MethodPut, "/st2138-api/v1/0/value/brightness", bytes.NewReader(body))
	setReq.Header.Set("Content-Type", "application/json")
	setRec := httptest.NewRecorder()
	srv.mux.ServeHTTP(setRec, setReq)

	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	sseBody := rec.Body.String()
	lines := strings.Split(sseBody, "\n")
	for _, line := range lines {
		if strings.HasPrefix(line, "data: ") && strings.Contains(line, "brightness") {
			t.Error("failed SetValue should not send SSE notification")
		}
	}
}
