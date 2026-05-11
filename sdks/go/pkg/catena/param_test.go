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
	p := NewParamInt32(42).Build()
	if p.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", p.GetType())
	}
	if p.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", p.GetValue().GetInt32Value())
	}
}

func TestNewParamFloat32(t *testing.T) {
	p := NewParamFloat32(3.14).Build()
	if p.GetType() != protos.ParamType_FLOAT32 {
		t.Errorf("expected FLOAT32, got %v", p.GetType())
	}
	if p.GetValue().GetFloat32Value() != 3.14 {
		t.Errorf("expected 3.14, got %f", p.GetValue().GetFloat32Value())
	}
}

func TestNewParamString(t *testing.T) {
	p := NewParamString("hello").Build()
	if p.GetType() != protos.ParamType_STRING {
		t.Errorf("expected STRING, got %v", p.GetType())
	}
	if p.GetValue().GetStringValue() != "hello" {
		t.Errorf("expected 'hello', got %q", p.GetValue().GetStringValue())
	}
}

func TestNewParamStruct(t *testing.T) {
	p := NewParamStruct().Build()
	if p.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", p.GetType())
	}
}

func TestNewParamStructVariant(t *testing.T) {
	p := NewParamStructVariant().Build()
	if p.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", p.GetType())
	}
}

func TestNewParamInt32Array(t *testing.T) {
	p := NewParamInt32Array([]int32{1, 2, 3}).Build()
	if p.GetType() != protos.ParamType_INT32_ARRAY {
		t.Errorf("expected INT32_ARRAY, got %v", p.GetType())
	}
	vals := p.GetValue().GetInt32ArrayValues().GetInts()
	if len(vals) != 3 || vals[0] != 1 {
		t.Errorf("unexpected int32 array: %v", vals)
	}
}

func TestNewParamFloat32Array(t *testing.T) {
	p := NewParamFloat32Array([]float32{1.1, 2.2}).Build()
	if p.GetType() != protos.ParamType_FLOAT32_ARRAY {
		t.Errorf("expected FLOAT32_ARRAY, got %v", p.GetType())
	}
	vals := p.GetValue().GetFloat32ArrayValues().GetFloats()
	if len(vals) != 2 {
		t.Errorf("expected 2 floats, got %d", len(vals))
	}
}

func TestNewParamStringArray(t *testing.T) {
	p := NewParamStringArray([]string{"a", "b"}).Build()
	if p.GetType() != protos.ParamType_STRING_ARRAY {
		t.Errorf("expected STRING_ARRAY, got %v", p.GetType())
	}
	vals := p.GetValue().GetStringArrayValues().GetStrings()
	if len(vals) != 2 || vals[0] != "a" {
		t.Errorf("unexpected string array: %v", vals)
	}
}

func TestNewParamBinary(t *testing.T) {
	p := NewParamBinary([]byte{0xDE, 0xAD}).Build()
	if p.GetType() != protos.ParamType_BINARY {
		t.Errorf("expected BINARY, got %v", p.GetType())
	}
	payload := p.GetValue().GetDataPayload().GetPayload()
	if len(payload) != 2 || payload[0] != 0xDE {
		t.Errorf("unexpected binary payload: %v", payload)
	}
}

func TestNewParamStructArray(t *testing.T) {
	p := NewParamStructArray().Build()
	if p.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", p.GetType())
	}
}

func TestNewParamStructVariantArray(t *testing.T) {
	p := NewParamStructVariantArray().Build()
	if p.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", p.GetType())
	}
}

func TestNewParamEmpty(t *testing.T) {
	p := NewParamEmpty().Build()
	if p.GetType() != protos.ParamType_EMPTY {
		t.Errorf("expected EMPTY, got %v", p.GetType())
	}
	if p.GetValue().GetEmptyValue() == nil {
		t.Error("expected EmptyValue to be set")
	}
}

func TestNewParamData(t *testing.T) {
	dp := ToPayload([]byte("content"), "text/plain", "test.txt")
	p := NewParamData(dp).Build()
	if p.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", p.GetType())
	}
	if p.GetValue().GetDataPayload() == nil {
		t.Error("expected DataPayload to be set")
	}
}

// --- With* method tests ---

func TestWithName(t *testing.T) {
	p := NewParamInt32(0).WithName(NewPolyglotText("en", "Temperature")).Build()
	if p.GetName().GetDisplayStrings()["en"] != "Temperature" {
		t.Errorf("expected 'Temperature', got %q", p.GetName().GetDisplayStrings()["en"])
	}
}

func TestWithReadOnly(t *testing.T) {
	p := NewParamInt32(0).WithReadOnly(true).Build()
	if !p.GetReadOnly() {
		t.Error("expected read_only=true")
	}
}

func TestWithWidget(t *testing.T) {
	p := NewParamInt32(0).WithWidget("slider").Build()
	if p.GetWidget() != "slider" {
		t.Errorf("expected 'slider', got %q", p.GetWidget())
	}
}

func TestWithPrecision(t *testing.T) {
	p := NewParamFloat32(0).WithPrecision(3).Build()
	if p.GetPrecision() != 3 {
		t.Errorf("expected precision 3, got %d", p.GetPrecision())
	}
}

func TestWithPrecision_WrongType(t *testing.T) {
	p := NewParamInt32(0).WithPrecision(3).Build()
	if p.GetPrecision() != 0 {
		t.Error("expected precision to remain 0 for non-FLOAT32 param")
	}
}

func TestWithMaxLength(t *testing.T) {
	p := NewParamString("").WithMaxLength(255).Build()
	if p.GetMaxLength() != 255 {
		t.Errorf("expected max_length 255, got %d", p.GetMaxLength())
	}
}

func TestWithMaxLength_WrongType(t *testing.T) {
	p := NewParamInt32(0).WithMaxLength(255).Build()
	if p.GetMaxLength() != 0 {
		t.Error("expected max_length to remain 0 for non-STRING param")
	}
}

func TestWithAccessScope(t *testing.T) {
	p := NewParamInt32(0).WithAccessScope("admin").Build()
	if p.GetAccessScope() != "admin" {
		t.Errorf("expected 'admin', got %q", p.GetAccessScope())
	}
}

func TestWithClientHint(t *testing.T) {
	p := NewParamInt32(0).
		WithClientHint("display", "compact").
		WithClientHint("color", "red").
		Build()
	if p.GetClientHints()["display"] != "compact" {
		t.Errorf("expected client_hint 'display'='compact', got %q", p.GetClientHints()["display"])
	}
	if p.GetClientHints()["color"] != "red" {
		t.Errorf("expected client_hint 'color'='red', got %q", p.GetClientHints()["color"])
	}
}

func TestWithParam_Struct(t *testing.T) {
	child := NewParamInt32(10).WithName(NewPolyglotText("en", "child"))
	p := NewParamStruct().WithParam("brightness", child).Build()

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
	parent := NewParamStruct().WithParam("x", child).Build()

	child.param.Value = &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 999}}
	if parent.GetParams()["x"].GetValue().GetInt32Value() != 1 {
		t.Error("expected sub-param to be a deep clone, not affected by later mutation")
	}
}

func TestWithParam_WrongType(t *testing.T) {
	child := NewParamInt32(0)
	p := NewParamInt32(0).WithParam("x", child).Build()
	if len(p.GetParams()) != 0 {
		t.Error("expected no sub-params on INT32 param")
	}
}

func TestWithParam_Nil(t *testing.T) {
	// Passing a nil sub-param must not panic and must not add an entry,
	// preserving the contract that method chaining is never interrupted
	// by error handling.
	cp := NewParamStruct().WithParam("x", nil)
	p := cp.Build()
	if len(p.GetParams()) != 0 {
		t.Error("expected nil sub-param to be a no-op")
	}
	// Chaining must continue to work after the no-op.
	child := NewParamInt32(7)
	p = cp.WithParam("y", child).Build()
	if p.GetParams()["y"].GetValue().GetInt32Value() != 7 {
		t.Error("expected chaining to continue working after nil sub-param")
	}
}

func TestWithResponse(t *testing.T) {
	p := NewParamEmpty().WithResponse(true).Build()
	if !p.GetResponse() {
		t.Error("expected response=true")
	}
}

func TestWithHelp(t *testing.T) {
	p := NewParamInt32(0).WithHelp(NewPolyglotText("en", "Adjusts volume")).Build()
	if p.GetHelp().GetDisplayStrings()["en"] != "Adjusts volume" {
		t.Errorf("unexpected help text: %q", p.GetHelp().GetDisplayStrings()["en"])
	}
}

func TestWithOidAliases(t *testing.T) {
	p := NewParamInt32(0).WithOidAliases("/old/path", "/legacy/path").Build()
	aliases := p.GetOidAliases()
	if len(aliases) != 2 || aliases[0] != "/old/path" {
		t.Errorf("unexpected aliases: %v", aliases)
	}
}

func TestWithMinimalSet(t *testing.T) {
	p := NewParamInt32(0).WithMinimalSet(true).Build()
	if !p.GetMinimalSet() {
		t.Error("expected minimal_set=true")
	}
}

func TestWithStateless(t *testing.T) {
	p := NewParamInt32(0).WithStateless(true).Build()
	if !p.GetStateless() {
		t.Error("expected stateless=true")
	}
}

func TestWithTemplateOid(t *testing.T) {
	p := NewParamInt32(0).WithTemplateOid("templates.volume").Build()
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
	p := NewParamInt32(0).WithConstraint(c).Build()
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
	p := NewParamInt32(50).WithConstraint(c).Build()
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
	p := NewParamFloat32(0.5).WithConstraint(c).Build()
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
	p := NewParamString("a").WithConstraint(c).Build()
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
	p := NewParamString("v").WithConstraint(c).Build()
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
	p := NewParamInt32(0).WithConstraint(c).Build()
	if p.GetConstraint() == nil {
		t.Fatal("expected alarm table constraint to be applied to INT32 param")
	}
}

func TestWithConstraint_RefOid_AnyType(t *testing.T) {
	c := &protos.Constraint{
		Type: protos.Constraint_UNDEFINED,
		Kind: &protos.Constraint_RefOid{RefOid: "shared.my_constraint"},
	}
	for _, factory := range []func() *CatenaParam{
		func() *CatenaParam { return NewParamInt32(0) },
		func() *CatenaParam { return NewParamFloat32(0) },
		func() *CatenaParam { return NewParamString("") },
		func() *CatenaParam { return NewParamStruct() },
	} {
		p := factory().WithConstraint(c).Build()
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
	p := NewParamInt32(0).WithConstraint(c).Build()
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
	p := NewParamFloat32(0).WithConstraint(c).Build()
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
	p := NewParamString("x").WithConstraint(c).Build()
	if p.GetConstraint() != nil {
		t.Error("expected float range constraint to be rejected on STRING param")
	}
}

func TestWithConstraint_Nil(t *testing.T) {
	p := NewParamInt32(0).WithConstraint(nil).Build()
	if p.GetConstraint() != nil {
		t.Error("expected nil constraint to be a no-op")
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
		Build()

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
