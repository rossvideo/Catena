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

package st2138

import (
	"errors"
	"testing"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// --- Factory tests ---

func TestNewParamInt32(t *testing.T) {
	p, err := NewParamInt32(42).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", p.Proto.GetValue().GetInt32Value())
	}
}

func TestNewParamFloat32(t *testing.T) {
	p, err := NewParamFloat32(3.14).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_FLOAT32 {
		t.Errorf("expected FLOAT32, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue().GetFloat32Value() != 3.14 {
		t.Errorf("expected 3.14, got %f", p.Proto.GetValue().GetFloat32Value())
	}
}

func TestNewParamString(t *testing.T) {
	p, err := NewParamString("hello").Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRING {
		t.Errorf("expected STRING, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue().GetStringValue() != "hello" {
		t.Errorf("expected 'hello', got %q", p.Proto.GetValue().GetStringValue())
	}
}

func TestNewParamStruct(t *testing.T) {
	p, err := NewParamStruct().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", p.Proto.GetType())
	}
}

func TestNewParamStructVariant(t *testing.T) {
	p, err := NewParamStructVariant().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", p.Proto.GetType())
	}
}

func TestNewParamInt32Array(t *testing.T) {
	p, err := NewParamInt32Array([]int32{1, 2, 3}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_INT32_ARRAY {
		t.Errorf("expected INT32_ARRAY, got %v", p.Proto.GetType())
	}
	vals := p.Proto.GetValue().GetInt32ArrayValues().GetInts()
	if len(vals) != 3 || vals[0] != 1 {
		t.Errorf("unexpected int32 array: %v", vals)
	}
}

func TestNewParamFloat32Array(t *testing.T) {
	p, err := NewParamFloat32Array([]float32{1.1, 2.2}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_FLOAT32_ARRAY {
		t.Errorf("expected FLOAT32_ARRAY, got %v", p.Proto.GetType())
	}
	vals := p.Proto.GetValue().GetFloat32ArrayValues().GetFloats()
	if len(vals) != 2 {
		t.Errorf("expected 2 floats, got %d", len(vals))
	}
}

func TestNewParamStringArray(t *testing.T) {
	p, err := NewParamStringArray([]string{"a", "b"}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRING_ARRAY {
		t.Errorf("expected STRING_ARRAY, got %v", p.Proto.GetType())
	}
	vals := p.Proto.GetValue().GetStringArrayValues().GetStrings()
	if len(vals) != 2 || vals[0] != "a" {
		t.Errorf("unexpected string array: %v", vals)
	}
}

func TestNewParamBinary(t *testing.T) {
	p, err := NewParamBinary([]byte{0xDE, 0xAD}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_BINARY {
		t.Errorf("expected BINARY, got %v", p.Proto.GetType())
	}
	payload := p.Proto.GetValue().GetDataPayload().GetPayload()
	if len(payload) != 2 || payload[0] != 0xDE {
		t.Errorf("unexpected binary payload: %v", payload)
	}
}

func TestNewParamStructArray(t *testing.T) {
	p, err := NewParamStructArray().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", p.Proto.GetType())
	}
}

func TestNewParamStructVariantArray(t *testing.T) {
	p, err := NewParamStructVariantArray().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", p.Proto.GetType())
	}
}

func TestNewParamEmpty(t *testing.T) {
	p, err := NewParamEmpty().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_EMPTY {
		t.Errorf("expected EMPTY, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue().GetEmptyValue() == nil {
		t.Error("expected EmptyValue to be set")
	}
}

func TestNewParamData(t *testing.T) {
	dp := ToPayload([]byte("content"), "text/plain", "test.txt")
	p, err := NewParamData(dp).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue().GetDataPayload() == nil {
		t.Error("expected DataPayload to be set")
	}
}

// --- With* method tests ---

func TestWithName(t *testing.T) {
	p, err := NewParamInt32(0).WithName(PolyglotText{"en": "Temperature"}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetName().GetDisplayStrings()["en"] != "Temperature" {
		t.Errorf("expected 'Temperature', got %q", p.Proto.GetName().GetDisplayStrings()["en"])
	}
}

func TestWithName_Nil(t *testing.T) {
	cp := NewParamInt32(0).WithName(nil)
	if cp.proto.GetName() != nil {
		t.Error("expected nil name to leave the proto name unset")
	}
	p, err := cp.Build()
	if p != (Param{}) {
		t.Errorf("expected empty Param, got %v", p)
	}
	if err == nil {
		t.Error("expected error for builder with nil name")
	}
}

func TestWithName_EmptyPolyglotText(t *testing.T) {
	cp := NewParamInt32(0).WithName(PolyglotText{})
	if cp.proto.GetName() != nil {
		t.Error("expected empty polyglot text to leave the proto name unset")
	}
	p, err := cp.Build()
	if p != (Param{}) {
		t.Errorf("expected empty Param, got %v", p)
	}
	if err == nil {
		t.Error("expected error for builder with nil PolyglotText")
	}
}

func TestWithReadOnly(t *testing.T) {
	p, err := NewParamInt32(0).WithReadOnly(true).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if !p.Proto.GetReadOnly() {
		t.Error("expected read_only=true")
	}
}

// A read-only parent propagates read_only to sub-params added afterward,
// including nested descendants.
func TestWithReadOnly_PropagatesToChildrenAddedAfter(t *testing.T) {
	grandchild := NewParamString("deep")
	child := NewParamStruct().WithParam("grandchild", grandchild)
	p, err := NewParamStruct().
		WithReadOnly(true).
		WithParam("child", child).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	sub := p.Proto.GetParams()["child"]
	if sub == nil {
		t.Fatal("expected sub-param 'child'")
	}
	if !sub.GetReadOnly() {
		t.Error("expected child of read-only parent to be read_only")
	}
	if !sub.GetParams()["grandchild"].GetReadOnly() {
		t.Error("expected grandchild of read-only parent to be read_only")
	}
}

// Marking a parent read-only after its children were added propagates
// read_only down to the existing children recursively.
func TestWithReadOnly_PropagatesToExistingChildren(t *testing.T) {
	grandchild := NewParamString("deep")
	child := NewParamStruct().WithParam("grandchild", grandchild)
	p, err := NewParamStruct().
		WithParam("child", child).
		WithReadOnly(true).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	sub := p.Proto.GetParams()["child"]
	if !sub.GetReadOnly() {
		t.Error("expected existing child to become read_only when parent set read-only")
	}
	if !sub.GetParams()["grandchild"].GetReadOnly() {
		t.Error("expected existing grandchild to become read_only when parent set read-only")
	}
}

// A writable parent leaves children untouched, so a child may still be
// independently marked read-only.
func TestWithReadOnly_WritableParentKeepsChildReadOnly(t *testing.T) {
	child := NewParamInt32(0).WithReadOnly(true)
	p, err := NewParamStruct().
		WithReadOnly(false).
		WithParam("child", child).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if p.Proto.GetReadOnly() {
		t.Error("expected parent to remain writable")
	}
	if !p.Proto.GetParams()["child"].GetReadOnly() {
		t.Error("expected child under writable parent to keep its own read_only=true")
	}
}

// A writable parent must not force its children to writable: a read-only child
// added before the parent is marked writable stays read-only.
func TestWithReadOnly_WritableParentDoesNotClearChild(t *testing.T) {
	child := NewParamInt32(0).WithReadOnly(true)
	p, err := NewParamStruct().
		WithParam("child", child).
		WithReadOnly(false).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if !p.Proto.GetParams()["child"].GetReadOnly() {
		t.Error("expected read-only child to stay read_only under writable parent")
	}
}

// A nil entry in the sub-param map must be skipped by read-only propagation
// without panicking, while non-nil siblings are still marked read-only.
func TestWithReadOnly_SkipsNilSubParam(t *testing.T) {
	cp := &ParamBuilder{proto: &protos.Param{
		Type: protos.ParamType_STRUCT,
		Params: map[string]*protos.Param{
			"nilChild":  nil,
			"realChild": {Type: protos.ParamType_INT32},
		},
	}}

	p, err := cp.WithReadOnly(true).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if p.Proto.GetParams()["nilChild"] != nil {
		t.Error("expected nil sub-param to remain nil")
	}
	if !p.Proto.GetParams()["realChild"].GetReadOnly() {
		t.Error("expected non-nil sibling to be marked read_only")
	}
}

func TestWithWidget(t *testing.T) {
	p, err := NewParamInt32(0).WithWidget("slider").Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetWidget() != "slider" {
		t.Errorf("expected 'slider', got %q", p.Proto.GetWidget())
	}
}

func TestWithPrecision(t *testing.T) {
	p, err := NewParamFloat32(0).WithPrecision(3).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetPrecision() != 3 {
		t.Errorf("expected precision 3, got %d", p.Proto.GetPrecision())
	}
}

// precision is part of the Catena protocol for both FLOAT32 and FLOAT32_ARRAY
// params (see device.one_of_everything.yaml, which sets precision: 3 on a
// FLOAT32_ARRAY).
func TestWithPrecision_Float32Array(t *testing.T) {
	p, err := NewParamFloat32Array([]float32{1.1, 2.2}).WithPrecision(3).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetPrecision() != 3 {
		t.Errorf("expected precision 3 on FLOAT32_ARRAY, got %d", p.Proto.GetPrecision())
	}
}

func TestWithPrecision_WrongType(t *testing.T) {
	cp := NewParamInt32(0).WithPrecision(3)
	if cp.proto.GetPrecision() != 0 {
		t.Error("expected precision to remain 0 for non-float param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with valid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithMaxLength(t *testing.T) {
	p, err := NewParamString("").WithMaxLength(255).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetMaxLength() != 255 {
		t.Errorf("expected max_length 255, got %d", p.Proto.GetMaxLength())
	}
}

// max_length is part of the Catena protocol for both STRING and STRING_ARRAY
// params (see param.string_array_length.yaml, which sets max_length: 10 on a
// STRING_ARRAY to cap the number of strings in the array).
func TestWithMaxLength_StringArray(t *testing.T) {
	p, err := NewParamStringArray([]string{"a", "b"}).WithMaxLength(10).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetMaxLength() != 10 {
		t.Errorf("expected max_length 10 on STRING_ARRAY, got %d", p.Proto.GetMaxLength())
	}
}

func TestWithMaxLength_WrongType(t *testing.T) {
	cp := NewParamInt32(0).WithMaxLength(255)
	if cp.proto.GetMaxLength() != 0 {
		t.Error("expected max_length to remain 0 for param type that does not support max_length")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with valid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithAccessScope(t *testing.T) {
	p, err := NewParamInt32(0).WithAccessScope("admin").Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetAccessScope() != "admin" {
		t.Errorf("expected 'admin', got %q", p.Proto.GetAccessScope())
	}
}

func TestWithClientHint(t *testing.T) {
	p, err := NewParamInt32(0).
		WithClientHint("display", "compact").
		WithClientHint("color", "red").
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetClientHints()["display"] != "compact" {
		t.Errorf("expected client_hint 'display'='compact', got %q", p.Proto.GetClientHints()["display"])
	}
	if p.Proto.GetClientHints()["color"] != "red" {
		t.Errorf("expected client_hint 'color'='red', got %q", p.Proto.GetClientHints()["color"])
	}
}

func TestWithParam_Struct(t *testing.T) {
	child := NewParamInt32(10).WithName(PolyglotText{"en": "child"})
	p, err := NewParamStruct().WithParam("brightness", child).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	sub := p.Proto.GetParams()["brightness"]
	if sub == nil {
		t.Fatal("expected sub-param 'brightness'")
	}
	if sub.GetName().GetDisplayStrings()["en"] != "child" {
		t.Errorf("expected sub-param name 'child', got %q", sub.GetName().GetDisplayStrings()["en"])
	}
	// The child's value is moved into the parent struct's Value; the stored
	// descriptor stays valueless.
	if sub.GetValue() != nil {
		t.Error("expected stored sub-param descriptor to be valueless")
	}
	if p.Proto.GetValue().GetStructValue().GetFields()["brightness"].GetInt32Value() != 10 {
		t.Errorf("expected parent struct value field 'brightness'=10, got %d",
			p.Proto.GetValue().GetStructValue().GetFields()["brightness"].GetInt32Value())
	}
}

func TestWithParam_DeepClone(t *testing.T) {
	child := NewParamInt32(1)
	childValue := child.proto.GetValue()
	parent := NewParamStruct().WithParam("x", child)
	parentValue := parent.proto.GetValue().
		GetStructValue().
		GetFields()["x"]
	if parent.proto.GetParams()["x"] == child.proto {
		t.Error("expected sub-param descriptor to be cloned")
	}
	if parentValue == childValue {
		t.Error("expected sub-param value to be cloned")
	}
	if !proto.Equal(parentValue, childValue) {
		t.Error("expected cloned value to equal the original")
	}
}

func TestWithParam_WrongType(t *testing.T) {
	child := NewParamInt32(0)
	cp := NewParamInt32(0).WithParam("x", child)
	if len(cp.proto.GetParams()) != 0 {
		t.Error("expected no sub-params on INT32 param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with valid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithParam_Nil(t *testing.T) {
	// Passing a nil sub-param must not panic and must not add an entry,
	// subsequent method calls must short-circuit without panicking.
	cp := NewParamStruct().WithParam("x", nil)
	if cp.err == nil {
		t.Error("expected error for builder with nil sub-param")
	}
	if len(cp.proto.GetParams()) != 0 {
		t.Error("expected nil sub-param to be a no-op")
	}
	child := NewParamInt32(7)
	cp.WithParam("y", child)
	if cp.proto.GetParams()["y"] != nil {
		t.Fatal("expected chaining to short-circuit after nil sub-param")
	}
	if cp.proto.GetValue().GetStructValue().GetFields()["y"].GetInt32Value() == 7 {
		t.Error("expected sub-param value to be ignored after nil sub-param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid sub-param")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithParam_NestedErrors(t *testing.T) {
	child := NewParamInt32(0, 1)
	cp := NewParamStruct().WithParam("x", child)
	if len(cp.proto.GetParams()) != 0 {
		t.Error("expected nested builder error to leave the parent params unset")
	}
	p, err := cp.Build()
	if p != (Param{}) {
		t.Errorf("expected empty Param, got %v", p)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}

func TestWithParam_EmptyParam(t *testing.T) {
	cp := NewParamStruct().WithParam("x", &ParamBuilder{})
	if len(cp.proto.GetParams()) != 0 {
		t.Error("expected empty param builder to leave the parent params unset")
	}
	p, err := cp.Build()
	if p != (Param{}) {
		t.Errorf("expected empty Param, got %v", p)
	}
	if err == nil {
		t.Error("expected error for builder with empty param")
	}
}

func TestWithResponse(t *testing.T) {
	p, err := NewParamEmpty().WithResponse(true).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if !p.Proto.GetResponse() {
		t.Error("expected response=true")
	}
}

func TestWithHelp(t *testing.T) {
	p, err := NewParamInt32(0).WithHelp(PolyglotText{"en": "Adjusts volume"}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetHelp().GetDisplayStrings()["en"] != "Adjusts volume" {
		t.Errorf("unexpected help text: %q", p.Proto.GetHelp().GetDisplayStrings()["en"])
	}
}

func TestWithHelp_Nil(t *testing.T) {
	cp := NewParamInt32(0).WithHelp(nil)
	if cp.proto.GetHelp() != nil {
		t.Error("expected nil help to leave the proto help unset")
	}
	p, err := cp.Build()
	if p != (Param{}) {
		t.Errorf("expected empty Param, got %v", p)
	}
	if err == nil {
		t.Error("expected error for builder with nil help")
	}
}

func TestWithHelp_EmptyPolyglotText(t *testing.T) {
	cp := NewParamInt32(0).WithHelp(PolyglotText{})
	if cp.proto.GetHelp() != nil {
		t.Error("expected empty polyglot text to leave the proto help unset")
	}
	p, err := cp.Build()
	if p != (Param{}) {
		t.Errorf("expected empty Param, got %v", p)
	}
	if err == nil {
		t.Error("expected error for builder with empty help")
	}
}

func TestWithOidAliases(t *testing.T) {
	p, err := NewParamInt32(0).WithOidAliases("/old/path", "/legacy/path").Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	aliases := p.Proto.GetOidAliases()
	if len(aliases) != 2 || aliases[0] != "/old/path" {
		t.Errorf("unexpected aliases: %v", aliases)
	}
}

func TestWithMinimalSet(t *testing.T) {
	p, err := NewParamInt32(0).WithMinimalSet(true).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if !p.Proto.GetMinimalSet() {
		t.Error("expected minimal_set=true")
	}
}

func TestWithStateless(t *testing.T) {
	p, err := NewParamInt32(0).WithStateless(true).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if !p.Proto.GetStateless() {
		t.Error("expected stateless=true")
	}
}

func TestWithTemplateOid(t *testing.T) {
	p, err := NewParamInt32(0).WithTemplateOid("templates.volume").Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetTemplateOid() != "templates.volume" {
		t.Errorf("expected 'templates.volume', got %q", p.Proto.GetTemplateOid())
	}
}

// --- Constraint validation tests ---

func TestWithConstraint_ValidInt32Choice(t *testing.T) {
	c := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	})
	p, err := NewParamInt32(0).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
	if p.Proto.GetConstraint().GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", p.Proto.GetConstraint().GetType())
	}
}

func TestWithConstraint_ValidIntRange(t *testing.T) {
	c := NewConstraintInt32Range(0, 100, 1)
	p, err := NewParamInt32(50).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidFloatRange(t *testing.T) {
	c := NewConstraintFloatRange(0, 1, 0.01)
	p, err := NewParamFloat32(0.5).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidStringChoice(t *testing.T) {
	c := NewConstraintStringChoice(true, "a", "b")
	p, err := NewParamString("a").WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidStringStringChoice(t *testing.T) {
	c := NewConstraintStringStringChoice(false, []StringStringChoice{
		{Value: "v", Name: PolyglotText{"en": "V"}},
	})
	p, err := NewParamString("v").WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected constraint to be applied")
	}
}

func TestWithConstraint_ValidAlarmTable(t *testing.T) {
	c := NewConstraintAlarmTable([]AlarmEntry{
		{BitValue: 0, Severity: AlarmSeverityInfo},
	})
	p, err := NewParamInt32(0).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected alarm table constraint to be applied to INT32 param")
	}
}

func TestWithConstraint_RefOid_AnyType(t *testing.T) {
	c := NewConstraintRefOid("shared.my_constraint")
	for _, factory := range []func() *ParamBuilder{
		func() *ParamBuilder { return NewParamInt32(0) },
		func() *ParamBuilder { return NewParamFloat32(0) },
		func() *ParamBuilder { return NewParamString("") },
		func() *ParamBuilder { return NewParamStruct() },
	} {
		p, err := factory().WithConstraint(c).Build()
		if err != nil {
			t.Fatal("expected no error for builder with valid value", err)
		}
		if p.Proto.GetConstraint() == nil {
			t.Errorf("expected RefOid constraint to be valid for param type %v", p.Proto.GetType())
		}
	}
}

func TestWithConstraint_InvalidStringOnInt(t *testing.T) {
	c := NewConstraintStringChoice(true, "a", "b")
	cp := NewParamInt32(0).WithConstraint(c)
	if cp.proto.GetConstraint() != nil {
		t.Error("expected string constraint to be rejected on INT32 param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid constraint")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithConstraint_InvalidIntOnFloat(t *testing.T) {
	c := NewConstraintInt32Range(0, 100, 1)
	cp := NewParamFloat32(0).WithConstraint(c)
	if cp.proto.GetConstraint() != nil {
		t.Error("expected int32 range constraint to be rejected on FLOAT32 param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid constraint")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithConstraint_InvalidFloatOnString(t *testing.T) {
	c := NewConstraintFloatRange(0, 1, 0.1)
	cp := NewParamString("x").WithConstraint(c)
	if cp.proto.GetConstraint() != nil {
		t.Error("expected float range constraint to be rejected on STRING param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid constraint")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithConstraint_Nil(t *testing.T) {
	cp := NewParamInt32(0).WithConstraint(nil)
	if cp.proto.GetConstraint() != nil {
		t.Error("expected nil constraint to be a no-op")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid constraint")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

// Scalar constraints must also be accepted on their corresponding array
// param types, matching the Catena protocol

func TestWithConstraint_ValidIntRangeOnInt32Array(t *testing.T) {
	c := NewConstraintInt32Range(0, 10, 2)
	p, err := NewParamInt32Array([]int32{0, 2, 4}).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected int32 range constraint to be applied to INT32_ARRAY param")
	}
}

func TestWithConstraint_ValidInt32ChoiceOnInt32Array(t *testing.T) {
	c := NewConstraintInt32Choice([]Int32Choice{
		{Value: 0, Name: PolyglotText{"en": "Off"}},
		{Value: 1, Name: PolyglotText{"en": "On"}},
	})
	p, err := NewParamInt32Array([]int32{0, 1, 0}).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected int32 choice constraint to be applied to INT32_ARRAY param")
	}
}

func TestWithConstraint_ValidAlarmTableOnInt32Array(t *testing.T) {
	c := NewConstraintAlarmTable([]AlarmEntry{
		{BitValue: 0, Severity: AlarmSeverityInfo},
	})
	p, err := NewParamInt32Array([]int32{0, 1, 2}).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected alarm table constraint to be applied to INT32_ARRAY param")
	}
}

func TestWithConstraint_ValidFloatRangeOnFloat32Array(t *testing.T) {
	c := NewConstraintFloatRange(0, 100, 0.5)
	p, err := NewParamFloat32Array([]float32{1.0, 2.0}).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected float range constraint to be applied to FLOAT32_ARRAY param")
	}
}

func TestWithConstraint_ValidStringChoiceOnStringArray(t *testing.T) {
	c := NewConstraintStringChoice(true, "a", "b")
	p, err := NewParamStringArray([]string{"a", "b"}).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected string choice constraint to be applied to STRING_ARRAY param")
	}
}

func TestWithConstraint_ValidStringStringChoiceOnStringArray(t *testing.T) {
	c := NewConstraintStringStringChoice(true, []StringStringChoice{
		{Value: "<#FF0000>", Name: PolyglotText{"en": "Red"}},
	})
	p, err := NewParamStringArray([]string{"<#FF0000>"}).WithConstraint(c).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetConstraint() == nil {
		t.Fatal("expected string-string choice constraint to be applied to STRING_ARRAY param")
	}
}

func TestWithConstraint_InvalidIntOnStringArray(t *testing.T) {
	c := NewConstraintInt32Range(0, 10, 1)
	cp := NewParamStringArray([]string{"a"}).WithConstraint(c)
	if cp.proto.GetConstraint() != nil {
		t.Error("expected int32 range constraint to be rejected on STRING_ARRAY param")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid constraint")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

// --- Full chaining test ---

func TestFullChaining(t *testing.T) {
	c := NewConstraintInt32RangeWithDisplay(0, 100, 1, 0, 100)
	p, err := NewParamInt32(0).
		WithName(PolyglotText{"en": "Volume"}).
		WithConstraint(c).
		WithReadOnly(false).
		WithWidget("slider").
		WithAccessScope("operator").
		WithClientHint("unit", "dB").
		WithHelp(PolyglotText{"en": "Audio volume"}).
		WithOidAliases("/old/volume").
		WithMinimalSet(true).
		WithStateless(false).
		WithTemplateOid("templates.volume").
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if p.Proto.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", p.Proto.GetType())
	}
	if p.Proto.GetName().GetDisplayStrings()["en"] != "Volume" {
		t.Error("name mismatch")
	}
	if p.Proto.GetConstraint().GetInt32Range().GetMaxValue() != 100 {
		t.Error("constraint mismatch")
	}
	if p.Proto.GetWidget() != "slider" {
		t.Error("widget mismatch")
	}
	if p.Proto.GetAccessScope() != "operator" {
		t.Error("access_scope mismatch")
	}
	if p.Proto.GetClientHints()["unit"] != "dB" {
		t.Error("client_hint mismatch")
	}
	if p.Proto.GetHelp().GetDisplayStrings()["en"] != "Audio volume" {
		t.Error("help mismatch")
	}
	if len(p.Proto.GetOidAliases()) != 1 {
		t.Error("oid_aliases mismatch")
	}
	if !p.Proto.GetMinimalSet() {
		t.Error("minimal_set mismatch")
	}
	if p.Proto.GetTemplateOid() != "templates.volume" {
		t.Error("template_oid mismatch")
	}
}

// --- WithValue tests ---

func TestWithValue_MatchingType(t *testing.T) {
	v, res := ToValue(int32(42))
	if res != nil {
		t.Fatal(res)
	}
	p, err := NewParamInt32(0).WithValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", p.Proto.GetValue().GetInt32Value())
	}
}

func TestWithValue_Struct(t *testing.T) {
	v, res := ToValue(map[string]any{"name": "test"})
	if res != nil {
		t.Fatal(res)
	}
	p, err := NewParamStruct().WithValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	fields := p.Proto.GetValue().GetStructValue().GetFields()
	if fields["name"].GetStringValue() != "test" {
		t.Errorf("expected struct field 'name'='test', got %q", fields["name"].GetStringValue())
	}
}

func TestWithValue_StructVariant(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "type_a", Value: int32(10)}
	v, res := ToValue(sv)
	if res != nil {
		t.Fatal(res)
	}
	p, err := NewParamStructVariant().WithValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue().GetStructVariantValue().GetStructVariantType() != "type_a" {
		t.Errorf("expected struct_variant_type 'type_a', got %q",
			p.Proto.GetValue().GetStructVariantValue().GetStructVariantType())
	}
}

func TestWithValue_MismatchedType(t *testing.T) {
	v, res := ToValue("hello")
	if res != nil {
		t.Fatal(res)
	}
	cp := NewParamInt32(0).WithValue(v)
	if cp.proto.GetValue().GetInt32Value() != 0 {
		t.Error("expected mismatched WithValue to be a no-op")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

// --- SetValue / GetValue tests ---

func TestSetValue_OK(t *testing.T) {
	cp, err := NewParamInt32(0).Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	res := cp.SetValue(int32(99))
	if res != nil {
		t.Fatalf("expected OK, got %v", res)
	}
	p := cp.Proto
	if p.GetValue().GetInt32Value() != 99 {
		t.Errorf("expected 99, got %d", p.GetValue().GetInt32Value())
	}
}

func TestSetValue_TypeMismatch(t *testing.T) {
	cp, err := NewParamInt32(0).Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	res := cp.SetValue("not an int")
	if !errors.Is(res, ErrInvalid) {
		t.Errorf("expected ErrInvalid, got %v", res)
	}
	if cp.Proto.GetValue().GetInt32Value() != 0 {
		t.Error("expected value to remain unchanged after type mismatch")
	}
}

func TestSetValue_ConversionError(t *testing.T) {
	cp, err := NewParamInt32(0).Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	res := cp.SetValue(struct{}{})
	if res == nil {
		t.Error("expected conversion error for unsupported type")
	}
}

func TestGetValue_OK(t *testing.T) {
	cp, err := NewParamString("hello").Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	v, res := cp.GetValue()
	if res != nil {
		t.Fatalf("expected OK, got %v", res)
	}
	s, ok := v.(string)
	if !ok || s != "hello" {
		t.Errorf("expected 'hello', got %v", v)
	}
}

func TestGetValue_Nil(t *testing.T) {
	cp, err := NewParamStruct().Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	v, res := cp.GetValue()
	if res != nil {
		t.Fatalf("expected OK for nil value, got %v", res)
	}
	if v != nil {
		t.Errorf("expected nil value, got %v", v)
	}
}

func TestSetGetValue_Roundtrip_Struct(t *testing.T) {
	cp, err := NewParamStruct().Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	input := map[string]any{"x": int32(1), "y": int32(2)}
	res := cp.SetValue(input)
	if res != nil {
		t.Fatalf("SetValue failed: %s", res)
	}
	out, res := cp.GetValue()
	if res != nil {
		t.Fatalf("GetValue failed: %s", res)
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
	p, err := NewParamStruct(map[string]any{"a": int32(1)}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	fields := p.Proto.GetValue().GetStructValue().GetFields()
	if fields["a"].GetInt32Value() != 1 {
		t.Errorf("expected struct field 'a'=1, got %d", fields["a"].GetInt32Value())
	}
}

func TestNewParamStruct_NoValue(t *testing.T) {
	p, err := NewParamStruct().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue() != nil {
		t.Error("expected nil value when no arg provided")
	}
}

func TestNewParamStructVariant_WithValue(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "kind_a", Value: int32(5)}
	p, err := NewParamStructVariant(sv).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue().GetStructVariantValue().GetStructVariantType() != "kind_a" {
		t.Errorf("expected struct_variant_type 'kind_a', got %q",
			p.Proto.GetValue().GetStructVariantValue().GetStructVariantType())
	}
}

func TestNewParamStructArray_WithValue(t *testing.T) {
	arr := []map[string]any{
		{"x": int32(1)},
		{"x": int32(2)},
	}
	p, err := NewParamStructArray(arr).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	list := p.Proto.GetValue().GetStructArrayValues().GetStructValues()
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
	p, err := NewParamStructVariantArray(arr).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	list := p.Proto.GetValue().GetStructVariantArrayValues().GetStructVariants()
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
	cp, err := NewParamFromValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cp.Proto.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected INT32, got %v", cp.Proto.GetType())
	}
	if cp.Proto.GetValue().GetInt32Value() != 42 {
		t.Errorf("expected 42, got %d", cp.Proto.GetValue().GetInt32Value())
	}
}

func TestNewParamFromValue_Struct(t *testing.T) {
	v, _ := ToValue(map[string]any{"k": "v"})
	cp, err := NewParamFromValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cp.Proto.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_StructVariant(t *testing.T) {
	v, _ := ToValue(StructVariantValue{StructVariantType: "t", Value: int32(0)})
	cp, err := NewParamFromValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cp.Proto.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_StructArray(t *testing.T) {
	v, _ := ToValue([]map[string]any{{"a": int32(1)}})
	cp, err := NewParamFromValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cp.Proto.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_StructVariantArray(t *testing.T) {
	v, _ := ToValue([]StructVariantValue{{StructVariantType: "t", Value: int32(0)}})
	cp, err := NewParamFromValue(v).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cp.Proto.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", cp.Proto.GetType())
	}
}

func TestNewParamFromValue_Nil(t *testing.T) {
	cp := NewParamFromValue(Value{})
	if cp.proto.GetType() != protos.ParamType_UNDEFINED {
		t.Errorf("expected UNDEFINED for nil value, got %v", cp.proto.GetType())
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
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
			if res != nil {
				t.Fatalf("ToValue failed: %s", res)
			}
			cp, err := NewParamFromValue(v).Build()
			if err != nil {
				t.Fatal("expected no error for builder with valid value", err)
			}
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

func TestIsValueValidForParamType_AllKinds(t *testing.T) {
	tests := []struct {
		name     string
		input    any
		wantType protos.ParamType
	}{
		{"int32", int32(1), protos.ParamType_INT32},
		{"float32", float32(1.5), protos.ParamType_FLOAT32},
		{"string", "hello", protos.ParamType_STRING},
		{"struct", map[string]any{"a": int32(1)}, protos.ParamType_STRUCT},
		{"struct_variant", StructVariantValue{StructVariantType: "a", Value: int32(1)}, protos.ParamType_STRUCT_VARIANT},
		{"int32_array", []int32{1, 2}, protos.ParamType_INT32_ARRAY},
		{"float32_array", []float32{1.0}, protos.ParamType_FLOAT32_ARRAY},
		{"string_array", []string{"a"}, protos.ParamType_STRING_ARRAY},
		{"data", DataPayload{Payload: []byte("hello")}, protos.ParamType_DATA},
		{"struct_array", []map[string]any{{"a": int32(1)}}, protos.ParamType_STRUCT_ARRAY},
		{"struct_variant_array", []StructVariantValue{{StructVariantType: "a", Value: int32(1)}}, protos.ParamType_STRUCT_VARIANT_ARRAY},
		{"empty", EmptyValue{}, protos.ParamType_EMPTY},
		{"undefined", UndefinedValue(0), protos.ParamType_UNDEFINED},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			v, res := ToValue(tt.input)
			if res != nil {
				t.Fatalf("ToValue failed: %s", res)
			}
			if !isValueValidForParamType(v.Proto, tt.wantType) {
				t.Errorf("expected %v to be valid for %v", tt.input, tt.wantType)
			}
		})
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

func TestWithParam_SelfReferenceNoClone(t *testing.T) {
	cp := NewParamStruct()
	cp.WithParam("self", cp)
	if cp.proto.GetParams()["self"] == nil {
		t.Fatal("expected self-referencing sub-param to be set")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with self-reference")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithParam_SelfReferenceClone(t *testing.T) {
	cp := NewParamStruct()
	cp.WithParam("self", cp.Clone())
	if cp.proto.GetParams()["self"] == nil {
		t.Fatal("expected cloned self sub-param to be set")
	}
	p, err := cp.Build()
	if err != nil {
		t.Fatal("expected no error for builder with cloned self")
	}
	if p.Proto.GetParams()["self"] == nil {
		t.Fatal("expected cloned self sub-param to be set")
	}
}

// --- Error path tests ---

func TestNewParamStruct_InvalidValue(t *testing.T) {
	cp := NewParamStruct(map[string]any{"bad": struct{}{}})
	if cp.proto.GetType() != protos.ParamType_STRUCT {
		t.Errorf("expected STRUCT, got %v", cp.proto.GetType())
	}
	if cp.proto.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestNewParamStructVariant_InvalidValue(t *testing.T) {
	sv := StructVariantValue{StructVariantType: "t", Value: struct{}{}}
	cp := NewParamStructVariant(sv)
	if cp.proto.GetType() != protos.ParamType_STRUCT_VARIANT {
		t.Errorf("expected STRUCT_VARIANT, got %v", cp.proto.GetType())
	}
	if cp.proto.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestNewParamStructArray_InvalidValue(t *testing.T) {
	arr := []map[string]any{{"bad": struct{}{}}}
	cp := NewParamStructArray(arr)
	if cp.proto.GetType() != protos.ParamType_STRUCT_ARRAY {
		t.Errorf("expected STRUCT_ARRAY, got %v", cp.proto.GetType())
	}
	if cp.proto.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestNewParamStructVariantArray_InvalidValue(t *testing.T) {
	arr := []StructVariantValue{{StructVariantType: "t", Value: struct{}{}}}
	cp := NewParamStructVariantArray(arr)
	if cp.proto.GetType() != protos.ParamType_STRUCT_VARIANT_ARRAY {
		t.Errorf("expected STRUCT_VARIANT_ARRAY, got %v", cp.proto.GetType())
	}
	if cp.proto.GetValue() != nil {
		t.Error("expected nil value when conversion fails")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestNewParamData_EmptyPayload(t *testing.T) {
	dp := DataPayload{}
	p, err := NewParamData(dp).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", p.Proto.GetType())
	}
	if p.Proto.GetValue() == nil || p.Proto.GetValue().GetDataPayload() == nil {
		t.Fatal("expected DataPayload value")
	}
	if payload, ok := p.Proto.GetValue().GetDataPayload().GetKind().(*protos.DataPayload_Payload); !ok || len(payload.Payload) != 0 {
		t.Errorf("expected empty payload kind, got %T", p.Proto.GetValue().GetDataPayload().GetKind())
	}
}

func TestNewParamData_BothPayloadAndUrl(t *testing.T) {
	dp := DataPayload{Payload: []byte("data"), Url: "http://example.com"}
	cp := NewParamData(dp)
	if cp.proto.GetType() != protos.ParamType_DATA {
		t.Errorf("expected DATA, got %v", cp.proto.GetType())
	}
	if cp.proto.GetValue() != nil {
		t.Error("expected nil value when both payload and url are set")
	}
	p, err := cp.Build()
	if err == nil {
		t.Fatal("expected error for builder with invalid value")
	}
	if p != (Param{}) {
		t.Errorf("expected empty Param for builder with error, got %v", p)
	}
}

func TestWithParam_StructVariant(t *testing.T) {
	child := NewParamInt32(5)
	p, err := NewParamStructVariant().WithParam("field", child).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	sub := p.Proto.GetParams()["field"]
	if sub == nil {
		t.Fatal("expected sub-param on STRUCT_VARIANT")
	}
	if sub.GetValue().GetInt32Value() != 5 {
		t.Errorf("expected 5, got %d", sub.GetValue().GetInt32Value())
	}
}

func TestWithParam_StructArray(t *testing.T) {
	child := NewParamString("val")
	p, err := NewParamStructArray().WithParam("item", child).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	sub := p.Proto.GetParams()["item"]
	if sub == nil {
		t.Fatal("expected sub-param on STRUCT_ARRAY")
	}
	if sub.GetValue().GetStringValue() != "val" {
		t.Errorf("expected 'val', got %q", sub.GetValue().GetStringValue())
	}
}

func TestWithParam_StructVariantArray(t *testing.T) {
	child := NewParamFloat32(1.5)
	p, err := NewParamStructVariantArray().WithParam("entry", child).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	sub := p.Proto.GetParams()["entry"]
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
	c := &Constraint{Proto: &protos.Constraint{}}
	if isConstraintValidForParam(c, protos.ParamType_INT32) {
		t.Error("expected constraint with nil Kind to be invalid")
	}
}

func TestSetValue_InvalidThenValid(t *testing.T) {
	cp, err := NewParamInt32(10).Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	res := cp.SetValue("wrong type")
	if !errors.Is(res, ErrInvalid) {
		t.Errorf("expected ErrInvalid, got %v", res)
	}
	if cp.Proto.GetValue().GetInt32Value() != 10 {
		t.Error("expected original value to be preserved after failed SetValue")
	}
	res = cp.SetValue(int32(20))
	if res != nil {
		t.Fatalf("expected OK, got %v", res)
	}
	if cp.Proto.GetValue().GetInt32Value() != 20 {
		t.Errorf("expected 20, got %d", cp.Proto.GetValue().GetInt32Value())
	}
}

func TestWithParam_StructValuesFromSubParams(t *testing.T) {
	param, err := NewParamStruct().
		WithName(PolyglotText{"en": "Struct Example"}).
		WithParam("number", NewParamInt32(7).WithConstraint(NewConstraintInt32Range(0, 10, 1))).
		WithParam("text", NewParamString("hello")).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if param.Proto.GetType() != protos.ParamType_STRUCT {
		t.Fatalf("expected STRUCT, got %v", param.Proto.GetType())
	}
	fields := param.Proto.GetValue().GetStructValue().GetFields()
	if fields["number"].GetInt32Value() != 7 {
		t.Errorf("expected struct value field 'number'=7, got %d", fields["number"].GetInt32Value())
	}
	if fields["text"].GetStringValue() != "hello" {
		t.Errorf("expected struct value field 'text'='hello', got %q", fields["text"].GetStringValue())
	}
	number := param.Proto.GetParams()["number"]
	if number.GetValue() != nil {
		t.Error("expected number descriptor to be valueless")
	}
	if number.GetConstraint().GetInt32Range().GetMaxValue() != 10 {
		t.Fatal("expected nested int32 range constraint max 10")
	}
	if param.Proto.GetParams()["text"].GetValue() != nil {
		t.Error("expected text descriptor to be valueless")
	}
}

// --- Variadic factory value tests ---

func TestNewParamX_NoValue(t *testing.T) {
	tests := []struct {
		name     string
		param    *ParamBuilder
		wantType protos.ParamType
	}{
		{"int32", NewParamInt32(), protos.ParamType_INT32},
		{"float32", NewParamFloat32(), protos.ParamType_FLOAT32},
		{"string", NewParamString(), protos.ParamType_STRING},
		{"struct", NewParamStruct(), protos.ParamType_STRUCT},
		{"struct_variant", NewParamStructVariant(), protos.ParamType_STRUCT_VARIANT},
		{"int32_array", NewParamInt32Array(), protos.ParamType_INT32_ARRAY},
		{"float32_array", NewParamFloat32Array(), protos.ParamType_FLOAT32_ARRAY},
		{"string_array", NewParamStringArray(), protos.ParamType_STRING_ARRAY},
		{"binary", NewParamBinary(), protos.ParamType_BINARY},
		{"struct_array", NewParamStructArray(), protos.ParamType_STRUCT_ARRAY},
		{"struct_variant_array", NewParamStructVariantArray(), protos.ParamType_STRUCT_VARIANT_ARRAY},
		{"data", NewParamData(), protos.ParamType_DATA},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			p, err := tt.param.Build()
			if err != nil {
				t.Fatal("expected no error for builder with valid value", err)
			}
			if p.Proto.GetType() != tt.wantType {
				t.Errorf("expected %v, got %v", tt.wantType, p.Proto.GetType())
			}
			if p.Proto.GetValue() != nil {
				t.Error("expected nil value when no value given")
			}
		})
	}
}

// Intentionally default values (zero int, empty string, EMPTY) still produce
// a non-nil value, which is what distinguishes them from no-arg factories.
func TestNewParamX_DefaultValueIsSet(t *testing.T) {
	p, err := NewParamInt32(0).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue() == nil {
		t.Error("expected non-nil value for intentionally default int32")
	}
	p, err = NewParamString("").Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue() == nil {
		t.Error("expected non-nil value for intentionally empty string")
	}
	p, err = NewParamEmpty().Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue() == nil {
		t.Error("expected non-nil value for EMPTY param")
	}
}

func TestNewParamX_ExtraValuesErrors(t *testing.T) {
	tests := []struct {
		name     string
		param    *ParamBuilder
		wantType protos.ParamType
	}{
		{"INT32", NewParamInt32(1, 2), protos.ParamType_INT32},
		{"FLOAT32", NewParamFloat32(1.1, 2.2), protos.ParamType_FLOAT32},
		{"STRING", NewParamString("a", "b"), protos.ParamType_STRING},
		{"STRUCT", NewParamStruct(map[string]any{"a": 1}, map[string]any{"b": 2}), protos.ParamType_STRUCT},
		{"STRUCT_VARIANT", NewParamStructVariant(
			StructVariantValue{StructVariantType: "a", Value: NewParamInt32(1)},
			StructVariantValue{StructVariantType: "b", Value: NewParamInt32(2)},
		), protos.ParamType_STRUCT_VARIANT},
		{"INT32_ARRAY", NewParamInt32Array([]int32{1, 2, 3}, []int32{4, 5, 6}), protos.ParamType_INT32_ARRAY},
		{"FLOAT32_ARRAY", NewParamFloat32Array([]float32{1.1, 2.2}, []float32{3.3, 4.4}), protos.ParamType_FLOAT32_ARRAY},
		{"STRING_ARRAY", NewParamStringArray([]string{"a", "b"}, []string{"c", "d"}), protos.ParamType_STRING_ARRAY},
		{"BINARY", NewParamBinary([]byte{0xDE, 0xAD}, []byte{0xBE, 0xEF}), protos.ParamType_BINARY},
		{"STRUCT_ARRAY", NewParamStructArray([]map[string]any{{"a": 1, "b": 2}}, []map[string]any{{"c": 3, "d": 4}}), protos.ParamType_STRUCT_ARRAY},
		{"STRUCT_VARIANT_ARRAY", NewParamStructVariantArray(
			[]StructVariantValue{{StructVariantType: "a", Value: NewParamInt32(1)}},
			[]StructVariantValue{{StructVariantType: "b", Value: NewParamInt32(2)}},
		), protos.ParamType_STRUCT_VARIANT_ARRAY},
		{"DATA", NewParamData(
			ToPayload([]byte("content"), "text/plain", "test.txt"),
			ToPayload([]byte("content2"), "text/plain", "test2.txt"),
		), protos.ParamType_DATA},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if tt.param.proto.GetType() != tt.wantType {
				t.Errorf("expected %v, got %v", tt.wantType, tt.param.proto.GetType())
			}
			if tt.param.proto.GetValue() != nil {
				t.Error("expected nil value when extra values are rejected")
			}
			p, err := tt.param.Build()
			if err == nil {
				t.Error("expected error for builder with extra values")
			}
			if p != (Param{}) {
				t.Errorf("expected empty Param, got %v", p)
			}
		})
	}
}

func TestSetValue_FailureLeavesValueNil(t *testing.T) {
	cp, err := NewParamInt32().Build()
	if err != nil {
		t.Fatalf("failed to build param: %v", err)
	}
	if err := cp.SetValue("wrong type"); err == nil {
		t.Fatal("expected SetValue to fail")
	}
	if cp.Proto.GetValue() != nil {
		t.Error("expected value to remain nil after failed SetValue")
	}
}

func TestNewParamStruct_AutoDescriptors(t *testing.T) {
	p, err := NewParamStruct(map[string]any{
		"number": int32(42),
		"text":   "hi",
		"ratio":  float32(0.5),
		"nested": map[string]any{"inner": int32(1)},
	}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	params := p.Proto.GetParams()
	if len(params) != 4 {
		t.Fatalf("expected 4 auto-created descriptors, got %d", len(params))
	}
	wantTypes := map[string]protos.ParamType{
		"number": protos.ParamType_INT32,
		"text":   protos.ParamType_STRING,
		"ratio":  protos.ParamType_FLOAT32,
		"nested": protos.ParamType_STRUCT,
	}
	for oid, wantType := range wantTypes {
		sub := params[oid]
		if sub == nil {
			t.Fatalf("expected auto-created descriptor for %q", oid)
		}
		if sub.GetType() != wantType {
			t.Errorf("expected %q descriptor type %v, got %v", oid, wantType, sub.GetType())
		}
		if sub.GetValue() != nil {
			t.Errorf("expected %q descriptor to be valueless", oid)
		}
	}
	inner := params["nested"].GetParams()["inner"]
	if inner == nil {
		t.Fatal("expected recursive descriptor for nested field 'inner'")
	}
	if inner.GetType() != protos.ParamType_INT32 {
		t.Errorf("expected nested 'inner' descriptor type INT32, got %v", inner.GetType())
	}
}

// structFieldDescriptors is unreachable with empty fields via NewParamStruct
// (Toproto rejects nil/empty maps), so exercise the guard directly: a nil or
// field-less struct value must yield no descriptors rather than an empty map.
func TestStructFieldDescriptors_NoFields(t *testing.T) {
	if got := structFieldDescriptors(nil); got != nil {
		t.Errorf("expected nil descriptors for nil struct value, got %v", got)
	}
	if got := structFieldDescriptors(&protos.StructValue{}); got != nil {
		t.Errorf("expected nil descriptors for struct value without fields, got %v", got)
	}
	if got := structFieldDescriptors(&protos.StructValue{Fields: map[string]*protos.Value{}}); got != nil {
		t.Errorf("expected nil descriptors for struct value with empty fields map, got %v", got)
	}
}

// A valueless sub-param can refine an auto-created descriptor (name,
// constraint, ...) without overriding the value that came from the map.
func TestWithParam_ValuelessDoesNotOverrideMapValue(t *testing.T) {
	p, err := NewParamStruct(map[string]any{"field2": int32(42)}).
		WithParam("field2", NewParamInt32().WithName(PolyglotText{"en": "Field 2"})).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if p.Proto.GetValue().GetStructValue().GetFields()["field2"].GetInt32Value() != 42 {
		t.Errorf("expected map-provided value 42 to be kept, got %d",
			p.Proto.GetValue().GetStructValue().GetFields()["field2"].GetInt32Value())
	}
	sub := p.Proto.GetParams()["field2"]
	if sub.GetName().GetDisplayStrings()["en"] != "Field 2" {
		t.Errorf("expected replaced descriptor to carry name 'Field 2', got %q",
			sub.GetName().GetDisplayStrings()["en"])
	}
	if sub.GetValue() != nil {
		t.Error("expected descriptor to remain valueless")
	}
}

// A valued sub-param overrides the value that came from the map.
func TestWithParam_ValueOverridesMapValue(t *testing.T) {
	p, err := NewParamStruct(map[string]any{"field3": float32(0.0)}).
		WithParam("field3", NewParamFloat32(1.2)).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if p.Proto.GetValue().GetStructValue().GetFields()["field3"].GetFloat32Value() != 1.2 {
		t.Errorf("expected overridden value 1.2, got %f",
			p.Proto.GetValue().GetStructValue().GetFields()["field3"].GetFloat32Value())
	}
}

// The clean builder style produces the same struct value as passing the full
// map up front, and a struct populated via WithParam propagates its value
// when nested into another struct.
func TestWithParam_CleanBuilderEquivalence(t *testing.T) {
	cleanBuilder := NewParamStruct().
		WithParam("field1", NewParamString("value1")).
		WithParam("field2", NewParamInt32(42))
	cleanClone := cleanBuilder.Clone()
	clean, err := cleanBuilder.Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	upfront, err := NewParamStruct(map[string]any{
		"field1": "value1",
		"field2": int32(42),
	}).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if !proto.Equal(clean.Proto, upfront.Proto) {
		t.Errorf("expected clean builder proto to equal up-front map proto:\nclean:   %v\nupfront: %v",
			clean.Proto, upfront.Proto)
	}
	if clean.Proto.GetValue() == nil {
		t.Error("expected struct built via valued WithParam calls to have a non-nil value")
	}

	outer, err := NewParamStruct().WithParam("inner", cleanClone).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	innerValue := outer.Proto.GetValue().GetStructValue().GetFields()["inner"]
	if innerValue.GetStructValue().GetFields()["field1"].GetStringValue() != "value1" {
		t.Error("expected nested struct value to propagate into outer struct value")
	}
	if outer.Proto.GetParams()["inner"].GetValue() != nil {
		t.Error("expected nested struct descriptor to be valueless")
	}
}

func TestParamBuilder_BuildsValidParam(t *testing.T) {
	cp := NewParamInt32(42)
	p, err := cp.Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue().GetInt32Value() != 42 {
		t.Fatal("expected value 42, got", p.Proto.GetValue().GetInt32Value())
	}
}

func TestParamBuilder_BuildSurfacesErrors(t *testing.T) {
	cp := NewParamInt32()
	cp.err = errors.New("test error")
	if _, err := cp.Build(); err == nil {
		t.Fatal("expected error for builder with error")
	}
}

func TestParamBuilder_WithShortCircuitErrors(t *testing.T) {
	cp := NewParamInt32()
	cp.err = errors.New("test error")
	before := proto.Clone(cp.proto)
	value, convErr := ToValue(int32(7))
	if convErr != nil {
		t.Fatal(convErr)
	}

	p, err := cp.
		WithName(PolyglotText{"en": "Test"}).
		WithValue(value).
		WithConstraint(NewConstraintInt32Range(0, 10, 1)).
		WithReadOnly(true).
		WithWidget("slider").
		WithPrecision(2).
		WithMaxLength(8).
		WithAccessScope("st2138:mon").
		WithClientHint("ui", "slider").
		WithParam("child", NewParamInt32(1)).
		WithResponse(true).
		WithHelp(PolyglotText{"en": "Help"}).
		WithOidAliases("alias").
		WithMinimalSet(true).
		WithStateless(true).
		WithTemplateOid("tpl").
		Build()
	if err == nil || err.Error() != "test error" {
		t.Fatalf("expected 'test error', got %v", err)
	}
	if p != (Param{}) {
		t.Fatal("expected empty param for builder with error")
	}
	if !proto.Equal(before, cp.proto) {
		t.Fatal("expected With* to leave the param unchanged after an error")
	}
}

func TestParam_Clone(t *testing.T) {
	cp := NewParamInt32(42)
	clone := cp.Clone()
	if !proto.Equal(cp.proto, clone.proto) {
		t.Fatal("expected clone to be equal to original")
	}
	clone.WithName(PolyglotText{"en": "Test"})
	if proto.Equal(cp.proto, clone.proto) {
		t.Fatal("expected original param to be unchanged")
	}
}

func TestParamBuilder_CloneInheritsError(t *testing.T) {
	cp := NewParamInt32()
	cp.err = errors.New("test error")
	clone := cp.Clone()
	if clone.err == nil || clone.err.Error() != "test error" {
		t.Fatal("expected clone to inherit error")
	}
}

func TestParam_AlreadyBuilt(t *testing.T) {
	cp := NewParamInt32(42)
	p, err := cp.Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue().GetInt32Value() != 42 {
		t.Fatal("expected value 42, got", p.Proto.GetValue().GetInt32Value())
	}
	p, err = cp.WithName(PolyglotText{"en": "Test"}).Build()
	if err == nil || !errors.Is(err, ErrAlreadyBuilt) {
		t.Fatal("expected error for builder after Build(), got", err)
	}
	if p != (Param{}) {
		t.Fatal("expected empty param for building already built param, got", p)
	}
}

func TestParamBuilder_CloneAlreadyBuilt(t *testing.T) {
	cp := NewParamInt32(42)
	p, err := cp.Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetValue().GetInt32Value() != 42 {
		t.Fatal("expected value 42, got", p.Proto.GetValue().GetInt32Value())
	}

	clone := cp.Clone()
	c, err := clone.Build()
	if err == nil || !errors.Is(err, ErrAlreadyBuilt) {
		t.Fatal("expected error for clone after Build(), got", err)
	}
	if c != (Param{}) {
		t.Fatal("expected empty param for clone after Build(), got", c)
	}
}
