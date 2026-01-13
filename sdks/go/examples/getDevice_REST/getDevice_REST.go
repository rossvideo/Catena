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
	"strconv"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "get_device_example"

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

	// Define device metadata for multiple slots
	devices := map[int]map[string]any{
		0: {
			"slot":              0,
			"name":              "Primary Video Processor",
			"product":           "VP-2000",
			"version":           "1.2.3",
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
			"capabilities": map[string]any{
				"max_channels":   8,
				"supports_4k":    true,
				"supports_hdr":   true,
				"input_formats":  []string{"SDI", "HDMI", "IP"},
				"output_formats": []string{"SDI", "HDMI"},
			},
		},
		1: {
			"slot":              1,
			"name":              "Audio Mixer",
			"product":           "AM-500",
			"version":           "2.0.1",
			"multi_set_enabled": false,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op"},
			"default_scope":     "st2138:mon",
			"capabilities": map[string]any{
				"max_inputs":    16,
				"max_outputs":   8,
				"sample_rates":  []int{44100, 48000, 96000},
				"bit_depths":    []int{16, 24, 32},
				"supports_aes3": true,
			},
		},
		2: {
			"slot":              2,
			"name":              "Router Controller",
			"product":           "RC-100",
			"version":           "3.1.0",
			"multi_set_enabled": true,
			"subscriptions":     false,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg"},
			"default_scope":     "st2138:op",
			"capabilities": map[string]any{
				"matrix_size":    "32x32",
				"supports_locks": true,
				"supports_salvo": true,
			},
		},
	}

	slotList := []int{0, 1, 2}
	srv := rest.NewServer(slotList)

	// Register GetDevice handler for each slot
	for _, slot := range slotList {
		srv.RegisterGetDeviceHandler(slot, func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
			// Parse slot from URL path: /st2138-api/v1/{slot}
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
	}

	// Register not-found handler
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		logger.Warning("Endpoint not found", "method", r.Method, "path", r.URL.Path)
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	logger.Info("GetDevice Example listening", "port", port)
	srv.StartHTTPServer(port)

	select {} // Block forever
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

