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

// CatenaParam wraps a protos.Param and exposes a fluent builder API.
type CatenaParam struct {
	param *protos.Param
}

// --- Factory functions (one per ParamType) ---

func NewParamInt32(value int32) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_INT32,
			Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: value}},
		},
	}
}

func NewParamFloat32(value float32) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_FLOAT32,
			Value: &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: value}},
		},
	}
}

func NewParamString(value string) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_STRING,
			Value: &protos.Value{Kind: &protos.Value_StringValue{StringValue: value}},
		},
	}
}

func NewParamStruct() *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type: protos.ParamType_STRUCT,
		},
	}
}

func NewParamStructVariant() *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type: protos.ParamType_STRUCT_VARIANT,
		},
	}
}

func NewParamInt32Array(value []int32) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_INT32_ARRAY,
			Value: &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: value}}},
		},
	}
}

func NewParamFloat32Array(value []float32) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_FLOAT32_ARRAY,
			Value: &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: value}}},
		},
	}
}

func NewParamStringArray(value []string) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_STRING_ARRAY,
			Value: &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: value}}},
		},
	}
}

func NewParamBinary(value []byte) *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_BINARY,
			Value: &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: &protos.DataPayload{Kind: &protos.DataPayload_Payload{Payload: value}}}},
		},
	}
}

func NewParamStructArray() *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type: protos.ParamType_STRUCT_ARRAY,
		},
	}
}

func NewParamStructVariantArray() *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type: protos.ParamType_STRUCT_VARIANT_ARRAY,
		},
	}
}

func NewParamEmpty() *CatenaParam {
	return &CatenaParam{
		param: &protos.Param{
			Type:  protos.ParamType_EMPTY,
			Value: &protos.Value{Kind: &protos.Value_EmptyValue{EmptyValue: &protos.Empty{}}},
		},
	}
}

func NewParamData(payload DataPayload) *CatenaParam {
	cp := &CatenaParam{
		param: &protos.Param{
			Type: protos.ParamType_DATA,
		},
	}
	pdp, res := dataPayloadToProto(payload)
	if res.Code != OK {
		logger.Warning("NewParamData: failed to convert DataPayload; value left nil",
			"error", res.Error)
		return cp
	}
	cp.param.Value = &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: pdp}}
	return cp
}

// --- Chainable With* methods ---

func (cp *CatenaParam) WithName(name PolyglotText) *CatenaParam {
	cp.param.Name = &protos.PolyglotText{DisplayStrings: name}
	return cp
}

func (cp *CatenaParam) WithConstraint(c *protos.Constraint) *CatenaParam {
	if c == nil {
		return cp
	}
	if !isConstraintValidForParam(c, cp.param.Type) {
		logger.Warning("WithConstraint: constraint type incompatible with param type; ignoring",
			"constraint_type", c.GetType().String(),
			"param_type", cp.param.Type.String())
		return cp
	}
	cp.param.Constraint = c
	return cp
}

func (cp *CatenaParam) WithReadOnly(readOnly bool) *CatenaParam {
	cp.param.ReadOnly = readOnly
	return cp
}

func (cp *CatenaParam) WithWidget(widget string) *CatenaParam {
	cp.param.Widget = widget
	return cp
}

func (cp *CatenaParam) WithPrecision(precision uint32) *CatenaParam {
	if cp.param.Type != protos.ParamType_FLOAT32 {
		logger.Warning("WithPrecision called on non-FLOAT32 param; ignoring",
			"param_type", cp.param.Type.String())
		return cp
	}
	cp.param.Precision = precision
	return cp
}

func (cp *CatenaParam) WithMaxLength(maxLength uint32) *CatenaParam {
	if cp.param.Type != protos.ParamType_STRING {
		logger.Warning("WithMaxLength called on non-STRING param; ignoring",
			"param_type", cp.param.Type.String())
		return cp
	}
	cp.param.MaxLength = maxLength
	return cp
}

func (cp *CatenaParam) WithAccessScope(scope string) *CatenaParam {
	cp.param.AccessScope = scope
	return cp
}

func (cp *CatenaParam) WithClientHint(key, value string) *CatenaParam {
	if cp.param.ClientHints == nil {
		cp.param.ClientHints = map[string]string{}
	}
	cp.param.ClientHints[key] = value
	return cp
}

var paramTypesWithSubParams = map[protos.ParamType]bool{
	protos.ParamType_STRUCT:               true,
	protos.ParamType_STRUCT_VARIANT:       true,
	protos.ParamType_STRUCT_ARRAY:         true,
	protos.ParamType_STRUCT_VARIANT_ARRAY: true,
}

func (cp *CatenaParam) WithParam(oid string, param *CatenaParam) *CatenaParam {
	if !paramTypesWithSubParams[cp.param.Type] {
		logger.Warning("WithParam called on param type that does not support sub-params; ignoring",
			"param_type", cp.param.Type.String())
		return cp
	}
	if cp.param.Params == nil {
		cp.param.Params = map[string]*protos.Param{}
	}
	cp.param.Params[oid] = proto.Clone(param.param).(*protos.Param)
	return cp
}

func (cp *CatenaParam) WithResponse(response bool) *CatenaParam {
	cp.param.Response = response
	return cp
}

func (cp *CatenaParam) WithHelp(help PolyglotText) *CatenaParam {
	cp.param.Help = &protos.PolyglotText{DisplayStrings: help}
	return cp
}

func (cp *CatenaParam) WithOidAliases(aliases ...string) *CatenaParam {
	cp.param.OidAliases = aliases
	return cp
}

func (cp *CatenaParam) WithMinimalSet(minimalSet bool) *CatenaParam {
	cp.param.MinimalSet = minimalSet
	return cp
}

func (cp *CatenaParam) WithStateless(stateless bool) *CatenaParam {
	cp.param.Stateless = stateless
	return cp
}

func (cp *CatenaParam) WithTemplateOid(oid string) *CatenaParam {
	cp.param.TemplateOid = oid
	return cp
}

// Build returns the underlying protos.Param.
func (cp *CatenaParam) Build() *protos.Param {
	return cp.param
}

// isConstraintValidForParam checks whether a constraint kind is compatible
// with the given ParamType. RefOid constraints are always valid.
func isConstraintValidForParam(c *protos.Constraint, pt protos.ParamType) bool {
	switch c.Kind.(type) {
	case *protos.Constraint_RefOid:
		return true
	case *protos.Constraint_Int32Range, *protos.Constraint_Int32Choice, *protos.Constraint_AlarmTable:
		return pt == protos.ParamType_INT32
	case *protos.Constraint_FloatRange:
		return pt == protos.ParamType_FLOAT32
	case *protos.Constraint_StringChoice, *protos.Constraint_StringStringChoice:
		return pt == protos.ParamType_STRING
	default:
		return false
	}
}
