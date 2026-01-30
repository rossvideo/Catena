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
func FromProto(pv *protos.Value) any {
	if pv == nil {
		return nil
	}
	switch pv.GetKind().(type) {
	case *protos.Value_UndefinedValue:
		return UndefinedValue(pv.GetUndefinedValue())
	case *protos.Value_EmptyValue:
		return EmptyValue{}
	case *protos.Value_Int32Value:
		return pv.GetInt32Value()
	case *protos.Value_Float32Value:
		return pv.GetFloat32Value()
	case *protos.Value_StringValue:
		return pv.GetStringValue()
	case *protos.Value_Int32ArrayValues:
		return pv.GetInt32ArrayValues().GetInts()
	case *protos.Value_Float32ArrayValues:
		return pv.GetFloat32ArrayValues().GetFloats()
	case *protos.Value_StringArrayValues:
		return pv.GetStringArrayValues().GetStrings()
	case *protos.Value_StructValue:
		fields := pv.GetStructValue().GetFields()
		m := make(map[string]any, len(fields))
		for k, v := range fields {
			m[k] = FromProto(v)
		}
		return m
	case *protos.Value_StructArrayValues:
		list := pv.GetStructArrayValues().GetStructValues()
		arr := make([]map[string]any, len(list))
		for i, sv := range list {
			fields := sv.GetFields()
			m := make(map[string]any, len(fields))
			for k, v := range fields {
				m[k] = FromProto(v)
			}
			arr[i] = m
		}
		return arr
	case *protos.Value_StructVariantValue:
		sv := pv.GetStructVariantValue()
		return StructVariantValue{
			StructVariantType: sv.GetStructVariantType(),
			Value:             FromProto(sv.GetValue()),
		}
	case *protos.Value_StructVariantArrayValues:
		list := pv.GetStructVariantArrayValues().GetStructVariants()
		arr := make([]StructVariantValue, 0, len(list))
		for _, sv := range list {
			arr = append(arr, StructVariantValue{
				StructVariantType: sv.GetStructVariantType(),
				Value:             FromProto(sv.GetValue()),
			})
		}
		return arr
	default:
		return nil
	}
}
