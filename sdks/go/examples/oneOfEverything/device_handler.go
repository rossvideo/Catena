package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// registerProductStructs hands each slot's mandatory product struct to the SDK.
// Once registered, the SDK injects the product param into the device on
// GetDevice and answers GetValue/ParamInfo for product/* (and rejects writes),
// so none of the value/param-info handlers below deal with product.
func registerProductStructs(srv catena.Server, state *ExampleState) {
	srv.RegisterProductStruct(0, state.slotZeroProduct)
	srv.RegisterProductStruct(1, state.slotOneProduct)
	srv.RegisterProductStruct(2, state.slotTwoProduct)
}

func registerDeviceHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	// GetDeviceHandler streams the device descriptor for a slot as
	// DeviceComponent chunks. Build the descriptor from the same data model
	// your app uses at runtime; this example rebuilds per request so "value"
	// fields stay current. Two chunking styles are shown:
	//
	//   - Slot 0 assembles its chunks by hand with the st2138.ComponentXxx
	//     constructors — the "extra fancy" form for handlers that want full
	//     control over what goes in each chunk.
	//   - Slots 1 and 2 build a whole st2138.Device and delegate the chunking
	//     to catena.DeviceComponentsForRequest — the everyday form, where
	//     business logic never touches the DeviceComponent protos. (A small
	//     model could also be sent as a single ComponentDevice chunk carrying
	//     the whole device; see the hello_world example.)

	// Slot 0: hand-rolled chunking. First the device skeleton (slot metadata,
	// scopes, menus), then one chunk per param and command. The chunks are
	// built from the same slotZero* pieces buildDeviceDefinition composes (the
	// GetParam / ParamInfo handlers use it), so the two views of slot 0 cannot
	// drift apart. ComponentConstraint, ComponentMenu, and
	// ComponentLanguagePack constructors exist for the other component kinds;
	// slot 0 keeps its menus on the skeleton.
	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx catena.HandlerContext, stream catena.Stream[st2138.DeviceComponent]) catena.StatusResult {
		logger.Info("GetDevice", "slot", slot)
		if err := stream.Send(st2138.ComponentDevice(slotZeroSkeleton())); err != nil {
			logger.Warning("GetDevice stream closed", "slot", slot, "error", err)
			return catena.StatusWithCode(catena.StatusCodeInternal, "failed to send device: "+err.Error())
		}
		for _, entry := range slotZeroParams(counter) {
			if err := stream.Send(st2138.ComponentParam(entry.oid, entry.param)); err != nil {
				logger.Warning("GetDevice stream closed", "slot", slot, "oid", entry.oid, "error", err)
				return catena.StatusWithCode(catena.StatusCodeInternal, "failed to send param: "+err.Error())
			}
		}
		for _, entry := range slotZeroCommands() {
			if err := stream.Send(st2138.ComponentCommand(entry.oid, entry.param)); err != nil {
				logger.Warning("GetDevice stream closed", "slot", slot, "oid", entry.oid, "error", err)
				return catena.StatusWithCode(catena.StatusCodeInternal, "failed to send command: "+err.Error())
			}
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	})

	// Slots 1 and 2: build the device, then let the SDK chunk and stream it.
	for _, slot := range slotList {
		if slot == 0 {
			continue
		}
		srv.RegisterGetDeviceHandler(slot, func(slot uint16, ctx catena.HandlerContext, stream catena.Stream[st2138.DeviceComponent]) catena.StatusResult {
			logger.Info("GetDevice", "slot", slot)
			device, ok := buildDeviceDefinition(slot, counter, state)
			if !ok {
				return catena.StatusWithCode(catena.StatusCodeNotFound, "device not found")
			}
			return catena.DeviceComponentsForRequest(device, stream)
		})
	}
}

// namedParam pairs an oid with its descriptor so slot 0's params and commands
// can be listed once and consumed two ways: attached to a Device by
// buildDeviceDefinition, or streamed chunk-by-chunk by the GetDevice handler.
type namedParam struct {
	oid   string
	param *st2138.Param
}

// slotZeroSkeleton returns slot 0's device minus its params and commands: slot
// metadata, access scopes, and menus. The GetDevice handler sends it as the
// leading ComponentDevice chunk; buildDeviceDefinition attaches the params and
// commands to it.
func slotZeroSkeleton() *st2138.Device {
	return st2138.NewDevice(0).
		WithDetailLevel(st2138.DetailLevelFull).
		WithMultiSetEnabled(true).
		WithSubscriptions(true).
		WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
		WithDefaultScope("st2138:cfg").
		WithMenuGroup("status", st2138.NewMenuGroup().
			WithName(st2138.NewPolyglotText("en", "Status")).
			WithOrder(0).
			WithMenu("status", st2138.NewMenu().
				WithName(st2138.NewPolyglotText("en", "Status")).
				WithParamOids("product", "counter", "running"))).
		WithMenuGroup("config", st2138.NewMenuGroup().
			WithName(st2138.NewPolyglotText("en", "Configuration")).
			WithOrder(1).
			WithMenu("control", st2138.NewMenu().
				WithName(st2138.NewPolyglotText("en", "Control")).
				WithCommandOids("start", "stop", "add10", "reset")))
}

// slotZeroParams lists slot 0's params in the order they are streamed.
func slotZeroParams(counter *CounterState) []namedParam {
	return []namedParam{
		{"counter", makeCounterParam(counter)},
		{"running", makeRunningParam(counter)},
	}
}

// slotZeroCommands lists slot 0's commands in the order they are streamed.
func slotZeroCommands() []namedParam {
	return []namedParam{
		{"start", st2138.NewParamEmpty().
			WithName(st2138.NewPolyglotText("en", "Start Counter"))},
		{"stop", st2138.NewParamEmpty().
			WithName(st2138.NewPolyglotText("en", "Stop Counter"))},
		{"add10", st2138.NewParamEmpty().
			WithName(st2138.NewPolyglotText("en", "Add 10 to Counter"))},
		{"reset", st2138.NewParamEmpty().
			WithName(st2138.NewPolyglotText("en", "Reset Counter"))},
	}
}

// buildDeviceDefinition returns the descriptor for one slot. It is a function
// (not a static YAML file) so every param's value field reflects live state at
// GetDevice time. The GetParam and ParamInfo handlers call this for every slot;
// the GetDevice handler calls it for slots 1 and 2 (slot 0's GetDevice streams
// the same slotZero* pieces directly, see registerDeviceHandlers). Keep param
// OIDs in sync with value_handlers and param_info_handler.
//
// st2138.NewDevice(slot) creates an empty device for the slot; params, commands,
// constraints, and menus are attached with the fluent With* builders. The
// mandatory "product" struct param is managed by the SDK, not built here:
// registerProductStructs registers one per slot via Server.RegisterProductStruct
// and the SDK injects it on GetDevice (so menus may still reference the "product"
// OID) and serves it on GetValue / ParamInfo.
//
// Slot roles:
//   - 0: counter, commands, constraints, subscriptions, cfg-scope reads
//   - 1: sync.Map-backed picture controls; ParamInfo delegates here via ParamInfosForRequest
//   - 2: prefix-dispatched params plus sample_* types (including INT32_ARRAY)
func buildDeviceDefinition(slot uint16, counter *CounterState, state *ExampleState) (*st2138.Device, bool) {
	switch slot {
	case 0:
		// Slot 0: INT32, STRUCT, EMPTY commands, INT32_CHOICE constraint.
		// Composed from the same slotZero* helpers the GetDevice handler
		// streams chunk-by-chunk, so the two views stay in sync.
		device := slotZeroSkeleton()
		for _, entry := range slotZeroParams(counter) {
			device.WithParam(entry.oid, entry.param)
		}
		for _, entry := range slotZeroCommands() {
			device.WithCommand(entry.oid, entry.param)
		}
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

		device := st2138.NewDevice(1).
			WithDetailLevel(st2138.DetailLevelFull).
			WithMultiSetEnabled(false).
			WithSubscriptions(true).
			WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
			WithDefaultScope("st2138:mon").
			WithParam("resolution", st2138.NewParamString(resolution).
				WithName(st2138.NewPolyglotText("en", "Resolution"))).
			WithParam("brightness", st2138.NewParamInt32(brightness).
				WithName(st2138.NewPolyglotText("en", "Brightness"))).
			WithParam("contrast", st2138.NewParamInt32(contrast).
				WithName(st2138.NewPolyglotText("en", "Contrast"))).
			WithParam("saturation", st2138.NewParamInt32(saturation).
				WithName(st2138.NewPolyglotText("en", "Saturation"))).
			WithMenuGroup("status", st2138.NewMenuGroup().
				WithName(st2138.NewPolyglotText("en", "Status")).
				WithOrder(0).
				WithMenu("status", st2138.NewMenu().
					WithName(st2138.NewPolyglotText("en", "Status")).
					WithParamOids("resolution"))).
			WithMenuGroup("config", st2138.NewMenuGroup().
				WithName(st2138.NewPolyglotText("en", "Configuration")).
				WithOrder(1).
				WithMenu("picture", st2138.NewMenu().
					WithName(st2138.NewPolyglotText("en", "Picture")).
					WithParamOids("brightness", "contrast", "saturation")))
		return device, true
	case 2:
		// Slot 2: prefix-dispatched identity/audio params plus FLOAT32, arrays,
		// BINARY, STRUCT_VARIANT, STRUCT_ARRAY, STRUCT_VARIANT_ARRAY examples.
		// Hold state.mu through param building: copied slices and maps alias
		// ExampleState backing storage, and SetValue holds the write lock for
		// the full handler. Release only after the builder clones each proto so
		// GetDevice cannot observe torn array/map data (same pattern as slot 2
		// GetValue, which keeps RLock through st2138.ToValue).
		state.mu.RLock()
		defer state.mu.RUnlock()
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

		device := st2138.NewDevice(2).
			WithDetailLevel(st2138.DetailLevelFull).
			WithMultiSetEnabled(true).
			WithSubscriptions(false).
			WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
			WithDefaultScope("st2138:op").
			WithParam("volume", st2138.NewParamInt32(volume).
				WithName(st2138.NewPolyglotText("en", "Volume"))).
			WithParam("muted", st2138.NewParamInt32(muted).
				WithName(st2138.NewPolyglotText("en", "Muted"))).
			WithParam("device_name", st2138.NewParamString(deviceName).
				WithName(st2138.NewPolyglotText("en", "Device Name"))).
			// NewParamStruct auto-creates the valueless "number" and "text"
			// field descriptors from the values map.
			WithParam("struct_example", st2138.NewParamStruct(map[string]any{
				"number": structNumber,
				"text":   structText,
			}).
				WithName(st2138.NewPolyglotText("en", "Struct Example"))).
			WithParam("sample_float", st2138.NewParamFloat32(sampleFloat).
				WithName(st2138.NewPolyglotText("en", "Sample Float"))).
			WithParam("sample_int_array", st2138.NewParamInt32Array(sampleIntArray).
				WithName(st2138.NewPolyglotText("en", "Sample Int Array"))).
			WithParam("sample_float_array", st2138.NewParamFloat32Array(sampleFloatArray).
				WithName(st2138.NewPolyglotText("en", "Sample Float Array"))).
			WithParam("sample_string_array", st2138.NewParamStringArray(sampleStringArray).
				WithName(st2138.NewPolyglotText("en", "Sample String Array"))).
			WithParam("sample_binary", st2138.NewParamBinary(sampleBinary).
				WithName(st2138.NewPolyglotText("en", "Sample Binary"))).
			// Valueless NewParamX() calls build sub-param descriptors without
			// dummy values; the actual values live in the parent param's value.
			WithParam("sample_struct_variant", st2138.NewParamStructVariant(sampleStructVariant).
				WithName(st2138.NewPolyglotText("en", "Sample Struct Variant")).
				WithParam("int_kind", st2138.NewParamInt32()).
				WithParam("string_kind", st2138.NewParamString())).
			WithParam("sample_struct_array", st2138.NewParamStructArray(sampleStructArray).
				WithName(st2138.NewPolyglotText("en", "Sample Struct Array")).
				WithParam("label", st2138.NewParamString()).
				WithParam("count", st2138.NewParamInt32())).
			WithParam("sample_struct_variant_array", st2138.NewParamStructVariantArray(sampleStructVariantArray).
				WithName(st2138.NewPolyglotText("en", "Sample Struct Variant Array")).
				WithParam("int_kind", st2138.NewParamInt32()).
				WithParam("string_kind", st2138.NewParamString())).
			WithMenuGroup("status", st2138.NewMenuGroup().
				WithName(st2138.NewPolyglotText("en", "Status")).
				WithOrder(0).
				WithMenu("identity", st2138.NewMenu().
					WithName(st2138.NewPolyglotText("en", "Identity")).
					WithParamOids("product", "device_name", "struct_example")).
				WithMenu("types", st2138.NewMenu().
					WithName(st2138.NewPolyglotText("en", "Catena Types")).
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
			WithMenuGroup("config", st2138.NewMenuGroup().
				WithName(st2138.NewPolyglotText("en", "Configuration")).
				WithOrder(1).
				WithMenu("audio", st2138.NewMenu().
					WithName(st2138.NewPolyglotText("en", "Audio")).
					WithParamOids("volume", "muted")))
		return device, true
	default:
		return nil, false
	}
}
