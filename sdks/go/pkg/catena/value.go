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
 * @file value.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-09
 */

package catena

import (
	"fmt"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

type Value struct {
	Value *protos.Value
}

type StructVariantValue struct {
	StructVariantType string `json:"struct_variant_type"`
	Value             any    `json:"value"`
}

type EmptyValue struct{}

type UndefinedValue int32

// ParamType represents the type of a parameter value
// Mirrors protos.ParamType for convenience
type ParamType = protos.ParamType

// ParamType constants matching the proto enum
const (
	ParamTypeUndefined          ParamType = protos.ParamType_UNDEFINED
	ParamTypeEmpty              ParamType = protos.ParamType_EMPTY
	ParamTypeInt32              ParamType = protos.ParamType_INT32
	ParamTypeFloat32            ParamType = protos.ParamType_FLOAT32
	ParamTypeString             ParamType = protos.ParamType_STRING
	ParamTypeStruct             ParamType = protos.ParamType_STRUCT
	ParamTypeStructVariant      ParamType = protos.ParamType_STRUCT_VARIANT
	ParamTypeInt32Array         ParamType = protos.ParamType_INT32_ARRAY
	ParamTypeFloat32Array       ParamType = protos.ParamType_FLOAT32_ARRAY
	ParamTypeStringArray        ParamType = protos.ParamType_STRING_ARRAY
	ParamTypeBinary             ParamType = protos.ParamType_BINARY
	ParamTypeStructArray        ParamType = protos.ParamType_STRUCT_ARRAY
	ParamTypeStructVariantArray ParamType = protos.ParamType_STRUCT_VARIANT_ARRAY
	ParamTypeData               ParamType = protos.ParamType_DATA
)

// ConstraintType represents the type of a constraint
// Mirrors protos.Constraint_ConstraintType for convenience
type ConstraintType = protos.Constraint_ConstraintType

// ConstraintType constants matching the proto enum
const (
	ConstraintTypeUndefined          ConstraintType = protos.Constraint_UNDEFINED
	ConstraintTypeIntRange           ConstraintType = protos.Constraint_INT_RANGE
	ConstraintTypeFloatRange         ConstraintType = protos.Constraint_FLOAT_RANGE
	ConstraintTypeIntChoice          ConstraintType = protos.Constraint_INT_CHOICE
	ConstraintTypeStringChoice       ConstraintType = protos.Constraint_STRING_CHOICE
	ConstraintTypeStringStringChoice ConstraintType = protos.Constraint_STRING_STRING_CHOICE
	ConstraintTypeAlarmTable         ConstraintType = protos.Constraint_ALARM_TABLE
)

func ToValue(v any) (Value, StatusResult) {
	val, res := ToProto(v)
	if res.Code != OK {
		return Value{}, StatusResult{Code: res.Code, Error: "ToValue: " + res.Error}
	}
	return Value{Value: val}, StatusResult{Code: OK}
}

// ToProto converts native Go types to protos.Value
func ToProto(v any) (*protos.Value, StatusResult) {
	switch val := v.(type) {
	case DataPayload:
		pdp, res := dataPayloadToProto(val)
		if res.Code != OK {
			return nil, res
		}
		return &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: pdp}}, StatusResult{Code: OK}
	case UndefinedValue:
		return &protos.Value{Kind: &protos.Value_UndefinedValue{UndefinedValue: protos.UndefinedValue(val)}}, StatusResult{Code: OK}
	case EmptyValue:
		return &protos.Value{Kind: &protos.Value_EmptyValue{EmptyValue: &protos.Empty{}}}, StatusResult{Code: OK}
	case int32:
		return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: val}}, StatusResult{Code: OK}
	case float32:
		return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: val}}, StatusResult{Code: OK}
	case string:
		return &protos.Value{Kind: &protos.Value_StringValue{StringValue: val}}, StatusResult{Code: OK}
	case []int32:
		return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: val}}}, StatusResult{Code: OK}
	case []float32:
		return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: val}}}, StatusResult{Code: OK}
	case []string:
		return &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: val}}}, StatusResult{Code: OK}
	case map[string]any:
		fields := make(map[string]*protos.Value)
		for k, v := range val {
			res, sr := ToProto(v)
			if sr.Code != OK {
				return nil, StatusResult{Code: sr.Code, Error: fmt.Sprintf("failed to convert map field %s: %s", k, sr.Error)}
			}
			fields[k] = res
		}
		return &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{Fields: fields}}}, StatusResult{Code: OK}
	case []map[string]any:
		structArr := make([]*protos.StructValue, len(val))
		for i, m := range val {
			fields := make(map[string]*protos.Value)
			for k, v := range m {
				res, sr := ToProto(v)
				if sr.Code != OK {
					return nil, StatusResult{Code: sr.Code, Error: fmt.Sprintf("failed to convert map field %s: %s", k, sr.Error)}
				}
				fields[k] = res
			}
			structArr[i] = &protos.StructValue{Fields: fields}
		}
		return &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{StructValues: structArr}}}, StatusResult{Code: OK}
	case StructVariantValue:
		protoVal, sr := ToProto(val.Value)
		if sr.Code != OK {
			return nil, StatusResult{Code: sr.Code, Error: "failed to convert StructVariantValue.Value: " + sr.Error}
		}
		return &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{
			StructVariantType: val.StructVariantType,
			Value:             protoVal,
		}}}, StatusResult{Code: OK}
	case []StructVariantValue:
		var structVariants []*protos.StructVariantValue
		for _, sv := range val {
			protoVal, sr := ToProto(sv.Value)
			if sr.Code != OK {
				return nil, StatusResult{Code: sr.Code, Error: "failed to convert StructVariantValue.Value in array: " + sr.Error}
			}
			structVariants = append(structVariants, &protos.StructVariantValue{
				StructVariantType: sv.StructVariantType,
				Value:             protoVal,
			})
		}
		return &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{
			StructVariants: structVariants,
		}}}, StatusResult{Code: OK}
	default:
		return nil, StatusResult{Code: INVALID_ARGUMENT, Error: fmt.Sprintf("unsupported type: %T", v)}
	}
}

// FromProto converts protos.Value to native Go types
func FromProto(pv *protos.Value) (any, StatusResult) {
	if pv == nil {
		return nil, StatusResult{Code: INVALID_ARGUMENT, Error: "nil Value"}
	}
	switch pv.GetKind().(type) {
	case *protos.Value_DataPayload:
		pdp := pv.GetDataPayload()
		if pdp == nil {
			return nil, StatusResult{Code: INVALID_ARGUMENT, Error: "nil DataPayload in Value"}
		}
		dp := DataPayload{
			Metadata:        pdp.GetMetadata(),
			Digest:          pdp.GetDigest(),
			PayloadEncoding: Encoding(pdp.GetPayloadEncoding()),
		}
		switch k := pdp.GetKind().(type) {
		case *protos.DataPayload_Url:
			dp.Url = k.Url
		case *protos.DataPayload_Payload:
			dp.Payload = k.Payload
		}
		return dp, StatusResult{Code: OK}
	case *protos.Value_UndefinedValue:
		return UndefinedValue(pv.GetUndefinedValue()), StatusResult{Code: OK}
	case *protos.Value_EmptyValue:
		return EmptyValue{}, StatusResult{Code: OK}
	case *protos.Value_Int32Value:
		return pv.GetInt32Value(), StatusResult{Code: OK}
	case *protos.Value_Float32Value:
		return pv.GetFloat32Value(), StatusResult{Code: OK}
	case *protos.Value_StringValue:
		return pv.GetStringValue(), StatusResult{Code: OK}
	case *protos.Value_Int32ArrayValues:
		return pv.GetInt32ArrayValues().GetInts(), StatusResult{Code: OK}
	case *protos.Value_Float32ArrayValues:
		return pv.GetFloat32ArrayValues().GetFloats(), StatusResult{Code: OK}
	case *protos.Value_StringArrayValues:
		return pv.GetStringArrayValues().GetStrings(), StatusResult{Code: OK}
	case *protos.Value_StructValue:
		fields := pv.GetStructValue().GetFields()
		m := make(map[string]any, len(fields))
		for k, v := range fields {
			val, sr := FromProto(v)
			if sr.Code != OK {
				return nil, StatusResult{Code: sr.Code, Error: fmt.Sprintf("failed to convert struct field %s: %s", k, sr.Error)}
			}
			m[k] = val
		}
		return m, StatusResult{Code: OK}
	case *protos.Value_StructArrayValues:
		list := pv.GetStructArrayValues().GetStructValues()
		arr := make([]map[string]any, len(list))
		for i, sv := range list {
			fields := sv.GetFields()
			m := make(map[string]any, len(fields))
			for k, v := range fields {
				val, sr := FromProto(v)
				if sr.Code != OK {
					return nil, StatusResult{Code: sr.Code, Error: fmt.Sprintf("failed to convert struct field %s: %s", k, sr.Error)}
				}
				m[k] = val
			}
			arr[i] = m
		}
		return arr, StatusResult{Code: OK}
	case *protos.Value_StructVariantValue:
		sv := pv.GetStructVariantValue()
		val, sr := FromProto(sv.GetValue())
		if sr.Code != OK {
			return nil, StatusResult{Code: sr.Code, Error: "failed to convert struct variant value: " + sr.Error}
		}
		return StructVariantValue{
			StructVariantType: sv.GetStructVariantType(),
			Value:             val,
		}, StatusResult{Code: OK}
	case *protos.Value_StructVariantArrayValues:
		list := pv.GetStructVariantArrayValues().GetStructVariants()
		arr := make([]StructVariantValue, 0, len(list))
		for _, sv := range list {
			val, sr := FromProto(sv.GetValue())
			if sr.Code != OK {
				return nil, StatusResult{Code: sr.Code, Error: "failed to convert struct variant value: " + sr.Error}
			}
			arr = append(arr, StructVariantValue{
				StructVariantType: sv.GetStructVariantType(),
				Value:             val,
			})
		}
		return arr, StatusResult{Code: OK}
	default:
		return nil, StatusResult{Code: INVALID_ARGUMENT, Error: fmt.Sprintf("unsupported Value type: %T", pv.GetKind())}
	}
}
