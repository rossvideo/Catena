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
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-04
 */

package rest

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
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

func TestWriteResponseJSON_EmitsDefaultValues(t *testing.T) {
	tests := []struct {
		name     string
		value    *protos.Value
		expected string
	}{
		{
			"zero int32",
			&protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 0}},
			`"int32_value":0`,
		},
		{
			"empty string",
			&protos.Value{Kind: &protos.Value_StringValue{StringValue: ""}},
			`"string_value":""`,
		},
		{
			"zero float32",
			&protos.Value{Kind: &protos.Value_Float32Value{Float32Value: 0}},
			`"float32_value":0`,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			w := httptest.NewRecorder()
			err := WriteResponseJSON(w, tt.value, http.StatusOK)
			if err != nil {
				t.Fatalf("unexpected error: %v", err)
			}
			if !strings.Contains(w.Body.String(), tt.expected) {
				t.Errorf("expected body to contain %q, got %q", tt.expected, w.Body.String())
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

func TestWriteCommandResponseJSON_NilResponse(t *testing.T) {
	w := httptest.NewRecorder()

	err := WriteCommandResponseJSON(w, nil, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if w.Code != http.StatusOK {
		t.Errorf("expected status 200, got %d", w.Code)
	}
	if ct := w.Header().Get("Content-Type"); ct != "application/json" {
		t.Errorf("expected Content-Type application/json, got %q", ct)
	}
	if w.Body.Len() != 0 {
		t.Errorf("expected empty body, got %q", w.Body.String())
	}
}

func TestWriteCommandResponseJSON_ValidNoResponse(t *testing.T) {
	w := httptest.NewRecorder()
	cmdResp := &protos.CommandResponse{
		Kind: &protos.CommandResponse_NoResponse{NoResponse: &protos.Empty{}},
	}

	err := WriteCommandResponseJSON(w, cmdResp, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if w.Code != http.StatusOK {
		t.Errorf("expected status 200, got %d", w.Code)
	}
	if !strings.Contains(w.Body.String(), "no_response") {
		t.Errorf("expected body to contain 'no_response', got %q", w.Body.String())
	}
}

func TestWriteCommandResponseJSON_ValidResponse(t *testing.T) {
	w := httptest.NewRecorder()
	cmdResp := &protos.CommandResponse{
		Kind: &protos.CommandResponse_Response{
			Response: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
		},
	}

	err := WriteCommandResponseJSON(w, cmdResp, http.StatusOK)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if !strings.Contains(w.Body.String(), "response") {
		t.Errorf("expected body to contain 'response', got %q", w.Body.String())
	}
}

func TestWriteCommandResponseJSON_MarshalError(t *testing.T) {
	w := httptest.NewRecorder()
	cmdResp := &protos.CommandResponse{
		Kind: &protos.CommandResponse_Response{
			Response: &protos.Value{Kind: &protos.Value_StringValue{StringValue: "\xff"}},
		},
	}

	err := WriteCommandResponseJSON(w, cmdResp, http.StatusOK)
	if err == nil {
		t.Fatal("expected error for invalid UTF-8 string")
	}
	if w.Code != http.StatusInternalServerError {
		t.Errorf("expected status 500, got %d", w.Code)
	}
	if !strings.Contains(w.Body.String(), "failed to marshal command response") {
		t.Errorf("expected error body, got %q", w.Body.String())
	}
}

func TestWriteCommandResponseJSON_WriteError(t *testing.T) {
	w := &errorWriter{}
	cmdResp := &protos.CommandResponse{
		Kind: &protos.CommandResponse_NoResponse{NoResponse: &protos.Empty{}},
	}

	err := WriteCommandResponseJSON(w, cmdResp, http.StatusOK)
	if err == nil {
		t.Fatal("expected error when Write fails")
	}
}

func TestMarshalProtoJSON_DoesNotEmitDefaults(t *testing.T) {
	device := &protos.Device{
		Slot: 0,
		Params: map[string]*protos.Param{
			"volume": {
				Type: protos.ParamType_INT32,
				Value: &protos.Value{
					Kind: &protos.Value_Int32Value{Int32Value: 50},
				},
			},
		},
	}

	b, err := MarshalProtoJSON(device)
	if err != nil {
		t.Fatalf("MarshalProtoJSON error: %v", err)
	}
	body := string(b)

	if strings.Contains(body, `"precision"`) {
		t.Errorf("MarshalProtoJSON should not emit default precision, got %s", body)
	}
	if strings.Contains(body, `"params":{}`) {
		t.Errorf("MarshalProtoJSON should not emit empty params map, got %s", body)
	}
}

func TestInjectJSONField_AddsSlotZero(t *testing.T) {
	device := &protos.Device{
		Slot: 0,
		Params: map[string]*protos.Param{
			"volume": {
				Type: protos.ParamType_INT32,
			},
		},
	}

	b, err := MarshalProtoJSON(device)
	if err != nil {
		t.Fatalf("MarshalProtoJSON error: %v", err)
	}

	if strings.Contains(string(b), `"slot"`) {
		t.Fatal("expected slot to be absent before injection")
	}

	b, err = injectJSONField(b, "slot", device.GetSlot())
	if err != nil {
		t.Fatalf("injectJSONField error: %v", err)
	}

	body := string(b)
	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0 after injection, got %s", body)
	}
}

func TestInjectJSONField_PreservesExistingSlot(t *testing.T) {
	device := &protos.Device{
		Slot: 5,
		Params: map[string]*protos.Param{
			"volume": {
				Type: protos.ParamType_INT32,
			},
		},
	}

	b, err := MarshalProtoJSON(device)
	if err != nil {
		t.Fatalf("MarshalProtoJSON error: %v", err)
	}

	b, err = injectJSONField(b, "slot", device.GetSlot())
	if err != nil {
		t.Fatalf("injectJSONField error: %v", err)
	}

	body := string(b)
	if !strings.Contains(body, `"slot":5`) {
		t.Errorf("expected slot:5 after injection, got %s", body)
	}
}

func TestInjectJSONField_PushUpdatesSlotZero(t *testing.T) {
	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: []uint32{0, 1},
			},
		},
	}

	b, err := MarshalProtoJSON(update)
	if err != nil {
		t.Fatalf("MarshalProtoJSON error: %v", err)
	}

	b, err = injectJSONField(b, "slot", update.GetSlot())
	if err != nil {
		t.Fatalf("injectJSONField error: %v", err)
	}

	body := string(b)
	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0 in PushUpdates output, got %s", body)
	}
	if !strings.Contains(body, `"slots_added"`) {
		t.Errorf("expected slots_added in output, got %s", body)
	}
}

// --- MarshalDeviceJSON tests (migrated from catena/device_test.go) ---

func TestMarshalDeviceJSON(t *testing.T) {
	cd, err := catena.ToCatenaDevice(map[string]any{
		"slot":         uint32(0),
		"detail_level": catena.DetailLevelFull,
		"params": map[string]any{
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type": catena.ParamTypeInt32,
			},
		},
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}

	jsonData, err2 := MarshalDeviceJSON(cd.GetProtoDevice())
	if err2 != nil {
		t.Fatalf("MarshalDeviceJSON error: %v", err2)
	}
	if jsonData == nil {
		t.Fatal("expected non-nil JSON data")
	}

	var result map[string]any
	if err := json.Unmarshal(jsonData, &result); err != nil {
		t.Fatalf("invalid JSON output: %v", err)
	}
	if _, ok := result["params"]; !ok {
		t.Error("expected 'params' in JSON output")
	}
}

func TestMarshalDeviceJSON_SlotZeroPresent(t *testing.T) {
	cd, err := catena.ToCatenaDevice(map[string]any{
		"slot":         uint32(0),
		"detail_level": catena.DetailLevelFull,
		"params": map[string]any{
			"volume": map[string]any{
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 0,
				},
			},
		},
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}

	jsonData, err2 := MarshalDeviceJSON(cd.GetProtoDevice())
	if err2 != nil {
		t.Fatalf("MarshalDeviceJSON error: %v", err2)
	}

	body := string(jsonData)
	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0 in JSON output, got %s", body)
	}
	if !strings.Contains(body, `"int32_value":0`) {
		t.Errorf("expected int32_value:0 in JSON output, got %s", body)
	}
	if strings.Contains(body, `"params":{}`) {
		t.Errorf("empty params map should not appear in output, got %s", body)
	}
}

func TestMarshalDeviceJSON_Nil(t *testing.T) {
	jsonData, err := MarshalDeviceJSON(nil)
	if err != nil {
		t.Fatalf("MarshalDeviceJSON error: %v", err)
	}
	if jsonData != nil {
		t.Error("expected nil JSON data for nil device")
	}
}

func TestMarshalDeviceJSON_SlotNonZero(t *testing.T) {
	cd, err := catena.ToCatenaDevice(map[string]any{
		"slot": uint32(5),
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err2 := MarshalDeviceJSON(cd.GetProtoDevice())
	if err2 != nil {
		t.Fatalf("MarshalDeviceJSON: %v", err2)
	}
	body := string(b)
	if !strings.Contains(body, `"slot":5`) {
		t.Errorf("expected slot:5; got %s", body)
	}
}

func TestMarshalDeviceJSON_EmptyMapsStripped(t *testing.T) {
	cd, err := catena.ToCatenaDevice(map[string]any{
		"slot":         uint32(0),
		"detail_level": catena.DetailLevelFull,
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err2 := MarshalDeviceJSON(cd.GetProtoDevice())
	if err2 != nil {
		t.Fatalf("MarshalDeviceJSON: %v", err2)
	}
	body := string(b)

	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0; got %s", body)
	}
	for _, field := range []string{"params", "constraints", "commands", "menu_groups", "language_packs"} {
		if strings.Contains(body, `"`+field+`":{}`) {
			t.Errorf("empty %s should be stripped; got %s", field, body)
		}
	}
}

func TestMarshalDeviceJSON_PopulatedMapsKept(t *testing.T) {
	cd, err := catena.ToCatenaDevice(map[string]any{
		"slot": uint32(0),
		"params": map[string]any{
			"brightness": map[string]any{
				"type": catena.ParamTypeInt32,
			},
		},
		"commands": map[string]any{
			"reboot": map[string]any{
				"type": catena.ParamTypeEmpty,
			},
		},
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err2 := MarshalDeviceJSON(cd.GetProtoDevice())
	if err2 != nil {
		t.Fatalf("MarshalDeviceJSON: %v", err2)
	}
	body := string(b)

	if !strings.Contains(body, `"params"`) {
		t.Errorf("expected params in output; got %s", body)
	}
	if !strings.Contains(body, `"commands"`) {
		t.Errorf("expected commands in output; got %s", body)
	}
}

func TestCleanDeviceJSON(t *testing.T) {
	tests := []struct {
		name     string
		input    string
		contains []string
		excludes []string
	}{
		{
			name:     "strip null",
			input:    `{"slot":0,"value":null,"type":"INT32"}`,
			contains: []string{`"slot":0`, `"type":"INT32"`},
			excludes: []string{`"value"`},
		},
		{
			name:     "strip empty object",
			input:    `{"slot":0,"params":{},"detail_level":"FULL"}`,
			contains: []string{`"slot":0`, `"detail_level":"FULL"`},
			excludes: []string{`"params"`},
		},
		{
			name:     "strip empty array",
			input:    `{"slot":0,"oid_aliases":[],"type":"INT32"}`,
			contains: []string{`"slot":0`, `"type":"INT32"`},
			excludes: []string{`"oid_aliases"`},
		},
		{
			name:     "strip empty string",
			input:    `{"slot":0,"widget":"","type":"INT32"}`,
			contains: []string{`"slot":0`, `"type":"INT32"`},
			excludes: []string{`"widget"`},
		},
		{
			name:     "keep populated values",
			input:    `{"slot":0,"params":{"a":1},"name":"test"}`,
			contains: []string{`"slot":0`, `"params"`, `"name":"test"`},
		},
		{
			name:     "cascading strip",
			input:    `{"outer":{"inner":null}}`,
			contains: []string{`{}`},
			excludes: []string{`"outer"`},
		},
		{
			name:     "strip multiple patterns",
			input:    `{"a":null,"b":{},"c":[],"d":"","e":1}`,
			contains: []string{`"e":1`},
			excludes: []string{`"a"`, `"b"`, `"c"`, `"d"`},
		},
		{
			name:     "nested null stripped",
			input:    `{"params":{"brightness":{"value":null,"type":"INT32"}}}`,
			contains: []string{`"params"`, `"brightness"`, `"type":"INT32"`},
			excludes: []string{`"value"`},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := cleanDeviceJSON([]byte(tt.input))
			if err != nil {
				t.Fatalf("cleanDeviceJSON error: %v", err)
			}
			body := string(got)
			for _, want := range tt.contains {
				if !strings.Contains(body, want) {
					t.Errorf("expected %q in output; got %s", want, body)
				}
			}
			for _, excl := range tt.excludes {
				if strings.Contains(body, excl) {
					t.Errorf("did not expect %q in output; got %s", excl, body)
				}
			}
		})
	}
}

func TestMarshalDeviceJSON_CompleteDevice(t *testing.T) {
	cd, err := catena.ToCatenaDevice(map[string]any{
		"slot":              uint32(0),
		"detail_level":      catena.DetailLevelFull,
		"multi_set_enabled": true,
		"subscriptions":     true,
		"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
		"default_scope":     "st2138:op",
		"params": map[string]any{
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type": catena.ParamTypeInt32,
				"constraint": map[string]any{
					"ref_oid": "brightness_range",
				},
				"read_only": false,
				"widget":    "SLIDER",
			},
		},
		"constraints": map[string]any{
			"brightness_range": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(0),
					"max_value": int32(100),
				},
			},
		},
		"commands": map[string]any{
			"reboot": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Reboot Device",
					},
				},
				"type": catena.ParamTypeEmpty,
			},
		},
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}

	jsonData, err2 := MarshalDeviceJSON(cd.GetProtoDevice())
	if err2 != nil {
		t.Fatalf("MarshalDeviceJSON error: %v", err2)
	}
	if len(jsonData) == 0 {
		t.Error("expected non-empty JSON")
	}
}

// --- MarshalAssetJSON tests (migrated from catena/asset_test.go) ---

func TestMarshalAssetJSON(t *testing.T) {
	dp := catena.DataPayload{
		Metadata: map[string]string{
			"content-type": "application/octet-stream",
		},
		Payload: []byte("binary data"),
	}

	asset, res := catena.ToCatenaAsset(dp, true)
	if res.Code != catena.OK {
		t.Fatalf("ToCatenaAsset error: %v", res.Error)
	}

	jsonData, err := MarshalAssetJSON(asset.GetProtoAsset())
	if err != nil {
		t.Fatalf("MarshalAssetJSON error: %v", err)
	}
	if jsonData == nil {
		t.Fatal("expected non-nil JSON data")
	}

	var result map[string]any
	if err := json.Unmarshal(jsonData, &result); err != nil {
		t.Fatalf("invalid JSON output: %v", err)
	}
	if result["cachable"] != true {
		t.Errorf("expected cachable true in JSON, got %v", result["cachable"])
	}
}

func TestMarshalAssetJSON_Nil(t *testing.T) {
	jsonData, err := MarshalAssetJSON(nil)
	if err != nil {
		t.Fatalf("MarshalAssetJSON error: %v", err)
	}
	if jsonData != nil {
		t.Error("expected nil JSON data for nil asset")
	}
}

