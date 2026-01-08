package catena

import (
	"github.com/rossvideo/catena/build/go/protos"
	"math"
)

type StructVariantValue struct {
	StructVariantType string `json:"struct_variant_type"`
	Value             any    `json:"value"`
}

// ToProto converts native Go types to protos.Value
func ToProto(v any) *protos.Value {
	switch val := v.(type) {
	case int32:
		return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: val}}
	case int:
		if val > int(math.MaxInt32) || val < int(math.MinInt32) {
			return nil
		}
		return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: int32(val)}}
	case float32:
		return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: val}}
	case float64:
		return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: float32(val)}}
	case string:
		return &protos.Value{Kind: &protos.Value_StringValue{StringValue: val}}
	case []int32:
		return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: val}}}
	case []int:
		out := make([]int32, len(val))
		for i, n := range val {
			if n > int(math.MaxInt32) || n < int(math.MinInt32) {
				return nil
			}
			out[i] = int32(n)
		}
		return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: out}}}
	case []float32:
		return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: val}}}
	case []float64:
		out := make([]float32, len(val))
		for i, n := range val {
			out[i] = float32(n)
		}
		return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: out}}}
	case []string:
		return &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: val}}}
	case map[string]any:
		fields := make(map[string]*protos.Value)
		for k, v := range val {
			fields[k] = ToProto(v)
		}
		return &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{Fields: fields}}}
	case []map[string]any:
		structArr := make([]*protos.StructValue, len(val))
		for i, m := range val {
			fields := make(map[string]*protos.Value)
			for k, v := range m {
				fields[k] = ToProto(v)
			}
			structArr[i] = &protos.StructValue{Fields: fields}
		}
		return &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{StructValues: structArr}}}
	case StructVariantValue:
		return &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{
			StructVariantType: val.StructVariantType,
			Value:             ToProto(val.Value),
		}}}
	case []StructVariantValue:
		var structVariants []*protos.StructVariantValue
		for _, sv := range val {
			structVariants = append(structVariants, &protos.StructVariantValue{
				StructVariantType: sv.StructVariantType,
				Value:             ToProto(sv.Value),
			})
		}
		return &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{
			StructVariants: structVariants,
		}}}
	default:
		return nil
	}
}

// FromProto converts protos.Value to native Go types
func FromProto(pv *protos.Value) any {
	if pv == nil {
		return nil
	}
	switch pv.GetKind().(type) {
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
		var arr []StructVariantValue
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
