/*
 * Copyright 2026 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Shared GetDevice example logic for REST and gRPC.
 * @file getdevice.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package getdevice

import (
	"time"

	"github.com/rossvideo/catena/sdks/go/examples/exampleutil"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

var SlotList = []uint16{0, 1, 2}

var Devices = map[uint16]map[string]any{
	0: {
		"slot":              uint32(0),
		"detail_level":      catena.DetailLevelFull,
		"multi_set_enabled": true,
		"subscriptions":     true,
		"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
		"default_scope":     "st2138:op",
		"params": map[string]any{
			"product": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Product",
					},
				},
				"type": catena.ParamTypeStruct,
				"params": map[string]any{
					"version": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Version",
							},
						},
						"type":      catena.ParamTypeString,
						"read_only": true,
						"widget":    "TEXT",
					},
				},
				"value": map[string]any{
					"struct_value": map[string]any{
						"fields": map[string]any{
							"version": map[string]any{
								"string_value": "1.0.0",
							},
						},
					},
				},
			},
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type": catena.ParamTypeInt32,
				"constraint": map[string]any{
					"ref_oid": "brightness_range",
				},
				"read_only": false,
				"widget":    "SLIDER",
			},
			"contrast": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Contrast",
					},
				},
				"type": catena.ParamTypeInt32,
				"constraint": map[string]any{
					"ref_oid": "brightness_range",
				},
				"read_only": false,
				"widget":    "SLIDER",
			},
			"input_source": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Input Source",
						"fr": "Source d'entrée",
					},
				},
				"type": catena.ParamTypeString,
				"constraint": map[string]any{
					"ref_oid": "input_source_choice",
				},
				"value": map[string]any{
					"string_value": "HDMI",
				},
				"read_only": false,
				"widget":    "DROPDOWN",
			},
		},
		"constraints": map[string]any{
			"brightness_range": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(0),
					"max_value": int32(100),
				},
			},
			"input_source_choice": map[string]any{
				"string_choice": map[string]any{
					"choices": []string{"SDI", "HDMI", "IP"},
				},
			},
		},
		"menu_groups": map[string]any{
			"status": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Status",
					},
				},
				"menus": map[string]any{
					"info": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Device Info",
							},
						},
						"param_oids": []string{"product/version"},
					},
				},
			},
			"config": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Config",
					},
				},
				"menus": map[string]any{
					"basic": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Basic",
							},
						},
						"param_oids": []string{"brightness", "contrast"},
					},
					"input": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Input Configuration",
							},
						},
						"param_oids": []string{"input_source"},
					},
				},
			},
		},
		"commands": map[string]any{
			"reboot": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Reboot Device",
						"fr": "Redémarrer l'appareil",
					},
				},
				"type": catena.ParamTypeEmpty,
			},
			"reset": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Reset to Defaults",
						"fr": "Réinitialiser aux valeurs par défaut",
					},
				},
				"type": catena.ParamTypeEmpty,
			},
		},
		"language_packs": map[string]any{
			"packs": map[string]any{
				"en": map[string]any{
					"name": "English",
					"words": map[string]string{
						"brightness":   "Brightness",
						"contrast":     "Contrast",
						"input_source": "Input Source",
						"status":       "Status",
						"config":       "Config",
						"basic":        "Basic",
						"input":        "Input Configuration",
						"info":         "Device Info",
						"reboot":       "Reboot Device",
						"reset":        "Reset to Defaults",
					},
				},
				"fr": map[string]any{
					"name": "Français",
					"words": map[string]string{
						"brightness":   "Luminosité",
						"contrast":     "Contraste",
						"input_source": "Source d'entrée",
						"status":       "Statut",
						"config":       "Configuration",
						"reboot":       "Redémarrer l'appareil",
						"reset":        "Réinitialiser aux valeurs par défaut",
					},
				},
			},
		},
	},
	1: {
		"slot":              uint32(1),
		"detail_level":      catena.DetailLevelFull,
		"multi_set_enabled": false,
		"subscriptions":     true,
		"access_scopes":     []string{"st2138:mon", "st2138:op"},
		"default_scope":     "st2138:mon",
		"constraints": map[string]any{
			"gain_range": map[string]any{
				"float_range": map[string]any{
					"min_value": float32(-60.0),
					"max_value": float32(12.0),
				},
			},
			"channel_count": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(1),
					"max_value": int32(16),
				},
			},
		},
		"params": map[string]any{
			"master_gain": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Master Gain",
					},
				},
				"type": catena.ParamTypeFloat32,
				"constraint": map[string]any{
					"ref_oid": "gain_range",
				},
				"widget":    "SLIDER",
				"read_only": false,
			},
			"input_1_gain": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Input 1 Gain",
					},
				},
				"type": catena.ParamTypeFloat32,
				"constraint": map[string]any{
					"ref_oid": "gain_range",
				},
				"widget":    "SLIDER",
				"read_only": false,
			},
			"mute": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Mute",
					},
				},
				"type":      catena.ParamTypeInt32,
				"widget":    "CHECKBOX",
				"read_only": false,
			},
		},
		"menu_groups": map[string]any{
			"audio": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Audio Settings",
					},
				},
				"menus": map[string]any{
					"levels": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Levels",
							},
						},
						"param_oids": []string{
							"master_gain",
							"input_1_gain",
							"mute",
						},
					},
				},
			},
		},
		"commands": map[string]any{
			"reset_meters": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Reset Peak Meters",
					},
				},
				"type": catena.ParamTypeEmpty,
			},
		},
		"language_packs": map[string]any{
			"packs": map[string]any{
				"en": map[string]any{
					"name": "English",
					"words": map[string]string{
						"master_gain":  "Master Gain",
						"input_1_gain": "Input 1 Gain",
						"mute":         "Mute",
						"reset_meters": "Reset Peak Meters",
						"audio":        "Audio Settings",
						"levels":       "Levels",
					},
				},
			},
		},
	},
	2: {
		"slot":              uint32(2),
		"detail_level":      catena.DetailLevelFull,
		"multi_set_enabled": true,
		"subscriptions":     false,
		"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg"},
		"default_scope":     "st2138:op",
		"constraints": map[string]any{
			"input_choice": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(1),
					"max_value": int32(32),
				},
			},
			"output_choice": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(1),
					"max_value": int32(32),
				},
			},
		},
		"params": map[string]any{
			"output_1_source": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Output 1 Source",
					},
				},
				"type": catena.ParamTypeInt32,
				"constraint": map[string]any{
					"ref_oid": "input_choice",
				},
				"widget":    "DROPDOWN",
				"read_only": false,
			},
			"lock_enabled": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Lock Enabled",
					},
				},
				"type":      catena.ParamTypeInt32,
				"widget":    "CHECKBOX",
				"read_only": false,
			},
		},
		"menu_groups": map[string]any{
			"routing": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Routing",
					},
				},
				"menus": map[string]any{
					"outputs": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Output Configuration",
							},
						},
						"param_oids": []string{
							"output_1_source",
						},
					},
					"settings": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Settings",
							},
						},
						"param_oids": []string{
							"lock_enabled",
						},
					},
				},
			},
		},
		"commands": map[string]any{
			"take": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Take (Execute Salvo)",
					},
				},
				"type": catena.ParamTypeEmpty,
			},
			"clear_locks": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Clear All Locks",
					},
				},
				"type": catena.ParamTypeEmpty,
			},
		},
		"language_packs": map[string]any{
			"packs": map[string]any{
				"en": map[string]any{
					"name": "English",
					"words": map[string]string{
						"output_1_source": "Output 1 Source",
						"lock_enabled":    "Lock Enabled",
						"take":            "Take (Execute Salvo)",
						"clear_locks":     "Clear All Locks",
						"routing":         "Routing",
						"outputs":         "Output Configuration",
						"settings":        "Settings",
					},
				},
			},
		},
	},
}

// RegisterHandlers registers the GetDevice handler for each slot on the given server.
// It also starts a heartbeat that broadcasts the version param every 5 seconds.
func RegisterHandlers(srv catena.Server) {
	for _, slot := range SlotList {
		slot := slot
		srv.RegisterGetDeviceHandler(slot, func() (catena.Device, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			deviceInfo, ok := Devices[slot]
			if !ok {
				logger.Error("GetDevice device not found", "slot", slot)
				return catena.ReplyError[catena.Device](catena.NOT_FOUND, "device not found")
			}

			device, err := catena.ToDevice(deviceInfo)
			if err != nil {
				logger.Error("failed to convert device", "slot", slot, "error", err)
				return catena.ReplyError[catena.Device](catena.INTERNAL, "failed to convert device")
			}

			return catena.Reply(device)
		})
	}

	// Start heartbeat — invokes all registered heartbeat handlers every 5 seconds.
	srv.RegisterHeartbeatHandler(0, func(slot uint16) {
		srv.BroadcastUpdate(slot, "product/version", "1.0.0")
	})
	srv.StartHeartbeat(5 * time.Second)
}

// RunExample encapsulates the full example lifecycle:
// SDK init, signal handling, server creation, handler registration, and graceful shutdown.
func RunExample(appName string) {
	exampleutil.RunExample(exampleutil.RunConfig{
		AppName:          appName,
		Slots:            SlotList,
		RegisterHandlers: RegisterHandlers,
		OnReady: func(cfg catena.Config) {
			logger.Info("=======================================================")
			logger.Info("GetDevice Example", "transport", appName)
			logger.Info("=======================================================")
			logger.Info("Listening", "port", cfg.Port)
			logger.Info("")
			logger.Info("Available devices:")
			logger.Info("  Primary Video Processor (slot 0)")
			logger.Info("  Audio Mixer (slot 1)")
			logger.Info("  Router Controller (slot 2)")
			logger.Info("=======================================================")
		},
	})
}
