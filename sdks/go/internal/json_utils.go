package internal

import (
	"encoding/json"
	"errors"
	"net/http"
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
	}
	
	//type not supported, set error
	err := errors.New("unsupported type in request body")
	return nil, err
}

func WriteResponseJSON(w http.ResponseWriter, v any) {
    //Check the type of v and convert to catena type
    switch val := v.(type) {
	case int:
		v = map[string]int32{"int32_value": int32(val)}
	case []int:
		int32Arr := make([]int32, len(val))
		for i, num := range val {
			int32Arr[i] = int32(num)
		}
		v = map[string][]int32{"int32_array_value": int32Arr}
	case float32:
        v = map[string]float32{"float32_value": val}
	case []float32:
		v = map[string][]float32{"float32_array_value": val}
    case string:
        v = map[string]string{"string_value": val}
    default:
        //type not supported error
		http.Error(w, "unsupported type for JSON response", http.StatusInternalServerError)
        return
    }
    w.Header().Set("Content-Type", "application/json")
    _ = json.NewEncoder(w).Encode(v)
}
