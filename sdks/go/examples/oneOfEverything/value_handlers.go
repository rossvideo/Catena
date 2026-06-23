package main

import (
	"reflect"
	"strconv"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerValueHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	// Slot 0: manual switch over typed state. This is the most explicit style
	// and works well when a device has a small number of params. This handler
	// also shows a per-endpoint auth restriction: callers must have read access
	// to the configuration scope before slot 0 values are returned.
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx catena.HandlerContext) (catena.Value, catena.StatusResult) {
		logger.Info("GetValue", "slot", slot, "fqoid", fqoid)
		if !ctx.HasReadScope(catena.ScopeCfg) {
			return catena.ReplyError[catena.Value](catena.StatusCodePermissionDenied, "configuration scope required")
		}

		var v any
		var ok bool
		switch fqoid {
		case "product", "product/name", "product/vendor", "product/version":
			v, ok = state.slotZeroProductValue(fqoid)
		case "counter":
			v, ok = counter.GetValue(), true
		case "running":
			v, ok = counter.RunningInt32(), true
		}

		return replyValue(fqoid, v, ok)
	})

	// Slot 1: direct sync.Map lookup. This demonstrates map-backed state where
	// parameter OIDs are the storage keys. Slot 1 uses the monitor scope for access
	srv.RegisterGetValueHandler(1, func(slot uint16, fqoid string, ctx catena.HandlerContext) (catena.Value, catena.StatusResult) {
		logger.Info("GetValue", "slot", slot, "fqoid", fqoid)
		if !ctx.HasReadScope(catena.ScopeMon) {
			return catena.ReplyError[catena.Value](catena.StatusCodePermissionDenied, "monitor scope required")
		}

		v, ok := state.slotOneParams.Load(fqoid)
		return replyValue(fqoid, v, ok)
	})

	// Slot 2: direct state access. This demonstrates a more traditional
	// approach where each handler manages its own set of parameters.
	// This handler requires operation-scope read access.
	srv.RegisterGetValueHandler(2, func(slot uint16, fqoid string, ctx catena.HandlerContext) (catena.Value, catena.StatusResult) {
		logger.Info("GetValue", "slot", slot, "fqoid", fqoid)
		if !ctx.HasReadScope(catena.ScopeOp) {
			return catena.ReplyError[catena.Value](catena.StatusCodePermissionDenied, "operation scope required")
		}

		state.mu.RLock()
		defer state.mu.RUnlock()

		var v any
		var ok bool
		if strings.HasPrefix(fqoid, "product") {
			v, ok = state.slotTwoProductValue(fqoid)
		} else if fqoid == "volume" {
			v, ok = state.volume, true
		} else if fqoid == "muted" {
			v, ok = state.muted, true
		} else if fqoid == "device_name" {
			v, ok = state.deviceName, true
		} else if strings.HasPrefix(fqoid, "struct_example") {
			switch fqoid {
			case "struct_example":
				v, ok = state.structExample, true
			case "struct_example/number":
				v, ok = state.structExample["number"]
			case "struct_example/text":
				v, ok = state.structExample["text"]
			}
		} else if strings.HasPrefix(fqoid, "sample_struct_variant_array") {
			v, ok = slotTwoSampleStructVariantArrayValue(fqoid, state.sampleStructVariantArray)
		} else if strings.HasPrefix(fqoid, "sample_struct_variant") {
			v, ok = slotTwoSampleStructVariantValue(fqoid, state.sampleStructVariant)
		} else if strings.HasPrefix(fqoid, "sample_struct_array") {
			v, ok = slotTwoSampleStructArrayValue(fqoid, state.sampleStructArray)
		} else if strings.HasPrefix(fqoid, "sample_int_array") {
			v, ok = slotTwoIndexedArrayValue(fqoid, "sample_int_array", state.sampleIntArray)
		} else if strings.HasPrefix(fqoid, "sample_float_array") {
			v, ok = slotTwoIndexedArrayValue(fqoid, "sample_float_array", state.sampleFloatArray)
		} else if strings.HasPrefix(fqoid, "sample_string_array") {
			v, ok = slotTwoIndexedArrayValue(fqoid, "sample_string_array", state.sampleStringArray)
		} else if fqoid == "sample_binary" {
			v, ok = catena.DataPayload{Payload: state.sampleBinary}, true
		} else if fqoid == "sample_float" {
			v, ok = state.sampleFloat, true
		}

		return replyValue(fqoid, v, ok)
	})

	// Slot 0: writes use the same manual switch strategy as reads. A single
	// SetValueHandler covers both single and multi set requests; validate every
	// entry before applying any so the batch is all-or-nothing.
	srv.RegisterSetValueHandler(0, func(slot uint16, entries []catena.SetValueEntry, ctx catena.HandlerContext) catena.StatusResult {
		logger.Info("SetValue", "slot", slot, "count", len(entries))

		for _, entry := range entries {
			if entry.Value == nil {
				logger.Error("SetValue nil value", "slot", slot, "fqoid", entry.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "nil value for "+entry.Fqoid)
			}
			if strings.HasPrefix(entry.Fqoid, "product") {
				return catena.StatusWithCode(catena.StatusCodePermissionDenied, "product params are read-only")
			}

			switch entry.Fqoid {
			case "running":
				logger.Warning("SetValue rejected for running param", "slot", slot, "fqoid", entry.Fqoid)
				return catena.StatusWithCode(catena.StatusCodePermissionDenied, "Running state is read-only")
			case "counter":
				if _, ok := entry.Value.(int32); !ok {
					logger.Error("SetValue type mismatch", "slot", slot, "fqoid", entry.Fqoid, "expected", "int32", "got", entry.Value)
					return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
				}
			default:
				logger.Error("SetValue param not found", "slot", slot, "fqoid", entry.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+entry.Fqoid)
			}
		}

		for _, entry := range entries {
			if entry.Fqoid == "counter" {
				counter.SetValue(entry.Value.(int32))
			}
			logger.Info("Parameter updated", "fqoid", entry.Fqoid, "value", entry.Value)
			srv.BroadcastUpdate(slot, entry.Fqoid, entry.Value, catena.ScopeCfg)
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	})

	// Slot 1: writes update the sync.Map directly after validating the stored
	// type. This slot only requires monitor-scope write access, demonstrating
	// that each handler can enforce a different authorization policy.
	srv.RegisterSetValueHandler(1, func(slot uint16, entries []catena.SetValueEntry, ctx catena.HandlerContext) catena.StatusResult {
		logger.Info("SetValue", "slot", slot, "count", len(entries))
		if !ctx.HasWriteScope(catena.ScopeMon) {
			return catena.StatusWithCode(catena.StatusCodePermissionDenied, "monitor scope required")
		}

		for _, entry := range entries {
			if entry.Value == nil {
				logger.Error("SetValue nil value", "slot", slot, "fqoid", entry.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "nil value for "+entry.Fqoid)
			}

			switch entry.Fqoid {
			case "resolution", "brightness", "contrast", "saturation":
				current, ok := state.slotOneParams.Load(entry.Fqoid)
				if !ok {
					logger.Error("SetValue param not found", "slot", slot, "fqoid", entry.Fqoid)
					return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+entry.Fqoid)
				}
				if reflect.TypeOf(current) != reflect.TypeOf(entry.Value) {
					logger.Error("SetValue type mismatch", "slot", slot, "fqoid", entry.Fqoid,
						"expected", reflect.TypeOf(current), "got", reflect.TypeOf(entry.Value))
					return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch for "+entry.Fqoid)
				}
			default:
				logger.Error("SetValue param not found", "slot", slot, "fqoid", entry.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+entry.Fqoid)
			}
		}

		for _, entry := range entries {
			state.slotOneParams.Store(entry.Fqoid, entry.Value)
			logger.Info("Parameter updated", "fqoid", entry.Fqoid, "value", entry.Value)
			srv.BroadcastUpdate(slot, entry.Fqoid, entry.Value, catena.ScopeMon)
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	})

	// Slot 2: writes use prefix-based dispatch and keep product params read-only.
	// Hold state.mu for the whole handler so validate, apply, and broadcast see
	// the same snapshot; validate every entry before applying any (all-or-nothing).
	srv.RegisterSetValueHandler(2, func(slot uint16, entries []catena.SetValueEntry, ctx catena.HandlerContext) catena.StatusResult {
		logger.Info("SetValue", "slot", slot, "count", len(entries))
		if !ctx.HasWriteScope(catena.ScopeOp) {
			return catena.StatusWithCode(catena.StatusCodePermissionDenied, "operation scope required")
		}

		state.mu.Lock()
		defer state.mu.Unlock()

		for _, entry := range entries {
			if status := slotTwoValidateSet(entry.Fqoid, entry.Value, state); status.Code != catena.StatusCodeOk {
				return status
			}
		}

		for _, entry := range entries {
			slotTwoApplySet(entry.Fqoid, entry.Value, state)
		}
		for _, entry := range entries {
			logger.Info("Parameter updated", "fqoid", entry.Fqoid, "value", entry.Value)
			srv.BroadcastUpdate(slot, entry.Fqoid, entry.Value, catena.ScopeMon)
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	})
}

func replyValue(fqoid string, v any, ok bool) (catena.Value, catena.StatusResult) {
	if !ok {
		return catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "parameter not found: "+fqoid)
	}
	catenaVal, res := catena.ToValue(v)
	if res.Code != catena.StatusCodeOk {
		return catena.ReplyError[catena.Value](catena.StatusCodeInternal, "failed to convert value")
	}
	return catena.Reply(catenaVal)
}

func parseArrayElementPath(fqoid, arrayOid string) (idx int, field string, wholeArray bool, ok bool) {
	if fqoid == arrayOid {
		return 0, "", true, true
	}
	prefix := arrayOid + "/"
	if !strings.HasPrefix(fqoid, prefix) {
		return 0, "", false, false
	}
	rest := strings.TrimPrefix(fqoid, prefix)
	if rest == "" {
		return 0, "", false, false
	}
	parts := strings.SplitN(rest, "/", 2)
	i, err := strconv.Atoi(parts[0])
	if err != nil || i < 0 {
		return 0, "", false, false
	}
	if len(parts) == 2 {
		field = parts[1]
	}
	return i, field, false, true
}

func slotTwoSampleStructArrayValue(fqoid string, arr []map[string]any) (any, bool) {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, "sample_struct_array")
	if !ok {
		return nil, false
	}
	if wholeArray {
		return arr, true
	}
	if idx >= len(arr) {
		return nil, false
	}
	switch field {
	case "":
		return arr[idx], true
	case "label", "count":
		v, ok := arr[idx][field]
		return v, ok
	default:
		return nil, false
	}
}

func slotTwoValidateSet(fqoid string, value any, state *ExampleState) catena.StatusResult {
	if value == nil {
		logger.Error("SetValue nil value", "slot", uint16(2), "fqoid", fqoid)
		return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "nil value for "+fqoid)
	}
	if strings.HasPrefix(fqoid, "product") {
		return catena.StatusWithCode(catena.StatusCodePermissionDenied, "product params are read-only")
	}

	switch {
	case fqoid == "volume", fqoid == "muted":
		if _, ok := value.(int32); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case fqoid == "device_name":
		if _, ok := value.(string); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case strings.HasPrefix(fqoid, "struct_example"):
		switch fqoid {
		case "struct_example":
			if _, ok := value.(map[string]any); !ok {
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
			}
		case "struct_example/number":
			if _, ok := value.(int32); !ok {
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
			}
		case "struct_example/text":
			if _, ok := value.(string); !ok {
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
			}
		default:
			return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
		}
	case strings.HasPrefix(fqoid, "sample_struct_variant_array"):
		return slotTwoValidateSampleStructVariantArraySet(fqoid, value, state.sampleStructVariantArray)
	case strings.HasPrefix(fqoid, "sample_struct_variant"):
		return slotTwoValidateSampleStructVariantSet(fqoid, value)
	case strings.HasPrefix(fqoid, "sample_struct_array"):
		return slotTwoValidateSampleStructArraySet(fqoid, value, state.sampleStructArray)
	case strings.HasPrefix(fqoid, "sample_int_array"):
		return slotTwoValidateIndexedArraySet(fqoid, "sample_int_array", value, state.sampleIntArray)
	case strings.HasPrefix(fqoid, "sample_float_array"):
		return slotTwoValidateIndexedArraySet(fqoid, "sample_float_array", value, state.sampleFloatArray)
	case strings.HasPrefix(fqoid, "sample_string_array"):
		return slotTwoValidateIndexedArraySet(fqoid, "sample_string_array", value, state.sampleStringArray)
	case fqoid == "sample_binary":
		if _, ok := value.(catena.DataPayload); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case fqoid == "sample_float":
		if _, ok := value.(float32); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoApplySet(fqoid string, value any, state *ExampleState) catena.StatusResult {
	switch {
	case fqoid == "volume":
		state.volume = value.(int32)
	case fqoid == "muted":
		state.muted = value.(int32)
	case fqoid == "device_name":
		state.deviceName = value.(string)
	case strings.HasPrefix(fqoid, "struct_example"):
		switch fqoid {
		case "struct_example":
			state.structExample = value.(map[string]any)
		case "struct_example/number":
			state.structExample["number"] = value.(int32)
		case "struct_example/text":
			state.structExample["text"] = value.(string)
		default:
			return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
		}
	case strings.HasPrefix(fqoid, "sample_struct_variant_array"):
		return slotTwoApplySampleStructVariantArraySet(fqoid, value, &state.sampleStructVariantArray)
	case strings.HasPrefix(fqoid, "sample_struct_variant"):
		return slotTwoApplySampleStructVariantSet(fqoid, value, &state.sampleStructVariant)
	case strings.HasPrefix(fqoid, "sample_struct_array"):
		return slotTwoApplySampleStructArraySet(fqoid, value, &state.sampleStructArray)
	case strings.HasPrefix(fqoid, "sample_int_array"):
		return slotTwoApplyIndexedArraySet(fqoid, "sample_int_array", value, &state.sampleIntArray)
	case strings.HasPrefix(fqoid, "sample_float_array"):
		return slotTwoApplyIndexedArraySet(fqoid, "sample_float_array", value, &state.sampleFloatArray)
	case strings.HasPrefix(fqoid, "sample_string_array"):
		return slotTwoApplyIndexedArraySet(fqoid, "sample_string_array", value, &state.sampleStringArray)
	case fqoid == "sample_binary":
		state.sampleBinary = value.(catena.DataPayload).Payload
	case fqoid == "sample_float":
		state.sampleFloat = value.(float32)
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoValidateSampleStructArraySet(fqoid string, value any, arr []map[string]any) catena.StatusResult {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, "sample_struct_array")
	if !ok {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if wholeArray {
		if _, typed := value.([]map[string]any); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if idx >= len(arr) {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	switch field {
	case "":
		if _, typed := value.(map[string]any); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case "label":
		if _, typed := value.(string); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case "count":
		if _, typed := value.(int32); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoApplySampleStructArraySet(fqoid string, value any, arr *[]map[string]any) catena.StatusResult {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, "sample_struct_array")
	if !ok {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if wholeArray {
		v, typed := value.([]map[string]any)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		*arr = v
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if idx >= len(*arr) {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	switch field {
	case "":
		v, typed := value.(map[string]any)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		(*arr)[idx] = v
	case "label":
		v, typed := value.(string)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		(*arr)[idx]["label"] = v
	case "count":
		v, typed := value.(int32)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		(*arr)[idx]["count"] = v
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoIndexedArrayValue[T any](fqoid, arrayOid string, arr []T) (any, bool) {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, arrayOid)
	if !ok {
		return nil, false
	}
	if wholeArray {
		return arr, true
	}
	if field != "" || idx >= len(arr) {
		return nil, false
	}
	return arr[idx], true
}

func slotTwoValidateIndexedArraySet[T any](fqoid, arrayOid string, value any, arr []T) catena.StatusResult {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, arrayOid)
	if !ok {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if wholeArray {
		if _, typed := value.([]T); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if field != "" {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if idx >= len(arr) {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if _, typed := value.(T); !typed {
		return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoApplyIndexedArraySet[T any](fqoid, arrayOid string, value any, arr *[]T) catena.StatusResult {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, arrayOid)
	if !ok {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if wholeArray {
		v, typed := value.([]T)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		*arr = v
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if field != "" {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if idx >= len(*arr) {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	v, typed := value.(T)
	if !typed {
		return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
	}
	(*arr)[idx] = v
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoSampleStructVariantValue(fqoid string, sv catena.StructVariantValue) (any, bool) {
	switch fqoid {
	case "sample_struct_variant":
		return sv, true
	case "sample_struct_variant/int_kind":
		if sv.StructVariantType != "int_kind" {
			return nil, false
		}
		return sv.Value, true
	case "sample_struct_variant/string_kind":
		if sv.StructVariantType != "string_kind" {
			return nil, false
		}
		return sv.Value, true
	default:
		return nil, false
	}
}

func slotTwoValidateSampleStructVariantSet(fqoid string, value any) catena.StatusResult {
	switch fqoid {
	case "sample_struct_variant":
		if _, ok := value.(catena.StructVariantValue); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case "sample_struct_variant/int_kind":
		if _, ok := value.(int32); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case "sample_struct_variant/string_kind":
		if _, ok := value.(string); !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoApplySampleStructVariantSet(fqoid string, value any, sv *catena.StructVariantValue) catena.StatusResult {
	switch fqoid {
	case "sample_struct_variant":
		v, ok := value.(catena.StructVariantValue)
		if !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		*sv = v
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	case "sample_struct_variant/int_kind":
		v, ok := value.(int32)
		if !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		*sv = catena.StructVariantValue{StructVariantType: "int_kind", Value: v}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	case "sample_struct_variant/string_kind":
		v, ok := value.(string)
		if !ok {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		*sv = catena.StructVariantValue{StructVariantType: "string_kind", Value: v}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
}

func slotTwoSampleStructVariantArrayValue(fqoid string, arr []catena.StructVariantValue) (any, bool) {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, "sample_struct_variant_array")
	if !ok {
		return nil, false
	}
	if wholeArray {
		return arr, true
	}
	if idx >= len(arr) {
		return nil, false
	}
	switch field {
	case "":
		return arr[idx], true
	case "int_kind":
		if arr[idx].StructVariantType != "int_kind" {
			return nil, false
		}
		return arr[idx].Value, true
	case "string_kind":
		if arr[idx].StructVariantType != "string_kind" {
			return nil, false
		}
		return arr[idx].Value, true
	default:
		return nil, false
	}
}

func slotTwoValidateSampleStructVariantArraySet(fqoid string, value any, arr []catena.StructVariantValue) catena.StatusResult {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, "sample_struct_variant_array")
	if !ok {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if wholeArray {
		if _, typed := value.([]catena.StructVariantValue); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if idx >= len(arr) {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	switch field {
	case "":
		if _, typed := value.(catena.StructVariantValue); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case "int_kind":
		if _, typed := value.(int32); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	case "string_kind":
		if _, typed := value.(string); !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}

func slotTwoApplySampleStructVariantArraySet(fqoid string, value any, arr *[]catena.StructVariantValue) catena.StatusResult {
	idx, field, wholeArray, ok := parseArrayElementPath(fqoid, "sample_struct_variant_array")
	if !ok {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	if wholeArray {
		v, typed := value.([]catena.StructVariantValue)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		*arr = v
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if idx >= len(*arr) {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	switch field {
	case "":
		v, typed := value.(catena.StructVariantValue)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		(*arr)[idx] = v
	case "int_kind":
		v, typed := value.(int32)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		(*arr)[idx] = catena.StructVariantValue{StructVariantType: "int_kind", Value: v}
	case "string_kind":
		v, typed := value.(string)
		if !typed {
			return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "type mismatch")
		}
		(*arr)[idx] = catena.StructVariantValue{StructVariantType: "string_kind", Value: v}
	default:
		return catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
	return catena.StatusWithCode(catena.StatusCodeOk, "")
}
