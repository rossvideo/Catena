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
	switch kind := pv.Kind.(type) {
	case *protos.Value_Int32Value:
		return kind.Int32Value
	case *protos.Value_Float32Value:
		return kind.Float32Value
	case *protos.Value_StringValue:
		return kind.StringValue
	case *protos.Value_Int32ArrayValues:
		return kind.Int32ArrayValues.Ints
	case *protos.Value_Float32ArrayValues:
		return kind.Float32ArrayValues.Floats
	case *protos.Value_StringArrayValues:
		return kind.StringArrayValues.Strings
	case *protos.Value_StructValue:
		m := make(map[string]any)
		for k, v := range kind.StructValue.Fields {
			m[k] = FromProto(v)
		}
		return m
	case *protos.Value_StructArrayValues:
		arr := make([]map[string]any, len(kind.StructArrayValues.StructValues))
		for i, sv := range kind.StructArrayValues.StructValues {
			m := make(map[string]any)
			for k, v := range sv.Fields {
				m[k] = FromProto(v)
			}
			arr[i] = m
		}
		return arr
	case *protos.Value_StructVariantValue:
		return StructVariantValue{
			StructVariantType: kind.StructVariantValue.StructVariantType,
			Value:             FromProto(kind.StructVariantValue.Value),
		}
	case *protos.Value_StructVariantArrayValues:
		var arr []StructVariantValue
		for _, sv := range kind.StructVariantArrayValues.StructVariants {
			arr = append(arr, StructVariantValue{
				StructVariantType: sv.StructVariantType,
				Value:             FromProto(sv.Value),
			})
		}
		return arr
	default:
		return nil
	}
}
