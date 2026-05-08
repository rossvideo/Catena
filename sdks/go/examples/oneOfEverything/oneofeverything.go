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
 */

package oneofeverything

import (
	"embed"
	"fmt"
	"os"
	"reflect"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/examples/exampleutil"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// normalizeFqoid strips the leading "/" so that handlers can be called from
// either gRPC (where the dashboard sends fully-qualified OIDs like "/brightness")
// or REST (where path-derived OIDs arrive without a leading slash, e.g. "brightness").
func normalizeFqoid(fqoid string) string {
	return strings.TrimPrefix(fqoid, "/")
}

//go:embed static/*
var StaticFS embed.FS

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
			"counter": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Counter",
					},
				},
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 0,
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
						"param_oids": []string{"counter"},
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
			"resolution": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Resolution",
					},
				},
				"type": catena.ParamTypeString,
				"value": map[string]any{
					"string_value": "1920x1080",
				},
			},
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 50,
				},
			},
			"contrast": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Contrast",
					},
				},
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 50,
				},
			},
			"saturation": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Saturation",
					},
				},
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 50,
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
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 75,
				},
			},
			"muted": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Muted",
					},
				},
				"type": catena.ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 0,
				},
			},
			"device_name": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Device Name",
					},
				},
				"type": catena.ParamTypeString,
				"value": map[string]any{
					"string_value": "Demo Device",
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

func InitSlotParams() map[uint16]*sync.Map {
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
	return slotParams
}

// BuildCounterCommandResponse builds the command-result payload that includes
// both the current counter value and the running flag. It is used as the
// response body for start/stop/add10/reset commands so callers can observe
// the side effects of a command in a single round trip. The "counter" param
// itself is a plain int32 — see the GetValue handler and the broadcasts below.
func BuildCounterCommandResponse(counter *CounterState) map[string]any {
	var runningVal int32 = 0
	if counter.IsRunning() {
		runningVal = 1
	}
	return map[string]any{
		"counter": int32(counter.GetValue()),
		"running": runningVal,
	}
}

// RegisterHandlers registers all GetDevice, GetValue, SetValue, ExecuteCommand,
// and GetAsset handlers. It also starts a background goroutine that increments
// the counter every second while running.
func RegisterHandlers(srv catena.CatenaServer) {
	counter := &CounterState{}
	slotParams := InitSlotParams()

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

	commands := map[string]CommandHandler{
		"start": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if counter.IsRunning() {
				logger.Info("Start command - already running")
			} else {
				counter.Start()
				logger.Info("Counter started", "value", counter.GetValue())
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(BuildCounterCommandResponse(counter))
			return catena.CommandReply(val)
		},

		"stop": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
			} else {
				counter.Stop()
				logger.Info("Counter stopped", "value", counter.GetValue())
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(BuildCounterCommandResponse(counter))
			return catena.CommandReply(val)
		},

		"add10": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(BuildCounterCommandResponse(counter))
			return catena.CommandReply(val)
		},

		"reset": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue())
			val, _ := catena.ToCatenaValue(BuildCounterCommandResponse(counter))
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

	for _, slot := range SlotList {
		slot := slot
		srv.RegisterGetDeviceHandler(slot, func() (catena.CatenaDevice, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			deviceInfo, ok := Devices[slot]
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

	for _, slot := range SlotList {
		slot := slot
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

	for _, slot := range SlotList {
		slot := slot
		p := slotParams[slot]

		srv.RegisterSetValueHandler(slot, func(value any, slot uint16, fqoid string) catena.StatusResult {
			logger.Info("SetValue", "slot", slot, "fqoid", fqoid, "value", value)
			key := normalizeFqoid(fqoid)

			if value == nil {
				logger.Error("SetValue nil value received", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.INVALID_ARGUMENT, "nil value received")
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

	for _, slot := range SlotList {
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
}

// RunExample encapsulates the full example lifecycle:
// SDK init, signal handling, server creation, handler registration, and graceful shutdown.
func RunExample(appName string, makeServer func(slots []uint16, cfg catena.Config) catena.CatenaServer, onReady func(port int)) {
	exampleutil.RunExample(exampleutil.RunConfig{
		AppName:          appName,
		Slots:            SlotList,
		MakeServer:       makeServer,
		RegisterHandlers: RegisterHandlers,
		OnReady:          onReady,
	})
}
