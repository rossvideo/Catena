package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerDeviceHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	// GetDeviceHandler returns the complete device descriptor for a slot.
	// Build the descriptor from the same data model your app uses at runtime;
	// this example rebuilds per request so "value" fields stay current.
	for _, slot := range slotList {
		srv.RegisterGetDeviceHandler(slot, func(slot uint16, ctx catena.HandlerContext) (catena.Device, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			device, ok := buildDeviceDefinition(slot, counter, state)
			if !ok {
				return catena.ReplyError[catena.Device](catena.StatusCodeNotFound, "device not found")
			}
			return catena.Reply(*device)
		})
	}
}

// productString safely extracts a string field from a product map.
func productString(product map[string]any, key string) string {
	if value, ok := product[key].(string); ok {
		return value
	}
	return ""
}

// buildDeviceDefinition returns the descriptor for one slot. It is a function
// (not a static YAML file) so every param's value field reflects live state at
// GetDevice time. device_handler.go calls this and returns the result directly.
// Keep param OIDs in sync with value_handlers and param_info_handler.
//
// catena.NewDevice seeds the mandatory "product" struct param from the
// identity fields passed to it; additional params, commands, constraints, and
// menus are attached with the fluent With* builders.
//
// Slot roles:
//   - 0: counter, commands, constraints, subscriptions, cfg-scope reads
//   - 1: sync.Map-backed picture controls; ParamInfo delegates here via ParamInfosForRequest
//   - 2: prefix-dispatched params plus sample_* types (including INT32_ARRAY)
func buildDeviceDefinition(slot uint16, counter *CounterState, state *ExampleState) (*catena.Device, bool) {
	switch slot {
	case 0:
		// Slot 0: INT32, STRUCT, EMPTY commands, INT32_CHOICE constraint.
		state.mu.RLock()
		product := state.slotZeroProduct
		name := productString(product, "name")
		vendor := productString(product, "vendor")
		version := productString(product, "version")
		state.mu.RUnlock()

		device := catena.NewDevice(0, name, vendor, version, "SN-SLOT0-0001").
			WithDetailLevel(catena.DetailLevelFull).
			WithMultiSetEnabled(true).
			WithSubscriptions(true).
			WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
			WithDefaultScope("st2138:cfg").
			WithParam("counter", catena.NewParamInt32(counter.GetValue()).
				WithName(catena.NewPolyglotText("en", "Counter"))).
			WithParam("running", catena.NewParamInt32(counter.RunningInt32()).
				WithName(catena.NewPolyglotText("en", "Counter Running Status")).
				WithReadOnly(true).
				WithConstraint(catena.NewConstraintInt32Choice([]catena.Int32Choice{
					{Value: 0, Name: catena.NewPolyglotText("en", "Not Counting")},
					{Value: 1, Name: catena.NewPolyglotText("en", "Counting")},
				}))).
			WithCommand("start", catena.NewParamEmpty().
				WithName(catena.NewPolyglotText("en", "Start Counter"))).
			WithCommand("stop", catena.NewParamEmpty().
				WithName(catena.NewPolyglotText("en", "Stop Counter"))).
			WithCommand("add10", catena.NewParamEmpty().
				WithName(catena.NewPolyglotText("en", "Add 10 to Counter"))).
			WithCommand("reset", catena.NewParamEmpty().
				WithName(catena.NewPolyglotText("en", "Reset Counter"))).
			WithMenuGroup("status", catena.NewMenuGroup().
				WithName(catena.NewPolyglotText("en", "Status")).
				WithOrder(0).
				WithMenu("status", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Status")).
					WithParamOids("product", "counter", "running"))).
			WithMenuGroup("config", catena.NewMenuGroup().
				WithName(catena.NewPolyglotText("en", "Configuration")).
				WithOrder(1).
				WithMenu("control", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Control")).
					WithCommandOids("start", "stop", "add10", "reset")))
		return device, true
	case 1:
		// Slot 1 intentionally stores its business data in a sync.Map to show
		// adopters they can back handlers with any application data structure,
		// not just typed fields like slots 0 and 2 use. Hold state.mu so
		// multi-key reads stay consistent with concurrent SetValue updates.
		state.mu.RLock()
		resolution := "1920x1080"
		if value, ok := state.slotOneParams.Load("resolution"); ok {
			if typed, ok := value.(string); ok {
				resolution = typed
			}
		}
		brightness := int32(50)
		if value, ok := state.slotOneParams.Load("brightness"); ok {
			if typed, ok := value.(int32); ok {
				brightness = typed
			}
		}
		contrast := int32(50)
		if value, ok := state.slotOneParams.Load("contrast"); ok {
			if typed, ok := value.(int32); ok {
				contrast = typed
			}
		}
		saturation := int32(50)
		if value, ok := state.slotOneParams.Load("saturation"); ok {
			if typed, ok := value.(int32); ok {
				saturation = typed
			}
		}
		state.mu.RUnlock()

		device := catena.NewDevice(1, "Map-Backed Slot 1 Product", "Ross Video", "1.0.0", "SN12345678").
			WithDetailLevel(catena.DetailLevelFull).
			WithMultiSetEnabled(false).
			WithSubscriptions(true).
			WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
			WithDefaultScope("st2138:mon").
			WithParam("resolution", catena.NewParamString(resolution).
				WithName(catena.NewPolyglotText("en", "Resolution"))).
			WithParam("brightness", catena.NewParamInt32(brightness).
				WithName(catena.NewPolyglotText("en", "Brightness"))).
			WithParam("contrast", catena.NewParamInt32(contrast).
				WithName(catena.NewPolyglotText("en", "Contrast"))).
			WithParam("saturation", catena.NewParamInt32(saturation).
				WithName(catena.NewPolyglotText("en", "Saturation"))).
			WithMenuGroup("status", catena.NewMenuGroup().
				WithName(catena.NewPolyglotText("en", "Status")).
				WithOrder(0).
				WithMenu("status", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Status")).
					WithParamOids("resolution"))).
			WithMenuGroup("config", catena.NewMenuGroup().
				WithName(catena.NewPolyglotText("en", "Configuration")).
				WithOrder(1).
				WithMenu("picture", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Picture")).
					WithParamOids("brightness", "contrast", "saturation")))
		return device, true
	case 2:
		// Slot 2: prefix-dispatched identity/audio params plus FLOAT32, arrays,
		// BINARY, STRUCT_VARIANT, STRUCT_ARRAY, STRUCT_VARIANT_ARRAY examples.
		// Hold state.mu through param building: copied slices and maps alias
		// ExampleState backing storage, and SetValue holds the write lock for
		// the full handler. Release only after the builder clones each proto so
		// GetDevice cannot observe torn array/map data (same pattern as slot 2
		// GetValue, which keeps RLock through catena.ToValue).
		state.mu.RLock()
		defer state.mu.RUnlock()
		product := state.slotTwoProduct
		volume := state.volume
		muted := state.muted
		deviceName := state.deviceName
		structExample := state.structExample
		sampleFloat := state.sampleFloat
		sampleIntArray := state.sampleIntArray
		sampleFloatArray := state.sampleFloatArray
		sampleStringArray := state.sampleStringArray
		sampleBinary := state.sampleBinary
		sampleStructVariant := state.sampleStructVariant
		sampleStructArray := state.sampleStructArray
		sampleStructVariantArray := state.sampleStructVariantArray

		structNumber, _ := structExample["number"].(int32)
		structText, _ := structExample["text"].(string)

		device := catena.NewDevice(2,
			productString(product, "name"),
			productString(product, "vendor"),
			productString(product, "version"),
			productString(product, "serial_number")).
			WithDetailLevel(catena.DetailLevelFull).
			WithMultiSetEnabled(true).
			WithSubscriptions(false).
			WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
			WithDefaultScope("st2138:op").
			WithParam("volume", catena.NewParamInt32(volume).
				WithName(catena.NewPolyglotText("en", "Volume"))).
			WithParam("muted", catena.NewParamInt32(muted).
				WithName(catena.NewPolyglotText("en", "Muted"))).
			WithParam("device_name", catena.NewParamString(deviceName).
				WithName(catena.NewPolyglotText("en", "Device Name"))).
			WithParam("struct_example", catena.NewParamStruct().
				WithName(catena.NewPolyglotText("en", "Struct Example")).
				WithParam("number", catena.NewParamInt32(structNumber)).
				WithParam("text", catena.NewParamString(structText))).
			WithParam("sample_float", catena.NewParamFloat32(sampleFloat).
				WithName(catena.NewPolyglotText("en", "Sample Float"))).
			WithParam("sample_int_array", catena.NewParamInt32Array(sampleIntArray).
				WithName(catena.NewPolyglotText("en", "Sample Int Array"))).
			WithParam("sample_float_array", catena.NewParamFloat32Array(sampleFloatArray).
				WithName(catena.NewPolyglotText("en", "Sample Float Array"))).
			WithParam("sample_string_array", catena.NewParamStringArray(sampleStringArray).
				WithName(catena.NewPolyglotText("en", "Sample String Array"))).
			WithParam("sample_binary", catena.NewParamBinary(sampleBinary).
				WithName(catena.NewPolyglotText("en", "Sample Binary"))).
			WithParam("sample_struct_variant", catena.NewParamStructVariant(&sampleStructVariant).
				WithName(catena.NewPolyglotText("en", "Sample Struct Variant")).
				WithParam("int_kind", catena.NewParamInt32(0)).
				WithParam("string_kind", catena.NewParamString(""))).
			WithParam("sample_struct_array", catena.NewParamStructArray(sampleStructArray).
				WithName(catena.NewPolyglotText("en", "Sample Struct Array")).
				WithParam("label", catena.NewParamString("")).
				WithParam("count", catena.NewParamInt32(0))).
			WithParam("sample_struct_variant_array", catena.NewParamStructVariantArray(sampleStructVariantArray).
				WithName(catena.NewPolyglotText("en", "Sample Struct Variant Array")).
				WithParam("int_kind", catena.NewParamInt32(0)).
				WithParam("string_kind", catena.NewParamString(""))).
			WithMenuGroup("status", catena.NewMenuGroup().
				WithName(catena.NewPolyglotText("en", "Status")).
				WithOrder(0).
				WithMenu("identity", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Identity")).
					WithParamOids("product", "device_name", "struct_example")).
				WithMenu("types", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Catena Types")).
					WithParamOids(
						"sample_float",
						"sample_int_array",
						"sample_float_array",
						"sample_string_array",
						"sample_binary",
						"sample_struct_variant",
						"sample_struct_array",
						"sample_struct_variant_array",
					))).
			WithMenuGroup("config", catena.NewMenuGroup().
				WithName(catena.NewPolyglotText("en", "Configuration")).
				WithOrder(1).
				WithMenu("audio", catena.NewMenu().
					WithName(catena.NewPolyglotText("en", "Audio")).
					WithParamOids("volume", "muted")))
		return device, true
	default:
		return nil, false
	}
}
