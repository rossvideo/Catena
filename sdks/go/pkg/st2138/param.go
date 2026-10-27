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
 * @brief Param handling for the Catena SDK.
 * @file param.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-11
 */

package st2138

import (
	"fmt"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Param wraps a protos.Param and exposes a fluent builder API.
type Param struct {
	Proto *protos.Param
}

type ParamBuilder struct {
	proto *protos.Param
	err   error
}

// --- Factory functions (one per ParamType) ---
//
// Each factory accepts an optional value. With no arguments the param is
// created without a value, which is useful for valueless sub-param
// descriptors. If more than one value is passed, the builder returns an error.

func NewParamInt32(value ...int32) *ParamBuilder {
	return newParamBuilder(protos.ParamType_INT32, "NewParamInt32", value, func(v int32) *protos.Value {
		return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: v}}
	})
}

func NewParamFloat32(value ...float32) *ParamBuilder {
	return newParamBuilder(protos.ParamType_FLOAT32, "NewParamFloat32", value, func(v float32) *protos.Value {
		return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: v}}
	})
}

func NewParamString(value ...string) *ParamBuilder {
	return newParamBuilder(protos.ParamType_STRING, "NewParamString", value, func(v string) *protos.Value {
		return &protos.Value{Kind: &protos.Value_StringValue{StringValue: v}}
	})
}

// NewParamStruct creates a STRUCT param. When a map of field values is given,
// it becomes the struct's Value and a valueless sub-param descriptor is
// auto-created for every field (recursively for nested maps), so trivial
// fields need no explicit WithParam calls.
func NewParamStruct(value ...map[string]any) *ParamBuilder {
	cp := &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT}}
	if cp.err = extraValueCheck(value, "NewParamStruct"); cp.err != nil {
		return cp
	}
	if len(value) == 0 {
		return cp
	}
	pv, err := ToProto(value[0])
	if err != nil {
		cp.err = fmt.Errorf("NewParamStruct: failed to convert value: %w", err)
		return cp
	}
	params, err := structFieldDescriptors(pv.GetStructValue())
	return setInferredValue(cp, "NewParamStruct", pv, params, err)
}

// NewParamStructVariant creates a STRUCT_VARIANT param. When a value is given,
// it becomes the param's Value and a valueless sub-param descriptor is
// auto-created under the variant type, typed from the inner value. Nested
// struct values recurse. An empty variant type errors.
// Alternatives that are not the current value still need WithParam.
func NewParamStructVariant(value ...StructVariantValue) *ParamBuilder {
	cp := &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT_VARIANT}}
	if cp.err = extraValueCheck(value, "NewParamStructVariant"); cp.err != nil {
		return cp
	}
	if len(value) == 0 {
		return cp
	}
	pv, err := ToProto(value[0])
	if err != nil {
		cp.err = fmt.Errorf("NewParamStructVariant: failed to convert value: %w", err)
		return cp
	}
	params, err := variantDescriptors(pv.GetStructVariantValue())
	return setInferredValue(cp, "NewParamStructVariant", pv, params, err)
}

func NewParamInt32Array(value ...[]int32) *ParamBuilder {
	return newParamBuilder(protos.ParamType_INT32_ARRAY, "NewParamInt32Array", value, func(v []int32) *protos.Value {
		return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: v}}}
	})
}

func NewParamFloat32Array(value ...[]float32) *ParamBuilder {
	return newParamBuilder(protos.ParamType_FLOAT32_ARRAY, "NewParamFloat32Array", value, func(v []float32) *protos.Value {
		return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: v}}}
	})
}

func NewParamStringArray(value ...[]string) *ParamBuilder {
	return newParamBuilder(protos.ParamType_STRING_ARRAY, "NewParamStringArray", value, func(v []string) *protos.Value {
		return &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: v}}}
	})
}

func NewParamBinary(value ...[]byte) *ParamBuilder {
	return newParamBuilder(protos.ParamType_BINARY, "NewParamBinary", value, func(v []byte) *protos.Value {
		return &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: &protos.DataPayload{Kind: &protos.DataPayload_Payload{Payload: v}}}}
	})
}

// NewParamStructArray creates a STRUCT_ARRAY param. When element values are
// given, they become the param's Value and valueless field descriptors are
// merged across every element, including nested structs. Fields of the same
// type union their nested descriptors. A field name that appears as two
// different types fails the builder.
func NewParamStructArray(value ...[]map[string]any) *ParamBuilder {
	cp := &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT_ARRAY}}
	if cp.err = extraValueCheck(value, "NewParamStructArray"); cp.err != nil {
		return cp
	}
	if len(value) == 0 {
		return cp
	}
	pv, err := ToProto(value[0])
	if err != nil {
		cp.err = fmt.Errorf("NewParamStructArray: failed to convert value: %w", err)
		return cp
	}
	params, err := mergeStructDescriptors(pv.GetStructArrayValues().GetStructValues())
	return setInferredValue(cp, "NewParamStructArray", pv, params, err)
}

// NewParamStructVariantArray creates a STRUCT_VARIANT_ARRAY param. When
// elements are given, they become the param's Value and a valueless descriptor
// is auto-created for each variant type present. Repeated types of the same
// inner type union nested descriptors. The same variant type with two different
// inner types fails the builder. An empty variant type errors.
func NewParamStructVariantArray(value ...[]StructVariantValue) *ParamBuilder {
	cp := &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT_VARIANT_ARRAY}}
	if cp.err = extraValueCheck(value, "NewParamStructVariantArray"); cp.err != nil {
		return cp
	}
	if len(value) == 0 {
		return cp
	}
	pv, err := ToProto(value[0])
	if err != nil {
		cp.err = fmt.Errorf("NewParamStructVariantArray: failed to convert value: %w", err)
		return cp
	}
	params, err := mergeVariantDescriptors(pv.GetStructVariantArrayValues().GetStructVariants())
	return setInferredValue(cp, "NewParamStructVariantArray", pv, params, err)
}

func NewParamEmpty() *ParamBuilder {
	return &ParamBuilder{
		proto: &protos.Param{
			Type:  protos.ParamType_EMPTY,
			Value: &protos.Value{Kind: &protos.Value_EmptyValue{EmptyValue: &protos.Empty{}}},
		},
	}
}

func NewParamData(payload ...DataPayload) *ParamBuilder {
	cp := &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_DATA}}
	if cp.err = extraValueCheck(payload, "NewParamData"); cp.err != nil {
		return cp
	}
	if len(payload) == 0 {
		return cp
	}
	pdp, err := dataPayloadToProto(payload[0])
	if err != nil {
		cp.err = fmt.Errorf("NewParamData: failed to convert DataPayload: %w", err)

		return cp
	}
	cp.proto.Value = &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: pdp}}
	return cp
}

// NewParamFromValue creates a Param by inferring the ParamType from the
// Value's proto kind. This allows dynamic param creation regardless of type.
// DataPayload values are mapped to ParamType_DATA since the two
// cannot be distinguished from the value alone.
func NewParamFromValue(v Value) *ParamBuilder {
	pt := paramTypeFromValueKind(v.Proto)
	var err error
	if pt == protos.ParamType_UNDEFINED {
		err = fmt.Errorf("NewParamFromValue: could not infer param type from value")
	}
	return &ParamBuilder{
		proto: &protos.Param{
			Type:  pt,
			Value: v.Proto,
		},
		err: err,
	}
}

// structFieldDescriptors builds a valueless sub-param descriptor for each
// field of a struct value, inferring each descriptor's type from the field's
// value kind. Nested struct fields recurse so their fields get descriptors
// too. A type conflict inside a nested array is returned.
func structFieldDescriptors(sv *protos.StructValue) (map[string]*protos.Param, error) {
	fields := sv.GetFields()
	if len(fields) == 0 {
		return nil, nil
	}
	params := make(map[string]*protos.Param, len(fields))
	for oid, fv := range fields {
		if oid == "" {
			return nil, fmt.Errorf("struct field name cannot be empty")
		}
		p, err := descriptorForValue(fv)
		if err != nil {
			return nil, err
		}
		params[oid] = p
	}
	return params, nil
}

// variantDescriptors builds one valueless descriptor keyed by the variant
// type. An empty type is skipped so it does not become an oid.
func variantDescriptors(sv *protos.StructVariantValue) (map[string]*protos.Param, error) {
	if sv == nil || sv.GetStructVariantType() == "" {
		return nil, fmt.Errorf("struct variant type cannot be nil or empty")
	}
	p, err := descriptorForValue(sv.GetValue())
	if err != nil {
		return nil, err
	}
	return map[string]*protos.Param{sv.GetStructVariantType(): p}, nil
}

// descriptorForValue builds a valueless descriptor whose type matches v.
// Also handles nested descriptors.
func descriptorForValue(v *protos.Value) (*protos.Param, error) {
	nested, err := nestedParams(v)
	if err != nil {
		return nil, err
	}
	return &protos.Param{Type: paramTypeFromValueKind(v), Params: nested}, nil
}

// nestedParams returns valueless descriptors for a struct-like value.
func nestedParams(v *protos.Value) (map[string]*protos.Param, error) {
	if v == nil {
		return nil, nil
	}
	switch kind := v.Kind.(type) {
	case *protos.Value_StructValue:
		return structFieldDescriptors(kind.StructValue)
	case *protos.Value_StructArrayValues:
		return mergeStructDescriptors(kind.StructArrayValues.GetStructValues())
	case *protos.Value_StructVariantValue:
		return variantDescriptors(kind.StructVariantValue)
	case *protos.Value_StructVariantArrayValues:
		return mergeVariantDescriptors(kind.StructVariantArrayValues.GetStructVariants())
	default:
		return nil, nil
	}
}

// mergeStructDescriptors unions field descriptors across array elements, in
// order. Same-type fields union their nested descriptors.
func mergeStructDescriptors(elements []*protos.StructValue) (map[string]*protos.Param, error) {
	var merged map[string]*protos.Param
	for _, el := range elements {
		fields, err := structFieldDescriptors(el)
		if err != nil {
			return nil, err
		}
		merged, err = mergeDescriptors(merged, fields)
		if err != nil {
			return nil, err
		}
	}
	return merged, nil
}

// mergeVariantDescriptors unions one descriptor per variant type present, in
// element order. An empty variant type is skipped.
func mergeVariantDescriptors(variants []*protos.StructVariantValue) (map[string]*protos.Param, error) {
	var merged map[string]*protos.Param
	for _, sv := range variants {
		one, err := variantDescriptors(sv)
		if err != nil {
			return nil, err
		}
		merged, err = mergeDescriptors(merged, one)
		if err != nil {
			return nil, err
		}
	}
	return merged, nil
}

// mergeDescriptors inserts src into dst. A key already present with the same
// ParamType unions its nested descriptors. A different ParamType is an error
// and the partial map is discarded.
func mergeDescriptors(dst, src map[string]*protos.Param) (map[string]*protos.Param, error) {
	if len(src) == 0 {
		return dst, nil
	}
	if dst == nil {
		dst = make(map[string]*protos.Param, len(src))
	}
	for oid, incoming := range src {
		existing, ok := dst[oid]
		if !ok {
			dst[oid] = incoming
			continue
		}
		if existing.GetType() != incoming.GetType() {
			return nil, fmt.Errorf("conflicting sub-param types for %q: is %s, got %s",
				oid, existing.GetType().String(), incoming.GetType().String())
		}
		nested, err := mergeDescriptors(existing.GetParams(), incoming.GetParams())
		if err != nil {
			return nil, err
		}
		if len(nested) > 0 {
			existing.Params = nested
		}
	}
	return dst, nil
}

// setInferredValue stores pv and its valueless descriptors. On err neither is
// stored, so Build reports the failure with an empty Param.
func setInferredValue(cp *ParamBuilder, fn string, pv *protos.Value, params map[string]*protos.Param, err error) *ParamBuilder {
	if err != nil {
		cp.err = fmt.Errorf("%s: %w", fn, err)
		return cp
	}
	cp.proto.Value = pv
	cp.proto.Params = params
	return cp
}

// extraValueCheck checks that there is at most one value provided.
// Returns an error, with the given name of the calling function, if more than one value is provided.
// Returns nil otherwise.
func extraValueCheck[T any](values []T, name string) error {
	if len(values) > 1 {
		return fmt.Errorf("%s: more than one value provided", name)
	}
	return nil
}

// newParamBuilder builds a scalar or array paramBuilder. valueOf should create a proto.Value by
// taking in value; extra values are recorded as the builder's error.
func newParamBuilder[T any](typ protos.ParamType, name string, value []T, valueOf func(T) *protos.Value) *ParamBuilder {
	cp := &ParamBuilder{proto: &protos.Param{Type: typ}}
	if cp.err = extraValueCheck(value, name); cp.err != nil {
		return cp
	}
	if len(value) > 0 {
		cp.proto.Value = valueOf(value[0])
	}
	return cp
}

// --- Chainable With* methods ---

func (cp *ParamBuilder) WithName(name PolyglotText) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.Name = name.Proto()
	return cp
}

func (cp *ParamBuilder) WithValue(v Value) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	if !isValueValidForParamType(v.Proto, cp.proto.Type) {
		cp.err = fmt.Errorf("WithValue: value kind incompatible with param type: param_type=%s",
			cp.proto.Type.String(),
		)
		return cp
	}
	cp.proto.Value = v.Proto
	return cp
}

func (cp *ParamBuilder) WithConstraint(cb *ConstraintBuilder) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	c, ok := buildChild(&cp.err, cb, (*ConstraintBuilder).Build, "WithConstraint")
	if !ok {
		return cp
	}
	if !isConstraintValidForParam(&c, cp.proto.Type) {
		cp.err = fmt.Errorf("WithConstraint: constraint type incompatible with param type: constraint_type=%s, param_type=%s",
			c.Proto.GetType().String(),
			cp.proto.Type.String(),
		)
		return cp
	}
	cp.proto.Constraint = c.Proto
	return cp
}

// WithReadOnly sets the param's read_only flag. Read-only is inherited
// downward: when set to true, every existing sub-param (recursively) is also
// marked read-only, since a child of a read-only parent cannot be writable.
// Setting it to false leaves sub-params untouched, so a child may still be
// independently marked read-only under a writable parent.
func (cp *ParamBuilder) WithReadOnly(readOnly bool) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.ReadOnly = readOnly
	if readOnly {
		propagateReadOnly(cp.proto.Params)
	}
	return cp
}

// propagateReadOnly recursively marks every param in the map (and their
// sub-params) as read-only.
func propagateReadOnly(params map[string]*protos.Param) {
	for _, p := range params {
		if p == nil {
			continue
		}
		p.ReadOnly = true
		propagateReadOnly(p.Params)
	}
}

func (cp *ParamBuilder) WithWidget(widget string) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.Widget = widget
	return cp
}

func (cp *ParamBuilder) WithPrecision(precision uint32) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	if cp.proto.Type != protos.ParamType_FLOAT32 && cp.proto.Type != protos.ParamType_FLOAT32_ARRAY {
		cp.err = fmt.Errorf("WithPrecision called on non-float param: param_type=%s",
			cp.proto.Type.String(),
		)
		return cp
	}
	cp.proto.Precision = precision
	return cp
}

func (cp *ParamBuilder) WithMaxLength(maxLength uint32) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	if cp.proto.Type != protos.ParamType_STRING && cp.proto.Type != protos.ParamType_STRING_ARRAY {
		cp.err = fmt.Errorf("WithMaxLength called on param type that does not support max_length: param_type=%s",
			cp.proto.Type.String(),
		)
		return cp
	}
	cp.proto.MaxLength = maxLength
	return cp
}

func (cp *ParamBuilder) WithAccessScope(scope string) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.AccessScope = scope
	return cp
}

func (cp *ParamBuilder) WithClientHint(key, value string) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	if cp.proto.ClientHints == nil {
		cp.proto.ClientHints = map[string]string{}
	}
	cp.proto.ClientHints[key] = value
	return cp
}

var paramTypesWithSubParams = map[protos.ParamType]struct{}{
	protos.ParamType_STRUCT:               {},
	protos.ParamType_STRUCT_VARIANT:       {},
	protos.ParamType_STRUCT_ARRAY:         {},
	protos.ParamType_STRUCT_VARIANT_ARRAY: {},
}

// WithParam attaches a sub-param descriptor under oid. On a STRUCT parent, a
// sub-param with a value contributes that value to the struct's own
// Value.Fields[oid]; the stored descriptor is kept valueless, since field
// values live in the struct's Value and sub-params are descriptors only. A
// valueless sub-param never touches the parent's Value, so it can refine a
// descriptor (name, constraint, ...) without overriding a value that came
// from NewParamStruct, NewParamStructArray, NewParamStructVariant, or
// NewParamStructVariantArray.
func (cp *ParamBuilder) WithParam(oid string, param *ParamBuilder) *ParamBuilder {
	p, ok := buildChild(&cp.err, param, (*ParamBuilder).Build, "WithParam")
	if !ok {
		return cp
	}
	if _, ok := paramTypesWithSubParams[cp.proto.Type]; !ok {
		cp.err = fmt.Errorf("WithParam called on param type that does not support sub-params: param_type=%s",
			cp.proto.Type.String(),
		)
		return cp
	}

	if cp.proto.Type == protos.ParamType_STRUCT && p.Proto.Value != nil {
		sv := cp.proto.Value.GetStructValue()
		if sv == nil {
			sv = &protos.StructValue{}
			cp.proto.Value = &protos.Value{Kind: &protos.Value_StructValue{StructValue: sv}}
		}
		if sv.Fields == nil {
			sv.Fields = map[string]*protos.Value{}
		}
		sv.Fields[oid] = p.Proto.Value
		p.Proto.Value = nil
	}

	// A read-only parent forces its children read-only. When the parent is
	// writable the child keeps its own read_only flag, so a child can still be
	// independently marked read-only.
	if cp.proto.ReadOnly {
		p.Proto.ReadOnly = true
		propagateReadOnly(p.Proto.Params)
	}
	put(&cp.proto.Params, oid, p.Proto)
	return cp
}

// TODO: WithResponse only applies to commands. When we add a CatenaCommand helper,
// move this there. Consider an unexported baseParam that both Param and
// CatenaCommand compose from, since other fields may also be command-specific.
func (cp *ParamBuilder) WithResponse(response bool) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.Response = response
	return cp
}

func (cp *ParamBuilder) WithHelp(help PolyglotText) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.Help = help.Proto()
	return cp
}

func (cp *ParamBuilder) WithOidAliases(aliases ...string) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.OidAliases = aliases
	return cp
}

func (cp *ParamBuilder) WithMinimalSet(minimalSet bool) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.MinimalSet = minimalSet
	return cp
}

func (cp *ParamBuilder) WithStateless(stateless bool) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.Stateless = stateless
	return cp
}

// TODO: Ideally this would accept a *Param reference instead of a raw
// string, but Param doesn't track its own OID path (the OID is only
// assigned externally via WithParam). Auto-linking would require params to
// know their full mounted path, which adds significant complexity. Revisit if
// we ever add path tracking to the param tree.
func (cp *ParamBuilder) WithTemplateOid(oid string) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	cp.proto.TemplateOid = oid
	return cp
}

// Build returns a usable Param from the builder. If the builder has an error, it
// returns an empty Param and the error. Must be called after all builder methods have been called.
func (cp *ParamBuilder) Build() (Param, error) {
	if cp.err != nil {
		return Param{}, cp.err
	}
	cp.err = ErrAlreadyBuilt
	return Param{Proto: cp.proto}, nil
}

// Clone returns a deep copy of the ParamBuilder.
// Inherits the builder's error state, including ErrAlreadyBuilt.
// Must be called before Build(), or the clone will be unusable.
func (cp *ParamBuilder) Clone() *ParamBuilder {
	return &ParamBuilder{proto: proto.Clone(cp.proto).(*protos.Param), err: cp.err}
}

// SetValue converts v to a proto value via ToProto, validates it against the
// param type, and sets it.
func (cp *Param) SetValue(v any) error {
	pv, err := ToProto(v)
	if err != nil {
		return err
	}
	if !isValueValidForParamType(pv, cp.Proto.Type) {
		return fmt.Errorf("value kind incompatible with param type %s: %w", cp.Proto.Type.String(), ErrInvalid)
	}
	cp.Proto.Value = pv
	return nil
}

// GetValue reads the current value and converts it to a native Go type via
// FromProto. Returns (nil, nil) if no value is set.
func (cp *Param) GetValue() (any, error) {
	if cp.Proto.Value == nil {
		return nil, nil
	}
	return FromProto(cp.Proto.Value)
}

// --- Validation helpers ---

// isValueValidForParamType checks whether a Value kind is compatible with the
// given ParamType.
func isValueValidForParamType(v *protos.Value, pt protos.ParamType) bool {
	if v == nil {
		return false
	}
	switch v.Kind.(type) {
	case *protos.Value_Int32Value:
		return pt == protos.ParamType_INT32
	case *protos.Value_Float32Value:
		return pt == protos.ParamType_FLOAT32
	case *protos.Value_StringValue:
		return pt == protos.ParamType_STRING
	case *protos.Value_StructValue:
		return pt == protos.ParamType_STRUCT
	case *protos.Value_StructVariantValue:
		return pt == protos.ParamType_STRUCT_VARIANT
	case *protos.Value_Int32ArrayValues:
		return pt == protos.ParamType_INT32_ARRAY
	case *protos.Value_Float32ArrayValues:
		return pt == protos.ParamType_FLOAT32_ARRAY
	case *protos.Value_StringArrayValues:
		return pt == protos.ParamType_STRING_ARRAY
	case *protos.Value_DataPayload:
		return pt == protos.ParamType_BINARY || pt == protos.ParamType_DATA
	case *protos.Value_StructArrayValues:
		return pt == protos.ParamType_STRUCT_ARRAY
	case *protos.Value_StructVariantArrayValues:
		return pt == protos.ParamType_STRUCT_VARIANT_ARRAY
	case *protos.Value_EmptyValue:
		return pt == protos.ParamType_EMPTY
	case *protos.Value_UndefinedValue:
		return pt == protos.ParamType_UNDEFINED
	default:
		return false
	}
}

// paramTypeFromValueKind infers the ParamType from a Value's proto kind.
// DataPayload maps to DATA (not BINARY) since the two share the same value
// kind. Returns UNDEFINED for nil or unknown kinds.
func paramTypeFromValueKind(v *protos.Value) protos.ParamType {
	if v == nil {
		return protos.ParamType_UNDEFINED
	}
	switch v.Kind.(type) {
	case *protos.Value_Int32Value:
		return protos.ParamType_INT32
	case *protos.Value_Float32Value:
		return protos.ParamType_FLOAT32
	case *protos.Value_StringValue:
		return protos.ParamType_STRING
	case *protos.Value_StructValue:
		return protos.ParamType_STRUCT
	case *protos.Value_StructVariantValue:
		return protos.ParamType_STRUCT_VARIANT
	case *protos.Value_Int32ArrayValues:
		return protos.ParamType_INT32_ARRAY
	case *protos.Value_Float32ArrayValues:
		return protos.ParamType_FLOAT32_ARRAY
	case *protos.Value_StringArrayValues:
		return protos.ParamType_STRING_ARRAY
	case *protos.Value_DataPayload:
		return protos.ParamType_DATA
	case *protos.Value_StructArrayValues:
		return protos.ParamType_STRUCT_ARRAY
	case *protos.Value_StructVariantArrayValues:
		return protos.ParamType_STRUCT_VARIANT_ARRAY
	case *protos.Value_EmptyValue:
		return protos.ParamType_EMPTY
	case *protos.Value_UndefinedValue:
		return protos.ParamType_UNDEFINED
	default:
		return protos.ParamType_UNDEFINED
	}
}

// isConstraintValidForParam checks whether a constraint kind is compatible
// with the given ParamType. RefOid constraints are always valid. Scalar
// constraints are also valid on their corresponding array param type
// (e.g. Int32Range on INT32 or INT32_ARRAY).
func isConstraintValidForParam(c *Constraint, pt protos.ParamType) bool {
	switch c.Proto.Kind.(type) {
	case *protos.Constraint_RefOid:
		return true
	case *protos.Constraint_Int32Range, *protos.Constraint_Int32Choice, *protos.Constraint_AlarmTable:
		return pt == protos.ParamType_INT32 || pt == protos.ParamType_INT32_ARRAY
	case *protos.Constraint_FloatRange:
		return pt == protos.ParamType_FLOAT32 || pt == protos.ParamType_FLOAT32_ARRAY
	case *protos.Constraint_StringChoice, *protos.Constraint_StringStringChoice, *protos.Constraint_FormattedString:
		return pt == protos.ParamType_STRING || pt == protos.ParamType_STRING_ARRAY
	default:
		return false
	}
}
