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
	//Convert catena value type to go native type
	switch val := v.(type) {
	case map[string]interface{}:
		if int32Val, ok := val["int32_value"]; ok {
			if f, ok := int32Val.(float64); ok {
				return int(f), nil
			}
		} else if floatVal, ok := val["float_value"]; ok {
			if f, ok := floatVal.(float64); ok {
				return float32(f), nil
			}
		} else if stringVal, ok := val["string_value"]; ok {
			if s, ok := stringVal.(string); ok {
				return s, nil
			}
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
    case float32:
        v = map[string]float32{"float_value": val}
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
