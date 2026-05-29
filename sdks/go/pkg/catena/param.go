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

package catena

import (
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Param wraps a protos.Param and exposes a fluent builder API.
type Param struct {
	Proto *protos.Param
}

// --- Factory functions (one per ParamType) ---

func NewParamInt32(value int32) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_INT32,
			Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: value}},
		},
	}
}

func NewParamFloat32(value float32) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_FLOAT32,
			Value: &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: value}},
		},
	}
}

func NewParamString(value string) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_STRING,
			Value: &protos.Value{Kind: &protos.Value_StringValue{StringValue: value}},
		},
	}
}

func NewParamStruct(value map[string]any) *Param {
	cp := &Param{
		Proto: &protos.Param{
			Type: protos.ParamType_STRUCT,
		},
	}
	pv, res := ToProto(value)
	if res.Code != OK {
		logger.Warning("NewParamStruct: failed to convert value; value left nil",
			"error", res.Error)
		return cp
	}
	cp.Proto.Value = pv
	return cp
}

func NewParamStructVariant(value *StructVariantValue) *Param {
	cp := &Param{
		Proto: &protos.Param{
			Type: protos.ParamType_STRUCT_VARIANT,
		},
	}
	if value != nil {
		pv, res := ToProto(*value)
		if res.Code != OK {
			logger.Warning("NewParamStructVariant: failed to convert value; value left nil",
				"error", res.Error)
			return cp
		}
		cp.Proto.Value = pv
	}
	return cp
}

func NewParamInt32Array(value []int32) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_INT32_ARRAY,
			Value: &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: value}}},
		},
	}
}

func NewParamFloat32Array(value []float32) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_FLOAT32_ARRAY,
			Value: &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: value}}},
		},
	}
}

func NewParamStringArray(value []string) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_STRING_ARRAY,
			Value: &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: value}}},
		},
	}
}

func NewParamBinary(value []byte) *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_BINARY,
			Value: &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: &protos.DataPayload{Kind: &protos.DataPayload_Payload{Payload: value}}}},
		},
	}
}

func NewParamStructArray(value []map[string]any) *Param {
	cp := &Param{
		Proto: &protos.Param{
			Type: protos.ParamType_STRUCT_ARRAY,
		},
	}
	pv, res := ToProto(value)
	if res.Code != OK {
		logger.Warning("NewParamStructArray: failed to convert value; value left nil",
			"error", res.Error)
		return cp
	}
	cp.Proto.Value = pv
	return cp
}

func NewParamStructVariantArray(value []StructVariantValue) *Param {
	cp := &Param{
		Proto: &protos.Param{
			Type: protos.ParamType_STRUCT_VARIANT_ARRAY,
		},
	}
	pv, res := ToProto(value)
	if res.Code != OK {
		logger.Warning("NewParamStructVariantArray: failed to convert value; value left nil",
			"error", res.Error)
		return cp
	}
	cp.Proto.Value = pv
	return cp
}

func NewParamEmpty() *Param {
	return &Param{
		Proto: &protos.Param{
			Type:  protos.ParamType_EMPTY,
			Value: &protos.Value{Kind: &protos.Value_EmptyValue{EmptyValue: &protos.Empty{}}},
		},
	}
}

func NewParamData(payload DataPayload) *Param {
	cp := &Param{
		Proto: &protos.Param{
			Type: protos.ParamType_DATA,
		},
	}
	pdp, res := dataPayloadToProto(payload)
	if res.Code != OK {
		logger.Warning("NewParamData: failed to convert DataPayload; value left nil",
			"error", res.Error)
		return cp
	}
	cp.Proto.Value = &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: pdp}}
	return cp
}

// NewParamFromValue creates a Param by inferring the ParamType from the
// Value's proto kind. This allows dynamic param creation regardless of type.
// DataPayload values are mapped to ParamType_DATA since the two
// cannot be distinguished from the value alone.
func NewParamFromValue(v Value) *Param {
	pt := paramTypeFromValueKind(v.Value)
	if pt == protos.ParamType_UNDEFINED {
		logger.Warning("NewParamFromValue: could not infer param type from value; using UNDEFINED")
	}
	return &Param{
		Proto: &protos.Param{
			Type:  pt,
			Value: v.Value,
		},
	}
}

// --- Chainable With* methods ---

func (cp *Param) WithName(name PolyglotText) *Param {
	cp.Proto.Name = &protos.PolyglotText{DisplayStrings: name}
	return cp
}

func (cp *Param) WithValue(v Value) *Param {
	if !isValueValidForParamType(v.Value, cp.Proto.Type) {
		logger.Warning("WithValue: value kind incompatible with param type; ignoring",
			"param_type", cp.Proto.Type.String())
		return cp
	}
	cp.Proto.Value = v.Value
	return cp
}

func (cp *Param) WithConstraint(c *Constraint) *Param {
	if c == nil || c.Proto == nil {
		return cp
	}
	if !isConstraintValidForParam(c.Proto, cp.Proto.Type) {
		logger.Warning("WithConstraint: constraint type incompatible with param type; ignoring",
			"constraint_type", c.Proto.GetType().String(),
			"param_type", cp.Proto.Type.String())
		return cp
	}
	cp.Proto.Constraint = c.Proto
	return cp
}

func (cp *Param) WithReadOnly(readOnly bool) *Param {
	cp.Proto.ReadOnly = readOnly
	return cp
}

func (cp *Param) WithWidget(widget string) *Param {
	cp.Proto.Widget = widget
	return cp
}

func (cp *Param) WithPrecision(precision uint32) *Param {
	if cp.Proto.Type != protos.ParamType_FLOAT32 && cp.Proto.Type != protos.ParamType_FLOAT32_ARRAY {
		logger.Warning("WithPrecision called on non-float param; ignoring",
			"param_type", cp.Proto.Type.String())
		return cp
	}
	cp.Proto.Precision = precision
	return cp
}

func (cp *Param) WithMaxLength(maxLength uint32) *Param {
	if cp.Proto.Type != protos.ParamType_STRING && cp.Proto.Type != protos.ParamType_STRING_ARRAY {
		logger.Warning("WithMaxLength called on param type that does not support max_length; ignoring",
			"param_type", cp.Proto.Type.String())
		return cp
	}
	cp.Proto.MaxLength = maxLength
	return cp
}

func (cp *Param) WithAccessScope(scope string) *Param {
	cp.Proto.AccessScope = scope
	return cp
}

func (cp *Param) WithClientHint(key, value string) *Param {
	if cp.Proto.ClientHints == nil {
		cp.Proto.ClientHints = map[string]string{}
	}
	cp.Proto.ClientHints[key] = value
	return cp
}

var paramTypesWithSubParams = map[protos.ParamType]struct{}{
	protos.ParamType_STRUCT:               {},
	protos.ParamType_STRUCT_VARIANT:       {},
	protos.ParamType_STRUCT_ARRAY:         {},
	protos.ParamType_STRUCT_VARIANT_ARRAY: {},
}

func (cp *Param) WithParam(oid string, param *Param) *Param {
	if param == nil {
		logger.Warning("WithParam called with nil sub-param; ignoring",
			"oid", oid)
		return cp
	}

	cloned := proto.Clone(param.Proto).(*protos.Param)

	if _, ok := paramTypesWithSubParams[cp.Proto.Type]; !ok {
		logger.Warning("WithParam called on param type that does not support sub-params; ignoring",
			"param_type", cp.Proto.Type.String())
		return cp
	}
	if cp.Proto.Params == nil {
		cp.Proto.Params = map[string]*protos.Param{}
	}
	cp.Proto.Params[oid] = cloned
	return cp
}

// TODO: WithResponse only applies to commands. When we add a CatenaCommand helper,
// move this there. Consider an unexported baseParam that both Param and
// CatenaCommand compose from, since other fields may also be command-specific.
func (cp *Param) WithResponse(response bool) *Param {
	cp.Proto.Response = response
	return cp
}

func (cp *Param) WithHelp(help PolyglotText) *Param {
	cp.Proto.Help = &protos.PolyglotText{DisplayStrings: help}
	return cp
}

func (cp *Param) WithOidAliases(aliases ...string) *Param {
	cp.Proto.OidAliases = aliases
	return cp
}

func (cp *Param) WithMinimalSet(minimalSet bool) *Param {
	cp.Proto.MinimalSet = minimalSet
	return cp
}

func (cp *Param) WithStateless(stateless bool) *Param {
	cp.Proto.Stateless = stateless
	return cp
}

// TODO: Ideally this would accept a *Param reference instead of a raw
// string, but Param doesn't track its own OID path (the OID is only
// assigned externally via WithParam). Auto-linking would require params to
// know their full mounted path, which adds significant complexity. Revisit if
// we ever add path tracking to the param tree.
func (cp *Param) WithTemplateOid(oid string) *Param {
	cp.Proto.TemplateOid = oid
	return cp
}

// SetValue converts v to a proto value via ToProto, validates it against the
// param type, and sets it.
func (cp *Param) SetValue(v any) StatusResult {
	pv, res := ToProto(v)
	if res.Code != OK {
		return res
	}
	if !isValueValidForParamType(pv, cp.Proto.Type) {
		return StatusResult{Code: INVALID_ARGUMENT, Error: "value kind incompatible with param type " + cp.Proto.Type.String()}
	}
	cp.Proto.Value = pv
	return StatusResult{Code: OK}
}

// GetValue reads the current value and converts it to a native Go type via
// FromProto. Returns (nil, OK) if no value is set.
func (cp *Param) GetValue() (any, StatusResult) {
	if cp.Proto.Value == nil {
		return nil, StatusResult{Code: OK}
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
func isConstraintValidForParam(c *protos.Constraint, pt protos.ParamType) bool {
	switch c.Kind.(type) {
	case *protos.Constraint_RefOid:
		return true
	case *protos.Constraint_Int32Range, *protos.Constraint_Int32Choice, *protos.Constraint_AlarmTable:
		return pt == protos.ParamType_INT32 || pt == protos.ParamType_INT32_ARRAY
	case *protos.Constraint_FloatRange:
		return pt == protos.ParamType_FLOAT32 || pt == protos.ParamType_FLOAT32_ARRAY
	case *protos.Constraint_StringChoice, *protos.Constraint_StringStringChoice:
		return pt == protos.ParamType_STRING || pt == protos.ParamType_STRING_ARRAY
	default:
		return false
	}
}
