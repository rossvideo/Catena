package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerDeviceHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	// GetDeviceHandler returns the complete device descriptor for a slot.
	// Build the descriptor from the same data model your app uses at runtime;
	// this example rebuilds per request so "value" fields stay current.
	// catena.ToDevice converts a plain map into the SDK Device wrapper.
	for _, slot := range slotList {
		srv.RegisterGetDeviceHandler(slot, func(slot uint16, ctx catena.HandlerContext) (catena.Device, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			deviceInfo, ok := buildDeviceDefinition(slot, counter, state)
			if !ok {
				return catena.ReplyError[catena.Device](catena.StatusCodeNotFound, "device not found")
			}
			device, err := catena.ToDevice(deviceInfo)
			if err != nil {
				return catena.ReplyError[catena.Device](catena.StatusCodeInternal, err.Error())
			}
			return catena.Reply(device)
		})
	}
}

// buildDeviceDefinition returns the descriptor for one slot. It is a function
// (not a static YAML file) so every param's value field reflects live state at
// GetDevice time. device_handler.go calls this and passes the result to
// catena.ToDevice. Keep param OIDs in sync with value_handlers and param_info_handler.
//
// Slot roles:
//   - 0: counter, commands, constraints, subscriptions, cfg-scope reads
//   - 1: sync.Map-backed picture controls; ParamInfo delegates here via ParamInfosForRequest
//   - 2: prefix-dispatched params plus sample_* types (including INT32_ARRAY)
func buildDeviceDefinition(slot uint16, counter *CounterState, state *ExampleState) (map[string]any, bool) {
	switch slot {
	case 0:
		// Slot 0: INT32, STRUCT, EMPTY commands, INT32_CHOICE constraint.
		productValue, _ := state.slotZeroProductValue("product")
		productParam := catena.NewParamStruct(productValue.(map[string]any)).
			WithReadOnly(true).
			WithParam("name", catena.NewParamString("")).
			WithParam("vendor", catena.NewParamString("")).
			WithParam("version", catena.NewParamString("")).ToMap()
		counterParam := catena.NewParamInt32(counter.GetValue()).
			WithName(catena.NewPolyglotText("en", "Counter")).ToMap()
		runningParam := catena.NewParamInt32(counter.RunningInt32()).
			WithName(catena.NewPolyglotText("en", "Counter Running Status")).
			WithReadOnly(true).
			WithConstraint(catena.NewConstraintInt32Choice([]catena.Int32Choice{
				{Value: 0, Name: catena.NewPolyglotText("en", "Not Counting")},
				{Value: 1, Name: catena.NewPolyglotText("en", "Counting")},
			})).ToMap()
		startCommand := catena.NewParamEmpty().
			WithName(catena.NewPolyglotText("en", "Start Counter")).ToMap()
		stopCommand := catena.NewParamEmpty().
			WithName(catena.NewPolyglotText("en", "Stop Counter")).ToMap()
		add10Command := catena.NewParamEmpty().
			WithName(catena.NewPolyglotText("en", "Add 10 to Counter")).ToMap()
		resetCommand := catena.NewParamEmpty().
			WithName(catena.NewPolyglotText("en", "Reset Counter")).ToMap()
		return map[string]any{
			"slot":              uint32(0),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:cfg",
			"params": map[string]any{
				"product": productParam,
				"counter": counterParam,
				"running": runningParam,
			},
			"commands": map[string]any{
				"start": startCommand,
				"stop":  stopCommand,
				"add10": add10Command,
				"reset": resetCommand,
			},
			"menu_groups": map[string]any{
				"status": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Status",
						},
					},
					"order": 0,
					"menus": map[string]any{
						"status": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Status",
								},
							},
							"param_oids": []string{"product", "counter", "running"},
						},
					},
				},
				"config": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Configuration",
						},
					},
					"order": 1,
					"menus": map[string]any{
						"control": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Control",
								},
							},
							"command_oids": []string{"start", "stop", "add10", "reset"},
						},
					},
				},
			},
		}, true
	case 1:
		// Slot 1 intentionally stores its business data in a sync.Map to show
		// adopters they can back handlers with any application data structure,
		// not just typed fields like slots 0 and 2 use.
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
		productParam := catena.NewParamStruct(map[string]any{
			"name":               "Map-Backed Slot 1 Product",
			"vendor":             "Ross Video",
			"version":            "1.0.0",
			"catena_sdk":         "github.com/rossvideo/catena/sdks/go",
			"catena_sdk_version": "v0.1.0",
			"serial_number":      "SN12345678",
		}).
			WithReadOnly(true).
			WithParam("name", catena.NewParamString("")).
			WithParam("vendor", catena.NewParamString("")).
			WithParam("version", catena.NewParamString("")).
			WithParam("catena_sdk", catena.NewParamString("")).
			WithParam("catena_sdk_version", catena.NewParamString("")).
			WithParam("serial_number", catena.NewParamString("")).ToMap()
		resolutionParam := catena.NewParamString(resolution).
			WithName(catena.NewPolyglotText("en", "Resolution")).ToMap()
		brightnessParam := catena.NewParamInt32(brightness).
			WithName(catena.NewPolyglotText("en", "Brightness")).ToMap()
		contrastParam := catena.NewParamInt32(contrast).
			WithName(catena.NewPolyglotText("en", "Contrast")).ToMap()
		saturationParam := catena.NewParamInt32(saturation).
			WithName(catena.NewPolyglotText("en", "Saturation")).ToMap()
		return map[string]any{
			"slot":              uint32(1),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": false,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:mon",
			"params": map[string]any{
				"product":    productParam,
				"resolution": resolutionParam,
				"brightness": brightnessParam,
				"contrast":   contrastParam,
				"saturation": saturationParam,
			},
			"menu_groups": map[string]any{
				"status": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Status",
						},
					},
					"order": 0,
					"menus": map[string]any{
						"status": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Status",
								},
							},
							"param_oids": []string{"resolution"},
						},
					},
				},
				"config": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Configuration",
						},
					},
					"order": 1,
					"menus": map[string]any{
						"picture": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Picture",
								},
							},
							"param_oids": []string{"brightness", "contrast", "saturation"},
						},
					},
				},
			},
		}, true
	case 2:
		// Slot 2: prefix-dispatched identity/audio params plus FLOAT32, arrays,
		// BINARY, STRUCT_VARIANT, STRUCT_ARRAY, STRUCT_VARIANT_ARRAY examples.
		state.mu.RLock()
		productValue := state.slotTwoProduct
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
		state.mu.RUnlock()
		productParam := catena.NewParamStruct(productValue).
			WithReadOnly(true).
			WithParam("name", catena.NewParamString("")).
			WithParam("vendor", catena.NewParamString("")).
			WithParam("version", catena.NewParamString("")).
			WithParam("catena_sdk", catena.NewParamString("")).
			WithParam("catena_sdk_version", catena.NewParamString("")).
			WithParam("serial_number", catena.NewParamString("")).ToMap()
		volumeParam := catena.NewParamInt32(volume).
			WithName(catena.NewPolyglotText("en", "Volume")).ToMap()
		mutedParam := catena.NewParamInt32(muted).
			WithName(catena.NewPolyglotText("en", "Muted")).ToMap()
		deviceNameParam := catena.NewParamString(deviceName).
			WithName(catena.NewPolyglotText("en", "Device Name")).ToMap()
		structExampleParam := catena.NewParamStruct(structExample).
			WithName(catena.NewPolyglotText("en", "Struct Example")).
			WithParam("number", catena.NewParamInt32(0)).
			WithParam("text", catena.NewParamString("")).ToMap()
		sampleFloatParam := catena.NewParamFloat32(sampleFloat).
			WithName(catena.NewPolyglotText("en", "Sample Float")).ToMap()
		sampleIntArrayParam := catena.NewParamInt32Array(sampleIntArray).
			WithName(catena.NewPolyglotText("en", "Sample Int Array")).ToMap()
		sampleFloatArrayParam := catena.NewParamFloat32Array(sampleFloatArray).
			WithName(catena.NewPolyglotText("en", "Sample Float Array")).ToMap()
		sampleStringArrayParam := catena.NewParamStringArray(sampleStringArray).
			WithName(catena.NewPolyglotText("en", "Sample String Array")).ToMap()
		sampleBinaryParam := catena.NewParamBinary(sampleBinary).
			WithName(catena.NewPolyglotText("en", "Sample Binary")).ToMap()
		sampleStructVariantParam := catena.NewParamStructVariant(&sampleStructVariant).
			WithName(catena.NewPolyglotText("en", "Sample Struct Variant")).
			WithParam("int_kind", catena.NewParamInt32(0)).
			WithParam("string_kind", catena.NewParamString("")).ToMap()
		sampleStructArrayParam := catena.NewParamStructArray(sampleStructArray).
			WithName(catena.NewPolyglotText("en", "Sample Struct Array")).
			WithParam("label", catena.NewParamString("")).
			WithParam("count", catena.NewParamInt32(0)).ToMap()
		sampleStructVariantArrayParam := catena.NewParamStructVariantArray(sampleStructVariantArray).
			WithName(catena.NewPolyglotText("en", "Sample Struct Variant Array")).
			WithParam("int_kind", catena.NewParamInt32(0)).
			WithParam("string_kind", catena.NewParamString("")).ToMap()
		return map[string]any{
			"slot":              uint32(2),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": true,
			"subscriptions":     false,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"params": map[string]any{
				"product":                     productParam,
				"volume":                      volumeParam,
				"muted":                       mutedParam,
				"device_name":                 deviceNameParam,
				"struct_example":              structExampleParam,
				"sample_float":                sampleFloatParam,
				"sample_int_array":            sampleIntArrayParam,
				"sample_float_array":          sampleFloatArrayParam,
				"sample_string_array":         sampleStringArrayParam,
				"sample_binary":               sampleBinaryParam,
				"sample_struct_variant":       sampleStructVariantParam,
				"sample_struct_array":         sampleStructArrayParam,
				"sample_struct_variant_array": sampleStructVariantArrayParam,
			},
			"menu_groups": map[string]any{
				"status": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Status",
						},
					},
					"order": 0,
					"menus": map[string]any{
						"identity": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Identity",
								},
							},
							"param_oids": []string{"product", "device_name", "struct_example"},
						},
						"types": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Catena Types",
								},
							},
							"param_oids": []string{
								"sample_float",
								"sample_int_array",
								"sample_float_array",
								"sample_string_array",
								"sample_binary",
								"sample_struct_variant",
								"sample_struct_array",
								"sample_struct_variant_array",
							},
						},
					},
				},
				"config": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Configuration",
						},
					},
					"order": 1,
					"menus": map[string]any{
						"audio": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Audio",
								},
							},
							"param_oids": []string{"volume", "muted"},
						},
					},
				},
			},
		}, true
	default:
		return nil, false
	}
}
