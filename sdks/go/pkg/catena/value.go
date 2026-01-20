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

type DataPayload struct {
	Metadata        map[string]string `json:"metadata,omitempty"`
	Digest          []byte            `json:"digest,omitempty"`
	PayloadEncoding string            `json:"payload_encoding,omitempty"` // "uncompressed", "gzip", or "deflate"
	URL             string            `json:"url,omitempty"`
	Payload         []byte            `json:"payload,omitempty"`
}

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
	case DataPayload:
		// Convert payload encoding string to enum
		var encoding protos.DataPayload_PayloadEncoding
		switch val.PayloadEncoding {
		case "gzip":
			encoding = protos.DataPayload_GZIP
		case "deflate":
			encoding = protos.DataPayload_DEFLATE
		default:
			encoding = protos.DataPayload_UNCOMPRESSED
		}

		dp := &protos.DataPayload{
			Metadata:        val.Metadata,
			Digest:          val.Digest,
			PayloadEncoding: encoding,
		}

		// Set either URL or Payload based on which is provided
		if val.URL != "" && val.Payload == nil {
			dp.Kind = &protos.DataPayload_Url{Url: val.URL}
		} else if val.Payload != nil && val.URL == "" {
			dp.Kind = &protos.DataPayload_Payload{Payload: val.Payload}
		} else {
			return nil, fmt.Errorf("DataPayload must have either URL or Payload set, but not both")
		}

		return &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: dp}}, nil
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
	case *protos.Value_DataPayload:
		dp := pv.GetDataPayload()

		// Convert encoding enum to string
		var encoding string
		switch dp.GetPayloadEncoding() {
		case protos.DataPayload_GZIP:
			encoding = "gzip"
		case protos.DataPayload_DEFLATE:
			encoding = "deflate"
		default:
			encoding = "uncompressed"
		}

		result := DataPayload{
			Metadata:        dp.GetMetadata(),
			Digest:          dp.GetDigest(),
			PayloadEncoding: encoding,
		}

		// Get either URL or Payload based on Kind
		switch kind := dp.GetKind().(type) {
		case *protos.DataPayload_Url:
			result.URL = kind.Url
		case *protos.DataPayload_Payload:
			result.Payload = kind.Payload
		}

		return result
	default:
		return nil
	}
}
