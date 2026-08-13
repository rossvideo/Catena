package main

import (
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func registerParamInfoHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	// Slot 0: fully manual ParamInfo construction with a switch. This is useful
	// when an application has only a few params or does not keep a full device
	// definition around for ParamInfo requests.
	srv.RegisterParamInfoHandler(0, func(slot uint16, fqoid string, recursive bool, ctx catena.HandlerContext, stream catena.Stream[st2138.ParamInfo]) catena.StatusResult {
		logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid, "recursive", recursive)
		infos, res := slotZeroParamInfos(fqoid, recursive)
		return streamParamInfos(stream, infos, res)
	})

	// Slot 1: generate the current device definition and delegate to the SDK
	// helper. This demonstrates the simplest path when your business logic can
	// already produce device["params"].
	srv.RegisterParamInfoHandler(1, func(slot uint16, fqoid string, recursive bool, ctx catena.HandlerContext, stream catena.Stream[st2138.ParamInfo]) catena.StatusResult {
		logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid, "recursive", recursive)

		deviceInfo, ok := buildDeviceDefinition(slot, counter, state)
		if !ok {
			return catena.StatusWithCode(catena.StatusCodeNotFound, "device not found")
		}

		return catena.ParamInfosForRequest(fqoid, deviceInfo, recursive, stream)
	})

	// Slot 2: prefix-based if statements with manual responses. This is useful
	// when params are grouped by naming convention or owned by different pieces
	// of application code.
	srv.RegisterParamInfoHandler(2, func(slot uint16, fqoid string, recursive bool, ctx catena.HandlerContext, stream catena.Stream[st2138.ParamInfo]) catena.StatusResult {
		logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid, "recursive", recursive)
		infos, res := slotTwoParamInfos(fqoid, recursive, state)
		return streamParamInfos(stream, infos, res)
	})
}

// streamParamInfos emits each ParamInfo from a slice-producing helper through
// the stream, bridging the existing slice-based builders to the streaming
// handler signature. If res is an error nothing is sent; a Send failure stops
// the stream and is reported as an internal error.
func streamParamInfos(stream catena.Stream[st2138.ParamInfo], infos []st2138.ParamInfo, res catena.StatusResult) catena.StatusResult {
	if res.IsError() {
		return res
	}
	for _, info := range infos {
		if err := stream.Send(info); err != nil {
			return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
		}
	}
	return res
}

func slotZeroParamInfos(fqoid string, recursive bool) ([]st2138.ParamInfo, catena.StatusResult) {
	// product/* is served by the SDK (RegisterProductStruct); this handler only
	// covers the business-logic params.
	switch fqoid {
	case "":
		return []st2138.ParamInfo{newDashboardUIParamInfo(), newCounterParamInfo(), newRunningParamInfo()}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "counter":
		return []st2138.ParamInfo{newCounterParamInfo()}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "running":
		return []st2138.ParamInfo{newRunningParamInfo()}, catena.StatusWithCode(catena.StatusCodeOk, "")
	case "dashboard_UI":
		return []st2138.ParamInfo{newDashboardUIParamInfo()}, catena.StatusWithCode(catena.StatusCodeOk, "")
	default:
		return []st2138.ParamInfo{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
	}
}

func slotTwoParamInfos(fqoid string, recursive bool, state *ExampleState) ([]st2138.ParamInfo, catena.StatusResult) {
	if fqoid == "" {
		// product/* is served by the SDK (RegisterProductStruct); omit it here.
		return []st2138.ParamInfo{
			st2138.NewParamInfo("volume", st2138.NewPolyglotText("en", "Volume"), st2138.ParamTypeInt32, "", 0),
			st2138.NewParamInfo("muted", st2138.NewPolyglotText("en", "Muted"), st2138.ParamTypeInt32, "", 0),
			st2138.NewParamInfo("device_name", st2138.NewPolyglotText("en", "Device Name"), st2138.ParamTypeString, "", 0),
			st2138.NewParamInfo("struct_example", st2138.NewPolyglotText("en", "Struct Example"), st2138.ParamTypeStruct, "", 0),
			st2138.NewParamInfo("sample_float", st2138.NewPolyglotText("en", "Sample Float"), st2138.ParamTypeFloat32, "", 0),
			newSampleIntArrayParamInfo(state),
			st2138.NewParamInfo("sample_float_array", st2138.NewPolyglotText("en", "Sample Float Array"), st2138.ParamTypeFloat32Array, "", uint32(len(state.sampleFloatArrayValue()))),
			st2138.NewParamInfo("sample_string_array", st2138.NewPolyglotText("en", "Sample String Array"), st2138.ParamTypeStringArray, "", uint32(len(state.sampleStringArrayValue()))),
			st2138.NewParamInfo("sample_binary", st2138.NewPolyglotText("en", "Sample Binary"), st2138.ParamTypeBinary, "", 0),
			st2138.NewParamInfo("sample_struct_variant", st2138.NewPolyglotText("en", "Sample Struct Variant"), st2138.ParamTypeStructVariant, "", 0),
			st2138.NewParamInfo("sample_struct_array", st2138.NewPolyglotText("en", "Sample Struct Array"), st2138.ParamTypeStructArray, "", uint32(len(state.sampleStructArrayValue()))),
			st2138.NewParamInfo("sample_struct_variant_array", st2138.NewPolyglotText("en", "Sample Struct Variant Array"), st2138.ParamTypeStructVariantArray, "", uint32(len(state.sampleStructVariantArrayValue()))),
		}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	if strings.HasPrefix(fqoid, "struct_example") {
		infos := []st2138.ParamInfo{
			st2138.NewParamInfo("struct_example", st2138.NewPolyglotText("en", "Struct Example"), st2138.ParamTypeStruct, "", 0),
		}
		if recursive {
			infos = append(infos,
				st2138.NewParamInfo("struct_example/number", nil, st2138.ParamTypeInt32, "", 0),
				st2138.NewParamInfo("struct_example/text", nil, st2138.ParamTypeString, "", 0),
			)
		}
		switch fqoid {
		case "struct_example":
			return infos, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "struct_example/number":
			return []st2138.ParamInfo{st2138.NewParamInfo("struct_example/number", nil, st2138.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "struct_example/text":
			return []st2138.ParamInfo{st2138.NewParamInfo("struct_example/text", nil, st2138.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}
	}

	if strings.HasPrefix(fqoid, "device_name") {
		return []st2138.ParamInfo{st2138.NewParamInfo("device_name", st2138.NewPolyglotText("en", "Device Name"), st2138.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "volume") {
		return []st2138.ParamInfo{st2138.NewParamInfo("volume", st2138.NewPolyglotText("en", "Volume"), st2138.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "muted") {
		return []st2138.ParamInfo{st2138.NewParamInfo("muted", st2138.NewPolyglotText("en", "Muted"), st2138.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	if strings.HasPrefix(fqoid, "sample_struct_variant_array") {
		return []st2138.ParamInfo{st2138.NewParamInfo("sample_struct_variant_array", st2138.NewPolyglotText("en", "Sample Struct Variant Array"), st2138.ParamTypeStructVariantArray, "", uint32(len(state.sampleStructVariantArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_struct_variant") {
		infos := []st2138.ParamInfo{
			st2138.NewParamInfo("sample_struct_variant", st2138.NewPolyglotText("en", "Sample Struct Variant"), st2138.ParamTypeStructVariant, "", 0),
		}
		if recursive {
			infos = append(infos,
				st2138.NewParamInfo("sample_struct_variant/int_kind", nil, st2138.ParamTypeInt32, "", 0),
				st2138.NewParamInfo("sample_struct_variant/string_kind", nil, st2138.ParamTypeString, "", 0),
			)
		}
		switch fqoid {
		case "sample_struct_variant":
			return infos, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "sample_struct_variant/int_kind":
			return []st2138.ParamInfo{st2138.NewParamInfo("sample_struct_variant/int_kind", nil, st2138.ParamTypeInt32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "sample_struct_variant/string_kind":
			return []st2138.ParamInfo{st2138.NewParamInfo("sample_struct_variant/string_kind", nil, st2138.ParamTypeString, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}
	}
	if strings.HasPrefix(fqoid, "sample_struct_array") {
		return []st2138.ParamInfo{st2138.NewParamInfo("sample_struct_array", st2138.NewPolyglotText("en", "Sample Struct Array"), st2138.ParamTypeStructArray, "", uint32(len(state.sampleStructArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_int_array") {
		return []st2138.ParamInfo{newSampleIntArrayParamInfo(state)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_float_array") {
		return []st2138.ParamInfo{st2138.NewParamInfo("sample_float_array", st2138.NewPolyglotText("en", "Sample Float Array"), st2138.ParamTypeFloat32Array, "", uint32(len(state.sampleFloatArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_string_array") {
		return []st2138.ParamInfo{st2138.NewParamInfo("sample_string_array", st2138.NewPolyglotText("en", "Sample String Array"), st2138.ParamTypeStringArray, "", uint32(len(state.sampleStringArrayValue())))}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_binary") {
		return []st2138.ParamInfo{st2138.NewParamInfo("sample_binary", st2138.NewPolyglotText("en", "Sample Binary"), st2138.ParamTypeBinary, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}
	if strings.HasPrefix(fqoid, "sample_float") {
		return []st2138.ParamInfo{st2138.NewParamInfo("sample_float", st2138.NewPolyglotText("en", "Sample Float"), st2138.ParamTypeFloat32, "", 0)}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	return []st2138.ParamInfo{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
}

func newDashboardUIParamInfo() st2138.ParamInfo {
	return st2138.NewParamInfo("dashboard_UI", st2138.NewPolyglotText("en", "DashBoard UI"), st2138.ParamTypeString, "", 0)
}

func newCounterParamInfo() st2138.ParamInfo {
	return st2138.NewParamInfo("counter", st2138.NewPolyglotText("en", "Counter"), st2138.ParamTypeInt32, "", 0)
}

func newRunningParamInfo() st2138.ParamInfo {
	return st2138.NewParamInfo("running", st2138.NewPolyglotText("en", "Counter Running Status"), st2138.ParamTypeInt32, "", 0)
}

func newSampleIntArrayParamInfo(state *ExampleState) st2138.ParamInfo {
	return st2138.NewParamInfo("sample_int_array", st2138.NewPolyglotText("en", "Sample Int Array"), st2138.ParamTypeInt32Array, "", uint32(len(state.sampleIntArrayValue())))
}
