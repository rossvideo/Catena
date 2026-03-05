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
 * @date 2026-02-04
 */

package catena

import (
	"fmt"

	"github.com/rossvideo/catena/build/go/protos"
)

type CatenaValue struct {
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

func ToCatenaValue(v any) (CatenaValue, error) {
	val, err := ToProto(v)
	if err != nil {
		return CatenaValue{}, fmt.Errorf("ToCatenaValue: %w", err)
	}
	return CatenaValue{Value: val}, nil
}

// ToProto converts native Go types to protos.Value
func ToProto(v any) (*protos.Value, error) {
	switch val := v.(type) {
	case DataPayload:
		pdp, err := dataPayloadToProto(val)
		if err != nil {
			return nil, err
		}
		return &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: pdp}}, nil
	case UndefinedValue:
		return &protos.Value{Kind: &protos.Value_UndefinedValue{UndefinedValue: protos.UndefinedValue(val)}}, nil
	case EmptyValue:
		return &protos.Value{Kind: &protos.Value_EmptyValue{EmptyValue: &protos.Empty{}}}, nil
	case int32:
		return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: val}}, nil
	case float32:
		return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: val}}, nil
	case string:
		return &protos.Value{Kind: &protos.Value_StringValue{StringValue: val}}, nil
	case []int32:
		return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: val}}}, nil
	case []float32:
		return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: val}}}, nil
	case []string:
		return &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: val}}}, nil
	case map[string]any:
		fields := make(map[string]*protos.Value)
		for k, v := range val {
			res, err := ToProto(v)
			if err != nil {
				return nil, fmt.Errorf("failed to convert map field %s: %w", k, err)
			}
			fields[k] = res
		}
		return &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{Fields: fields}}}, nil
	case []map[string]any:
		structArr := make([]*protos.StructValue, len(val))
		for i, m := range val {
			fields := make(map[string]*protos.Value)
			for k, v := range m {
				res, err := ToProto(v)
				if err != nil {
					return nil, fmt.Errorf("failed to convert map field %s: %w", k, err)
				}
				fields[k] = res
			}
			structArr[i] = &protos.StructValue{Fields: fields}
		}
		return &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{StructValues: structArr}}}, nil
	case StructVariantValue:
		protoVal, err := ToProto(val.Value)
		if err != nil {
			return nil, fmt.Errorf("failed to convert StructVariantValue.Value: %w", err)
		}
		return &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{
			StructVariantType: val.StructVariantType,
			Value:             protoVal,
		}}}, nil
	case []StructVariantValue:
		var structVariants []*protos.StructVariantValue
		for _, sv := range val {
			protoVal, err := ToProto(sv.Value)
			if err != nil {
				return nil, fmt.Errorf("failed to convert StructVariantValue.Value in array: %w", err)
			}
			structVariants = append(structVariants, &protos.StructVariantValue{
				StructVariantType: sv.StructVariantType,
				Value:             protoVal,
			})
		}
		return &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{
			StructVariants: structVariants,
		}}}, nil
	default:
		return nil, fmt.Errorf("unsupported type: %T", v)
	}
}

// FromProto converts protos.Value to native Go types
func FromProto(pv *protos.Value) (any, error) {
	if pv == nil {
		return nil, fmt.Errorf("nil Value")
	}
	switch pv.GetKind().(type) {
	case *protos.Value_DataPayload:
		pdp := pv.GetDataPayload()
		if pdp == nil {
			return nil, fmt.Errorf("nil DataPayload in Value")
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
		return dp, nil
	case *protos.Value_UndefinedValue:
		return UndefinedValue(pv.GetUndefinedValue()), nil
	case *protos.Value_EmptyValue:
		return EmptyValue{}, nil
	case *protos.Value_Int32Value:
		return pv.GetInt32Value(), nil
	case *protos.Value_Float32Value:
		return pv.GetFloat32Value(), nil
	case *protos.Value_StringValue:
		return pv.GetStringValue(), nil
	case *protos.Value_Int32ArrayValues:
		return pv.GetInt32ArrayValues().GetInts(), nil
	case *protos.Value_Float32ArrayValues:
		return pv.GetFloat32ArrayValues().GetFloats(), nil
	case *protos.Value_StringArrayValues:
		return pv.GetStringArrayValues().GetStrings(), nil
	case *protos.Value_StructValue:
		fields := pv.GetStructValue().GetFields()
		m := make(map[string]any, len(fields))
		for k, v := range fields {
			val, err := FromProto(v)
			if err != nil {
				return nil, fmt.Errorf("failed to convert struct field %s: %w", k, err)
			}
			m[k] = val
		}
		return m, nil
	case *protos.Value_StructArrayValues:
		list := pv.GetStructArrayValues().GetStructValues()
		arr := make([]map[string]any, len(list))
		for i, sv := range list {
			fields := sv.GetFields()
			m := make(map[string]any, len(fields))
			for k, v := range fields {
				val, err := FromProto(v)
				if err != nil {
					return nil, fmt.Errorf("failed to convert struct field %s: %w", k, err)
				}
				m[k] = val
			}
			arr[i] = m
		}
		return arr, nil
	case *protos.Value_StructVariantValue:
		sv := pv.GetStructVariantValue()
		val, err := FromProto(sv.GetValue())
		if err != nil {
			return nil, fmt.Errorf("failed to convert struct variant value: %w", err)
		}
		return StructVariantValue{
			StructVariantType: sv.GetStructVariantType(),
			Value:             val,
		}, nil
	case *protos.Value_StructVariantArrayValues:
		list := pv.GetStructVariantArrayValues().GetStructVariants()
		arr := make([]StructVariantValue, 0, len(list))
		for _, sv := range list {
			val, err := FromProto(sv.GetValue())
			if err != nil {
				return nil, fmt.Errorf("failed to convert struct variant value: %w", err)
			}
			arr = append(arr, StructVariantValue{
				StructVariantType: sv.GetStructVariantType(),
				Value:             val,
			})
		}
		return arr, nil
	default:
		return nil, fmt.Errorf("unsupported Value type: %T", pv.GetKind())
	}
}
