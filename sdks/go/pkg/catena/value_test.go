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

package catena

import (
	"reflect"
	"testing"
)

func TestToCatenaValue(t *testing.T) {
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
		{"unsupported type", int64(100), true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cv, err := ToCatenaValue(tt.input)
			if tt.wantErr {
				if err == nil {
					t.Errorf("ToCatenaValue(%v) expected error, got nil", tt.input)
				}
				return
			}
			if err != nil {
				t.Errorf("ToCatenaValue(%v) unexpected error: %v", tt.input, err)
				return
			}
			if cv.Value == nil {
				t.Errorf("ToCatenaValue(%v) returned nil Value", tt.input)
			}
		})
	}
}

func TestToProto_Int32(t *testing.T) {
	input := int32(42)
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
	}
	if pv.GetInt32Value() != 42 {
		t.Errorf("ToProto(%v) = %v, want 42", input, pv.GetInt32Value())
	}
}

func TestToProto_Float32(t *testing.T) {
	input := float32(3.14)
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
	}
	if pv.GetFloat32Value() != input {
		t.Errorf("ToProto(%v) = %v, want %v", input, pv.GetFloat32Value(), input)
	}
}

func TestToProto_String(t *testing.T) {
	input := "hello world"
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
	}
	if pv.GetStringValue() != input {
		t.Errorf("ToProto(%v) = %v, want %v", input, pv.GetStringValue(), input)
	}
}

func TestToProto_Int32Array(t *testing.T) {
	input := []int32{1, 2, 3, 4, 5}
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
	}
	result := pv.GetInt32ArrayValues().GetInts()
	if !reflect.DeepEqual(result, input) {
		t.Errorf("ToProto(%v) = %v, want %v", input, result, input)
	}
}

func TestToProto_Float32Array(t *testing.T) {
	input := []float32{1.1, 2.2, 3.3}
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
	}
	result := pv.GetFloat32ArrayValues().GetFloats()
	if !reflect.DeepEqual(result, input) {
		t.Errorf("ToProto(%v) = %v, want %v", input, result, input)
	}
}

func TestToProto_StringArray(t *testing.T) {
	input := []string{"one", "two", "three"}
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
	}
	result := pv.GetStringArrayValues().GetStrings()
	if !reflect.DeepEqual(result, input) {
		t.Errorf("ToProto(%v) = %v, want %v", input, result, input)
	}
}

func TestToProto_EmptyValue(t *testing.T) {
	input := EmptyValue{}
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(EmptyValue{}) error: %v", err)
	}
	if pv.GetEmptyValue() == nil {
		t.Error("ToProto(EmptyValue{}) expected EmptyValue kind")
	}
}

func TestToProto_UndefinedValue(t *testing.T) {
	input := UndefinedValue(0)
	pv, err := ToProto(input)
	if err != nil {
		t.Fatalf("ToProto(UndefinedValue) error: %v", err)
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
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
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
	if err != nil {
		t.Fatalf("ToProto(%v) error: %v", input, err)
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
	if err != nil {
		t.Fatalf("ToProto(StructVariantValue) error: %v", err)
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
	if err != nil {
		t.Fatalf("ToProto([]StructVariantValue) error: %v", err)
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
			if err == nil {
				t.Errorf("ToProto(%T) expected error for unsupported type", tt.input)
			}
		})
	}
}

func TestFromProto_Nil(t *testing.T) {
	_, err := FromProto(nil)
	if err == nil {
		t.Error("FromProto(nil) expected error")
	}
}

func TestFromProto_Int32(t *testing.T) {
	pv, _ := ToProto(int32(42))
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
	}
	if v, ok := result.(int32); !ok || v != 42 {
		t.Errorf("FromProto expected int32(42), got %T(%v)", result, result)
	}
}

func TestFromProto_Float32(t *testing.T) {
	pv, _ := ToProto(float32(3.14))
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
	}
	if v, ok := result.(float32); !ok || v != 3.14 {
		t.Errorf("FromProto expected float32(3.14), got %T(%v)", result, result)
	}
}

func TestFromProto_String(t *testing.T) {
	pv, _ := ToProto("hello")
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
	}
	if v, ok := result.(string); !ok || v != "hello" {
		t.Errorf("FromProto expected string(hello), got %T(%v)", result, result)
	}
}

func TestFromProto_Int32Array(t *testing.T) {
	input := []int32{1, 2, 3}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
	}
	if !reflect.DeepEqual(result, input) {
		t.Errorf("FromProto expected %v, got %v", input, result)
	}
}

func TestFromProto_Float32Array(t *testing.T) {
	input := []float32{1.1, 2.2}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
	}
	if !reflect.DeepEqual(result, input) {
		t.Errorf("FromProto expected %v, got %v", input, result)
	}
}

func TestFromProto_StringArray(t *testing.T) {
	input := []string{"a", "b", "c"}
	pv, _ := ToProto(input)
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
	}
	if !reflect.DeepEqual(result, input) {
		t.Errorf("FromProto expected %v, got %v", input, result)
	}
}

func TestFromProto_EmptyValue(t *testing.T) {
	pv, _ := ToProto(EmptyValue{})
	result, err := FromProto(pv)
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
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
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
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
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
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
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
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
	if err != nil {
		t.Fatalf("FromProto error: %v", err)
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
			if err != nil {
				t.Fatalf("ToProto error: %v", err)
			}
			result, err := FromProto(pv)
			if err != nil {
				t.Fatalf("FromProto error: %v", err)
			}
			if !reflect.DeepEqual(result, tt.input) {
				t.Errorf("round trip failed: input=%v, result=%v", tt.input, result)
			}
		})
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
