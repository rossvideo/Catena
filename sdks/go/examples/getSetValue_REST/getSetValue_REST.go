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
 * @brief GetParam and SetParam REST example.
 * @file param_REST.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-19
 */

package main

import (
	"fmt"
	"net/http"
	"os"
	"strconv"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "param_example"

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
	// Slot 0: Video Parameters
	// ==========================================================================
	videoParams := &sync.Map{}
	videoParams.Store("resolution", map[string]any{
		"value": map[string]any{
			"string_value": "1920x1080",
		},
	})
	videoParams.Store("framerate", map[string]any{
		"value": map[string]any{
			"float32_value": 59.94,
		},
	})
	videoParams.Store("brightness", map[string]any{
		"value": map[string]any{
			"int32_value": 50,
		},
	})
	videoParams.Store("counter", map[string]any{
		"value": map[string]any{
			"int32_value": 0,
		},
	})

	// ==========================================================================
	// Slot 1: Audio Parameters
	// ==========================================================================
	audioParams := &sync.Map{}
	audioParams.Store("sample_rate", map[string]any{
		"value": map[string]any{
			"int32_value": 48000,
		},
	})
	audioParams.Store("volume", map[string]any{
		"value": map[string]any{
			"float32_value": 0.75,
		},
	})
	audioParams.Store("muted", map[string]any{
		"value": map[string]any{
			"int32_value": 0,
		},
	})

	// ==========================================================================
	// Slot 2: Network Parameters
	// ==========================================================================
	networkParams := &sync.Map{}
	networkParams.Store("ip_address", map[string]any{
		"value": map[string]any{
			"string_value": "192.168.1.100",
		},
	})
	networkParams.Store("port", map[string]any{
		"value": map[string]any{
			"int32_value": 5000,
		},
	})
	networkParams.Store("device_name", map[string]any{
		"value": map[string]any{
			"string_value": "Catena Device",
		},
	})
	networkParams.Store("standby_mode", map[string]any{
		"value": map[string]any{
			"int32_value": 0,
		},
	})

	slotList := []int{0, 1, 2}
	srv := rest.NewServer(slotList)

	paramStores := map[int]*sync.Map{
		0: videoParams,
		1: audioParams,
		2: networkParams,
	}

	slotDescriptions := map[int]string{
		0: "Video Parameters (resolution, framerate, brightness, counter)",
		1: "Audio Parameters (sample_rate, volume, muted)",
		2: "Network Parameters (ip_address, port, device_name, standby_mode)",
	}

	for slot := range paramStores {
		slot := slot
		store := paramStores[slot]
		desc := slotDescriptions[slot]

		// Register GetValue handler (GET /value/{fqoid})
		srv.RegisterGetValueHandler(slot, func(slotNum int, fqoid string) catena.StatusResult {
			logger.Info("GetParam", "slot", slotNum, "fqoid", fqoid, "type", desc)

			val, ok := store.Load(fqoid)
			if !ok {
				logger.Warning("GetParam not found", "slot", slotNum, "fqoid", fqoid)
				return catena.NotFound("parameter not found: " + fqoid)
			}

			param := val.(map[string]any)
			logger.Info("GetParam returning", "slot", slotNum, "fqoid", fqoid, "value", param)
			return catena.OK(param)
		})

		// Register SetValue handler (PUT /value/{fqoid})
		srv.RegisterSetValueHandler(slot, func(value any, slotNum int, fqoid string) catena.StatusResult {
			logger.Info("SetParam", "slot", slotNum, "fqoid", fqoid, "value", value, "type", desc)

			// Check if parameter exists (only allow setting existing parameters)
			_, exists := store.Load(fqoid)
			if !exists {
				logger.Warning("SetParam parameter not found", "slot", slotNum, "fqoid", fqoid)
				return catena.NotFound("parameter not found: " + fqoid)
			}

			// Store the new value - wrap it in the expected format if needed
			var wrappedValue map[string]any
			if valueMap, ok := value.(map[string]any); ok {
				// Check if value is already wrapped
				if _, hasValue := valueMap["value"]; hasValue {
					wrappedValue = valueMap
				} else {
					// Wrap the value
					wrappedValue = map[string]any{"value": valueMap}
				}
			} else {
				// Wrap non-map values
				wrappedValue = map[string]any{"value": value}
			}

			store.Store(fqoid, wrappedValue)
			logger.Info("SetParam stored", "slot", slotNum, "fqoid", fqoid, "stored_value", wrappedValue)

			// Return the updated value
			return catena.OK(wrappedValue)
		})
	}

	// Not found handler
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	// Logger info about the example
	logger.Info("=======================================================")
	logger.Info("GetParam & SetParam Example")
	logger.Info("=======================================================")
	logger.Info("Listening", "port", port)
	logger.Info("")
	logger.Info("Available parameters:")
	logger.Info("  Slot 0: resolution, framerate, brightness, counter")
	logger.Info("  Slot 1: sample_rate, volume, muted")
	logger.Info("  Slot 2: ip_address, port, device_name, standby_mode")
	logger.Info("=======================================================")

	srv.StartHTTPServer(port)
	select {}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
