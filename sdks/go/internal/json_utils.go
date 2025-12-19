package internal

import (
	"encoding/json"
	"errors"
	"net/http"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// Helper to recursively convert JSON structure into protos.Value
func jsonToValue(data any) (*protos.Value, error) {
	if m, ok := data.(map[string]any); ok {
		// Check which value type this represents
		if int32Val, ok := m["int32_value"]; ok {
			if f, ok := int32Val.(float64); ok {
				return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: int32(f)}}, nil
			}
		}
		if float32Val, ok := m["float32_value"]; ok {
			if f, ok := float32Val.(float64); ok {
				return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: float32(f)}}, nil
			}
		}
		if stringVal, ok := m["string_value"]; ok {
			if s, ok := stringVal.(string); ok {
				return &protos.Value{Kind: &protos.Value_StringValue{StringValue: s}}, nil
			}
		}
		if structVal, ok := m["struct_value"]; ok {
			if fields, ok := structVal.(map[string]any); ok {
				protoFields := make(map[string]*protos.Value)
				for k, v := range fields {
					pv, err := jsonToValue(v)
					if err != nil {
						return nil, err
					}
					protoFields[k] = pv
				}
				return &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{Fields: protoFields}}}, nil
			}
		}
		if structVariantVal, ok := m["struct_variant_value"]; ok {
			if svMap, ok := structVariantVal.(map[string]any); ok {
				svType, okType := svMap["struct_variant_type"].(string)
				svValue, okValue := svMap["value"]
				if okType && okValue {
					pv, err := jsonToValue(svValue)
					if err != nil {
						return nil, err
					}
					return &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{StructVariantType: svType, Value: pv}}}, nil
				}
			}
		}
	}
	return nil, errors.New("unsupported value format")
}

func ReadValueFromRequest(r *http.Request) (*protos.Value, error) {
	//Decode body into catena value type
	var v any

	if err := json.NewDecoder(r.Body).Decode(&v); err != nil {
		return nil, err
	}

	val, ok := v.(map[string]any)
	if !ok {
		return nil, errors.New("invalid request body format")
	}

	//Convert catena value type to protos.Value
	if int32Val, ok := val["int32_value"]; ok {
		if f, ok := int32Val.(float64); ok {
			return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: int32(f)}}, nil
		}
	} else if int32ArrVal, ok := val["int32_array_value"]; ok {
		if arr, ok := int32ArrVal.([]any); ok {
			intArr := make([]int32, len(arr))
			for i, num := range arr {
				if f, ok := num.(float64); ok {
					intArr[i] = int32(f)
				} else {
					return nil, errors.New("invalid int32 array value in request body")
				}
			}
			return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: intArr}}}, nil
		}
	} else if floatVal, ok := val["float32_value"]; ok {
		if f, ok := floatVal.(float64); ok {
			return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: float32(f)}}, nil
		}
	} else if float32ArrVal, ok := val["float32_array_value"]; ok {
		if arr, ok := float32ArrVal.([]any); ok {
			floatArr := make([]float32, len(arr))
			for i, num := range arr {
				if f, ok := num.(float64); ok {
					floatArr[i] = float32(f)
				} else {
					return nil, errors.New("invalid float32 array value in request body")
				}
			}
			return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: floatArr}}}, nil
		}
	} else if stringVal, ok := val["string_value"]; ok {
		if s, ok := stringVal.(string); ok {
			return &protos.Value{Kind: &protos.Value_StringValue{StringValue: s}}, nil
		}
	} else if structVal, ok := val["struct_value"]; ok {
		if m, ok := structVal.(map[string]any); ok {
			fields := make(map[string]*protos.Value)
			for k, v := range m {
				pv, err := jsonToValue(v)
				if err != nil {
					return nil, err
				}
				fields[k] = pv
			}
			return &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{Fields: fields}}}, nil
		}
	} else if structArrVal, ok := val["struct_array_value"]; ok {
		if arr, ok := structArrVal.([]any); ok {
			structArr := make([]*protos.StructValue, len(arr))
			for i, item := range arr {
				if m, ok := item.(map[string]any); ok {
					fields := make(map[string]*protos.Value)
					for k, v := range m {
						pv, err := jsonToValue(v)
						if err != nil {
							return nil, err
						}
						fields[k] = pv
					}
					structArr[i] = &protos.StructValue{Fields: fields}
				} else {
					return nil, errors.New("invalid struct array value in request body")
				}
			}
			return &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{StructValues: structArr}}}, nil
		}
	} else if structVariantVal, ok := val["struct_variant_value"]; ok {
		if m, ok := structVariantVal.(map[string]any); ok {
			svType, okType := m["struct_variant_type"].(string)
			svValue, okValue := m["value"]
			if okType && okValue {
				pv, err := jsonToValue(svValue)
				if err != nil {
					return nil, err
				}
				return &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{StructVariantType: svType, Value: pv}}}, nil
			}
		}
	} else if structVariantArrVal, ok := val["struct_variant_array_value"]; ok {
		if arr, ok := structVariantArrVal.([]any); ok {
			var structVariantArr []*protos.StructVariantValue
			for _, item := range arr {
				if m, ok := item.(map[string]any); ok {
					svType, okType := m["struct_variant_type"].(string)
					svValue, okValue := m["value"]
					if okType && okValue {
						pv, err := jsonToValue(svValue)
						if err != nil {
							return nil, err
						}
						structVariantArr = append(structVariantArr, &protos.StructVariantValue{StructVariantType: svType, Value: pv})
					} else {
						return nil, errors.New("invalid struct variant array value in request body")
					}
				} else {
					return nil, errors.New("invalid struct variant array value in request body")
				}
			}
			return &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{StructVariants: structVariantArr}}}, nil
		}
	}

	//type not supported, set error
	err := errors.New("unsupported type in request body")
	return nil, err
}

func WriteResponseJSON(w http.ResponseWriter, res catena.StatusResult) {
	// Encode the protobuf Value into the REST JSON shape based on its Kind.
	// This uses a type switch over the oneof behind Value.Kind.

	// Helper to recursively convert protos.Value into a JSON-friendly structure.
	var valueToJSON func(v *protos.Value) any
	valueToJSON = func(v *protos.Value) any {
		if v == nil {
			return nil
		}
		switch k := v.GetKind().(type) {
		case *protos.Value_UndefinedValue:
			return map[string]any{"undefined_value": k.UndefinedValue.String()}
		case *protos.Value_EmptyValue:
			return map[string]any{"empty_value": struct{}{}}
		case *protos.Value_Int32Value:
			return map[string]int32{"int32_value": k.Int32Value}
		case *protos.Value_Float32Value:
			return map[string]float32{"float32_value": k.Float32Value}
		case *protos.Value_StringValue:
			return map[string]string{"string_value": k.StringValue}
		case *protos.Value_StructValue:
			fields := k.StructValue.GetFields()
			obj := make(map[string]any, len(fields))
			for name, val := range fields {
				obj[name] = valueToJSON(val)
			}
			return map[string]any{"struct_value": obj}
		case *protos.Value_Int32ArrayValues:
			return map[string]any{"int32_array_value": k.Int32ArrayValues.GetInts()}
		case *protos.Value_Float32ArrayValues:
			return map[string]any{"float32_array_value": k.Float32ArrayValues.GetFloats()}
		case *protos.Value_StringArrayValues:
			return map[string]any{"string_array_value": k.StringArrayValues.GetStrings()}
		case *protos.Value_StructArrayValues:
			list := k.StructArrayValues.GetStructValues()
			arr := make([]any, 0, len(list))
			for _, sv := range list {
				inner := make(map[string]any, len(sv.GetFields()))
				for n, vv := range sv.GetFields() {
					inner[n] = valueToJSON(vv)
				}
				arr = append(arr, inner)
			}
			return map[string]any{"struct_array_value": arr}
		case *protos.Value_StructVariantValue:
			sv := k.StructVariantValue
			return map[string]any{
				"struct_variant_value": map[string]any{
					"struct_variant_type": sv.GetStructVariantType(),
					"value":               valueToJSON(sv.GetValue()),
				},
			}
		case *protos.Value_StructVariantArrayValues:
			list := k.StructVariantArrayValues.GetStructVariants()
			arr := make([]any, 0, len(list))
			for _, sv := range list {
				arr = append(arr, map[string]any{
					"struct_variant_type": sv.GetStructVariantType(),
					"value":               valueToJSON(sv.GetValue()),
				})
			}
			return map[string]any{"struct_variant_array_value": arr}
		case *protos.Value_DataPayload:
			dp := k.DataPayload
			if dp.GetUrl() != "" {
				return map[string]any{"data_payload": map[string]any{
					"url":      dp.GetUrl(),
					"metadata": dp.GetMetadata(),
				}}
			}
			return map[string]any{"data_payload": map[string]any{
				"payload":  dp.GetPayload(),
				"metadata": dp.GetMetadata(),
			}}
		default:
			return nil
		}
	}

	body := valueToJSON(res.Payload)

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(res.Status)
	if body == nil {
		return
	}
	_ = json.NewEncoder(w).Encode(body)
}
