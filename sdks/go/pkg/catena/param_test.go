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
 * @brief Param test for the Catena SDK.
 * @file param_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-11
 */

package catena

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// --- Factory tests ---

func TestNewParamInt32(t *testing.T) {
	p := NewParamInt32(42).Proto
	if p.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", p.GetType())
	}
	if p.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", p.GetValue().GetInt32Value())
	}
}

func TestNewParamFloat32(t *testing.T) {
	p := NewParamFloat32(3.14).Proto
	if p.GetType() != protos.ParamType_FLOAT32 {
		t.Errorf("expected FLOAT32, got %v", p.GetType())
	}
	if p.GetValue().GetFloat32Value() != 3.14 {
		t.Errorf("expected 3.14, got %f", p.GetValue().GetFloat32Value())
	}
}

func TestNewParamString(t *testing.T) {
	p := NewParamString("hello").Proto
	if p.GetType() != protos.ParamType_STRING {
		t.Errorf("expected STRING, got %v", p.GetType())
	}
	if p.GetValue().GetStringValue() != "hello" {
		t.Errorf("expected 'hello', got %q", p.GetValue().GetStringValue())
	}
}

func TestNewParamStruct(t *testing.T) {
	p := NewParamStruct().Proto
	if p.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", p.GetType())
	}
}

func TestNewParamStructVariant(t *testing.T) {
	p := NewParamStructVariant().Proto
	if p.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", p.GetType())
	}
}

func TestNewParamInt32Array(t *testing.T) {
	p := NewParamInt32Array([]int32{1, 2, 3}).Proto
	if p.GetType() != protos.ParamType_INT32_ARRAY {
		t.Errorf("expected INT32_ARRAY, got %v", p.GetType())
	}
	vals := p.GetValue().GetInt32ArrayValues().GetInts()
	if len(vals) != 3 || vals[0] != 1 {
		t.Errorf("unexpected int32 array: %v", vals)
	}
}

func TestNewParamFloat32Array(t *testing.T) {
	p := NewParamFloat32Array([]float32{1.1, 2.2}).Proto
	if p.GetType() != protos.ParamType_FLOAT32_ARRAY {
		t.Errorf("expected FLOAT32_ARRAY, got %v", p.GetType())
	}
	vals := p.GetValue().GetFloat32ArrayValues().GetFloats()
	if len(vals) != 2 {
		t.Errorf("expected 2 floats, got %d", len(vals))
	}
}

func TestNewParamStringArray(t *testing.T) {
	p := NewParamStringArray([]string{"a", "b"}).Proto
	if p.GetType() != protos.ParamType_STRING_ARRAY {
		t.Errorf("expected STRING_ARRAY, got %v", p.GetType())
	}
	vals := p.GetValue().GetStringArrayValues().GetStrings()
	if len(vals) != 2 || vals[0] != "a" {
		t.Errorf("unexpected string array: %v", vals)
	}
}

func TestNewParamBinary(t *testing.T) {
	p := NewParamBinary([]byte{0xDE, 0xAD}).Proto
	if p.GetType() != protos.ParamType_BINARY {
		t.Errorf("expected BINARY, got %v", p.GetType())
	}
	payload := p.GetValue().GetDataPayload().GetPayload()
	if len(payload) != 2 || payload[0] != 0xDE {
		t.Errorf("unexpected binary payload: %v", payload)
	}
}

func TestNewParamStructArray(t *testing.T) {
	p := NewParamStructArray().Proto
	if p.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", p.GetType())
	}
}

func TestNewParamStructVariantArray(t *testing.T) {
	p := NewParamStructVariantArray().Proto
	if p.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", p.GetType())
	}
}

func TestNewParamEmpty(t *testing.T) {
	p := NewParamEmpty().Proto
	if p.GetType() != protos.ParamType_EMPTY {
		t.Errorf("expected EMPTY, got %v", p.GetType())
	}
	if p.GetValue().GetEmptyValue() == nil {
		t.Error("expected EmptyValue to be set")
	}
}

func TestNewParamData(t *testing.T) {
	dp := ToPayload([]byte("content"), "text/plain", "test.txt")
	p := NewParamData(dp).Proto
	if p.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", p.GetType())
	}
	if p.GetValue().GetDataPayload() == nil {
		t.Error("expected DataPayload to be set")
	}
}

// --- With* method tests ---

func TestWithName(t *testing.T) {
	p := NewParamInt32(0).WithName(NewPolyglotText("en", "Temperature")).Proto
	if p.GetName().GetDisplayStrings()["en"] != "Temperature" {
		t.Errorf("expected 'Temperature', got %q", p.GetName().GetDisplayStrings()["en"])
	}
}

func TestWithReadOnly(t *testing.T) {
	p := NewParamInt32(0).WithReadOnly(true).Proto
	if !p.GetReadOnly() {
		t.Error("expected read_only=true")
	}
}

func TestWithWidget(t *testing.T) {
	p := NewParamInt32(0).WithWidget("slider").Proto
	if p.GetWidget() != "slider" {
		t.Errorf("expected 'slider', got %q", p.GetWidget())
	}
}

func TestWithPrecision(t *testing.T) {
	p := NewParamFloat32(0).WithPrecision(3).Proto
	if p.GetPrecision() != 3 {
		t.Errorf("expected precision 3, got %d", p.GetPrecision())
	}
}

// precision is part of the Catena protocol for both FLOAT32 and FLOAT32_ARRAY
// params (see device.one_of_everything.yaml, which sets precision: 3 on a
// FLOAT32_ARRAY).
func TestWithPrecision_Float32Array(t *testing.T) {
	p := NewParamFloat32Array([]float32{1.1, 2.2}).WithPrecision(3).Proto
	if p.GetPrecision() != 3 {
		t.Errorf("expected precision 3 on FLOAT32_ARRAY, got %d", p.GetPrecision())
	}
}

func TestWithPrecision_WrongType(t *testing.T) {
	p := NewParamInt32(0).WithPrecision(3).Proto
	if p.GetPrecision() != 0 {
		t.Error("expected precision to remain 0 for non-float param")
	}
}

func TestWithMaxLength(t *testing.T) {
	p := NewParamString("").WithMaxLength(255).Proto
	if p.GetMaxLength() != 255 {
		t.Errorf("expected max_length 255, got %d", p.GetMaxLength())
	}
}

// max_length is part of the Catena protocol for both STRING and STRING_ARRAY
// params (see param.string_array_length.yaml, which sets max_length: 10 on a
// STRING_ARRAY to cap the number of strings in the array).
func TestWithMaxLength_StringArray(t *testing.T) {
	p := NewParamStringArray([]string{"a", "b"}).WithMaxLength(10).Proto
	if p.GetMaxLength() != 10 {
		t.Errorf("expected max_length 10 on STRING_ARRAY, got %d", p.GetMaxLength())
	}
}

func TestWithMaxLength_WrongType(t *testing.T) {
	p := NewParamInt32(0).WithMaxLength(255).Proto
	if p.GetMaxLength() != 0 {
		t.Error("expected max_length to remain 0 for param type that does not support max_length")
	}
}

func TestWithAccessScope(t *testing.T) {
	p := NewParamInt32(0).WithAccessScope("admin").Proto
	if p.GetAccessScope() != "admin" {
		t.Errorf("expected 'admin', got %q", p.GetAccessScope())
	}
}

func TestWithClientHint(t *testing.T) {
	p := NewParamInt32(0).
		WithClientHint("display", "compact").
		WithClientHint("color", "red").
		Proto
	if p.GetClientHints()["display"] != "compact" {
		t.Errorf("expected client_hint 'display'='compact', got %q", p.GetClientHints()["display"])
	}
	if p.GetClientHints()["color"] != "red" {
		t.Errorf("expected client_hint 'color'='red', got %q", p.GetClientHints()["color"])
	}
}

func TestWithParam_Struct(t *testing.T) {
	child := NewParamInt32(10).WithName(NewPolyglotText("en", "child"))
	p := NewParamStruct().WithParam("brightness", child).Proto

	sub := p.GetParams()["brightness"]
	if sub == nil {
		t.Fatal("expected sub-param 'brightness'")
	}
	if sub.GetValue().GetInt32Value() != 10 {
		t.Errorf("expected sub-param value 10, got %d", sub.GetValue().GetInt32Value())
	}
}

func TestWithParam_DeepClone(t *testing.T) {
	child := NewParamInt32(1)
	parent := NewParamStruct().WithParam("x", child).Proto

	child.SetValue(int32(999))
	if parent.GetParams()["x"].GetValue().GetInt32Value() != 1 {
		t.Error("expected sub-param to be a deep clone, not affected by later mutation")
	}
}

func TestWithParam_WrongType(t *testing.T) {
	child := NewParamInt32(0)
	p := NewParamInt32(0).WithParam("x", child).Proto
	if len(p.GetParams()) != 0 {
		t.Error("expected no sub-params on INT32 param")
	}
}

func TestWithParam_Nil(t *testing.T) {
	// Passing a nil sub-param must not panic and must not add an entry,
	// preserving the contract that method chaining is never interrupted
	// by error handling.
	cp := NewParamStruct().WithParam("x", nil)
	p := cp.Proto
	if len(p.GetParams()) != 0 {
		t.Error("expected nil sub-param to be a no-op")
	}
	// Chaining must continue to work after the no-op.
	child := NewParamInt32(7)
	p = cp.WithParam("y", child).Proto
	if p.GetParams()["y"].GetValue().GetInt32Value() != 7 {
		t.Error("expected chaining to continue working after nil sub-param")
	}
}

func TestWithResponse(t *testing.T) {
	p := NewParamEmpty().WithResponse(true).Proto
	if !p.GetResponse() {
		t.Error("expected response=true")
	}
}

func TestWithHelp(t *testing.T) {
	p := NewParamInt32(0).WithHelp(NewPolyglotText("en", "Adjusts volume")).Proto
	if p.GetHelp().GetDisplayStrings()["en"] != "Adjusts volume" {
		t.Errorf("unexpected help text: %q", p.GetHelp().GetDisplayStrings()["en"])
	}
}

func TestWithOidAliases(t *testing.T) {
	p := NewParamInt32(0).WithOidAliases("/old/path", "/legacy/path").Proto
	aliases := p.GetOidAliases()
	if len(aliases) != 2 || aliases[0] != "/old/path" {
		t.Errorf("unexpected aliases: %v", aliases)
	}
}

func TestWithMinimalSet(t *testing.T) {
	p := NewParamInt32(0).WithMinimalSet(true).Proto
	if !p.GetMinimalSet() {
		t.Error("expected minimal_set=true")
	}
}

func TestWithStateless(t *testing.T) {
	p := NewParamInt32(0).WithStateless(true).Proto
	if !p.GetStateless() {
		t.Error("expected stateless=true")
	}
}

func TestWithTemplateOid(t *testing.T) {
	p := NewParamInt32(0).WithTemplateOid("templates.volume").Proto
	if p.GetTemplateOid() != "templates.volume" {
		t.Errorf("expected 'templates.volume', got %q", p.GetTemplateOid())
	}
}

// --- Constraint validation tests ---

func TestWithConstraint_ValidInt32Choice(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_CHOICE,
		Kind: &protos.Constraint_Int32Choice{
			Int32Choice: &protos.Int32ChoiceConstraint{
				Choices: []*protos.Int32ChoiceConstraint_IntChoice{
					{Value: 1, Name: &protos.PolyglotText{DisplayStrings: map[string]string{"en": "one"}}},
				},
			},
		},
	}
	p := NewParamInt32(0).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
	if p.GetConstraint().GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", p.GetConstraint().GetType())
	}
}

func TestWithConstraint_ValidIntRange(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_RANGE,
		Kind: &protos.Constraint_Int32Range{
			Int32Range: &protos.Int32RangeConstraint{MinValue: 0, MaxValue: 100, Step: 1},
		},
	}
	p := NewParamInt32(50).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidFloatRange(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_FLOAT_RANGE,
		Kind: &protos.Constraint_FloatRange{
			FloatRange: &protos.FloatRangeConstraint{MinValue: 0, MaxValue: 1, Step: 0.01},
		},
	}
	p := NewParamFloat32(0.5).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidStringChoice(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_STRING_CHOICE,
		Kind: &protos.Constraint_StringChoice{
			StringChoice: &protos.StringChoiceConstraint{Choices: []string{"a", "b"}, Strict: true},
		},
	}
	p := NewParamString("a").WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidStringStringChoice(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_STRING_STRING_CHOICE,
		Kind: &protos.Constraint_StringStringChoice{
			StringStringChoice: &protos.StringStringChoiceConstraint{
				Choices: []*protos.StringStringChoiceConstraint_StringStringChoice{
					{Value: "v", Name: &protos.PolyglotText{DisplayStrings: map[string]string{"en": "V"}}},
				},
			},
		},
	}
	p := NewParamString("v").WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidAlarmTable(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_ALARM_TABLE,
		Kind: &protos.Constraint_AlarmTable{
			AlarmTable: &protos.AlarmTableConstraint{
				Alarms: []*protos.Alarm{{BitValue: 0, Severity: protos.Alarm_INFO}},
			},
		},
	}
	p := NewParamInt32(0).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected alarm table constraint to be applied to INT32 param")
	}
}

func TestWithConstraint_RefOid_AnyType(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_UNDEFINED,
		Kind: &protos.Constraint_RefOid{RefOid: "shared.my_constraint"},
	}
	for _, factory := range []func() *Param{
		func() *Param { return NewParamInt32(0) },
		func() *Param { return NewParamFloat32(0) },
		func() *Param { return NewParamString("") },
		func() *Param { return NewParamStruct() },
	} {
		p := factory().WithConstraint(c).Proto
		if p.GetConstraint() == nil {
			t.Errorf("expected RefOid constraint to be valid for param type %v", p.GetType())
		}
	}
}

func TestWithConstraint_InvalidStringOnInt(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_STRING_CHOICE,
		Kind: &protos.Constraint_StringChoice{
			StringChoice: &protos.StringChoiceConstraint{Choices: []string{"a", "b"}, Strict: true},
		},
	}
	p := NewParamInt32(0).WithConstraint(c).Proto
	if p.GetConstraint() != nil {
		t.Error("expected string constraint to be rejected on INT32 param")
	}
}

func TestWithConstraint_InvalidIntOnFloat(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_RANGE,
		Kind: &protos.Constraint_Int32Range{
			Int32Range: &protos.Int32RangeConstraint{MinValue: 0, MaxValue: 100, Step: 1},
		},
	}
	p := NewParamFloat32(0).WithConstraint(c).Proto
	if p.GetConstraint() != nil {
		t.Error("expected int32 range constraint to be rejected on FLOAT32 param")
	}
}

func TestWithConstraint_InvalidFloatOnString(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_FLOAT_RANGE,
		Kind: &protos.Constraint_FloatRange{
			FloatRange: &protos.FloatRangeConstraint{MinValue: 0, MaxValue: 1, Step: 0.1},
		},
	}
	p := NewParamString("x").WithConstraint(c).Proto
	if p.GetConstraint() != nil {
		t.Error("expected float range constraint to be rejected on STRING param")
	}
}

func TestWithConstraint_Nil(t *testing.T) {
	p := NewParamInt32(0).WithConstraint(nil).Proto
	if p.GetConstraint() != nil {
		t.Error("expected nil constraint to be a no-op")
	}
}

// Scalar constraints must also be accepted on their corresponding array
// param types, matching the Catena protocol (see param.int_array_range.yaml,
// param.int_array_choice.yaml, param.int_array_alarm.yaml,
// param.float_array_range.yaml, param.string_array_choice.yaml,
// param.string_string_array_choice.yaml).

func TestWithConstraint_ValidIntRangeOnInt32Array(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_RANGE,
		Kind: &protos.Constraint_Int32Range{
			Int32Range: &protos.Int32RangeConstraint{MinValue: 0, MaxValue: 10, Step: 2},
		},
	}
	p := NewParamInt32Array([]int32{0, 2, 4}).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected int32 range constraint to be applied to INT32_ARRAY param")
	}
}

func TestWithConstraint_ValidInt32ChoiceOnInt32Array(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_CHOICE,
		Kind: &protos.Constraint_Int32Choice{
			Int32Choice: &protos.Int32ChoiceConstraint{
				Choices: []*protos.Int32ChoiceConstraint_IntChoice{
					{Value: 0, Name: &protos.PolyglotText{DisplayStrings: map[string]string{"en": "Off"}}},
					{Value: 1, Name: &protos.PolyglotText{DisplayStrings: map[string]string{"en": "On"}}},
				},
			},
		},
	}
	p := NewParamInt32Array([]int32{0, 1, 0}).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected int32 choice constraint to be applied to INT32_ARRAY param")
	}
}

func TestWithConstraint_ValidAlarmTableOnInt32Array(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_ALARM_TABLE,
		Kind: &protos.Constraint_AlarmTable{
			AlarmTable: &protos.AlarmTableConstraint{
				Alarms: []*protos.Alarm{{BitValue: 0, Severity: protos.Alarm_INFO}},
			},
		},
	}
	p := NewParamInt32Array([]int32{0, 1, 2}).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected alarm table constraint to be applied to INT32_ARRAY param")
	}
}

func TestWithConstraint_ValidFloatRangeOnFloat32Array(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_FLOAT_RANGE,
		Kind: &protos.Constraint_FloatRange{
			FloatRange: &protos.FloatRangeConstraint{MinValue: 0, MaxValue: 100, Step: 0.5},
		},
	}
	p := NewParamFloat32Array([]float32{1.0, 2.0}).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected float range constraint to be applied to FLOAT32_ARRAY param")
	}
}

func TestWithConstraint_ValidStringChoiceOnStringArray(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_STRING_CHOICE,
		Kind: &protos.Constraint_StringChoice{
			StringChoice: &protos.StringChoiceConstraint{Choices: []string{"a", "b"}, Strict: true},
		},
	}
	p := NewParamStringArray([]string{"a", "b"}).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected string choice constraint to be applied to STRING_ARRAY param")
	}
}

func TestWithConstraint_ValidStringStringChoiceOnStringArray(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_STRING_STRING_CHOICE,
		Kind: &protos.Constraint_StringStringChoice{
			StringStringChoice: &protos.StringStringChoiceConstraint{
				Choices: []*protos.StringStringChoiceConstraint_StringStringChoice{
					{Value: "<#FF0000>", Name: &protos.PolyglotText{DisplayStrings: map[string]string{"en": "Red"}}},
				},
				Strict: true,
			},
		},
	}
	p := NewParamStringArray([]string{"<#FF0000>"}).WithConstraint(c).Proto
	if p.GetConstraint() == nil {
		t.Fatal("expected string-string choice constraint to be applied to STRING_ARRAY param")
	}
}

func TestWithConstraint_InvalidIntOnStringArray(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_RANGE,
		Kind: &protos.Constraint_Int32Range{
			Int32Range: &protos.Int32RangeConstraint{MinValue: 0, MaxValue: 10, Step: 1},
		},
	}
	p := NewParamStringArray([]string{"a"}).WithConstraint(c).Proto
	if p.GetConstraint() != nil {
		t.Error("expected int32 range constraint to be rejected on STRING_ARRAY param")
	}
}

// --- Full chaining test ---

func TestFullChaining(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_INT_RANGE,
		Kind: &protos.Constraint_Int32Range{
			Int32Range: &protos.Int32RangeConstraint{MinValue: 0, MaxValue: 100, Step: 1, DisplayMin: 0, DisplayMax: 100},
		},
	}
	p := NewParamInt32(0).
		WithName(NewPolyglotText("en", "Volume")).
		WithConstraint(c).
		WithReadOnly(false).
		WithWidget("slider").
		WithAccessScope("operator").
		WithClientHint("unit", "dB").
		WithHelp(NewPolyglotText("en", "Audio volume")).
		WithOidAliases("/old/volume").
		WithMinimalSet(true).
		WithStateless(false).
		WithTemplateOid("templates.volume").
		Proto

	if p.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", p.GetType())
	}
	if p.GetName().GetDisplayStrings()["en"] != "Volume" {
		t.Error("name mismatch")
	}
	if p.GetConstraint().GetInt32Range().GetMaxValue() != 100 {
		t.Error("constraint mismatch")
	}
	if p.GetWidget() != "slider" {
		t.Error("widget mismatch")
	}
	if p.GetAccessScope() != "operator" {
		t.Error("access_scope mismatch")
	}
	if p.GetClientHints()["unit"] != "dB" {
		t.Error("client_hint mismatch")
	}
	if p.GetHelp().GetDisplayStrings()["en"] != "Audio volume" {
		t.Error("help mismatch")
	}
	if len(p.GetOidAliases()) != 1 {
		t.Error("oid_aliases mismatch")
	}
	if !p.GetMinimalSet() {
		t.Error("minimal_set mismatch")
	}
	if p.GetTemplateOid() != "templates.volume" {
		t.Error("template_oid mismatch")
	}
}

// --- WithValue tests ---

func TestWithValue_MatchingType(t *testing.T) {
	v, res := ToValue(int32(42))
	if res.Code != OK {
		t.Fatal(res.Error)
	}
	p := NewParamInt32(0).WithValue(v).Proto
	if p.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", p.GetValue().GetInt32Value())
	}
}

func TestWithValue_Struct(t *testing.T) {
	v, res := ToValue(map[string]any{"name": "test"})
	if res.Code != OK {
		t.Fatal(res.Error)
	}
	p := NewParamStruct().WithValue(v).Proto
	fields := p.GetValue().GetStructValue().GetFields()
	if fields["name"].GetStringValue() != "test" {
		t.Errorf("expected struct field 'name'='test', got %q", fields["name"].GetStringValue())
	}
}

func TestWithValue_StructVariant(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "type_a", Value: int32(10)}
	v, res := ToValue(sv)
	if res.Code != OK {
		t.Fatal(res.Error)
	}
	p := NewParamStructVariant().WithValue(v).Proto
	if p.GetValue().GetStructVariantValue().GetStructVariantType() != "type_a" {
		t.Errorf("expected struct_variant_type 'type_a', got %q",
			p.GetValue().GetStructVariantValue().GetStructVariantType())
	}
}

func TestWithValue_MismatchedType(t *testing.T) {
	v, res := ToValue("hello")
	if res.Code != OK {
		t.Fatal(res.Error)
	}
	p := NewParamInt32(0).WithValue(v).Proto
	if p.GetValue().GetInt32Value() != 0 {
		t.Error("expected mismatched WithValue to be a no-op")
	}
}

// --- SetValue / GetValue tests ---

func TestSetValue_OK(t *testing.T) {
	cp := NewParamInt32(0)
	res := cp.SetValue(int32(99))
	if res.Code != OK {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	p := cp.Proto
	if p.GetValue().GetInt32Value() != 99 {
		t.Errorf("expected 99, got %d", p.GetValue().GetInt32Value())
	}
}

func TestSetValue_TypeMismatch(t *testing.T) {
	cp := NewParamInt32(0)
	res := cp.SetValue("not an int")
	if res.Code != INVALID_ARGUMENT {
		t.Errorf("expected INVALID_ARGUMENT, got %v", res.Code)
	}
	if cp.Proto.GetValue().GetInt32Value() != 0 {
		t.Error("expected value to remain unchanged after type mismatch")
	}
}

func TestSetValue_ConversionError(t *testing.T) {
	cp := NewParamInt32(0)
	res := cp.SetValue(struct{}{})
	if res.Code == OK {
		t.Error("expected conversion error for unsupported type")
	}
}

func TestGetValue_OK(t *testing.T) {
	cp := NewParamString("hello")
	v, res := cp.GetValue()
	if res.Code != OK {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	s, ok := v.(string)
	if !ok || s != "hello" {
		t.Errorf("expected 'hello', got %v", v)
	}
}

func TestGetValue_Nil(t *testing.T) {
	cp := NewParamStruct()
	v, res := cp.GetValue()
	if res.Code != OK {
		t.Fatalf("expected OK for nil value, got %v: %s", res.Code, res.Error)
	}
	if v != nil {
		t.Errorf("expected nil value, got %v", v)
	}
}

func TestSetGetValue_Roundtrip_Struct(t *testing.T) {
	cp := NewParamStruct()
	input := map[string]any{"x": int32(1), "y": int32(2)}
	res := cp.SetValue(input)
	if res.Code != OK {
		t.Fatalf("SetValue failed: %s", res.Error)
	}
	out, res := cp.GetValue()
	if res.Code != OK {
		t.Fatalf("GetValue failed: %s", res.Error)
	}
	m, ok := out.(map[string]any)
	if !ok {
		t.Fatalf("expected map[string]any, got %T", out)
	}
	if m["x"] != int32(1) || m["y"] != int32(2) {
		t.Errorf("round-trip mismatch: got %v", m)
	}
}

// --- Typed factory value tests ---

func TestNewParamStruct_WithValue(t *testing.T) {
	p := NewParamStruct(map[string]any{"a": int32(1)}).Proto
	fields := p.GetValue().GetStructValue().GetFields()
	if fields["a"].GetInt32Value() != 1 {
		t.Errorf("expected struct field 'a'=1, got %d", fields["a"].GetInt32Value())
	}
}

func TestNewParamStruct_NoValue(t *testing.T) {
	p := NewParamStruct().Proto
	if p.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when no arg provided")
	}
}

func TestNewParamStructVariant_WithValue(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "kind_a", Value: int32(5)}
	p := NewParamStructVariant(sv).Proto
	if p.GetValue().GetStructVariantValue().GetStructVariantType() != "kind_a" {
		t.Errorf("expected struct_variant_type 'kind_a', got %q",
			p.GetValue().GetStructVariantValue().GetStructVariantType())
	}
}

func TestNewParamStructArray_WithValue(t *testing.T) {
	arr := []map[string]any{
		{"x": int32(1)},
		{"x": int32(2)},
	}
	p := NewParamStructArray(arr).Proto
	list := p.GetValue().GetStructArrayValues().GetStructValues()
	if len(list) != 2 {
		t.Fatalf("expected 2 structs, got %d", len(list))
	}
	if list[0].GetFields()["x"].GetInt32Value() != 1 {
		t.Error("expected first struct field x=1")
	}
}

func TestNewParamStructVariantArray_WithValue(t *testing.T) {
	arr := []StructVariantValue{
		{StructVariantType: "a", Value: int32(1)},
		{StructVariantType: "b", Value: int32(2)},
	}
	p := NewParamStructVariantArray(arr).Proto
	list := p.GetValue().GetStructVariantArrayValues().GetStructVariants()
	if len(list) != 2 {
		t.Fatalf("expected 2 variants, got %d", len(list))
	}
	if list[0].GetStructVariantType() != "a" {
		t.Error("expected first variant type 'a'")
	}
}

// --- NewParamFromValue tests ---

func TestNewParamFromValue_Int32(t *testing.T) {
	v, _ := ToValue(int32(42))
	cp := NewParamFromValue(v)
	p := cp.Proto
	if p.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", p.GetType())
	}
	if p.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", p.GetValue().GetInt32Value())
	}
}

func TestNewParamFromValue_Struct(t *testing.T) {
	v, _ := ToValue(map[string]any{"k": "v"})
	cp := NewParamFromValue(v)
	if cp.Proto.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_StructVariant(t *testing.T) {
	v, _ := ToValue(StructVariantValue{StructVariantType: "t", Value: int32(0)})
	cp := NewParamFromValue(v)
	if cp.Proto.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_StructArray(t *testing.T) {
	v, _ := ToValue([]map[string]any{{"a": int32(1)}})
	cp := NewParamFromValue(v)
	if cp.Proto.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_StructVariantArray(t *testing.T) {
	v, _ := ToValue([]StructVariantValue{{StructVariantType: "t", Value: int32(0)}})
	cp := NewParamFromValue(v)
	if cp.Proto.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_Nil(t *testing.T) {
	cp := NewParamFromValue(Value{})
	if cp.Proto.GetType() != protos.ParamType_UNDEFINED {
		t.Errorf("expected UNDEFINED for nil value, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_AllScalarTypes(t *testing.T) {
	tests := []struct {
		name     string
		input    any
		wantType protos.ParamType
	}{
		{"float32", float32(1.5), protos.ParamType_FLOAT32},
		{"string", "hello", protos.ParamType_STRING},
		{"int32_array", []int32{1, 2}, protos.ParamType_INT32_ARRAY},
		{"float32_array", []float32{1.0}, protos.ParamType_FLOAT32_ARRAY},
		{"string_array", []string{"a"}, protos.ParamType_STRING_ARRAY},
		{"empty", EmptyValue{}, protos.ParamType_EMPTY},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			v, res := ToValue(tt.input)
			if res.Code != OK {
				t.Fatalf("ToValue failed: %s", res.Error)
			}
			cp := NewParamFromValue(v)
			if cp.Proto.GetType() != tt.wantType {
				t.Errorf("expected %v, got %v", tt.wantType, cp.Proto.GetType())
			}
		})
	}
}

// --- Validation helper tests ---

func TestIsValueValidForParamType_NilValue(t *testing.T) {
	if isValueValidForParamType(nil, protos.ParamType_INT32) {
		t.Error("expected nil value to be invalid")
	}
}

func TestIsValueValidForParamType_DataPayload_Binary(t *testing.T) {
	v := &protos.Value{Kind: &protos.Value_DataPayload{
		DataPayload: &protos.DataPayload{Kind: &protos.DataPayload_Payload{Payload: []byte{1}}},
	}}
	if !isValueValidForParamType(v, protos.ParamType_BINARY) {
		t.Error("expected DataPayload to be valid for BINARY")
	}
	if !isValueValidForParamType(v, protos.ParamType_DATA) {
		t.Error("expected DataPayload to be valid for DATA")
	}
	if isValueValidForParamType(v, protos.ParamType_INT32) {
		t.Error("expected DataPayload to be invalid for INT32")
	}
}

func TestParamTypeFromValueKind_Nil(t *testing.T) {
	if paramTypeFromValueKind(nil) != protos.ParamType_UNDEFINED {
		t.Error("expected UNDEFINED for nil value")
	}
}

func TestParamTypeFromValueKind_DataPayload(t *testing.T) {
	v := &protos.Value{Kind: &protos.Value_DataPayload{}}
	if paramTypeFromValueKind(v) != protos.ParamType_DATA {
		t.Error("expected DATA for DataPayload value kind")
	}
}

func TestWithParam_SelfReference(t *testing.T) {
	cp := NewParamStruct()
	cp.WithParam("self", cp)
	sub := cp.Proto.GetParams()["self"]
	if sub == nil {
		t.Fatal("expected self-referencing sub-param to be set")
	}
}

// --- Error path tests ---

func TestNewParamStruct_InvalidValue(t *testing.T) {
	p := NewParamStruct(map[string]any{"bad": struct{}{}}).Proto
	if p.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
}

func TestNewParamStructVariant_InvalidValue(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "t", Value: struct{}{}}
	p := NewParamStructVariant(sv).Proto
	if p.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
}

func TestNewParamStructArray_InvalidValue(t *testing.T) {
	arr := []map[string]any{{"bad": struct{}{}}}
	p := NewParamStructArray(arr).Proto
	if p.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
}

func TestNewParamStructVariantArray_InvalidValue(t *testing.T) {
	arr := []StructVariantValue{{StructVariantType: "t", Value: struct{}{}}}
	p := NewParamStructVariantArray(arr).Proto
	if p.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
}

func TestNewParamData_InvalidPayload(t *testing.T) {
	dp := DataPayload{}
	p := NewParamData(dp).Proto
	if p.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when DataPayload conversion fails")
	}
}

func TestNewParamData_BothPayloadAndUrl(t *testing.T) {
	dp := DataPayload{Payload: []byte("data"), Url: "http://example.com"}
	p := NewParamData(dp).Proto
	if p.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", p.GetType())
	}
	if p.GetValue() != nil {
		t.Error("expected nil value when both payload and url are set")
	}
}

func TestWithParam_StructVariant(t *testing.T) {
	child := NewParamInt32(5)
	p := NewParamStructVariant().WithParam("field", child).Proto
	sub := p.GetParams()["field"]
	if sub == nil {
		t.Fatal("expected sub-param on STRUCT_VARIANT")
	}
	if sub.GetValue().GetInt32Value() != 5 {
		t.Errorf("expected 5, got %d", sub.GetValue().GetInt32Value())
	}
}

func TestWithParam_StructArray(t *testing.T) {
	child := NewParamString("val")
	p := NewParamStructArray().WithParam("item", child).Proto
	sub := p.GetParams()["item"]
	if sub == nil {
		t.Fatal("expected sub-param on STRUCT_ARRAY")
	}
	if sub.GetValue().GetStringValue() != "val" {
		t.Errorf("expected 'val', got %q", sub.GetValue().GetStringValue())
	}
}

func TestWithParam_StructVariantArray(t *testing.T) {
	child := NewParamFloat32(1.5)
	p := NewParamStructVariantArray().WithParam("entry", child).Proto
	sub := p.GetParams()["entry"]
	if sub == nil {
		t.Fatal("expected sub-param on STRUCT_VARIANT_ARRAY")
	}
	if sub.GetValue().GetFloat32Value() != 1.5 {
		t.Errorf("expected 1.5, got %f", sub.GetValue().GetFloat32Value())
	}
}

func TestIsValueValidForParamType_NilKind(t *testing.T) {
	v := &protos.Value{}
	if isValueValidForParamType(v, protos.ParamType_INT32) {
		t.Error("expected Value with nil Kind to be invalid")
	}
}

func TestIsValueValidForParamType_UndefinedValue(t *testing.T) {
	v := &protos.Value{Kind: &protos.Value_UndefinedValue{UndefinedValue: 0}}
	if !isValueValidForParamType(v, protos.ParamType_UNDEFINED) {
		t.Error("expected UndefinedValue to be valid for UNDEFINED")
	}
	if isValueValidForParamType(v, protos.ParamType_INT32) {
		t.Error("expected UndefinedValue to be invalid for INT32")
	}
}

func TestParamTypeFromValueKind_NilKind(t *testing.T) {
	v := &protos.Value{}
	if paramTypeFromValueKind(v) != protos.ParamType_UNDEFINED {
		t.Error("expected UNDEFINED for Value with nil Kind")
	}
}

func TestParamTypeFromValueKind_UndefinedValue(t *testing.T) {
	v := &protos.Value{Kind: &protos.Value_UndefinedValue{UndefinedValue: 0}}
	if paramTypeFromValueKind(v) != protos.ParamType_UNDEFINED {
		t.Error("expected UNDEFINED for UndefinedValue kind")
	}
}

func TestIsConstraintValidForParam_NilKind(t *testing.T) {
	c := &protos.Constraint{}
	if isConstraintValidForParam(c, protos.ParamType_INT32) {
		t.Error("expected constraint with nil Kind to be invalid")
	}
}

func TestSetValue_InvalidThenValid(t *testing.T) {
	cp := NewParamInt32(10)
	res := cp.SetValue("wrong type")
	if res.Code != INVALID_ARGUMENT {
		t.Errorf("expected INVALID_ARGUMENT, got %v", res.Code)
	}
	if cp.Proto.GetValue().GetInt32Value() != 10 {
		t.Error("expected original value to be preserved after failed SetValue")
	}
	res = cp.SetValue(int32(20))
	if res.Code != OK {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if cp.Proto.GetValue().GetInt32Value() != 20 {
		t.Errorf("expected 20, got %d", cp.Proto.GetValue().GetInt32Value())
	}
}
