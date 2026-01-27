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
 * @brief GetDevice REST example.
 * @file getDevice_REST.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-12
 */

package main

import (
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"strconv"
	"syscall"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	srv          *rest.Server
)

func main() {
	// Initialize SDK with prefix and app name
	cfg, err := catena.InitOptions(catena.Options{AppName: "getDevice_REST"})
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize SDK: %v\n", err)
		os.Exit(1)
	}
	defer catena.Close()

	// Handle signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	// Port comes from the unified config (parsed from CATENA_PORT)
	port := cfg.Port

	// Define device metadata for multiple slots
	devices := map[int]map[string]any{
		0: {
			"slot":              uint32(0),
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"params": map[string]any{
				"brightness": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Brightness",
						},
					},
					"type": int32(4), // INT32
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
					"type": int32(4), // INT32
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
					"type": int32(7), // STRING
					"constraint": map[string]any{
						"ref_oid": "input_source_choice",
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
				"video": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Video Settings",
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
				"system": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "System",
						},
					},
					"menus": map[string]any{
						"info": map[string]any{
							"name": map[string]any{
								"display_strings": map[string]string{
									"en": "Device Info",
								},
							},
							"param_oids": []string{},
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
					"type": int32(1), // EMPTY (command with no args)
				},
				"reset": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Reset to Defaults",
							"fr": "Réinitialiser aux valeurs par défaut",
						},
					},
					"type": int32(1), // EMPTY
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
							"video":        "Video Settings",
							"basic":        "Basic",
							"input":        "Input Configuration",
							"system":       "System",
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
							"reboot":       "Redémarrer l'appareil",
							"reset":        "Réinitialiser aux valeurs par défaut",
						},
					},
				},
			},
		},
		1: {
			"slot":              uint32(1),
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
					"type": int32(6), // FLOAT32
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
					"type": int32(6), // FLOAT32
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
					"type":      int32(4), // INT32 (treating boolean as int32)
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
					"type": int32(1), // EMPTY
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
					"type": int32(4), // INT32
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
					"type":      int32(4), // INT32 (treating boolean as int32)
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
					"type": int32(1), // EMPTY
				},
				"clear_locks": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Clear All Locks",
						},
					},
					"type": int32(1), // EMPTY
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

	slotList := []int{0, 1, 2}
	srv := rest.NewServer(slotList)

	// Register GetDevice handler for each slot
	for _, slot := range slotList {
		slot := slot
		srv.RegisterGetDeviceHandler(slot, func() (catena.CatenaDevice, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			_, ok := devices[slot]
			if !ok {
				logger.Error("GetDevice device not found", "slot", slot)
				return catena.ReplyDeviceNotFound("device not found")
			}

			device, err := catena.ToCatenaDevice(devices[slot])
			if err != nil {
				logger.Error("failed to convert device", "slot", slot, "error", err)
				return catena.ReplyDeviceInternalError("failed to convert device")
			}

			return catena.ReplyDevice(device)
		})
	}

	// Register fallback handler
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		logger.Warning("Endpoint not found")
		return catena.ReplyNotFound("endpoint not found")
	})

	// Logger info about the example
	logger.Info("=======================================================")
	logger.Info("GetDevice Example")
	logger.Info("=======================================================")
	logger.Info("Listening", "port", port)
	logger.Info("")
	logger.Info("Available devices:")
	logger.Info("  Primary Video Processor (slot 0)")
	logger.Info("  Audio Mixer (slot 1)")
	logger.Info("  Router Controller (slot 2)")
	logger.Info("=======================================================")

	// Start server in a goroutine so shutdown handling works
	go func() {
		if err := srv.Start(port); err != nil {
			logger.Error("server failed", "error", err)
			os.Exit(1)
		}
	}()

	// Wait for shutdown signal
	<-shutdownChan
	logger.Info("Server shutdown complete")
}
