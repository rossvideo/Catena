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
// Each factory accepts an optional value, except struct-like factories. With no arguments the param is
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

// NewParamStruct creates a STRUCT param. Values and their matching
// descriptors must be added using WithValue and WithParam before being built.
// The set struct value must implement all added descriptors.
func NewParamStruct() *ParamBuilder {
	return &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT}}
}

// NewParamStructVariant creates a STRUCT_VARIANT param. Values and descriptors
// must be added using WithValue and WithParam before being built.
// At least one descriptor matching the value's StructVariantType must be added.
func NewParamStructVariant() *ParamBuilder {
	return &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT_VARIANT}}
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

// NewParamStructArray creates a STRUCT_ARRAY param. Values and descriptors
// must be added using WithValue and WithParam before being built.
// All elements must implement all added descriptors.
func NewParamStructArray() *ParamBuilder {
	return &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT_ARRAY}}
}

// NewParamStructVariantArray creates a STRUCT_VARIANT_ARRAY param. Values and descriptors
// must be added using WithValue and WithParam before being built.
// All elements must implement one of the added descriptors.
func NewParamStructVariantArray() *ParamBuilder {
	return &ParamBuilder{proto: &protos.Param{Type: protos.ParamType_STRUCT_VARIANT_ARRAY}}
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
// cannot be distinguished from the value alone. Struct-like values must have
// a matching descriptor added using WithParam before being built.
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

// WithValue sets the param's value. If the param is a struct-like param,
// the value must implement all added descriptors, or will error on Build().
func (cp *ParamBuilder) WithValue(v any) *ParamBuilder {
	if cp.err != nil {
		return cp
	}
	pv, err := ToProto(v)
	if err != nil {
		cp.err = fmt.Errorf("WithValue: failed to convert value to proto: %w: %w", err, ErrInvalid)
		return cp
	}
	cp.proto.Value = pv
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

// WithParam attaches a valueless sub-param descriptor to a struct-like param under oid.
//
//   - STRUCT: Inserting a valued param sets the struct's own Value.Fields[oid] to the param's value.
//   - STRUCT_VARIANT: Inserting a valued param sets the struct variant's own Value to the param's value.
//     If the struct variant already has a value, the operation errors.
//   - STRUCT_ARRAY: Inserting a valued param errors.
//   - STRUCT_VARIANT_ARRAY: Inserting a valued param errors.
//
// A valueless sub-param never touches the parent's Value, so it can refine a
// descriptor (name, constraint, ...) without overriding a pre-existing value.
func (cp *ParamBuilder) WithParam(oid string, param *ParamBuilder) *ParamBuilder {
	p, ok := buildChild(&cp.err, param, (*ParamBuilder).Build, "WithParam")
	if !ok {
		return cp
	}
	if oid == "" {
		cp.err = fmt.Errorf("WithParam called with empty oid: %w", ErrInvalid)
		return cp
	}

	switch cp.proto.Type {
	case protos.ParamType_STRUCT:
		if p.Proto.Value != nil {
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
	case protos.ParamType_STRUCT_VARIANT:
		if p.Proto.Value != nil {
			sv := cp.proto.Value.GetStructVariantValue()
			if sv != nil {
				cp.err = fmt.Errorf("WithParam on struct variant: only one alternative may carry a value, because it selects the variant's active alternative; "+
					"alternative %q already selected with value %v, cannot also select %q with value %v",
					sv.StructVariantType,
					sv.Value.String(),
					oid,
					p.Proto.Value.String(),
				)
				return cp
			}
			sv = &protos.StructVariantValue{}
			cp.proto.Value = &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: sv}}
			sv.StructVariantType = oid
			sv.Value = p.Proto.Value
			p.Proto.Value = nil
		}
	case protos.ParamType_STRUCT_ARRAY, protos.ParamType_STRUCT_VARIANT_ARRAY:
		if p.Proto.Value != nil {
			cp.err = fmt.Errorf("WithParam called on param type that does not support value-bearing param inputs: param_type=%s, value=%v",
				cp.proto.Type.String(),
				p.Proto.Value.String(),
			)
			return cp
		}
	default:
		cp.err = fmt.Errorf("WithParam called on param type that does not support sub-params: param_type=%s",
			cp.proto.Type.String(),
		)
		return cp
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
// Validates the param's value against its type and descriptors, if applicable.
func (cp *ParamBuilder) Build() (Param, error) {
	if cp.err != nil {
		return Param{}, cp.err
	}
	if cp.proto.Value != nil {
		if err := isValueValidForParam(cp.proto.Value, cp.proto); err != nil {
			cp.err = fmt.Errorf("Build: %w: %w", err, ErrInvalid)
			return Param{}, cp.err
		}
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
// Param, and sets it.
func (cp *Param) SetValue(v any) error {
	pv, err := ToProto(v)
	if err != nil {
		return err
	}
	if err := isValueValidForParam(pv, cp.Proto); err != nil {
		return fmt.Errorf("%w: %w", err, ErrInvalid)
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

// isValueValidForParam checks whether a Value is compatible with the
// given Param.
//
//	-STRUCT: The value must implement all descriptors.
//	-STRUCT_VARIANT: The value must implement a descriptor.
//	-STRUCT_ARRAY: All elements must implement all descriptors.
//	-STRUCT_VARIANT_ARRAY: All elements must implement a descriptor.
//
// Returns an error if the value is not valid for the param.
func isValueValidForParam(v *protos.Value, p *protos.Param) error {
	if v == nil {
		return fmt.Errorf("value is nil")
	}
	vType := paramTypeFromValueKind(v)
	var defaultErr error = fmt.Errorf("value is not valid for param type: value_type=%s, param_type=%s", vType.String(), p.Type.String())
	switch vType {
	case protos.ParamType_DATA:
		// Default case can't map DataPayload to both valid types, so unique case is needed.
		if p.Type != protos.ParamType_BINARY && p.Type != protos.ParamType_DATA {
			return defaultErr
		}
	case protos.ParamType_STRUCT:
		if p.Type != protos.ParamType_STRUCT {
			return defaultErr
		}
		structValue := v.GetStructValue()
		if len(structValue.Fields) != len(p.Params) {
			return fmt.Errorf("struct value has different number of fields than descriptors: value_fields=%d, descriptors=%d",
				len(structValue.Fields), len(p.Params),
			)
		}
		for oid, fieldValue := range structValue.Fields {
			desc, ok := p.Params[oid]
			if !ok {
				return fmt.Errorf("struct value contains unknown field: oid=%s", oid)
			}
			if err := isValueValidForParam(fieldValue, desc); err != nil {
				return fmt.Errorf("struct value field %s: %w", oid, err)
			}
		}
		return nil
	case protos.ParamType_STRUCT_VARIANT:
		if p.Type != protos.ParamType_STRUCT_VARIANT {
			return defaultErr
		}
		variant := v.GetStructVariantValue()
		if desc, ok := p.Params[variant.StructVariantType]; ok {
			if err := isValueValidForParam(variant.Value, desc); err != nil {
				return fmt.Errorf("struct variant value mismatches descriptor: %w", err)
			}
			return nil
		}
		return fmt.Errorf("struct variant type does not match any descriptor: value_type=%s", variant.StructVariantType)
	case protos.ParamType_STRUCT_ARRAY:
		if p.Type != protos.ParamType_STRUCT_ARRAY {
			return defaultErr
		}
		values := v.GetStructArrayValues()
		elemParam := &protos.Param{Type: protos.ParamType_STRUCT, Params: p.Params}
		for _, elem := range values.StructValues {
			elemValue := &protos.Value{Kind: &protos.Value_StructValue{StructValue: elem}}
			if err := isValueValidForParam(elemValue, elemParam); err != nil {
				return err
			}
		}
		return nil
	case protos.ParamType_STRUCT_VARIANT_ARRAY:
		if p.Type != protos.ParamType_STRUCT_VARIANT_ARRAY {
			return defaultErr
		}
		values := v.GetStructVariantArrayValues()
		elemParam := &protos.Param{Type: protos.ParamType_STRUCT_VARIANT, Params: p.Params}
		for _, elem := range values.StructVariants {
			elemValue := &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: elem}}
			if err := isValueValidForParam(elemValue, elemParam); err != nil {
				return err
			}
		}
		return nil
	default:
		if p.Type != vType {
			return defaultErr
		}
	}
	return nil
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
