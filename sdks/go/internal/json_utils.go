package internal

import (
	"encoding/json"
	"errors"
	"net/http"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func ReadValueFromRequest(r *http.Request) (any, error) {
	//Decode body into catena value type
	var v any

	if err := json.NewDecoder(r.Body).Decode(&v); err != nil {
		return nil, err
	}

	val, ok := v.(map[string]any)
	if !ok {
		return nil, errors.New("invalid request body format")
	}

	//Convert catena value type to go native type
	if int32Val, ok := val["int32_value"]; ok {
		if f, ok := int32Val.(float64); ok {
			return int(f), nil
		}
	} else if int32ArrVal, ok := val["int32_array_value"]; ok {
		if arr, ok := int32ArrVal.([]any); ok {
			intArr := make([]int, len(arr))
			for i, num := range arr {
				if f, ok := num.(float64); ok {
					intArr[i] = int(f)
				} else {
					return nil, errors.New("invalid int32 array value in request body")
				}
			}
			return intArr, nil
		}
	} else if floatVal, ok := val["float32_value"]; ok {
		if f, ok := floatVal.(float64); ok {
			return float32(f), nil
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
			return floatArr, nil
		}
	} else if stringVal, ok := val["string_value"]; ok {
		if s, ok := stringVal.(string); ok {
			return s, nil
		}
	} else if structVal, ok := val["struct_value"]; ok {
		if m, ok := structVal.(map[string]any); ok {
			return m, nil
		}
	} else if structArrVal, ok := val["struct_array_value"]; ok {
		if arr, ok := structArrVal.([]any); ok {
			structArr := make([]map[string]any, len(arr))
			for i, item := range arr {
				if m, ok := item.(map[string]any); ok {
					structArr[i] = m
				} else {
					return nil, errors.New("invalid struct array value in request body")
				}
			}
			return structArr, nil
		}
	} else if structVariantVal, ok := val["struct_variant_value"]; ok {
		if m, ok := structVariantVal.(map[string]any); ok {
			svType, okType := m["struct_variant_type"].(string)
			svValue, okValue := m["value"]
			if okType && okValue {
				return catena.StructVariantValue{
					StructVariantType: svType,
					Value:             svValue,
				}, nil
			}
		}
	} else if structVariantArrVal, ok := val["struct_variant_array_value"]; ok {
		if arr, ok := structVariantArrVal.([]any); ok {
			var structVariantArr []catena.StructVariantValue
			for _, item := range arr {
				if m, ok := item.(map[string]any); ok {
					svType, okType := m["struct_variant_type"].(string)
					svValue, okValue := m["value"]
					if okType && okValue {
						structVariantArr = append(structVariantArr, catena.StructVariantValue{
							StructVariantType: svType,
							Value:             svValue,
						})
					} else {
						return nil, errors.New("invalid struct variant array value in request body")
					}
				} else {
					return nil, errors.New("invalid struct variant array value in request body")
				}
			}
			return structVariantArr, nil
		}
	}

	//type not supported, set error
	err := errors.New("unsupported type in request body")
	return nil, err
}

func WriteResponseJSON(w http.ResponseWriter, res catena.StatusResult) {
	//Check the type of v and convert to catena type
	switch val := res.Payload.(type) {
	case int:
		res.Payload = map[string]int32{"int32_value": int32(val)}
	case []int:
		int32Arr := make([]int32, len(val))
		for i, num := range val {
			int32Arr[i] = int32(num)
		}
		res.Payload = map[string][]int32{"int32_array_value": int32Arr}
	case float32:
		res.Payload = map[string]float32{"float32_value": val}
	case []float32:
		res.Payload = map[string][]float32{"float32_array_value": val}
	case string:
		res.Payload = map[string]string{"string_value": val}
	case map[string]any:
		res.Payload = map[string]map[string]any{"struct_value": val}
	case []map[string]any:
		res.Payload = map[string][]map[string]any{"struct_array_value": val}
	case catena.StructVariantValue:
		res.Payload = map[string]map[string]any{"struct_variant_value": {
			"struct_variant_type": val.StructVariantType,
			"value":              val.Value,
		}}
	case []catena.StructVariantValue:
		var structVariantArr []map[string]any
		for _, item := range val {
			structVariantArr = append(structVariantArr, map[string]any{
				"struct_variant_type": item.StructVariantType,
				"value":              item.Value,
			})
		}
		res.Payload = map[string][]map[string]any{"struct_variant_array_value": structVariantArr}
	default:
		http.Error(w, "failed to encode JSON response: ", http.StatusInternalServerError)
	}
	//Encode response as JSON
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(res.Status)
	_ = json.NewEncoder(w).Encode(res.Payload)
}
