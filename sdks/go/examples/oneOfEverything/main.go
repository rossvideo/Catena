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
 * @brief Shared oneOfEverything example logic for REST and gRPC.
 * @file oneofeverything.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 */

package main

import (
	"context"
	"embed"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"reflect"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/transports"
)

// normalizeFqoid strips the leading "/" so that handlers can be called from
// either gRPC (where the dashboard sends fully-qualified OIDs like "/brightness")
// or REST (where path-derived OIDs arrive without a leading slash, e.g. "brightness").
func normalizeFqoid(fqoid string) string {
	return strings.TrimPrefix(fqoid, "/")
}

//go:embed static/*
var StaticFS embed.FS

//go:embed webui/*
var webFS embed.FS

type CommandHandler func(payload any) (catena.CommandResult, catena.StatusResult)

type CounterState struct {
	mu      sync.RWMutex
	value   int32
	running bool
}

func (c *CounterState) GetValue() int32 {
	c.mu.RLock()
	defer c.mu.RUnlock()
	return c.value
}

func (c *CounterState) IsRunning() bool {
	c.mu.RLock()
	defer c.mu.RUnlock()
	return c.running
}

// RunningInt32 returns the running state as an int32 (1 = running, 0 = stopped),
// matching the on-the-wire representation of the "running" param.
func (c *CounterState) RunningInt32() int32 {
	if c.IsRunning() {
		return 1
	}
	return 0
}

func (c *CounterState) Start() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.running = true
}

func (c *CounterState) Stop() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.running = false
}

func (c *CounterState) Add(n int32) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.value += n
}

func (c *CounterState) Reset() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.value = 0
}

func (c *CounterState) Increment() {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.running {
		c.value++
	}
}

var slotList = []uint16{0, 1, 2}

// int32ParamValue returns a Catena value{} map containing the int32 currently
// stored at key in p, or fallback if the key is missing or the wrong type.
// Used by BuildDevices so slot descriptors serve current values at GetDevice
// time rather than init-time constants.
func int32ParamValue(p *sync.Map, key string, fallback int32) map[string]any {
	v := fallback
	if raw, ok := p.Load(key); ok {
		if iv, ok := raw.(int32); ok {
			v = iv
		}
	}
	return map[string]any{"int32_value": v}
}

// stringParamValue is the string analogue of int32ParamValue.
func stringParamValue(p *sync.Map, key string, fallback string) map[string]any {
	v := fallback
	if raw, ok := p.Load(key); ok {
		if sv, ok := raw.(string); ok {
			v = sv
		}
	}
	return map[string]any{"string_value": v}
}

// BuildDevices returns the slot -> device-descriptor map. It is a function so
// every param's "value" can be plugged in live at GetDevice time.
func BuildDevices(counter *CounterState, slotParams map[uint16]*sync.Map) map[uint16]map[string]any {
	s1 := slotParams[1]
	s2 := slotParams[2]
	return map[uint16]map[string]any{
		0: {
			"slot":              uint32(0),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"params": map[string]any{
				"counter": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Counter",
						},
					},
					"type": catena.ParamTypeInt32,
					"value": map[string]any{
						"int32_value": counter.GetValue(),
					},
				},
				"running": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Counter Running Status",
						},
					},
					"type":      catena.ParamTypeInt32,
					"read_only": true,
					"value": map[string]any{
						"int32_value": counter.RunningInt32(),
					},
					"constraint": map[string]any{
						"type": "INT_CHOICE",
						"int32_choice": map[string]any{
							"choices": []map[string]any{
								{
									"value": int32(0),
									"name": map[string]any{
										"display_strings": map[string]string{
											"en": "Not Counting",
										},
									},
								},
								{
									"value": int32(1),
									"name": map[string]any{
										"display_strings": map[string]string{
											"en": "Counting",
										},
									},
								},
							},
						},
					},
				},
			},
			"commands": map[string]any{
				"start": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Start Counter",
						},
					},
					"type": catena.ParamTypeEmpty,
				},
				"stop": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Stop Counter",
						},
					},
					"type": catena.ParamTypeEmpty,
				},
				"add10": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Add 10 to Counter",
						},
					},
					"type": catena.ParamTypeEmpty,
				},
				"reset": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Reset Counter",
						},
					},
					"type": catena.ParamTypeEmpty,
				},
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
							"param_oids": []string{"counter", "running"},
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
		},
		1: {
			"slot":              uint32(1),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": false,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"params": map[string]any{
				"product": map[string]any{
					"type":      catena.ParamTypeStruct,
					"read_only": true,
					"params": map[string]any{
						"name": map[string]any{
							"type": catena.ParamTypeString,
						},
						"vendor": map[string]any{
							"type": catena.ParamTypeString,
						},
						"version": map[string]any{
							"type": catena.ParamTypeString,
						},
						"catena_sdk": map[string]any{
							"type": catena.ParamTypeString,
						},
						"catana_sdk_version": map[string]any{
							"type": catena.ParamTypeString,
						},
						"serial_number": map[string]any{
							"type": catena.ParamTypeString,
						},
					},
					"value": map[string]any{
						"struct_value": map[string]any{
							"fields": map[string]any{
								"name": map[string]any{
									"string_value": "One of Everything Demo",
								},
								"vendor": map[string]any{
									"string_value": "Ross Video",
								},
								"version": map[string]any{
									"string_value": "1.0.0",
								},
								"catena_sdk": map[string]any{
									"string_value": "github.com/rossvideo/catena/sdks/go",
								},
								"catana_sdk_version": map[string]any{
									"string_value": "v0.1.0", // TODO: SDK should expose
								},
								"serial_number": map[string]any{
									"string_value": "SN12345678",
								},
							},
						},
					},
				},
				"resolution": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Resolution",
						},
					},
					"type":  catena.ParamTypeString,
					"value": stringParamValue(s1, "resolution", "1920x1080"),
				},
				"brightness": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Brightness",
						},
					},
					"type":  catena.ParamTypeInt32,
					"value": int32ParamValue(s1, "brightness", 50),
				},
				"contrast": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Contrast",
						},
					},
					"type":  catena.ParamTypeInt32,
					"value": int32ParamValue(s1, "contrast", 50),
				},
				"saturation": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Saturation",
						},
					},
					"type":  catena.ParamTypeInt32,
					"value": int32ParamValue(s1, "saturation", 50),
				},
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
		},
		2: {
			"slot":              uint32(2),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": true,
			"subscriptions":     false,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"params": map[string]any{
				"volume": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Volume",
						},
					},
					"type":  catena.ParamTypeInt32,
					"value": int32ParamValue(s2, "volume", 75),
				},
				"muted": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Muted",
						},
					},
					"type":  catena.ParamTypeInt32,
					"value": int32ParamValue(s2, "muted", 0),
				},
				"device_name": map[string]any{
					"name": map[string]any{
						"display_strings": map[string]string{
							"en": "Device Name",
						},
					},
					"type":  catena.ParamTypeString,
					"value": stringParamValue(s2, "device_name", "Demo Device"),
				},
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
							"param_oids": []string{"device_name"},
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
		},
	}
}

func main() {
	srv := catena.NewServer(100)
	counter := &CounterState{}
	slotParams := map[uint16]*sync.Map{
		0: {},
		1: {},
		2: {},
	}
	slotParams[0].Store("counter", int32(0))
	slotParams[1].Store("resolution", "1920x1080")
	slotParams[1].Store("brightness", int32(50))
	slotParams[1].Store("contrast", int32(50))
	slotParams[1].Store("saturation", int32(50))
	slotParams[2].Store("volume", int32(75))
	slotParams[2].Store("muted", int32(0))
	slotParams[2].Store("device_name", "Demo Device")

	go func() {
		ticker := time.NewTicker(1 * time.Second)
		defer ticker.Stop()
		for range ticker.C {
			if counter.IsRunning() {
				counter.Increment()
				logger.Info("Counter tick", "value", counter.GetValue())
				srv.BroadcastUpdate(0, "counter", counter.GetValue())
			}
		}
	}()
	counter.Start()

	// broadcastRunning publishes the current running flag on the "running" param
	// so subscribers (REST SSE / gRPC stream) see the state change live.
	broadcastRunning := func() {
		srv.BroadcastUpdate(0, "running", counter.RunningInt32())
	}

	commands := map[string]CommandHandler{
		"start": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if counter.IsRunning() {
				logger.Info("Start command - already running")
			} else {
				counter.Start()
				logger.Info("Counter started", "value", counter.GetValue())
				broadcastRunning()
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(counter.GetValue())
			return catena.CommandReply(val)
		},

		"stop": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
			} else {
				counter.Stop()
				logger.Info("Counter stopped", "value", counter.GetValue())
				broadcastRunning()
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(counter.GetValue())
			return catena.CommandReply(val)
		},

		"add10": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(counter.GetValue())
			return catena.CommandReply(val)
		},

		"reset": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(counter.GetValue())
			return catena.CommandReply(val)
		},
	}

	assets := &sync.Map{}
	payloads, err := catena.LoadPayloadsFromEmbed(StaticFS, "static")
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to load embedded assets: %v\n", err)
		os.Exit(1)
	}
	for id, payload := range payloads {
		assets.Store(id, payload)
	}

	for _, slot := range slotList {
		srv.RegisterGetDeviceHandler(slot, func() (catena.CatenaDevice, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)

			// Build per-request so every param's value reflects current state:
			deviceInfo, ok := BuildDevices(counter, slotParams)[slot]
			if !ok {
				return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
			}
			device, err := catena.ToCatenaDevice(deviceInfo)
			if err != nil {
				return catena.ReplyError[catena.CatenaDevice](catena.INTERNAL, err.Error())
			}
			return catena.Reply(device)
		})
	}

	for _, slot := range slotList {
		p := slotParams[slot]

		srv.RegisterGetValueHandler(slot, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
			logger.Info("GetValue", "slot", slot, "fqoid", fqoid)
			key := normalizeFqoid(fqoid)

			if slot == 0 && key == "counter" {
				val, res := catena.ToCatenaValue(counter.GetValue())
				if res.Code != catena.OK {
					return catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to convert counter value")
				}
				return catena.Reply(val)
			}

			// "running" is a status param backed by CounterState; report the
			// live state instead of any cached slot-param value.
			if slot == 0 && key == "running" {
				val, res := catena.ToCatenaValue(counter.RunningInt32())
				if res.Code != catena.OK {
					return catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to convert running value")
				}
				return catena.Reply(val)
			}

			v, ok := p.Load(key)
			if !ok {
				return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "parameter not found: "+fqoid)
			}
			catenaVal, res := catena.ToCatenaValue(v)
			if res.Code != catena.OK {
				return catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to convert value")
			}
			return catena.Reply(catenaVal)
		})
	}

	for _, slot := range slotList {
		p := slotParams[slot]

		srv.RegisterSetValueHandler(slot, func(value any, slot uint16, fqoid string) catena.StatusResult {
			logger.Info("SetValue", "slot", slot, "fqoid", fqoid, "value", value)
			key := normalizeFqoid(fqoid)

			if value == nil {
				logger.Error("SetValue nil value received", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.INVALID_ARGUMENT, "nil value received")
			}

			// "running" is driven by start/stop commands, not direct writes,
			// so reject SetValue with a clear hint to avoid silent drift
			// between CounterState and the slot-param cache.
			if slot == 0 && key == "running" {
				logger.Warning("SetValue rejected for running param", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.INVALID_ARGUMENT, "use start/stop commands to change running state")
			}

			val, ok := p.Load(key)
			if !ok {
				logger.Error("SetValue param not found", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.NOT_FOUND, "param not found: "+fqoid)
			}

			if reflect.TypeOf(val) != reflect.TypeOf(value) {
				logger.Error("SetValue type mismatch", "slot", slot, "fqoid", fqoid,
					"expected", reflect.TypeOf(val), "got", reflect.TypeOf(value))
				return catena.StatusWithCode(catena.INVALID_ARGUMENT, "type mismatch")
			}

			p.Store(key, value)
			logger.Info("Parameter updated", "fqoid", fqoid, "value", value)
			srv.BroadcastUpdate(slot, fqoid, value)
			return catena.StatusWithCode(catena.OK, "")
		})
	}

	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)
		key := normalizeFqoid(commandFqoid)

		handler, ok := commands[key]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.CommandExceptionResult("Invalid Command", "Command not found: "+commandFqoid, nil)
		}

		return handler(payload)
	})

	for _, slot := range slotList {
		srv.RegisterGetAssetHandler(slot, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
			logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)
			key := normalizeFqoid(fqoid)

			val, ok := assets.Load(key)
			if !ok {
				logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
				return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found: "+fqoid)
			}

			payload := val.(catena.DataPayload)

			catenaAsset, res := catena.ToCatenaAsset(payload, true)
			if res.Code != catena.OK {
				logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", res.Error)
				return catena.ReplyError[catena.CatenaAsset](catena.INTERNAL, "failed to convert asset: "+res.Error)
			}

			logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
			return catena.Reply(catenaAsset)
		})
	}

	for _, slot := range slotList {
		srv.RegisterParamInfoHandler(slot, func(slot uint16, fqoid string, recursive bool) ([]catena.CatenaParamInfo, catena.StatusResult) {
			if recursive {
				// TODO: implement recursive param info retrieval in example
				return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.UNIMPLEMENTED, "recursive param info not implemented in example")
			}
			logger.Info("GetParamInfo", "slot", slot, "fqoid", fqoid)
			key := normalizeFqoid(fqoid)
			pathParts := strings.Split(key, "/")

			deviceInfo, ok := BuildDevices(counter, slotParams)[slot]
			if !ok {
				return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.NOT_FOUND, "device not found")
			}

			params, ok := deviceInfo["params"].(map[string]any)
			if !ok {
				return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.INTERNAL, "invalid device params structure")
			}

			var paramInfo map[string]any
			found := false
			for _, part := range pathParts {
				if params == nil {
					return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.NOT_FOUND, "param not found: "+fqoid)
				}
				nextParam, exists := params[part]
				if !exists {
					return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.NOT_FOUND, "param not found: "+fqoid)
				}

				paramInfo, ok = nextParam.(map[string]any)
				if !ok {
					return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.NOT_FOUND, "param not found: "+fqoid)
				}
				found = true

				if nested, hasNested := paramInfo["params"]; hasNested {
					params, _ = nested.(map[string]any)
				} else {
					params = nil
				}
			}

			if !found {
				return []catena.CatenaParamInfo{}, catena.StatusWithCode(catena.NOT_FOUND, "param not found: "+fqoid)
			}

			oid := pathParts[len(pathParts)-1]
			name := catena.PolyglotText{}
			if paramInfo["name"] != nil {
				for lang, text := range paramInfo["name"].(map[string]any)["display_strings"].(map[string]string) {
					name.With(lang, text)
				}
			}
			catenaParamInfo := catena.NewParamInfo(oid, name, paramInfo["type"].(catena.ParamType), "", 0)
			return []catena.CatenaParamInfo{catenaParamInfo}, catena.StatusWithCode(catena.OK, "")
		})
	}

	// counter ticks often enough in slot 0 that a heartbeat isn't needed.

	srv.RegisterHeartbeatHandler(1, func(slot uint16) {
		// example ticking on the cannonical "product/version" param
		srv.BroadcastUpdate(1, "product/version", "1.0.0")
	})
	srv.RegisterHeartbeatHandler(2, func(slot uint16) {
		// example ticking on a different param to show any can be used for heartbeats.
		if val, ok := slotParams[2].Load("volume"); ok {
			srv.BroadcastUpdate(2, "volume", val)
		} else {
			logger.Warning("Volume param missing at heartbeat", "slot", slot)
		}
	})

	config, err := catena.InitOptions()
	if err != nil {
		logger.Error("Failed to initialize Catena SDK", "error", err)
		os.Exit(1)
	}

	if !config.UseGrpc && !config.UseRest {
		logger.Error("No transports enabled", "error", "at least one of gRPC or REST transport must be enabled in config")
		os.Exit(1)
	}

	logger.Info("=======================================================")
	logger.Info("One of Everything gRPC Example")
	logger.Info("=======================================================")

	// register the transports we want to serve on.
	if config.UseGrpc {
		logger.Info("gRPC transport starting")
		logger.Info("")
		// Register gRPC transport if enabled in config
		if err := srv.RegisterTransport(transports.NewDefaultGrpcTransport()); err != nil {
			logger.Error("Failed to register gRPC transport", "error", err)
			os.Exit(1)
		}
		logger.Info("Use grpcurl or a gRPC client to interact with the server:")
		logger.Info("  grpcurl -plaintext localhost:6254 list")
	} else {
		logger.Info("gRPC transport disabled by config")
	}
	logger.Info("")
	logger.Info("=======================================================")

	if config.UseRest {
		logger.Info("REST transport starting")
		logger.Info("")
		restTransport := transports.NewDefaultRestTransport()
		if err := srv.RegisterTransport(restTransport); err != nil {
			logger.Error("Failed to register REST transport", "error", err)
			os.Exit(1)
		}

		restTransport.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
			if r.URL.Path == "/assets-list" {
				var assetList []map[string]any
				assets.Range(func(key, value any) bool {
					payload := value.(catena.DataPayload)
					assetList = append(assetList, map[string]any{
						"id":           key.(string),
						"content_type": payload.Metadata["content-type"],
						"file_name":    payload.Metadata["file-name"],
						"size":         len(payload.Payload),
					})
					return true
				})
				w.Header().Set("Content-Type", "application/json")
				json.NewEncoder(w).Encode(assetList)
				return catena.Reply(catena.CatenaValue{})
			}

			fileMap := map[string]struct {
				path        string
				contentType string
			}{
				"/":           {"webui/index.htm", "text/html; charset=utf-8"},
				"/styles.css": {"webui/styles.css", "text/css; charset=utf-8"},
				"/script.js":  {"webui/script.js", "application/javascript; charset=utf-8"},
			}

			if file, ok := fileMap[r.URL.Path]; ok {
				data, err := webFS.ReadFile(file.path)
				if err != nil {
					return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "file not found: "+r.URL.Path)
				}
				w.Header().Set("Content-Type", file.contentType)
				w.Write(data)
				return catena.Reply(catena.CatenaValue{})
			}
			return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found: "+r.URL.Path)
		})

		logger.Info("Web UI available at:")
		logger.Info("  http://localhost:8080/")
	} else {
		logger.Info("REST transport disabled by config")
	}
	logger.Info("")
	logger.Info("=======================================================")

	// setup
	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()

	<-ctx.Done()
	logger.Info("Shutting down server...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), time.Second)
	defer shutdownCancel()
	srv.Shutdown(shutdownCtx)
	logger.Info("Server shutdown complete")
}
