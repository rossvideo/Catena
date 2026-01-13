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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Example program containing one of everything.
 * @file one_of_everything_REST.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-13
 */

package main

import (
	"bytes"
	"encoding/base64"
	"fmt"
	"io"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// Asset represents a binary asset with metadata
type Asset struct {
	ContentType string
	Data        []byte
	Filename    string
}

// CommandHandler processes a command and returns a response
type CommandHandler func(payload any) catena.StatusResult

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "all_endpoints_example"

	if err := logger.Init(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
	defer logger.Close()

	portStr := envOr("CATENA_PORT", "6254")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		logger.Error("invalid CATENA_PORT", "error", err)
		os.Exit(1)
	}

	// ==========================================================================
	// Device Metadata (for GetDevice endpoint)
	// ==========================================================================
	devices := map[int]map[string]any{
		0: {
			"slot":              0,
			"name":              "Video Processor",
			"product":           "VP-3000",
			"version":           "2.1.0",
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"capabilities": map[string]any{
				"max_channels":   16,
				"supports_4k":    true,
				"supports_8k":    true,
				"supports_hdr":   true,
				"input_formats":  []string{"SDI", "HDMI", "IP", "NDI"},
				"output_formats": []string{"SDI", "HDMI", "IP"},
			},
		},
	}

	// ==========================================================================
	// Parameters (for GetParam/SetParam endpoints)
	// ==========================================================================
	params := &sync.Map{}

	// Video parameters
	params.Store("video/resolution", map[string]any{
		"value": map[string]any{"string_value": "3840x2160"},
	})
	params.Store("video/brightness", map[string]any{
		"value": map[string]any{"int32_value": 50},
	})
	params.Store("video/counter", map[string]any{
		"value": map[string]any{"int32_value": 0},
	})

	// Audio parameters
	params.Store("audio/volume", map[string]any{
		"value": map[string]any{"float32_value": 0.8},
	})
	params.Store("audio/muted", map[string]any{
		"value": map[string]any{"int32_value": 0},
	})

	// System parameters
	params.Store("system/device_name", map[string]any{
		"value": map[string]any{"string_value": "Production Studio A"},
	})

	// Network parameters
	params.Store("network/ip_address", map[string]any{
		"value": map[string]any{"string_value": "192.168.1.100"},
	})

	// ==========================================================================
	// Commands (for ExecuteCommand endpoint)
	// ==========================================================================
	deviceState := &sync.Map{}
	deviceState.Store("running", false)
	deviceState.Store("counter", 0)
	deviceState.Store("preset_index", 0)

	commands := map[string]CommandHandler{
		// Simple commands
		"ping": func(payload any) catena.StatusResult {
			logger.Info("Executing ping command")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"string_value": "pong",
					"timestamp":    time.Now().Unix(),
				},
			})
		},

		"echo": func(payload any) catena.StatusResult {
			logger.Info("Executing echo command", "payload", payload)
			return catena.OK(map[string]any{
				"response": map[string]any{
					"echo": payload,
				},
			})
		},

		"get_status": func(payload any) catena.StatusResult {
			running, _ := deviceState.Load("running")
			counter, _ := deviceState.Load("counter")
			preset, _ := deviceState.Load("preset_index")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"running":      running,
					"counter":      counter,
					"preset_index": preset,
					"uptime":       time.Now().Unix(),
				},
			})
		},

		// State control commands
		"start": func(payload any) catena.StatusResult {
			running, _ := deviceState.Load("running")
			if running.(bool) {
				return catena.OK(map[string]any{
					"exception": map[string]any{
						"error_code":    1001,
						"error_message": "Device already running",
					},
				})
			}
			deviceState.Store("running", true)
			logger.Info("Device started")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"string_value": "started",
				},
			})
		},

		"stop": func(payload any) catena.StatusResult {
			running, _ := deviceState.Load("running")
			if !running.(bool) {
				return catena.OK(map[string]any{
					"exception": map[string]any{
						"error_code":    1002,
						"error_message": "Device not running",
					},
				})
			}
			deviceState.Store("running", false)
			logger.Info("Device stopped")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"string_value": "stopped",
				},
			})
		},

		"reset": func(payload any) catena.StatusResult {
			deviceState.Store("running", false)
			deviceState.Store("counter", 0)
			deviceState.Store("preset_index", 0)
			logger.Info("Device reset")
			return catena.OK(map[string]any{
				"no_response": map[string]any{},
			})
		},

		// Preset commands
		"load_preset": func(payload any) catena.StatusResult {
			if payloadMap, ok := payload.(map[string]any); ok {
				if index, ok := payloadMap["index"]; ok {
					deviceState.Store("preset_index", index)
					logger.Info("Preset loaded", "index", index)
					return catena.OK(map[string]any{
						"response": map[string]any{
							"preset_loaded": index,
						},
					})
				}
			}
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"error_code":    2001,
					"error_message": "Missing preset index",
				},
			})
		},

		// Complex routing command
		"route": func(payload any) catena.StatusResult {
			if payloadMap, ok := payload.(map[string]any); ok {
				source := payloadMap["source"]
				dest := payloadMap["destination"]
				if source != nil && dest != nil {
					logger.Info("Routing", "source", source, "destination", dest)
					return catena.OK(map[string]any{
						"response": map[string]any{
							"routed":      true,
							"source":      source,
							"destination": dest,
						},
					})
				}
			}
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"error_code":    2002,
					"error_message": "Invalid route parameters",
				},
			})
		},
	}

	// ==========================================================================
	// Assets (for GetAsset endpoint)
	// ==========================================================================
	assets := &sync.Map{}

	// Image assets (1x1 PNG pixels)
	redPixelPNG, _ := base64.StdEncoding.DecodeString(
		"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==")
	bluePixelPNG, _ := base64.StdEncoding.DecodeString(
		"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPj/HwADBwIAMCbHYQAAAABJRU5ErkJggg==")

	assets.Store("thumbnail", Asset{ContentType: "image/png", Data: redPixelPNG, Filename: "thumbnail.png"})
	assets.Store("icon", Asset{ContentType: "image/png", Data: bluePixelPNG, Filename: "icon.png"})

	// Configuration assets
	assets.Store("device_config", Asset{
		ContentType: "application/json",
		Data:        []byte(`{"device":"VP-3000","version":"2.1.0","serial":"SN-2026-001234"}`),
		Filename:    "device_config.json",
	})
	assets.Store("preset_default", Asset{
		ContentType: "application/json",
		Data:        []byte(`{"name":"Default","video":{"resolution":"3840x2160","colorspace":"BT.2020"}}`),
		Filename:    "preset_default.json",
	})

	// Documentation
	assets.Store("readme", Asset{
		ContentType: "text/plain",
		Data:        []byte("VP-3000 Video Processor\n=======================\nFirmware: 2.1.0\n\nThis device supports 4K/8K video processing with HDR."),
		Filename:    "README.txt",
	})

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []int{0}
	srv := rest.NewServer(slotList)

	// --------------------------------------------------------------------------
	// Register GetDevice handler
	// --------------------------------------------------------------------------
	srv.RegisterGetDeviceHandler(0, func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			logger.Error("GetDevice invalid path", "path", r.URL.Path)
			return catena.BadRequest("invalid path")
		}
		slotStr := parts[2]
		slot, err := strconv.Atoi(slotStr)
		if err != nil {
			logger.Error("GetDevice invalid slot", "slot", slotStr, "error", err)
			return catena.BadRequest("invalid slot")
		}

		logger.Info("GetDevice", "slot", slot)
		device, ok := devices[slot]
		if !ok {
			logger.Error("GetDevice device not found", "slot", slot)
			return catena.NotFound("device not found")
		}
		return catena.OK(device)
	})

	// --------------------------------------------------------------------------
	// Register GetValue handler (GetParam)
	// --------------------------------------------------------------------------
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) catena.StatusResult {
		logger.Info("GetParam", "slot", slot, "fqoid", fqoid)

		val, ok := params.Load(fqoid)
		if !ok {
			logger.Warning("GetParam not found", "slot", slot, "fqoid", fqoid)
			return catena.NotFound("parameter not found: " + fqoid)
		}

		param := val.(map[string]any)
		logger.Info("GetParam returning", "slot", slot, "fqoid", fqoid, "value", param)
		return catena.OK(param)
	})

	// --------------------------------------------------------------------------
	// Register SetValue handler (SetParam)
	// --------------------------------------------------------------------------
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		logger.Info("SetParam", "slot", slot, "fqoid", fqoid, "value", value)

		// Check if parameter exists
		_, exists := params.Load(fqoid)
		if !exists {
			logger.Warning("SetParam parameter not found", "slot", slot, "fqoid", fqoid)
			return catena.NotFound("parameter not found: " + fqoid)
		}

		// Wrap value in expected format
		var wrappedValue map[string]any
		if valueMap, ok := value.(map[string]any); ok {
			if _, hasValue := valueMap["value"]; hasValue {
				wrappedValue = valueMap
			} else {
				wrappedValue = map[string]any{"value": valueMap}
			}
		} else {
			wrappedValue = map[string]any{"value": value}
		}

		params.Store(fqoid, wrappedValue)
		logger.Info("SetParam stored", "slot", slot, "fqoid", fqoid, "stored_value", wrappedValue)
		return catena.OK(wrappedValue)
	})

	// --------------------------------------------------------------------------
	// Register ExecuteCommand handler
	// --------------------------------------------------------------------------
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) catena.StatusResult {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid, "payload", payload)

		handler, ok := commands[commandFqoid]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"error_code":    404,
					"error_message": "Command not found: " + commandFqoid,
				},
			})
		}

		return handler(payload)
	})

	// --------------------------------------------------------------------------
	// Register GetAsset handler
	// --------------------------------------------------------------------------
	srv.RegisterGetAssetHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult {
		logger.Info("GetAsset", "slot", slot, "fqoid", fqoid)

		val, ok := assets.Load(fqoid)
		if !ok {
			logger.Warning("GetAsset not found", "slot", slot, "fqoid", fqoid)
			return catena.NotFound("asset not found: " + fqoid)
		}

		asset := val.(Asset)

		// Set appropriate headers
		w.Header().Set("Content-Type", asset.ContentType)
		w.Header().Set("Content-Length", strconv.Itoa(len(asset.Data)))
		if asset.Filename != "" {
			w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=%q", asset.Filename))
		}

		// Stream the content
		reader := bytes.NewReader(asset.Data)
		if _, err := io.Copy(w, reader); err != nil {
			logger.Error("GetAsset streaming error", "slot", slot, "fqoid", fqoid, "error", err)
			return catena.InternalServerError("failed to stream asset")
		}

		return catena.StatusResult{Status: http.StatusOK}
	})

	// --------------------------------------------------------------------------
	// Register Connect handler (PLACEHOLDER - not fully implemented)
	// --------------------------------------------------------------------------
	srv.RegisterConnectHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		logger.Info("Connect endpoint called")

		// TODO: 

		return catena.OK(map[string]any{
			"status":  "placeholder",
			"message": "Connect endpoint not fully implemented",
			"info": map[string]any{
				"supported_protocols": []string{"websocket", "sse"},
				"subscriptions":       true,
				"available_slots":     slotList,
			},
		})
	})

	// --------------------------------------------------------------------------
	// Register NotFound handler
	// --------------------------------------------------------------------------
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		logger.Warning("Endpoint not found", "method", r.Method, "path", r.URL.Path)
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	// ==========================================================================
	// Start Server
	// ==========================================================================
	logger.Info("=======================================================")
	logger.Info("All Endpoints REST Example")
	logger.Info("=======================================================")
	logger.Info("Listening", "port", port)
	logger.Info("")
	logger.Info("Available endpoints:")
	logger.Info("  GET  /st2138-api/v1/0              - GetDevice")	
	logger.Info("  GET  /st2138-api/v1/0/value/{oid}  - GetParam")
	logger.Info("  PUT  /st2138-api/v1/0/value/{oid}  - SetParam")
	logger.Info("  POST /st2138-api/v1/0/command/{cmd}- ExecuteCommand")
	logger.Info("  GET  /st2138-api/v1/0/asset/{oid}  - GetAsset")
	logger.Info("  GET  /st2138-api/v1/connect        - Connect (NOT IMPLEMENTED)")
	logger.Info("")
	logger.Info("Example requests:")
	logger.Info("  curl http://localhost:6254/st2138-api/v1/0")
	logger.Info("  curl http://localhost:6254/st2138-api/v1/0/value/video/resolution")
	logger.Info("  curl -X PUT -d '{\"value\":{\"int32_value\":75}}' http://localhost:6254/st2138-api/v1/0/value/video/brightness")
	logger.Info("  curl -X POST http://localhost:6254/st2138-api/v1/0/command/ping")
	logger.Info("  curl -X POST -d '{\"source\":1,\"destination\":2}' http://localhost:6254/st2138-api/v1/0/command/route")
	logger.Info("  curl http://localhost:6254/st2138-api/v1/0/asset/device_config")
	logger.Info("  curl http://localhost:6254/st2138-api/v1/connect")
	logger.Info("")
	logger.Info("Available parameters:")
	logger.Info("  video/resolution, video/brightness, video/counter")
	logger.Info("  audio/volume, audio/muted")
	logger.Info("  system/device_name")
	logger.Info("  network/ip_address")
	logger.Info("")
	logger.Info("Available commands:")
	logger.Info("  ping, echo, get_status, start, stop, reset, load_preset, route")
	logger.Info("")
	logger.Info("Available assets:")
	logger.Info("  thumbnail, icon, device_config, preset_default, readme")
	logger.Info("=======================================================")

	srv.StartHTTPServer(port)
	select {} // Block forever
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
