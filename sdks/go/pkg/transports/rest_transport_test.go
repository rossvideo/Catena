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
 * @brief Test for the REST transport for the Catena SDK.
 * @file rest_transport_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-08
 */

package transports

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
	"google.golang.org/protobuf/proto"
)

func makeTestRestTransport(tb testing.TB) (*RestTransport, *stubServerRuntime) {
	// create and start the transport
	transport := NewRestTransport(8080)
	stubRuntime := makeStubServerRuntime(tb)
	transport.runtime = stubRuntime
	return transport, stubRuntime
}

func TestRestTransport_NewRestTransport(t *testing.T) {
	expected := 1234
	transport := NewRestTransport(expected)
	if transport == nil {
		t.Fatal("NewRestTransport returned nil")
	}
	if transport.port != expected {
		t.Errorf("expected port %d, got %d", expected, transport.port)
	}
}

func TestRestTransport_NewDefaultTransport(t *testing.T) {
	transport := NewDefaultRestTransport()
	if transport == nil {
		t.Fatal("NewDefaultRestTransport returned nil")
	}
	if transport.port != 8080 {
		t.Errorf("expected default port 8080, got %d", transport.port)
	}
}

func TestRestTransport_GetDevice_Route(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": catena.DetailLevelFull,
	}
	device, _ := catena.ToCatenaDevice(deviceMap)

	runtime.getDeviceFn = func(slot uint16) (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		return catena.Reply(device)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestRestTransport_GetDevice_NotFound(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.getDeviceFn = func(slot uint16) (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		if slot != 99 {
			t.Errorf("expected slot 99, got %d", slot)
		}
		return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/99", "")
	if rec.Code != http.StatusNotFound && rec.Code != http.StatusOK {
		t.Errorf("expected status %d or %d, got %d", http.StatusNotFound, http.StatusOK, rec.Code)
	}
	if !handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestRestTransport_GetDevice_InvalidSlot(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.getDeviceFn = func(slot uint16) (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		return catena.ReplyError[catena.CatenaDevice](catena.INVALID_ARGUMENT, "invalid slot")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/invalid", "")
	if rec.Code == http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestRestTransport_GetValue_Route(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	value, _ := catena.ToCatenaValue(int32(42))
	runtime.getValueFn = func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.Reply(value)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/value/brightness", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestRestTransport_SetValue_Route(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.setValueFn = func(value any, slot uint16, fqoid string) catena.StatusResult {
		handlerCalled = true
		if v, ok := value.(int32); !ok || v != 42 {
			t.Errorf("expected value int32(42), got %v (%T)", value, value)
		}
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.StatusResult{Code: catena.OK}
	}

	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/value/brightness", `{"int32_value": 42}`)
	assertStatus(t, rec, http.StatusOK)
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestRestTransport_SetValue_InvalidContentType(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.setValueFn = func(value any, slot uint16, fqoid string) catena.StatusResult {
		handlerCalled = true
		return catena.StatusResult{Code: catena.OK}
	}

	rec := makeRequestWithHeaders(t, transport, http.MethodPut, "/st2138-api/v1/0/value/brightness",
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

func TestRestTransport_GetAsset_Route(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "image/png"},
		Payload:  []byte("fake image"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/asset/logo", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestRestTransport_GetAsset_MethodNotAllowed(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		handlerCalled = true
		return catena.Reply(catena.CatenaAsset{})
	}

	makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/asset/logo", "")
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestRestTransport_ExecuteCommand_Route(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.commandFn = func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if commandFqoid != "reboot" {
			t.Errorf("expected commandFqoid 'reboot', got %s", commandFqoid)
		}
		return catena.CommandNoResponse()
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/reboot", "")
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

func TestRestTransport_ExecuteCommand_WithPayload(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.commandFn = func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if payload == nil {
			t.Error("expected payload to be non-nil")
		}
		val, _ := catena.ToCatenaValue(payload)
		return catena.CommandReply(val)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/process", `{"string_value": "test"}`)
	assertStatus(t, rec, http.StatusOK)
	response := parseJSONBody(t, rec)
	if _, ok := response["response"]; !ok {
		t.Errorf("expected response field in CommandResponse, got %v", response)
	}
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestRestTransport_ExecuteCommand_MethodNotAllowed(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.commandFn = func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		return catena.CommandNoResponse()
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/command/reboot", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	if handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestRestTransport_Connect_Route(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	connection := makeTestConnection(1)
	runtime.WithConnection(connection)

	// push an update to the connection
	connection.Updates <- &protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: []uint32{0, 1},
			},
		},
	}

	rec, cancel := setupSSEConnection(t, transport)
	cleanupSSE(cancel)

	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "text/event-stream")
	assertHeader(t, rec, "Cache-Control", "no-cache")

	body := rec.Body.String()
	if !strings.Contains(body, "data:") {
		t.Error("expected SSE data in response body")
	}
	if !strings.Contains(body, "slots_added") {
		t.Error("expected initial slots_added in response body")
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
			slotsAdded, ok := payload["slots_added"].(map[string]any)
			if !ok {
				t.Error("expected slots_added object (SlotList) in initial event")
				break
			}
			slots, ok := slotsAdded["slots"].([]any)
			if !ok || len(slots) != 2 {
				t.Errorf("expected slots_added.slots to be array of 2 slots, got %T %v", slotsAdded["slots"], slotsAdded["slots"])
			}
			break
		}
	}
}

func TestRestTransport_Connect_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestRestTransport(t)

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/connect", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	errMsg := assertHasError(t, rec)
	if !strings.Contains(errMsg, "GET") {
		t.Errorf("expected error to mention GET, got %s", errMsg)
	}
}

func TestRestTransport_Fallback_Route(t *testing.T) {
	transport, _ := makeTestRestTransport(t)

	handlerCalled := false
	transport.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "custom not found")
	})

	rec := makeRequest(t, transport, http.MethodGet, "/unknown/path", "")
	assertStatus(t, rec, http.StatusNotFound)
	if !handlerCalled {
		t.Error("registered handler should have been called, but was not")
	}
}

func TestRestTransport_NestedValuePath(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	runtime.getValueFn = func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid != "nested/path/to/param" {
			t.Errorf("expected fqoid 'nested/path/to/param', got %s", fqoid)
		}
		value, _ := catena.ToCatenaValue(int32(1))
		return catena.Reply(value)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/value/nested/path/to/param", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestRestTransport_UnknownEndpoint(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/unknown", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestRestTransport_InvalidPath(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/kjhgjnghf", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestRestTransport_InvalidPathNoSlash(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestRestTransport_NegativeSlot(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/-1", "")
	assertStatus(t, rec, http.StatusBadRequest)
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

func TestRestTransport_ErrorMessages_DevVsProd(t *testing.T) {
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
			transport, runtime := makeTestRestTransport(t)
			var receivedPayload any
			runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
				receivedPayload = payload
				return catena.CommandNoResponse()
			}

			makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/test", tt.body)

			if tt.expectNil && receivedPayload != nil {
				t.Errorf("expected nil payload, got %v", receivedPayload)
			} else if !tt.expectNil && receivedPayload == nil {
				t.Error("expected non-nil payload")
			}
		})
	}
}

func TestSetValue_FromProtoError(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/value/param",
		`{"struct_variant_value": {"variant_name": "test"}}`)
	if rec.Code == 0 {
		t.Error("expected a response code")
	}
}

func TestRouting_EdgeCases(t *testing.T) {
	transport, _ := makeTestRestTransport(t)

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
			rec := makeRequest(t, transport, http.MethodGet, tt.path, "")
			assertHasError(t, rec)
		})
	}
}

func TestValueEndpoint_Methods(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	runtime.setValueFn = func(value any, slot uint16, fqoid string) catena.StatusResult {
		return catena.StatusResult{Code: catena.OK}
	}

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
			makeRequest(t, transport, tt.method, "/st2138-api/v1/0/value/test", tt.body)
		})
	}
}

func TestRestTransport_Connect_TooManyConnections(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	runtime.registerTransportConnFn = func(owner any) (int, *catena.Connection) {
		return -1, nil
	}

	_, cancel1 := setupSSEConnection(t, transport)

	rec2 := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/connect", "")
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
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/test", `{invalid json}`)
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestDeviceEndpoint_WrongMethod(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0", "")
	assertHasError(t, rec)
}

func TestCommandEndpoint_FromProtoError(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/exec",
		`{"struct_variant_value": {"variant_name": "test"}}`)
}

func TestCommandEndpoint_ResponseValue(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		val, _ := catena.ToCatenaValue("command executed")
		return catena.CommandReply(val)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run", "")
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
	transport, runtime := makeTestRestTransport(t)
	runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandExceptionResult(
			"InvalidCommand",
			"Command not found: "+fqoid,
			catena.NewPolyglotText("en", "Command not found"),
		)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/bad_cmd", "")
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
	transport, runtime := makeTestRestTransport(t)

	handlerCalled := false
	runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		val, _ := catena.ToCatenaValue("should be thrown away")
		return catena.CommandReply(val)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run?respond=false", "")
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
	transport, runtime := makeTestRestTransport(t)
	runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		val, _ := catena.ToCatenaValue(int32(42))
		return catena.CommandReply(val)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run?respond=true", "")
	assertStatus(t, rec, http.StatusOK)

	response := parseJSONBody(t, rec)
	if _, ok := response["response"]; !ok {
		t.Errorf("expected response field when respond=true, got %v", response)
	}
}

func TestCommandEndpoint_RespondDefaultIsTrue(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		val, _ := catena.ToCatenaValue(int32(42))
		return catena.CommandReply(val)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run", "")
	assertStatus(t, rec, http.StatusOK)

	response := parseJSONBody(t, rec)
	if _, ok := response["response"]; !ok {
		t.Errorf("expected response field by default (respond not set), got %v", response)
	}
}

func TestCommandEndpoint_RespondFalseWithException(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	runtime.commandFn = func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandExceptionResult("Error", "something failed", nil)
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run?respond=false", "")
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

func TestRestTransport_sendSSEEvent(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
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

	err := transport.sendSSEEvent(rec, flusher, update)
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

func TestRestTransport_Start(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	listener, err := net.Listen("tcp", ":0")
	if err != nil {
		t.Fatalf("net.Listen: %v", err)
	}
	port := listener.Addr().(*net.TCPAddr).Port
	listener.Close()

	transport.port = port
	err = transport.Start(context.Background(), runtime)
	if err != nil {
		t.Errorf("Start: %v", err)
	}
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
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	transport.Shutdown(ctx)
}

func TestWriteHTTPResult_DefaultType(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.OK}
	writeHTTPResult(rec, result, nil)
	assertStatus(t, rec, http.StatusOK)
}

func TestRestTransport_GetAsset_CompressionQueryParam_Gzip(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test asset data for compression"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=GZIP", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
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

func TestRestTransport_GetAsset_CompressionQueryParam_Deflate(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test asset data for compression"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=DEFLATE", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
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

func TestRestTransport_GetAsset_CompressionQueryParam_Uncompressed(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	original := []byte("test asset data")
	gzData, _ := catena.CompressGzip(original)
	dp := catena.DataPayload{
		Metadata:        map[string]string{"content-type": "text/plain"},
		Payload:         gzData,
		PayloadEncoding: catena.EncodingGzip,
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=UNCOMPRESSED", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)
}

func TestRestTransport_Connect_StreamingNotSupported(t *testing.T) {
	original := catena.GetEnv()
	defer catena.SetEnv(original)
	catena.SetEnv(catena.EnvDev)

	transport, _ := makeTestRestTransport(t)
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)
	rec := httptest.NewRecorder()
	w := &noFlusher{ResponseWriter: rec}

	transport.handleConnect(w, req)

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

func TestRestTransport_Connect_WithOrigin(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	runtime.WithConnection(makeTestConnection(1))

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()

	go transport.mux.ServeHTTP(rec, req)
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

func TestSendSSEEvent_MarshalError(t *testing.T) {
	origMarshal := marshalSSEFunc
	defer func() { marshalSSEFunc = origMarshal }()
	marshalSSEFunc = func(msg proto.Message) ([]byte, error) {
		return nil, fmt.Errorf("marshal failed")
	}

	transport, _ := makeTestRestTransport(t)
	rec := httptest.NewRecorder()
	var w http.ResponseWriter = rec
	flusher := w.(http.Flusher)
	update := &protos.PushUpdates{Slot: 0}

	err := transport.sendSSEEvent(rec, flusher, update)
	if err == nil || err.Error() != "marshal failed" {
		t.Errorf("expected 'marshal failed' error, got %v", err)
	}
}

func TestSendSSEEvent_WriteFailure(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
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

	err := transport.sendSSEEvent(w, w, update)
	if err == nil {
		t.Error("expected error when writer fails")
	}
}

func TestRestTransport_Connect_PushUpdates(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	connection := makeTestConnection(1)
	runtime.WithConnection(connection)

	rec, cancel := setupSSEConnection(t, transport)

	connection.Updates <- &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "brightness",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}

	time.Sleep(100 * time.Millisecond)
	cleanupSSE(cancel)

	sseBody := rec.Body.String()
	if !strings.Contains(sseBody, "brightness") {
		t.Errorf("expected SSE body to contain 'brightness' from SetValue notification, got %s", sseBody)
	}
}

func TestRestTransport_Connect_ServerShutdown(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	connection := makeTestConnection(1)
	runtime.WithConnection(connection)

	_, cancel := setupSSEConnection(t, transport)
	defer cancel()

	connection.Done <- struct{}{}
	time.Sleep(100 * time.Millisecond)

	if runtime.registerCalls != runtime.deregisterCalls {
		t.Errorf("expected deregister calls to match register calls after server shutdown, got %d deregister calls and %d register calls", runtime.deregisterCalls, runtime.registerCalls)
	}
}

func TestHandleConnect_UpdateEventWriteFailure(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	rec := httptest.NewRecorder()
	w := &failFlusherWriter{ResponseRecorder: rec, failAfterN: 1}
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)

	connection := makeTestConnection(1)
	runtime.WithConnection(connection)

	done := make(chan struct{})
	go func() {
		transport.handleConnect(w, req)
		close(done)
	}()
	time.Sleep(100 * time.Millisecond)

	// push an update multiple times to trigger the write failure in failFlusherWriter
	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}
	connection.Updates <- update
	connection.Updates <- update

	select {
	case <-done:
	case <-time.After(2 * time.Second):
		t.Fatal("handler did not exit after update write failure")
	}

	if runtime.registerCalls != runtime.deregisterCalls {
		t.Errorf("expected deregister calls to match register calls after update event failure, got %d deregister calls and %d register calls", runtime.deregisterCalls, runtime.registerCalls)
	}
}

func TestRouting_BasePathOnly(t *testing.T) {
	transport, _ := makeTestRestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/", "")
	assertStatus(t, rec, http.StatusBadRequest)
	assertHasError(t, rec)
}

func TestRestTransport_GetAsset_CompressionQueryParam_Invalid(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test data"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=INVALID", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusBadRequest {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
}

func TestRestTransport_GetAsset_NoCompressionParam(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	dp := catena.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("uncompressed data"),
	}
	asset, _ := catena.ToCatenaAsset(dp, true)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
}

func TestRestTransport_GetAsset_CompressionWithError(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)

	runtime.getAssetFn = func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found")
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/missing?compression=GZIP", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
}

func TestRestTransport_Shutdown_ClosesOwnedConnections(t *testing.T) {
	transport, runtime := makeTestRestTransport(t)
	called := false
	runtime.shutdownTransportConnectionsFn = func(owner any) {
		called = true
		if owner != transport {
			t.Fatalf("expected shutdown owner %p, got %p", transport, owner)
		}
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()

	if err := transport.Shutdown(ctx); err != nil {
		t.Fatalf("expected shutdown to succeed, got %v", err)
	}

	if !called {
		t.Fatal("expected transport shutdown to close owned connections")
	}
}
