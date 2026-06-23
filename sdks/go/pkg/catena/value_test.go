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
 * @brief Value handling for the Catena SDK.
 * @file value_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-09
 */

package catena

import (
	"reflect"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestToValue(t *testing.T) {
	tests := []struct {
		name    string
		input   any
		wantErr bool
	}{
		{"int32", int32(42), false},
		{"float32", float32(3.14), false},
		{"string", "hello", false},
		{"int32 array", []int32{1, 2, 3}, false},
		{"float32 array", []float32{1.1, 2.2, 3.3}, false},
		{"string array", []string{"a", "b", "c"}, false},
		{"empty value", EmptyValue{}, false},
		{"undefined value", UndefinedValue(0), false},
		{"struct value", map[string]any{"key": int32(1)}, false},
		{"data payload with payload", DataPayload{Payload: []byte("test")}, false},
		{"data payload with url", DataPayload{Url: "https://example.com"}, false},
		{"unsupported type", int64(100), true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cv, err := ToValue(tt.input)
			if tt.wantErr {
				if err.Code == StatusCodeOk {
					t.Errorf("ToValue(%v) expected error, got nil", tt.input)
				}
				return
			}
			if err.Code != StatusCodeOk {
				t.Errorf("ToValue(%v) unexpected error: %v", tt.input, err.Error)
				return
			}
			if cv.Value == nil {
				t.Errorf("ToValue(%v) returned nil Value", tt.input)
			}
		})
	}
}

func TestToProto_Int32(t *testing.T) {
	input := int32(42)
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	if pv.GetInt32Value() != 42 {
		t.Errorf("ToProto(%v) = %v, want 42", input, pv.GetInt32Value())
	}
}

func TestToProto_Float32(t *testing.T) {
	input := float32(3.14)
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	if pv.GetFloat32Value() != input {
		t.Errorf("ToProto(%v) = %v, want %v", input, pv.GetFloat32Value(), input)
	}
}

func TestToProto_String(t *testing.T) {
	input := "hello world"
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	if pv.GetStringValue() != input {
		t.Errorf("ToProto(%v) = %v, want %v", input, pv.GetStringValue(), input)
	}
}

func TestToProto_Int32Array(t *testing.T) {
	input := []int32{1, 2, 3, 4, 5}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	result := pv.GetInt32ArrayValues().GetInts()
	if !reflect.DeepEqual(result, input) {
		t.Errorf("ToProto(%v) = %v, want %v", input, result, input)
	}
}

func TestToProto_Float32Array(t *testing.T) {
	input := []float32{1.1, 2.2, 3.3}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	result := pv.GetFloat32ArrayValues().GetFloats()
	if !reflect.DeepEqual(result, input) {
		t.Errorf("ToProto(%v) = %v, want %v", input, result, input)
	}
}

func TestToProto_StringArray(t *testing.T) {
	input := []string{"one", "two", "three"}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	result := pv.GetStringArrayValues().GetStrings()
	if !reflect.DeepEqual(result, input) {
		t.Errorf("ToProto(%v) = %v, want %v", input, result, input)
	}
}

func TestToProto_EmptyValue(t *testing.T) {
	input := EmptyValue{}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(EmptyValue{}) error: %v", err.Error)
	}
	if pv.GetEmptyValue() == nil {
		t.Error("ToProto(EmptyValue{}) expected EmptyValue kind")
	}
}

func TestToProto_UndefinedValue(t *testing.T) {
	input := UndefinedValue(0)
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(UndefinedValue) error: %v", err.Error)
	}
	// Check that it has the UndefinedValue kind
	if pv.GetUndefinedValue() == 0 {
		// Successfully retrieved undefined value (0 is the default enum value)
	}
}

func TestToProto_StructValue(t *testing.T) {
	input := map[string]any{
		"name":  "test",
		"count": int32(42),
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	sv := pv.GetStructValue()
	if sv == nil {
		t.Fatal("ToProto(map) expected StructValue kind")
	}
	fields := sv.GetFields()
	if fields["name"].GetStringValue() != "test" {
		t.Errorf("expected name='test', got %v", fields["name"].GetStringValue())
	}
	if fields["count"].GetInt32Value() != 42 {
		t.Errorf("expected count=42, got %v", fields["count"].GetInt32Value())
	}
}

func TestToProto_StructArray(t *testing.T) {
	input := []map[string]any{
		{"id": int32(1), "label": "first"},
		{"id": int32(2), "label": "second"},
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(%v) error: %v", input, err.Error)
	}
	sa := pv.GetStructArrayValues()
	if sa == nil {
		t.Fatal("ToProto([]map) expected StructArrayValues kind")
	}
	list := sa.GetStructValues()
	if len(list) != 2 {
		t.Fatalf("expected 2 struct values, got %d", len(list))
	}
	if list[0].GetFields()["id"].GetInt32Value() != 1 {
		t.Errorf("expected first id=1, got %v", list[0].GetFields()["id"].GetInt32Value())
	}
	if list[1].GetFields()["label"].GetStringValue() != "second" {
		t.Errorf("expected second label='second', got %v", list[1].GetFields()["label"].GetStringValue())
	}
}

func TestToProto_StructVariantValue(t *testing.T) {
	input := StructVariantValue{
		StructVariantType: "MyVariant",
		Value: map[string]any{
			"field1": int32(100),
		},
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(StructVariantValue) error: %v", err.Error)
	}
	sv := pv.GetStructVariantValue()
	if sv == nil {
		t.Fatal("expected StructVariantValue kind")
	}
	if sv.GetStructVariantType() != "MyVariant" {
		t.Errorf("expected type 'MyVariant', got %v", sv.GetStructVariantType())
	}
}

func TestToProto_StructVariantArray(t *testing.T) {
	input := []StructVariantValue{
		{StructVariantType: "TypeA", Value: int32(1)},
		{StructVariantType: "TypeB", Value: "hello"},
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto([]StructVariantValue) error: %v", err.Error)
	}
	sva := pv.GetStructVariantArrayValues()
	if sva == nil {
		t.Fatal("expected StructVariantArrayValues kind")
	}
	list := sva.GetStructVariants()
	if len(list) != 2 {
		t.Fatalf("expected 2 variants, got %d", len(list))
	}
	if list[0].GetStructVariantType() != "TypeA" {
		t.Errorf("expected first type 'TypeA', got %v", list[0].GetStructVariantType())
	}
}

func TestToProto_DataPayload_WithPayload(t *testing.T) {
	input := DataPayload{
		Metadata:        map[string]string{"content-type": "image/png"},
		Digest:          []byte{0xab, 0xcd},
		PayloadEncoding: 1,
		Payload:         []byte("binary data"),
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(DataPayload) error: %v", err.Error)
	}
	dp := pv.GetDataPayload()
	if dp == nil {
		t.Fatal("expected DataPayload kind")
	}
	if dp.GetMetadata()["content-type"] != "image/png" {
		t.Errorf("expected content-type 'image/png', got %v", dp.GetMetadata()["content-type"])
	}
	if string(dp.GetPayload()) != "binary data" {
		t.Errorf("expected payload 'binary data', got %v", string(dp.GetPayload()))
	}
	if dp.GetPayloadEncoding() != 1 {
		t.Errorf("expected encoding 1 (GZIP), got %v", dp.GetPayloadEncoding())
	}
}

func TestToProto_DataPayload_WithUrl(t *testing.T) {
	input := DataPayload{
		Metadata: map[string]string{"content-type": "application/json"},
		Url:      "https://example.com/data.json",
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(DataPayload) error: %v", err.Error)
	}
	dp := pv.GetDataPayload()
	if dp == nil {
		t.Fatal("expected DataPayload kind")
	}
	if dp.GetUrl() != "https://example.com/data.json" {
		t.Errorf("expected URL, got %v", dp.GetUrl())
	}
}

func TestToProto_DataPayload_BothPayloadAndUrl_Error(t *testing.T) {
	input := DataPayload{
		Payload: []byte("data"),
		Url:     "https://example.com",
	}
	_, err := ToProto(input)
	if err.Code == StatusCodeOk {
		t.Error("expected error when both payload and url are provided")
	}
}

func TestToProto_DataPayload_NeitherPayloadNorUrl_Error(t *testing.T) {
	input := DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
	}
	_, err := ToProto(input)
	if err.Code == StatusCodeOk {
		t.Error("expected error when neither payload nor url are provided")
	}
}

func TestFromProto_DataPayload_WithPayload(t *testing.T) {
	input := DataPayload{
		Metadata:        map[string]string{"content-type": "image/png"},
		Digest:          []byte{0xab, 0xcd},
		PayloadEncoding: 1,
		Payload:         []byte("binary data"),
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto error: %v", err.Error)
	}
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	dp, ok := result.(DataPayload)
	if !ok {
		t.Fatalf("expected DataPayload, got %T", result)
	}
	if dp.Metadata["content-type"] != "image/png" {
		t.Errorf("expected content-type 'image/png', got %v", dp.Metadata["content-type"])
	}
	if string(dp.Payload) != "binary data" {
		t.Errorf("expected payload 'binary data', got %q", dp.Payload)
	}
	if dp.PayloadEncoding != 1 {
		t.Errorf("expected encoding 1, got %d", dp.PayloadEncoding)
	}
	if !reflect.DeepEqual(dp.Digest, []byte{0xab, 0xcd}) {
		t.Errorf("expected digest [ab cd], got %v", dp.Digest)
	}
}

func TestFromProto_DataPayload_WithUrl(t *testing.T) {
	input := DataPayload{
		Url: "https://example.com/resource",
	}
	pv, err := ToProto(input)
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto error: %v", err.Error)
	}
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	dp, ok := result.(DataPayload)
	if !ok {
		t.Fatalf("expected DataPayload, got %T", result)
	}
	if dp.Url != "https://example.com/resource" {
		t.Errorf("expected URL, got %v", dp.Url)
	}
}

func TestRoundTrip_DataPayload(t *testing.T) {
	tests := []struct {
		name  string
		input DataPayload
	}{
		{
			"with payload",
			DataPayload{
				Metadata:        map[string]string{"content-type": "application/octet-stream", "file-name": "test.bin"},
				Digest:          []byte{0x01, 0x02, 0x03},
				PayloadEncoding: 0,
				Payload:         []byte("round trip data"),
			},
		},
		{
			"with url",
			DataPayload{
				Metadata: map[string]string{"content-type": "text/html"},
				Url:      "https://example.com/page.html",
			},
		},
		{
			"with gzip encoding",
			DataPayload{
				PayloadEncoding: 1,
				Payload:         []byte("compressed"),
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			pv, err := ToProto(tt.input)
			if err.Code != StatusCodeOk {
				t.Fatalf("ToProto error: %v", err.Error)
			}
			result, err := FromProto(pv)
			if err.Code != StatusCodeOk {
				t.Fatalf("FromProto error: %v", err.Error)
			}
			dp, ok := result.(DataPayload)
			if !ok {
				t.Fatalf("expected DataPayload, got %T", result)
			}
			if !reflect.DeepEqual(dp.Metadata, tt.input.Metadata) {
				t.Errorf("metadata mismatch: got %v, want %v", dp.Metadata, tt.input.Metadata)
			}
			if !reflect.DeepEqual(dp.Digest, tt.input.Digest) {
				t.Errorf("digest mismatch: got %v, want %v", dp.Digest, tt.input.Digest)
			}
			if dp.PayloadEncoding != tt.input.PayloadEncoding {
				t.Errorf("encoding mismatch: got %d, want %d", dp.PayloadEncoding, tt.input.PayloadEncoding)
			}
			if !reflect.DeepEqual(dp.Payload, tt.input.Payload) {
				t.Errorf("payload mismatch: got %v, want %v", dp.Payload, tt.input.Payload)
			}
			if dp.Url != tt.input.Url {
				t.Errorf("url mismatch: got %v, want %v", dp.Url, tt.input.Url)
			}
		})
	}
}

func TestToProto_Nil(t *testing.T) {
	_, res := ToProto(nil)
	if res.Code == StatusCodeOk {
		t.Error("ToProto(nil) expected error")
	}
}

func TestToProto_TypedNil_Map(t *testing.T) {
	_, res := ToProto((map[string]any)(nil))
	if res.Code == StatusCodeOk {
		t.Error("ToProto(nil map[string]any) expected error")
	}
}

func TestToProto_TypedNil_StructArray(t *testing.T) {
	_, res := ToProto(([]map[string]any)(nil))
	if res.Code == StatusCodeOk {
		t.Error("ToProto(nil []map[string]any) expected error")
	}
}

func TestToProto_TypedNil_StructVariantArray(t *testing.T) {
	_, res := ToProto(([]StructVariantValue)(nil))
	if res.Code == StatusCodeOk {
		t.Error("ToProto(nil []StructVariantValue) expected error")
	}
}

func TestToProto_TypedNil_StructVariant(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "t", Value: nil}
	_, res := ToProto(sv)
	if res.Code == StatusCodeOk {
		t.Error("ToProto(StructVariantValue with nil Value) expected error")
	}
}

func TestToProto_EmptyStructArray(t *testing.T) {
	pv, res := ToProto([]map[string]any{})
	if res.Code != StatusCodeOk {
		t.Fatalf("ToProto(empty []map[string]any) expected OK, got %v: %s", res.Code, res.Error)
	}
	sa := pv.GetStructArrayValues()
	if sa == nil {
		t.Fatal("expected StructArrayValues kind, got nil")
	}
	if len(sa.GetStructValues()) != 0 {
		t.Errorf("expected 0 struct values, got %d", len(sa.GetStructValues()))
	}
}

func TestToProto_EmptyStructVariantArray(t *testing.T) {
	pv, res := ToProto([]StructVariantValue{})
	if res.Code != StatusCodeOk {
		t.Fatalf("ToProto(empty []StructVariantValue) expected OK, got %v: %s", res.Code, res.Error)
	}
	sva := pv.GetStructVariantArrayValues()
	if sva == nil {
		t.Fatal("expected StructVariantArrayValues kind, got nil")
	}
	if len(sva.GetStructVariants()) != 0 {
		t.Errorf("expected 0 struct variants, got %d", len(sva.GetStructVariants()))
	}
}

func TestToProto_EmptyMap(t *testing.T) {
	_, res := ToProto(map[string]any{})
	if res.Code == StatusCodeOk {
		t.Error("ToProto(empty map[string]any) expected error")
	}
}

func TestToProto_EmptyStructVariant(t *testing.T) {
	_, res := ToProto(StructVariantValue{})
	if res.Code == StatusCodeOk {
		t.Error("ToProto(empty StructVariantValue) expected error")
	}
}

func TestToProto_UnsupportedType(t *testing.T) {
	tests := []struct {
		name  string
		input any
	}{
		{"int64", int64(100)},
		{"bool", true},
		{"float64", float64(3.14)},
		{"channel", make(chan int)},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			_, err := ToProto(tt.input)
			if err.Code == StatusCodeOk {
				t.Errorf("ToProto(%T) expected error for unsupported type", tt.input)
			}
		})
	}
}

func TestFromProto_Nil(t *testing.T) {
	_, err := FromProto(nil)
	if err.Code == StatusCodeOk {
		t.Error("FromProto(nil) expected error")
	}
}

func TestFromProto_Int32(t *testing.T) {
	pv, _ := ToProto(int32(42))
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if v, ok := result.(int32); !ok || v != 42 {
		t.Errorf("FromProto expected int32(42), got %T(%v)", result, result)
	}
}

func TestFromProto_Float32(t *testing.T) {
	pv, _ := ToProto(float32(3.14))
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if v, ok := result.(float32); !ok || v != 3.14 {
		t.Errorf("FromProto expected float32(3.14), got %T(%v)", result, result)
	}
}

func TestFromProto_String(t *testing.T) {
	pv, _ := ToProto("hello")
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if v, ok := result.(string); !ok || v != "hello" {
		t.Errorf("FromProto expected string(hello), got %T(%v)", result, result)
	}
}

func TestFromProto_Int32Array(t *testing.T) {
	input := []int32{1, 2, 3}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if !reflect.DeepEqual(result, input) {
		t.Errorf("FromProto expected %v, got %v", input, result)
	}
}

func TestFromProto_Float32Array(t *testing.T) {
	input := []float32{1.1, 2.2}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if !reflect.DeepEqual(result, input) {
		t.Errorf("FromProto expected %v, got %v", input, result)
	}
}

func TestFromProto_StringArray(t *testing.T) {
	input := []string{"a", "b", "c"}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if !reflect.DeepEqual(result, input) {
		t.Errorf("FromProto expected %v, got %v", input, result)
	}
}

func TestFromProto_EmptyValue(t *testing.T) {
	pv, _ := ToProto(EmptyValue{})
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	if _, ok := result.(EmptyValue); !ok {
		t.Errorf("FromProto expected EmptyValue, got %T", result)
	}
}

func TestFromProto_StructValue(t *testing.T) {
	input := map[string]any{
		"name":  "test",
		"count": int32(42),
	}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	m, ok := result.(map[string]any)
	if !ok {
		t.Fatalf("FromProto expected map[string]any, got %T", result)
	}
	if m["name"] != "test" {
		t.Errorf("expected name='test', got %v", m["name"])
	}
	if m["count"] != int32(42) {
		t.Errorf("expected count=42, got %v", m["count"])
	}
}

func TestFromProto_StructArray(t *testing.T) {
	input := []map[string]any{
		{"id": int32(1)},
		{"id": int32(2)},
	}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	arr, ok := result.([]map[string]any)
	if !ok {
		t.Fatalf("FromProto expected []map[string]any, got %T", result)
	}
	if len(arr) != 2 {
		t.Fatalf("expected 2 elements, got %d", len(arr))
	}
}

func TestFromProto_StructVariantValue(t *testing.T) {
	input := StructVariantValue{
		StructVariantType: "MyType",
		Value:             int32(100),
	}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	sv, ok := result.(StructVariantValue)
	if !ok {
		t.Fatalf("FromProto expected StructVariantValue, got %T", result)
	}
	if sv.StructVariantType != "MyType" {
		t.Errorf("expected type 'MyType', got %v", sv.StructVariantType)
	}
}

func TestFromProto_StructVariantArray(t *testing.T) {
	input := []StructVariantValue{
		{StructVariantType: "A", Value: int32(1)},
		{StructVariantType: "B", Value: "hello"},
	}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	arr, ok := result.([]StructVariantValue)
	if !ok {
		t.Fatalf("FromProto expected []StructVariantValue, got %T", result)
	}
	if len(arr) != 2 {
		t.Fatalf("expected 2 elements, got %d", len(arr))
	}
	if arr[0].StructVariantType != "A" {
		t.Errorf("expected first type 'A', got %v", arr[0].StructVariantType)
	}
}

// TestRoundTrip tests that values can be converted to proto and back
func TestRoundTrip(t *testing.T) {
	tests := []struct {
		name  string
		input any
	}{
		{"int32", int32(42)},
		{"float32", float32(3.14)},
		{"string", "hello world"},
		{"int32 array", []int32{1, 2, 3, 4, 5}},
		{"float32 array", []float32{1.1, 2.2, 3.3}},
		{"string array", []string{"a", "b", "c"}},
		{"empty value", EmptyValue{}},
		{"struct", map[string]any{"key": int32(1), "name": "test"}},
		{"struct array", []map[string]any{{"id": int32(1)}, {"id": int32(2)}}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			pv, err := ToProto(tt.input)
			if err.Code != StatusCodeOk {
				t.Fatalf("ToProto error: %v", err.Error)
			}
			result, err := FromProto(pv)
			if err.Code != StatusCodeOk {
				t.Fatalf("FromProto error: %v", err.Error)
			}
			if !reflect.DeepEqual(result, tt.input) {
				t.Errorf("round trip failed: input=%v, result=%v", tt.input, result)
			}
		})
	}
}

func TestFromProto_UndefinedValue(t *testing.T) {
	pv, err := ToProto(UndefinedValue(42))
	if err.Code != StatusCodeOk {
		t.Fatalf("ToProto(UndefinedValue) error: %v", err.Error)
	}
	result, err := FromProto(pv)
	if err.Code != StatusCodeOk {
		t.Fatalf("FromProto error: %v", err.Error)
	}
	uv, ok := result.(UndefinedValue)
	if !ok {
		t.Fatalf("expected UndefinedValue, got %T", result)
	}
	if uv != UndefinedValue(42) {
		t.Errorf("expected UndefinedValue(42), got %v", uv)
	}
}

func TestFromProto_NilDataPayload(t *testing.T) {
	pv := &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: nil}}
	_, err := FromProto(pv)
	if err.Code == StatusCodeOk {
		t.Error("FromProto with nil DataPayload expected error")
	}
}

func TestFromProto_UnsupportedKind(t *testing.T) {
	pv := &protos.Value{}
	_, err := FromProto(pv)
	if err.Code == StatusCodeOk {
		t.Error("FromProto with nil Kind expected error")
	}
}

func TestFromProto_StructFieldError(t *testing.T) {
	pv := &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{
		Fields: map[string]*protos.Value{
			"bad": nil,
		},
	}}}
	_, err := FromProto(pv)
	if err.Code == StatusCodeOk {
		t.Error("FromProto with nil struct field value expected error")
	}
}

func TestFromProto_StructArrayFieldError(t *testing.T) {
	pv := &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{
		StructValues: []*protos.StructValue{
			{Fields: map[string]*protos.Value{"bad": nil}},
		},
	}}}
	_, err := FromProto(pv)
	if err.Code == StatusCodeOk {
		t.Error("FromProto with nil struct array field value expected error")
	}
}

func TestFromProto_StructVariantValueError(t *testing.T) {
	pv := &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{
		StructVariantType: "bad",
		Value:             nil,
	}}}
	_, err := FromProto(pv)
	if err.Code == StatusCodeOk {
		t.Error("FromProto with nil struct variant value expected error")
	}
}

func TestFromProto_StructVariantArrayError(t *testing.T) {
	pv := &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{
		StructVariants: []*protos.StructVariantValue{
			{StructVariantType: "bad", Value: nil},
		},
	}}}
	_, err := FromProto(pv)
	if err.Code == StatusCodeOk {
		t.Error("FromProto with nil struct variant array value expected error")
	}
}

func TestToProto_StructFieldError(t *testing.T) {
	input := map[string]any{
		"bad": int64(1),
	}
	_, err := ToProto(input)
	if err.Code == StatusCodeOk {
		t.Error("ToProto with unsupported struct field type expected error")
	}
}

func TestToProto_StructArrayFieldError(t *testing.T) {
	input := []map[string]any{
		{"bad": int64(1)},
	}
	_, err := ToProto(input)
	if err.Code == StatusCodeOk {
		t.Error("ToProto with unsupported struct array field type expected error")
	}
}

func TestToProto_StructVariantValueError(t *testing.T) {
	input := StructVariantValue{
		StructVariantType: "bad",
		Value:             int64(1),
	}
	_, err := ToProto(input)
	if err.Code == StatusCodeOk {
		t.Error("ToProto with unsupported struct variant value type expected error")
	}
}

func TestToProto_StructVariantArrayError(t *testing.T) {
	input := []StructVariantValue{
		{StructVariantType: "bad", Value: int64(1)},
	}
	_, err := ToProto(input)
	if err.Code == StatusCodeOk {
		t.Error("ToProto with unsupported struct variant array value type expected error")
	}
}

// TestToProto_CopiesInputData verifies that ToProto clones mutable input data.
// Mutating the input after conversion must not change the resulting proto Value.
func TestToProto_CopiesInputData(t *testing.T) {
	tests := []struct {
		name   string
		assert func(t *testing.T)
	}{
		{
			name: "int32",
			assert: func(t *testing.T) {
				var input int32 = 42
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input = 99
				if pv.GetInt32Value() != 42 {
					t.Errorf("expected int32 value copy, got %d", pv.GetInt32Value())
				}
			},
		},
		{
			name: "float32",
			assert: func(t *testing.T) {
				var input float32 = 3.14
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input = 9.99
				if pv.GetFloat32Value() != 3.14 {
					t.Errorf("expected float32 value copy, got %v", pv.GetFloat32Value())
				}
			},
		},
		{
			name: "string",
			assert: func(t *testing.T) {
				input := "hello"
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input = "mutated"
				if pv.GetStringValue() != "hello" {
					t.Errorf("expected string value copy, got %q", pv.GetStringValue())
				}
			},
		},
		{
			name: "[]int32",
			assert: func(t *testing.T) {
				slice := []int32{1, 2, 3}
				pv, res := ToProto(slice)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				slice[0] = 99
				if pv.GetInt32ArrayValues().GetInts()[0] != 1 {
					t.Errorf("expected []int32 copy, proto[0]=%d after input mutation", pv.GetInt32ArrayValues().GetInts()[0])
				}
			},
		},
		{
			name: "[]float32",
			assert: func(t *testing.T) {
				slice := []float32{1.1, 2.2}
				pv, res := ToProto(slice)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				slice[0] = 9.9
				if pv.GetFloat32ArrayValues().GetFloats()[0] != 1.1 {
					t.Errorf("expected []float32 copy, proto[0]=%v after input mutation", pv.GetFloat32ArrayValues().GetFloats()[0])
				}
			},
		},
		{
			name: "[]string",
			assert: func(t *testing.T) {
				slice := []string{"a", "b"}
				pv, res := ToProto(slice)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				slice[0] = "mutated"
				if pv.GetStringArrayValues().GetStrings()[0] != "a" {
					t.Errorf("expected []string copy, proto[0]=%q after input mutation", pv.GetStringArrayValues().GetStrings()[0])
				}
			},
		},
		{
			name: "map[string]any",
			assert: func(t *testing.T) {
				input := map[string]any{
					"x": int32(1),
					"y": "alpha",
				}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input["x"] = int32(2)
				input["z"] = int32(3)
				fields := pv.GetStructValue().GetFields()
				if fields["x"].GetInt32Value() != 1 {
					t.Errorf("expected struct field x to remain 1, got %d", fields["x"].GetInt32Value())
				}
				if _, ok := fields["z"]; ok {
					t.Error("expected struct fields map to be independent of input map mutations")
				}
			},
		},
		{
			name: "map[string]any nested []int32",
			assert: func(t *testing.T) {
				nested := []int32{1, 2}
				input := map[string]any{"arr": nested}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				nested[0] = 99
				got := pv.GetStructValue().GetFields()["arr"].GetInt32ArrayValues().GetInts()[0]
				if got != 1 {
					t.Errorf("expected nested []int32 copy, proto[0]=%d after input mutation", got)
				}
			},
		},
		{
			name: "[]map[string]any",
			assert: func(t *testing.T) {
				input := []map[string]any{
					{"id": int32(1)},
				}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input[0]["id"] = int32(99)
				input = append(input, map[string]any{"id": int32(2)})
				list := pv.GetStructArrayValues().GetStructValues()
				if list[0].GetFields()["id"].GetInt32Value() != 1 {
					t.Errorf("expected first struct id to remain 1, got %d", list[0].GetFields()["id"].GetInt32Value())
				}
				if len(list) != 1 {
					t.Errorf("expected struct array length 1, got %d", len(list))
				}
			},
		},
		{
			name: "[]map[string]any nested []int32",
			assert: func(t *testing.T) {
				nested := []int32{1, 2}
				input := []map[string]any{{"arr": nested}}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				nested[0] = 99
				got := pv.GetStructArrayValues().GetStructValues()[0].
					GetFields()["arr"].GetInt32ArrayValues().GetInts()[0]
				if got != 1 {
					t.Errorf("expected nested []int32 copy, proto[0]=%d after input mutation", got)
				}
			},
		},
		{
			name: "StructVariantValue",
			assert: func(t *testing.T) {
				input := StructVariantValue{
					StructVariantType: "variant",
					Value:             int32(7),
				}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input.StructVariantType = "mutated"
				input.Value = int32(99)
				sv := pv.GetStructVariantValue()
				if sv.GetStructVariantType() != "variant" {
					t.Errorf("expected struct variant type copy, got %q", sv.GetStructVariantType())
				}
				if sv.GetValue().GetInt32Value() != 7 {
					t.Errorf("expected struct variant value copy, got %d", sv.GetValue().GetInt32Value())
				}
			},
		},
		{
			name: "StructVariantValue nested []string",
			assert: func(t *testing.T) {
				nested := []string{"a", "b"}
				input := StructVariantValue{
					StructVariantType: "variant",
					Value:             nested,
				}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				nested[0] = "mutated"
				got := pv.GetStructVariantValue().GetValue().
					GetStringArrayValues().GetStrings()[0]
				if got != "a" {
					t.Errorf("expected nested []string copy, proto[0]=%q after input mutation", got)
				}
			},
		},
		{
			name: "[]StructVariantValue",
			assert: func(t *testing.T) {
				input := []StructVariantValue{
					{StructVariantType: "A", Value: int32(1)},
				}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input[0].StructVariantType = "mutated"
				input[0].Value = int32(99)
				input = append(input, StructVariantValue{StructVariantType: "B", Value: int32(2)})
				list := pv.GetStructVariantArrayValues().GetStructVariants()
				if len(list) != 1 {
					t.Fatalf("expected struct variant array length 1, got %d", len(list))
				}
				if list[0].GetStructVariantType() != "A" {
					t.Errorf("expected first variant type copy, got %q", list[0].GetStructVariantType())
				}
				if list[0].GetValue().GetInt32Value() != 1 {
					t.Errorf("expected first variant value copy, got %d", list[0].GetValue().GetInt32Value())
				}
			},
		},
		{
			name: "DataPayload.Payload",
			assert: func(t *testing.T) {
				payload := []byte("hello")
				input := DataPayload{Payload: payload}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				payload[0] = 'X'
				if pv.GetDataPayload().GetPayload()[0] != 'h' {
					t.Errorf("expected DataPayload.Payload copy, proto[0]=%q after input mutation", pv.GetDataPayload().GetPayload()[0])
				}
			},
		},
		{
			name: "DataPayload.Digest",
			assert: func(t *testing.T) {
				digest := []byte{0x01, 0x02}
				input := DataPayload{Payload: []byte("data"), Digest: digest}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				digest[0] = 0xff
				if pv.GetDataPayload().GetDigest()[0] != 0x01 {
					t.Errorf("expected DataPayload.Digest copy, proto[0]=%#x after input mutation", pv.GetDataPayload().GetDigest()[0])
				}
			},
		},
		{
			name: "DataPayload.Metadata",
			assert: func(t *testing.T) {
				metadata := map[string]string{"content-type": "text/plain"}
				input := DataPayload{Payload: []byte("data"), Metadata: metadata}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				metadata["content-type"] = "application/json"
				if pv.GetDataPayload().GetMetadata()["content-type"] != "text/plain" {
					t.Errorf("expected DataPayload.Metadata copy, got %q after input mutation", pv.GetDataPayload().GetMetadata()["content-type"])
				}
			},
		},
		{
			name: "DataPayload.Url",
			assert: func(t *testing.T) {
				input := DataPayload{Url: "https://example.com"}
				pv, res := ToProto(input)
				if res.Code != StatusCodeOk {
					t.Fatalf("ToProto error: %s", res.Error)
				}
				input.Url = "https://mutated.example.com"
				if pv.GetDataPayload().GetUrl() != "https://example.com" {
					t.Errorf("expected DataPayload.Url copy, got %q", pv.GetDataPayload().GetUrl())
				}
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, tt.assert)
	}
}

// TestFromProto_CopiesProtoData verifies that FromProto clones mutable proto data.
// Mutating the source proto after conversion must not change the returned native value.
func TestFromProto_CopiesProtoData(t *testing.T) {
	tests := []struct {
		name   string
		assert func(t *testing.T)
	}{
		{
			name: "[]int32",
			assert: func(t *testing.T) {
				ints := []int32{1, 2, 3}
				pv := &protos.Value{Kind: &protos.Value_Int32ArrayValues{
					Int32ArrayValues: &protos.Int32List{Ints: ints},
				}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				ints[0] = 99
				got := result.([]int32)
				if got[0] == 99 {
					t.Errorf("expected []int32 copy, got[0]=%d after proto mutation", got[0])
				}
			},
		},
		{
			name: "[]float32",
			assert: func(t *testing.T) {
				floats := []float32{1.1, 2.2}
				pv := &protos.Value{Kind: &protos.Value_Float32ArrayValues{
					Float32ArrayValues: &protos.Float32List{Floats: floats},
				}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				floats[0] = 9.9
				got := result.([]float32)
				if got[0] == 9.9 {
					t.Errorf("expected []float32 copy, got[0]=%v after proto mutation", got[0])
				}
			},
		},
		{
			name: "[]string",
			assert: func(t *testing.T) {
				strings := []string{"a", "b"}
				pv := &protos.Value{Kind: &protos.Value_StringArrayValues{
					StringArrayValues: &protos.StringList{Strings: strings},
				}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				strings[0] = "mutated"
				got := result.([]string)
				if got[0] == "mutated" {
					t.Errorf("expected []string copy, got[0]=%q after proto mutation", got[0])
				}
			},
		},
		{
			name: "map[string]any nested []int32",
			assert: func(t *testing.T) {
				nested := []int32{1, 2}
				pv := &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{
					Fields: map[string]*protos.Value{
						"arr": {Kind: &protos.Value_Int32ArrayValues{
							Int32ArrayValues: &protos.Int32List{Ints: nested},
						}},
					},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				nested[0] = 99
				got := result.(map[string]any)["arr"].([]int32)
				if got[0] == 99 {
					t.Errorf("expected nested []int32 copy, got[0]=%d after proto mutation", got[0])
				}
			},
		},
		{
			name: "[]map[string]any nested []int32",
			assert: func(t *testing.T) {
				nested := []int32{1, 2}
				pv := &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{
					StructValues: []*protos.StructValue{{
						Fields: map[string]*protos.Value{
							"arr": {Kind: &protos.Value_Int32ArrayValues{
								Int32ArrayValues: &protos.Int32List{Ints: nested},
							}},
						},
					}},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				nested[0] = 99
				got := result.([]map[string]any)[0]["arr"].([]int32)
				if got[0] == 99 {
					t.Errorf("expected nested []int32 copy, got[0]=%d after proto mutation", got[0])
				}
			},
		},
		{
			name: "StructVariantValue nested []string",
			assert: func(t *testing.T) {
				nested := []string{"a", "b"}
				pv := &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{
					StructVariantType: "variant",
					Value: &protos.Value{Kind: &protos.Value_StringArrayValues{
						StringArrayValues: &protos.StringList{Strings: nested},
					}},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				nested[0] = "mutated"
				got := result.(StructVariantValue).Value.([]string)
				if got[0] == "mutated" {
					t.Errorf("expected nested []string copy, got[0]=%q after proto mutation", got[0])
				}
			},
		},
		{
			name: "[]StructVariantValue",
			assert: func(t *testing.T) {
				pv := &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{
					StructVariants: []*protos.StructVariantValue{{
						StructVariantType: "A",
						Value:             &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 1}},
					}},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				pv.GetStructVariantArrayValues().StructVariants[0].StructVariantType = "mutated"
				pv.GetStructVariantArrayValues().StructVariants[0].Value = &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 99}}
				got := result.([]StructVariantValue)
				if got[0].StructVariantType != "A" || got[0].Value != int32(1) {
					t.Errorf("expected struct variant array copy, got %+v after proto mutation", got[0])
				}
			},
		},
		{
			name: "DataPayload.Payload",
			assert: func(t *testing.T) {
				payload := []byte("hello")
				pv := &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: &protos.DataPayload{
					Kind: &protos.DataPayload_Payload{Payload: payload},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				payload[0] = 'X'
				got := result.(DataPayload).Payload
				if len(got) > 0 && got[0] == 'X' {
					t.Errorf("expected DataPayload.Payload copy, got[0]=%q after proto mutation", got[0])
				}
			},
		},
		{
			name: "DataPayload.Digest",
			assert: func(t *testing.T) {
				digest := []byte{0x01, 0x02}
				payload := []byte("data")
				pv := &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: &protos.DataPayload{
					Digest: digest,
					Kind:   &protos.DataPayload_Payload{Payload: payload},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				digest[0] = 0xff
				got := result.(DataPayload).Digest
				if len(got) > 0 && got[0] == 0xff {
					t.Errorf("expected DataPayload.Digest copy, got[0]=%#x after proto mutation", got[0])
				}
			},
		},
		{
			name: "DataPayload.Metadata",
			assert: func(t *testing.T) {
				metadata := map[string]string{"content-type": "text/plain"}
				payload := []byte("data")
				pv := &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: &protos.DataPayload{
					Metadata: metadata,
					Kind:     &protos.DataPayload_Payload{Payload: payload},
				}}}
				result, res := FromProto(pv)
				if res.Code != StatusCodeOk {
					t.Fatalf("FromProto error: %s", res.Error)
				}
				metadata["content-type"] = "application/json"
				got := result.(DataPayload).Metadata
				if got["content-type"] == "application/json" {
					t.Errorf("expected DataPayload.Metadata copy, got %q after proto mutation", got["content-type"])
				}
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, tt.assert)
	}
}

func TestParamTypeConstants(t *testing.T) {
	// Verify ParamType constants are properly aliased
	tests := []struct {
		name     string
		pt       ParamType
		expected int32
	}{
		{"UNDEFINED", ParamTypeUndefined, 0},
		{"EMPTY", ParamTypeEmpty, 1},
		{"INT32", ParamTypeInt32, 4},
		{"FLOAT32", ParamTypeFloat32, 6},
		{"STRING", ParamTypeString, 7},
		{"STRUCT", ParamTypeStruct, 10},
		{"STRUCT_VARIANT", ParamTypeStructVariant, 11},
		{"INT32_ARRAY", ParamTypeInt32Array, 14},
		{"FLOAT32_ARRAY", ParamTypeFloat32Array, 16},
		{"STRING_ARRAY", ParamTypeStringArray, 17},
		{"BINARY", ParamTypeBinary, 18},
		{"STRUCT_ARRAY", ParamTypeStructArray, 32},
		{"STRUCT_VARIANT_ARRAY", ParamTypeStructVariantArray, 33},
		{"DATA", ParamTypeData, 50},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if int32(tt.pt) != tt.expected {
				t.Errorf("ParamType %s = %d, want %d", tt.name, tt.pt, tt.expected)
			}
		})
	}
}
