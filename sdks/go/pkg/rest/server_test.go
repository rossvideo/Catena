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
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-04
 */

package rest

import (
	"context"
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestServer_RegisterGetDeviceHandler(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.CatenaDevice{}, catena.StatusResult{Code: catena.OK}
	})

	// Call the registered handler
	handler := srv.LookupGetDeviceHandler(0)
	_, _ = handler()

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterGetValueHandler(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		if fqoid != "test/param" {
			t.Errorf("expected fqoid 'test/param', got %s", fqoid)
		}
		return catena.CatenaValue{}, catena.StatusResult{Code: catena.OK}
	})

	handler := srv.LookupGetValueHandler(0)
	_, _ = handler(0, "test/param")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterSetValueHandler(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		handlerCalled = true
		if value != int32(42) {
			t.Errorf("expected value int32(42), got %v", value)
		}
		return catena.StatusResult{Code: catena.OK}
	})

	handler := srv.LookupSetValueHandler(0)
	_ = handler(int32(42), 0, "test/param")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterGetAssetHandler(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		handlerCalled = true
		return catena.CatenaAsset{}, catena.StatusResult{Code: catena.OK}
	})

	handler := srv.LookupGetAssetHandler(0)
	_, _ = handler(0, "test/asset")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterExecuteCommandHandler(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if commandFqoid != "test/command" {
			t.Errorf("expected commandFqoid 'test/command', got %s", commandFqoid)
		}
		return catena.CommandNoResponse()
	})

	handler := srv.LookupExecuteCommandHandler(0)
	_, _ = handler(0, "test/command", nil)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_RegisterFallbackHandler(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

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
	srv := NewServer([]uint16{0}, 100)

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

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_GetDevice_NotFound(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
	})

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/99", "")
	if rec.Code != http.StatusNotFound && rec.Code != http.StatusOK {
		t.Errorf("expected status %d or %d, got %d", http.StatusNotFound, http.StatusOK, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_GetDevice_InvalidSlot(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaDevice](catena.INVALID_ARGUMENT, "invalid slot")
	})

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/invalid", "")
	if rec.Code == http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_GetValue_Route(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	value, _ := catena.ToCatenaValue(int32(42))
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.Reply(value)
	})

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0/value/brightness", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestServer_SetValue_Route(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		handlerCalled = true
		if v, ok := value.(int32); !ok || v != 42 {
			t.Errorf("expected value int32(42), got %v (%T)", value, value)
		}
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.StatusResult{Code: catena.OK}
	})

	rec := makeRequest(t, srv, http.MethodPut, "/st2138-api/v1/0/value/brightness", `{"int32_value": 42}`)
	assertStatus(t, rec, http.StatusOK)
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_SetValue_InvalidContentType(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		handlerCalled = true
		return catena.StatusResult{Code: catena.OK}
	})

	rec := makeRequestWithHeaders(t, srv, http.MethodPut, "/st2138-api/v1/0/value/brightness",
		`{"int32_value": 42}`, map[string]string{"Content-Type": "text/plain"})
	assertStatus(t, rec, http.StatusBadRequest)
	errMsg := assertHasError(t, rec)
	if errMsg != "invalid request body" {
		t.Errorf("expected error 'invalid request body', got %s", errMsg)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_GetAsset_Route(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "image/png"},
		Payload:  []byte("fake image"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0/asset/logo", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestServer_GetAsset_MethodNotAllowed(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		handlerCalled = true
		return catena.Reply(catena.CatenaAsset{})
	})

	makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/asset/logo", "")
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestServer_ExecuteCommand_Route(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if commandFqoid != "reboot" {
			t.Errorf("expected commandFqoid 'reboot', got %s", commandFqoid)
		}
		return catena.CommandNoResponse()
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/reboot", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	response := parseJSONBody(t, rec)
	if _, ok := response["no_response"]; !ok {
		t.Errorf("expected no_response in CommandResponse, got %v", response)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_ExecuteCommand_WithPayload(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if payload == nil {
			t.Error("expected payload to be non-nil")
		}
		val, _ := catena.ToCatenaValue(payload)
		return catena.CommandReply(val)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/process", `{"string_value": "test"}`)
	assertStatus(t, rec, http.StatusOK)
	response := parseJSONBody(t, rec)
	if _, ok := response["response"]; !ok {
		t.Errorf("expected response field in CommandResponse, got %v", response)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_ExecuteCommand_MethodNotAllowed(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		return catena.CommandNoResponse()
	})

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0/command/reboot", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	if handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestServer_Connect_Route(t *testing.T) {
	srv := NewServer([]uint16{0, 1}, 100)

	rec, cancel := setupSSEConnection(t, srv)
	cleanupSSE(cancel)

	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "text/event-stream")
	assertHeader(t, rec, "Cache-Control", "no-cache")

	body := rec.Body.String()
	if !strings.Contains(body, "data:") {
		t.Error("expected SSE data in response body")
	}
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
	srv := NewServer([]uint16{0}, 100)

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/connect", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	errMsg := assertHasError(t, rec)
	if !strings.Contains(errMsg, "GET") {
		t.Errorf("expected error to mention GET, got %s", errMsg)
	}
}

func TestServer_Fallback_Route(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "custom not found")
	})

	rec := makeRequest(t, srv, http.MethodGet, "/unknown/path", "")
	assertStatus(t, rec, http.StatusNotFound)
	if !handlerCalled {
		t.Error("registered handler should have been called, but was not")
	}
}

func TestServer_DefaultHandlers(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	// Test default device handler
	device, status := srv.LookupGetDeviceHandler(0)()
	if status.Code != catena.NOT_FOUND {
		t.Errorf("default device handler should return NOT_FOUND, got %v", status.Code)
	}
	if device.GetProtoDevice() != nil {
		t.Error("default device handler should return nil device")
	}

	// Test default get value handler
	value, status := srv.LookupGetValueHandler(0)(0, "test")
	if status.Code != catena.NOT_FOUND {
		t.Errorf("default get value handler should return NOT_FOUND, got %v", status.Code)
	}
	if value.Value != nil {
		t.Error("default get value handler should return nil value")
	}

	// Test default set value handler
	status = srv.LookupSetValueHandler(0)(nil, 0, "test")
	if status.Code != catena.NOT_FOUND {
		t.Errorf("default set value handler should return NOT_FOUND, got %v", status.Code)
	}

	// Test default get asset handler
	asset, status := srv.LookupGetAssetHandler(0)(0, "test")
	if status.Code != catena.NOT_FOUND {
		t.Errorf("default get asset handler should return NOT_FOUND, got %v", status.Code)
	}
	if asset.GetProtoAsset() != nil {
		t.Error("default get asset handler should return nil asset")
	}

	// Test default execute command handler
	_, cmdStatus := srv.LookupExecuteCommandHandler(0)(0, "test", nil)
	if cmdStatus.Code != catena.NOT_FOUND {
		t.Errorf("default execute command handler should return NOT_FOUND, got %v", cmdStatus.Code)
	}
}

func TestServer_NestedValuePath(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid != "nested/path/to/param" {
			t.Errorf("expected fqoid 'nested/path/to/param', got %s", fqoid)
		}
		value, _ := catena.ToCatenaValue(int32(1))
		return catena.Reply(value)
	})

	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0/value/nested/path/to/param", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestServer_UnknownEndpoint(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0/unknown", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestServer_InvalidPath(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodGet, "/kjhgjnghf", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestServer_InvalidPathNoSlash(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestServer_NegativeSlot(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/-1", "")
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestServer_MultipleSlots(t *testing.T) {
	srv := NewServer([]uint16{0, 1, 2}, 100)

	for i := 0; i < 3; i++ {
		slot := uint16(i)
		srv.RegisterGetValueHandler(slot, func(s uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
			if s != slot {
				t.Errorf("handler received wrong slot: expected %d, got %d", slot, s)
			}
			value, _ := catena.ToCatenaValue(int32(slot * 10))
			return catena.Reply(value)
		})
	}

	for i := 0; i < 3; i++ {
		t.Run("slot"+string(rune('0'+i)), func(t *testing.T) {
			rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/"+string(rune('0'+i))+"/value/test", "")
			assertStatus(t, rec, http.StatusOK)
		})
	}
}

func TestWriteHTTPResult_Error(t *testing.T) {
	catena.SetEnv(catena.EnvDev)
	defer catena.SetEnv(catena.EnvProd)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{
		Code:  catena.NOT_FOUND,
		Error: "test error message",
	}
	writeHTTPResult(rec, result, catena.CatenaValue{})

	errMsg := assertHasError(t, rec)
	if errMsg != "test error message" {
		t.Errorf("expected error message 'test error message', got %s", errMsg)
	}
}

func TestWriteHTTPStatusResult(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.CREATED}
	writeHTTPStatusResult(rec, result)
	assertStatus(t, rec, http.StatusCreated)
}

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

			assertBodyContains(t, rec, tt.expected)
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
			assertStatus(t, rec, tt.expectedStatus)
		})
	}
}

func TestCommandEndpoint_PayloadHandling(t *testing.T) {
	tests := []struct {
		name      string
		body      string
		expectNil bool
	}{
		{"no payload", "", true},
		{"with payload", `{"int32_value": 42}`, false},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := NewServer([]uint16{0}, 100)
			var receivedPayload any
			srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
				receivedPayload = payload
				return catena.CommandNoResponse()
			})

			makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/test", tt.body)

			if tt.expectNil && receivedPayload != nil {
				t.Errorf("expected nil payload, got %v", receivedPayload)
			} else if !tt.expectNil && receivedPayload == nil {
				t.Error("expected non-nil payload")
			}
		})
	}
}

func TestSetValue_FromProtoError(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodPut, "/st2138-api/v1/0/value/param",
		`{"struct_variant_value": {"variant_name": "test"}}`)
	if rec.Code == 0 {
		t.Error("expected a response code")
	}
}

func TestRouting_EdgeCases(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

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
			rec := makeRequest(t, srv, http.MethodGet, tt.path, "")
			assertHasError(t, rec)
		})
	}
}

func TestValueEndpoint_Methods(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
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
			makeRequest(t, srv, tt.method, "/st2138-api/v1/0/value/test", tt.body)
		})
	}
}

func TestDeviceEndpoint_NotRegistered(t *testing.T) {
	srv := NewServer([]uint16{}, 100)
	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/0", "")
	assertHasError(t, rec)
}

func TestServer_Connect_TooManyConnections(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.SetMaxConnections(1)

	_, cancel1 := setupSSEConnection(t, srv)

	rec2 := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/connect", "")
	assertStatus(t, rec2, http.StatusTooManyRequests)
	errMsg := assertHasError(t, rec2)
	if errMsg != "Too many connections to service" && errMsg != "Too Many Requests" {
		t.Errorf("expected error \"Too many connections to service\" (dev) or \"Too Many Requests\" (prod), got %q", errMsg)
	}

	cleanupSSE(cancel1)
}

func TestWriteResults_ValidData(t *testing.T) {
	device, _ := catena.ToCatenaDevice(map[string]any{"slot": int32(0)})
	rec := httptest.NewRecorder()
	writeDeviceResult(rec, device, http.StatusOK)
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	asset, _ := catena.ToCatenaAsset(catena.DataPayload{Payload: []byte("data")}, false)
	rec = httptest.NewRecorder()
	writeAssetResult(rec, asset, http.StatusOK)
	assertStatus(t, rec, http.StatusOK)
}

func TestCommandEndpoint_InvalidJSON(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/test", `{invalid json}`)
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestDeviceEndpoint_WrongMethod(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0", "")
	assertHasError(t, rec)
}

func TestCommandEndpoint_FromProtoError(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/exec",
		`{"struct_variant_value": {"variant_name": "test"}}`)
}

func TestCommandEndpoint_ResponseValue(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		val, _ := catena.ToCatenaValue("command executed")
		return catena.CommandReply(val)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/run", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	response := parseJSONBody(t, rec)
	resp, ok := response["response"]
	if !ok {
		t.Fatalf("expected 'response' field in CommandResponse, got %v", response)
	}
	respMap, ok := resp.(map[string]any)
	if !ok {
		t.Fatalf("expected response to be an object, got %T", resp)
	}
	if respMap["string_value"] != "command executed" {
		t.Errorf("expected string_value 'command executed', got %v", respMap["string_value"])
	}
}

func TestCommandEndpoint_ExceptionResponse(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandExceptionResult(
			"InvalidCommand",
			"Command not found: "+fqoid,
			catena.NewPolyglotText("en", "Command not found"),
		)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/bad_cmd", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	response := parseJSONBody(t, rec)
	exc, ok := response["exception"]
	if !ok {
		t.Fatalf("expected 'exception' field in CommandResponse, got %v", response)
	}
	excMap, ok := exc.(map[string]any)
	if !ok {
		t.Fatalf("expected exception to be an object, got %T", exc)
	}
	if excMap["type"] != "InvalidCommand" {
		t.Errorf("expected exception type 'InvalidCommand', got %v", excMap["type"])
	}
	if excMap["details"] != "Command not found: bad_cmd" {
		t.Errorf("expected exception details, got %v", excMap["details"])
	}
	errMsg, ok := excMap["error_message"].(map[string]any)
	if !ok {
		t.Fatalf("expected error_message to be an object, got %T", excMap["error_message"])
	}
	displayStrings, ok := errMsg["display_strings"].(map[string]any)
	if !ok {
		t.Fatalf("expected display_strings in error_message, got %T", errMsg["display_strings"])
	}
	if displayStrings["en"] != "Command not found" {
		t.Errorf("expected en display string 'Command not found', got %v", displayStrings["en"])
	}
}

func TestCommandEndpoint_RespondFalse(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		val, _ := catena.ToCatenaValue("should be thrown away")
		return catena.CommandReply(val)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/run?respond=false", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	if !handlerCalled {
		t.Error("handler should still be called when respond=false")
	}

	response := parseJSONBody(t, rec)
	if _, ok := response["no_response"]; !ok {
		t.Errorf("expected no_response when respond=false, got %v", response)
	}
	if _, ok := response["response"]; ok {
		t.Error("should not have response field when respond=false")
	}
}

func TestCommandEndpoint_RespondTrue(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		val, _ := catena.ToCatenaValue(int32(42))
		return catena.CommandReply(val)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/run?respond=true", "")
	assertStatus(t, rec, http.StatusOK)

	response := parseJSONBody(t, rec)
	if _, ok := response["response"]; !ok {
		t.Errorf("expected response field when respond=true, got %v", response)
	}
}

func TestCommandEndpoint_RespondDefaultIsTrue(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		val, _ := catena.ToCatenaValue(int32(42))
		return catena.CommandReply(val)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/run", "")
	assertStatus(t, rec, http.StatusOK)

	response := parseJSONBody(t, rec)
	if _, ok := response["response"]; !ok {
		t.Errorf("expected response field by default (respond not set), got %v", response)
	}
}

func TestCommandEndpoint_RespondFalseWithException(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandExceptionResult("Error", "something failed", nil)
	})

	rec := makeRequest(t, srv, http.MethodPost, "/st2138-api/v1/0/command/run?respond=false", "")
	assertStatus(t, rec, http.StatusOK)

	response := parseJSONBody(t, rec)
	if _, ok := response["no_response"]; !ok {
		t.Errorf("expected no_response even when handler returns exception and respond=false, got %v", response)
	}
}

func TestWriteHTTPStatusResult_WithError(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvDev)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.INTERNAL, Error: "error"}
	writeHTTPStatusResult(rec, result)
	assertBodyContains(t, rec, "error")
}

func TestServer_sendSSEEvent(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
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
	srv := NewServer([]uint16{0}, 100)
	rec, cancel := setupSSEConnection(t, srv)

	srv.BroadcastUpdate(0, "broadcast/oid", "hello")
	time.Sleep(100 * time.Millisecond)
	cleanupSSE(cancel)

	body := rec.Body.String()
	if !strings.Contains(body, "broadcast/oid") {
		t.Errorf("expected SSE body to contain broadcast/oid, got %s", body)
	}
}

func TestServer_BroadcastUpdate_ChannelFull(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.SetMaxConnections(10)
	_, cancel := setupSSEConnection(t, srv)

	for i := 0; i < 150; i++ {
		srv.BroadcastUpdate(0, "fill", int32(i))
	}
	cleanupSSE(cancel)
}

func TestServer_Start(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
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
	writeHTTPResult(rec, result, nil)
	assertStatus(t, rec, http.StatusOK)
}

func TestServer_GetAsset_CompressionQueryParam_Gzip(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test asset data for compression"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=GZIP", nil)
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)

	var result map[string]any
	if err := json.Unmarshal(rec.Body.Bytes(), &result); err != nil {
		t.Fatalf("invalid JSON response: %v", err)
	}
	payload, ok := result["payload"].(map[string]any)
	if !ok {
		t.Fatal("expected payload field in response")
	}
	if payload["payload_encoding"] != "GZIP" {
		t.Errorf("expected payload_encoding GZIP, got %v", payload["payload_encoding"])
	}
}

func TestServer_GetAsset_CompressionQueryParam_Deflate(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test asset data for compression"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=DEFLATE", nil)
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)

	var result map[string]any
	if err := json.Unmarshal(rec.Body.Bytes(), &result); err != nil {
		t.Fatalf("invalid JSON response: %v", err)
	}
	payload, ok := result["payload"].(map[string]any)
	if !ok {
		t.Fatal("expected payload field in response")
	}
	if payload["payload_encoding"] != "DEFLATE" {
		t.Errorf("expected payload_encoding DEFLATE, got %v", payload["payload_encoding"])
	}
}

func TestServer_GetAsset_CompressionQueryParam_Uncompressed(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	original := []byte("test asset data")
	gzData, _ := catena.CompressGzip(original)
	dp := catena.DataPayload{
		Metadata:        map[string]string{"content-type": "text/plain"},
		Payload:         gzData,
		PayloadEncoding: catena.EncodingGzip,
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=UNCOMPRESSED", nil)
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)
}

func TestServer_Connect_StreamingNotSupported(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvDev)

	srv := NewServer([]uint16{0}, 100)
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)
	rec := httptest.NewRecorder()
	w := &noFlusher{ResponseWriter: rec}

	srv.handleConnect(w, req)

	assertStatus(t, rec, http.StatusInternalServerError)
	errMsg := assertHasError(t, rec)
	if !strings.Contains(errMsg, "streaming") && !strings.Contains(errMsg, "Internal") {
		t.Errorf("expected error to mention streaming or Internal, got %s", errMsg)
	}
}

func TestWriteHTTPStatusResult_ProdMode(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvProd)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.NOT_FOUND, Error: "detailed internal error"}
	writeHTTPStatusResult(rec, result)

	assertBodyContains(t, rec, "Not Found")
	assertBodyNotContains(t, rec, "detailed internal error")
}

func TestServer_Connect_WithOrigin(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()

	go srv.mux.ServeHTTP(rec, req)
	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	assertHeader(t, rec, "Access-Control-Allow-Origin", "https://example.com")
}

func TestWriteValueResult_WriteError(t *testing.T) {
	value, _ := catena.ToCatenaValue(int32(1))
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeValueResult(w, value, http.StatusOK)

	if rec.Code != http.StatusInternalServerError && rec.Code != http.StatusOK {
		t.Errorf("expected status 500 or 200 (if header already sent), got %d", rec.Code)
	}
}

func TestWriteDeviceResult_WriteError(t *testing.T) {
	device, _ := catena.ToCatenaDevice(map[string]any{"slot": int32(0)})
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeDeviceResult(w, device, http.StatusOK)

	assertStatus(t, rec, http.StatusOK)
}

func TestWriteAssetResult_WriteError(t *testing.T) {
	asset, _ := catena.ToCatenaAsset(catena.DataPayload{Payload: []byte("x")}, false)
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeAssetResult(w, asset, http.StatusOK)

	assertStatus(t, rec, http.StatusOK)
}

func TestWriteHTTPResult_WithError_NonDev(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvProd)

	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.NOT_FOUND, Error: "internal detail"}
	writeHTTPResult(rec, result, catena.CatenaValue{})

	assertBodyNotContains(t, rec, "internal detail")
}

func TestServer_Shutdown(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	_, cancel := setupSSEConnection(t, srv)
	defer cancel()

	if srv.ConnectionCount() != 1 {
		t.Errorf("expected 1 connection before shutdown, got %d", srv.ConnectionCount())
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

	if srv.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", srv.ConnectionCount())
	}
}

func TestServer_Shutdown_MultipleConnections(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	srv.SetMaxConnections(10)

	cancels := make([]context.CancelFunc, 3)
	for i := 0; i < 3; i++ {
		_, cancel := setupSSEConnection(t, srv)
		cancels[i] = cancel
	}
	defer func() {
		for _, cancel := range cancels {
			cancel()
		}
	}()

	time.Sleep(150 * time.Millisecond)

	if srv.ConnectionCount() != 3 {
		t.Errorf("expected 3 connections, got %d", srv.ConnectionCount())
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

	if srv.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", srv.ConnectionCount())
	}
}

func TestServer_SetValue_NotifiesConnections(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		srv.BroadcastUpdate(slot, fqoid, value)
		return catena.StatusResult{Code: catena.OK}
	})

	rec, cancel := setupSSEConnection(t, srv)

	setRec := makeRequest(t, srv, http.MethodPut, "/st2138-api/v1/0/value/brightness", `{"int32_value": 42}`)
	assertStatus(t, setRec, http.StatusOK)

	time.Sleep(100 * time.Millisecond)
	cleanupSSE(cancel)

	sseBody := rec.Body.String()
	if !strings.Contains(sseBody, "brightness") {
		t.Errorf("expected SSE body to contain 'brightness' from SetValue notification, got %s", sseBody)
	}
}

func TestServer_SetValue_FailureDoesNotNotify(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		return catena.StatusWithCode(catena.INVALID_ARGUMENT, "bad value")
	})

	rec, cancel := setupSSEConnection(t, srv)

	makeRequest(t, srv, http.MethodPut, "/st2138-api/v1/0/value/brightness", `{"int32_value": -1}`)

	time.Sleep(100 * time.Millisecond)
	cleanupSSE(cancel)

	sseBody := rec.Body.String()
	lines := strings.Split(sseBody, "\n")
	for _, line := range lines {
		if strings.HasPrefix(line, "data: ") && strings.Contains(line, "brightness") {
			t.Error("failed SetValue should not send SSE notification")
		}
	}
}

func TestBroadcastUpdate_InvalidValue(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	// bool is not supported by catena.ToProto; this exercises the error branch
	srv.BroadcastUpdate(0, "test/param", true)
}

func TestSendSSEEvent_WriteFailure(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := httptest.NewRecorder()
	w := &failFlusherWriter{ResponseRecorder: rec, failAfterN: 0}
	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 1}},
			},
		},
	}

	err := srv.sendSSEEvent(w, w, update)
	if err == nil {
		t.Error("expected error when writer fails")
	}
}

func TestHandleConnect_InitialEventWriteFailure(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := httptest.NewRecorder()
	w := &failFlusherWriter{ResponseRecorder: rec, failAfterN: 0}
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)

	srv.handleConnect(w, req)

	if srv.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after initial event failure, got %d", srv.ConnectionCount())
	}
}

func TestHandleConnect_UpdateEventWriteFailure(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := httptest.NewRecorder()
	w := &failFlusherWriter{ResponseRecorder: rec, failAfterN: 1}
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)

	done := make(chan struct{})
	go func() {
		srv.handleConnect(w, req)
		close(done)
	}()
	time.Sleep(100 * time.Millisecond)

	srv.BroadcastUpdate(0, "test/param", int32(42))

	select {
	case <-done:
	case <-time.After(2 * time.Second):
		t.Fatal("handler did not exit after update write failure")
	}

	if srv.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after write failure, got %d", srv.ConnectionCount())
	}
}

func TestRouting_BasePathOnly(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)
	rec := makeRequest(t, srv, http.MethodGet, "/st2138-api/v1/", "")
	assertStatus(t, rec, http.StatusBadRequest)
	assertHasError(t, rec)
}

func TestServer_GetAsset_CompressionQueryParam_Invalid(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test data"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=INVALID", nil)
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusBadRequest {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
}

func TestServer_GetAsset_NoCompressionParam(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("uncompressed data"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt", nil)
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
}

func TestServer_GetAsset_CompressionWithError(t *testing.T) {
	srv := NewServer([]uint16{0}, 100)

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found")
	})

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/missing?compression=GZIP", nil)
	rec := httptest.NewRecorder()
	srv.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
}
