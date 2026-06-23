package main

import (
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerParamInfoHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	// Slot 0: fully manual ParamInfo construction with a switch. This is useful
	// when an application has only a few params or does not keep a full device
	// definition around for ParamInfo requests.
	srv.RegisterParamInfoHandler(0, func(slot uint16, fqoid string, recursive bool, ctx catena.HandlerContext) ([]catena.ParamInfo, catena.StatusResult) {
		logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid, "recursive", recursive)
		return slotZeroParamInfos(fqoid, recursive)
	})

	// Slot 1: generate the current device definition and delegate to the SDK
	// helper. This demonstrates the simplest path when your business logic can
	// already produce device["params"].
	srv.RegisterParamInfoHandler(1, func(slot uint16, fqoid string, recursive bool, ctx catena.HandlerContext) ([]catena.ParamInfo, catena.StatusResult) {
		logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid, "recursive", recursive)

		deviceInfo, ok := buildDeviceDefinition(slot, counter, state)
		if !ok {
			return []catena.ParamInfo{}, catena.StatusWithCode(catena.StatusCodeNotFound, "device not found")
		}

		return catena.ParamInfosForRequest(fqoid, deviceInfo, recursive)
	})

	// Slot 2: prefix-based if statements with manual responses. This is useful
	// when params are grouped by naming convention or owned by different pieces
	// of application code.
	srv.RegisterParamInfoHandler(2, func(slot uint16, fqoid string, recursive bool, ctx catena.HandlerContext) ([]catena.ParamInfo, catena.StatusResult) {
		logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid, "recursive", recursive)
		return slotTwoParamInfos(fqoid, recursive, state)
	})
}

func slotZeroParamInfos(fqoid string, recursive bool) ([]catena.ParamInfo, catena.StatusResult) {
	switch fqoid {
	case "":
		infos := slotZeroProductParamInfos(recursive)
		infos = append(infos, newCounterParamInfo(), newRunningParamInfo())
		return infos, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "product":
		return slotZeroProductParamInfos(recursive), catena.StatusWithCode(catena.StatusCodeOk, "")
	case "product/name":
		return []catena.ParamInfo{catena.NewParamInfo("product/name", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "product/vendor":
		return []catena.ParamInfo{catena.NewParamInfo("product/vendor", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "product/version":
		return []catena.ParamInfo{catena.NewParamInfo("product/version", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "counter":
		return []catena.ParamInfo{newCounterParamInfo()}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "running":
		return []catena.ParamInfo{newRunningParamInfo()}, catena.StatusWithCode(catena.StatusCodeOk, "")
	default:
		return []catena.ParamInfo{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
}

func slotTwoParamInfos(fqoid string, recursive bool, state *ExampleState) ([]catena.ParamInfo, catena.StatusResult) {
	if fqoid == "" {
		return []catena.ParamInfo{
			catena.NewParamInfo("product", nil, catena.ParamTypeStruct, "", 0),
			catena.NewParamInfo("volume", catena.NewPolyglotText("en", "Volume"), catena.ParamTypeInt32, "", 0),
			catena.NewParamInfo("muted", catena.NewPolyglotText("en", "Muted"), catena.ParamTypeInt32, "", 0),
			catena.NewParamInfo("device_name", catena.NewPolyglotText("en", "Device Name"), catena.ParamTypeString, "", 0),
			catena.NewParamInfo("struct_example", catena.NewPolyglotText("en", "Struct Example"), catena.ParamTypeStruct, "", 0),
			catena.NewParamInfo("sample_float", catena.NewPolyglotText("en", "Sample Float"), catena.ParamTypeFloat32, "", 0),
			newSampleIntArrayParamInfo(state),
			catena.NewParamInfo("sample_float_array", catena.NewPolyglotText("en", "Sample Float Array"), catena.ParamTypeFloat32Array, "", uint32(len(state.sampleFloatArrayValue()))),
			catena.NewParamInfo("sample_string_array", catena.NewPolyglotText("en", "Sample String Array"), catena.ParamTypeStringArray, "", uint32(len(state.sampleStringArrayValue()))),
			catena.NewParamInfo("sample_binary", catena.NewPolyglotText("en", "Sample Binary"), catena.ParamTypeBinary, "", 0),
			catena.NewParamInfo("sample_struct_variant", catena.NewPolyglotText("en", "Sample Struct Variant"), catena.ParamTypeStructVariant, "", 0),
			catena.NewParamInfo("sample_struct_array", catena.NewPolyglotText("en", "Sample Struct Array"), catena.ParamTypeStructArray, "", uint32(len(state.sampleStructArrayValue()))),
			catena.NewParamInfo("sample_struct_variant_array", catena.NewPolyglotText("en", "Sample Struct Variant Array"), catena.ParamTypeStructVariantArray, "", uint32(len(state.sampleStructVariantArrayValue()))),
		}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	if strings.HasPrefix(fqoid, "product") {
		infos := []catena.ParamInfo{
			catena.NewParamInfo("product", nil, catena.ParamTypeStruct, "", 0),
		}
		if recursive {
			infos = append(infos,
				catena.NewParamInfo("product/name", nil, catena.ParamTypeString, "", 0),
				catena.NewParamInfo("product/vendor", nil, catena.ParamTypeString, "", 0),
				catena.NewParamInfo("product/version", nil, catena.ParamTypeString, "", 0),
				catena.NewParamInfo("product/catena_sdk", nil, catena.ParamTypeString, "", 0),
				catena.NewParamInfo("product/catena_sdk_version", nil, catena.ParamTypeString, "", 0),
				catena.NewParamInfo("product/serial_number", nil, catena.ParamTypeString, "", 0),
			)
		}
		switch fqoid {
		case "product":
			return infos, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "product/name":
			return []catena.ParamInfo{catena.NewParamInfo("product/name", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "product/vendor":
			return []catena.ParamInfo{catena.NewParamInfo("product/vendor", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "product/version":
			return []catena.ParamInfo{catena.NewParamInfo("product/version", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "product/catena_sdk":
			return []catena.ParamInfo{catena.NewParamInfo("product/catena_sdk", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "product/catena_sdk_version":
			return []catena.ParamInfo{catena.NewParamInfo("product/catena_sdk_version", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "product/serial_number":
			return []catena.ParamInfo{catena.NewParamInfo("product/serial_number", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}
	}

	if strings.HasPrefix(fqoid, "struct_example") {
		infos := []catena.ParamInfo{
			catena.NewParamInfo("struct_example", catena.NewPolyglotText("en", "Struct Example"), catena.ParamTypeStruct, "", 0),
		}
		if recursive {
			infos = append(infos,
				catena.NewParamInfo("struct_example/number", nil, catena.ParamTypeInt32, "", 0),
				catena.NewParamInfo("struct_example/text", nil, catena.ParamTypeString, "", 0),
			)
		}
		switch fqoid {
		case "struct_example":
			return infos, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "struct_example/number":
			return []catena.ParamInfo{catena.NewParamInfo("struct_example/number", nil, catena.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "struct_example/text":
			return []catena.ParamInfo{catena.NewParamInfo("struct_example/text", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}
	}

	if strings.HasPrefix(fqoid, "device_name") {
		return []catena.ParamInfo{catena.NewParamInfo("device_name", catena.NewPolyglotText("en", "Device Name"), catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "volume") {
		return []catena.ParamInfo{catena.NewParamInfo("volume", catena.NewPolyglotText("en", "Volume"), catena.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "muted") {
		return []catena.ParamInfo{catena.NewParamInfo("muted", catena.NewPolyglotText("en", "Muted"), catena.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	if strings.HasPrefix(fqoid, "sample_struct_variant_array") {
		return []catena.ParamInfo{catena.NewParamInfo("sample_struct_variant_array", catena.NewPolyglotText("en", "Sample Struct Variant Array"), catena.ParamTypeStructVariantArray, "", uint32(len(state.sampleStructVariantArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_struct_variant") {
		infos := []catena.ParamInfo{
			catena.NewParamInfo("sample_struct_variant", catena.NewPolyglotText("en", "Sample Struct Variant"), catena.ParamTypeStructVariant, "", 0),
		}
		if recursive {
			infos = append(infos,
				catena.NewParamInfo("sample_struct_variant/int_kind", nil, catena.ParamTypeInt32, "", 0),
				catena.NewParamInfo("sample_struct_variant/string_kind", nil, catena.ParamTypeString, "", 0),
			)
		}
		switch fqoid {
		case "sample_struct_variant":
			return infos, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "sample_struct_variant/int_kind":
			return []catena.ParamInfo{catena.NewParamInfo("sample_struct_variant/int_kind", nil, catena.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "sample_struct_variant/string_kind":
			return []catena.ParamInfo{catena.NewParamInfo("sample_struct_variant/string_kind", nil, catena.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}
	}
	if strings.HasPrefix(fqoid, "sample_struct_array") {
		return []catena.ParamInfo{catena.NewParamInfo("sample_struct_array", catena.NewPolyglotText("en", "Sample Struct Array"), catena.ParamTypeStructArray, "", uint32(len(state.sampleStructArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_int_array") {
		return []catena.ParamInfo{newSampleIntArrayParamInfo(state)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_float_array") {
		return []catena.ParamInfo{catena.NewParamInfo("sample_float_array", catena.NewPolyglotText("en", "Sample Float Array"), catena.ParamTypeFloat32Array, "", uint32(len(state.sampleFloatArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_string_array") {
		return []catena.ParamInfo{catena.NewParamInfo("sample_string_array", catena.NewPolyglotText("en", "Sample String Array"), catena.ParamTypeStringArray, "", uint32(len(state.sampleStringArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_binary") {
		return []catena.ParamInfo{catena.NewParamInfo("sample_binary", catena.NewPolyglotText("en", "Sample Binary"), catena.ParamTypeBinary, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_float") {
		return []catena.ParamInfo{catena.NewParamInfo("sample_float", catena.NewPolyglotText("en", "Sample Float"), catena.ParamTypeFloat32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	return []catena.ParamInfo{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
}

func newCounterParamInfo() catena.ParamInfo {
	return catena.NewParamInfo("counter", catena.NewPolyglotText("en", "Counter"), catena.ParamTypeInt32, "", 0)
}

func newSlotZeroProductParamInfo() catena.ParamInfo {
	return catena.NewParamInfo("product", nil, catena.ParamTypeStruct, "", 0)
}

func slotZeroProductParamInfos(recursive bool) []catena.ParamInfo {
	infos := []catena.ParamInfo{newSlotZeroProductParamInfo()}
	if recursive {
		infos = append(infos,
			catena.NewParamInfo("product/name", nil, catena.ParamTypeString, "", 0),
			catena.NewParamInfo("product/vendor", nil, catena.ParamTypeString, "", 0),
			catena.NewParamInfo("product/version", nil, catena.ParamTypeString, "", 0),
		)
	}
	return infos
}

func newRunningParamInfo() catena.ParamInfo {
	return catena.NewParamInfo("running", catena.NewPolyglotText("en", "Counter Running Status"), catena.ParamTypeInt32, "", 0)
}

func newSampleIntArrayParamInfo(state *ExampleState) catena.ParamInfo {
	return catena.NewParamInfo("sample_int_array", catena.NewPolyglotText("en", "Sample Int Array"), catena.ParamTypeInt32Array, "", uint32(len(state.sampleIntArrayValue())))
}
