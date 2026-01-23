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
 * @file main.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-13
 */

package main

import (
	"bytes"
	"embed"
	"fmt"
	"io"
	"io/fs"
	"mime"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

//go:embed static/*
var staticFS embed.FS

// Asset represents a binary asset with metadata
type Asset struct {
	ContentType string
	Data        []byte
	Filename    string
}

// CommandHandler processes a command and returns a response
type CommandHandler func(payload any) (catena.CatenaValue, catena.StatusResult)

// Counter state with thread-safe operations
type CounterState struct {
	mu      sync.RWMutex
	value   int64
	running bool
}

func (c *CounterState) GetValue() int64 {
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

func (c *CounterState) Add(n int64) {
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

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	srv          *rest.Server
)

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "one_of_everything_REST"

	if err := logger.Init(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
	defer logger.Close()

	// Handle signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	portStr := envOr("CATENA_PORT", "6254")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		logger.Error("invalid CATENA_PORT", "error", err)
		os.Exit(1)
	}

	// ==========================================================================
	// Counter State
	// ==========================================================================
	counter := &CounterState{}

	// Start counter goroutine - increments every second when running
	go func() {
		ticker := time.NewTicker(1 * time.Second)
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				if counter.IsRunning() {
					counter.Increment()
					logger.Info("Counter tick", "value", counter.GetValue())
				}
			case <-shutdownChan:
				return
			}
		}
	}()

	// ==========================================================================
	// Device Metadata (for GetDevice endpoint)
	// ==========================================================================
	devices := map[int]map[string]any{
		0: {
			"slot":              0,
			"multi_set_enabled": true,
			"detail_level":      "FULL",
			"access_scopes":     []string{"monitor", "operate", "configure", "administer"},
			"default_scope":     "operate",
		},
	}

	// ==========================================================================
	// Parameters (for GetParam/SetParam endpoints)
	// ==========================================================================
	params := &sync.Map{}

	params.Store("video/resolution", "1920x1080")
	params.Store("video/brightness", int32(50))
	params.Store("video/contrast", int32(50))
	params.Store("video/saturation", int32(50))

	params.Store("audio/volume", int32(75))
	params.Store("audio/muted", int32(0)) // 0 = false, 1 = true

	// System parameters
	params.Store("system/device_name", "Demo Device")

	// ==========================================================================
	// Commands (for ExecuteCommand endpoint)
	// ==========================================================================

	// Helper to build counter response
	// Note: ToProto only supports int32, float32, string - not int64 or bool
	buildCounterResponse := func() map[string]any {
		running := int32(0)
		if counter.IsRunning() {
			running = 1
		}
		return map[string]any{
			"counter": int32(counter.GetValue()),
			"running": running,
		}
	}

	commands := map[string]CommandHandler{
		// Counter commands
		"start": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			if counter.IsRunning() {
				logger.Info("Start command - already running")
			} else {
				counter.Start()
				logger.Info("Counter started", "value", counter.GetValue())
			}
			catenaVal, err := catena.ToCatenaValue(buildCounterResponse())
			if err != nil {
				return catena.ReplyInternalError("failed to create response")
			}
			return catena.ReplyOK(catenaVal)
		},

		"stop": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
			} else {
				counter.Stop()
				logger.Info("Counter stopped", "value", counter.GetValue())
			}
			catenaVal, err := catena.ToCatenaValue(buildCounterResponse())
			if err != nil {
				return catena.ReplyInternalError("failed to create response")
			}
			return catena.ReplyOK(catenaVal)
		},

		"add10": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			catenaVal, err := catena.ToCatenaValue(buildCounterResponse())
			if err != nil {
				return catena.ReplyInternalError("failed to create response")
			}
			return catena.ReplyOK(catenaVal)
		},

		"reset": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			catenaVal, err := catena.ToCatenaValue(buildCounterResponse())
			if err != nil {
				return catena.ReplyInternalError("failed to create response")
			}
			return catena.ReplyOK(catenaVal)
		},
	}

	// ==========================================================================
	// Assets (for GetAsset endpoint)
	// ==========================================================================
	assets := &sync.Map{}
	assetsList := &sync.Map{}

	// Load assets from embedded static directory
	loadAssetsFromEmbedded(staticFS, "static", assets, assetsList)

	// Build assets list for logging
	var assetNames []string
	assetsList.Range(func(key, value any) bool {
		assetNames = append(assetNames, key.(string))
		return true
	})

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []int{0}
	srv = rest.NewServer(slotList)

	// --------------------------------------------------------------------------
	// Register GetDevice handler
	// --------------------------------------------------------------------------
	srv.RegisterGetDeviceHandler(0, func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			return catena.ReplyBadRequest("invalid path")
		}
		slotStr := parts[2]
		slot, err := strconv.Atoi(slotStr)
		if err != nil {
			return catena.ReplyBadRequest("invalid slot")
		}

		logger.Info("GetDevice", "slot", slot)
		device, ok := devices[slot]
		if !ok {
			return catena.ReplyNotFound("device not found")
		}
		catenaVal, err := catena.ToCatenaValue(device)
		if err != nil {
			return catena.ReplyInternalError("failed to create device info")
		}
		return catena.ReplyOK(catenaVal)
	})

	// --------------------------------------------------------------------------
	// Register GetValue handler (GetParam)
	// --------------------------------------------------------------------------
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("GetParam", "slot", slot, "fqoid", fqoid)

		// Special case: return counter state
		if fqoid == "counter" {
			catenaVal, err := catena.ToCatenaValue(buildCounterResponse())
			if err != nil {
				return catena.ReplyInternalError("failed to convert counter response")
			}
			return catena.ReplyOK(catenaVal)
		}

		// Special case: return all params as a struct
		if fqoid == "all" {
			allParams := make(map[string]any)
			params.Range(func(key, value any) bool {
				allParams[key.(string)] = value
				return true
			})
			running := int32(0)
			if counter.IsRunning() {
				running = 1
			}
			allParams["counter"] = int32(counter.GetValue())
			allParams["counter_running"] = running
			catenaVal, err := catena.ToCatenaValue(allParams)
			if err != nil {
				return catena.ReplyInternalError("failed to convert params")
			}
			return catena.ReplyOK(catenaVal)
		}

		val, ok := params.Load(fqoid)
		if !ok {
			return catena.ReplyNotFound("parameter not found: " + fqoid)
		}

		catenaVal, err := catena.ToCatenaValue(val)
		if err != nil {
			return catena.ReplyInternalError("failed to convert value")
		}
		return catena.ReplyOK(catenaVal)
	})

	// --------------------------------------------------------------------------
	// Register SetValue handler (SetParam)
	// --------------------------------------------------------------------------
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("SetParam", "slot", slot, "fqoid", fqoid, "value", value)

		// Check if parameter exists
		_, exists := params.Load(fqoid)
		if !exists {
			return catena.ReplyNotFound("parameter not found: " + fqoid)
		}

		params.Store(fqoid, value)
		logger.Info("Parameter updated", "fqoid", fqoid, "value", value)
		return catena.ReplyNoContent()
	})

	// --------------------------------------------------------------------------
	// Register ExecuteCommand handler
	// --------------------------------------------------------------------------
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

		handler, ok := commands[commandFqoid]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			response := map[string]any{
				"exception": map[string]any{
					"type":    "Invalid Command",
					"details": "Command not found: " + commandFqoid,
				},
			}
			catenaVal, err := catena.ToCatenaValue(response)
			if err != nil {
				return catena.ReplyInternalError("failed to create exception response")
			}
			return catena.ReplyOK(catenaVal)
		}

		return handler(payload)
	})

	// --------------------------------------------------------------------------
	// Register GetAsset handler
	// --------------------------------------------------------------------------
	srv.RegisterGetAssetHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("GetAsset", "slot", slot, "fqoid", fqoid)

		val, ok := assets.Load(fqoid)
		if !ok {
			return catena.ReplyNotFound("asset not found: " + fqoid)
		}

		asset := val.(Asset)
		w.Header().Set("Content-Type", asset.ContentType)
		w.Header().Set("Content-Length", strconv.Itoa(len(asset.Data)))

		reader := bytes.NewReader(asset.Data)
		if _, err := io.Copy(w, reader); err != nil {
			return catena.ReplyInternalError("failed to stream asset")
		}

		// Asset already written to response writer, return empty value with OK status
		return catena.CatenaValue{}, catena.StatusResult{Code: catena.OK}
	})

	// --------------------------------------------------------------------------
	// Register Fallback handler - serves index.html at root
	// --------------------------------------------------------------------------
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		if r.URL.Path == "/" {
			data, err := staticFS.ReadFile("static/index.htm")
			if err != nil {
				return catena.ReplyNotFound("index.html not found")
			}
			w.Header().Set("Content-Type", "text/html; charset=utf-8")
			w.Write(data)
			// Content already written to response writer
			return catena.CatenaValue{}, catena.StatusResult{Code: catena.OK}
		}
		return catena.ReplyNotFound("endpoint not found: " + r.URL.Path)
	})

	// ==========================================================================
	// Start Server
	// ==========================================================================
	logger.Info("=======================================================")
	logger.Info("One of Everything REST Example")
	logger.Info("=======================================================")
	logger.Info("REST server starting", "port", port)
	logger.Info("")
	logger.Info("Web UI available at:")
	logger.Info(fmt.Sprintf("  http://localhost:%d/", port))
	logger.Info("")
	logger.Info("API endpoints:")
	logger.Info("  GET  /st2138-api/v1/0              - GetDevice")
	logger.Info("  GET  /st2138-api/v1/0/value/{oid}  - GetParam")
	logger.Info("  PUT  /st2138-api/v1/0/value/{oid}  - SetParam")
	logger.Info("  POST /st2138-api/v1/0/command/{cmd}- ExecuteCommand")
	logger.Info("  GET  /st2138-api/v1/0/asset/{oid}  - GetAsset")
	logger.Info("")
	logger.Info("Parameters: video/brightness, video/contrast, video/saturation,")
	logger.Info("            video/resolution, audio/volume, audio/muted, system/device_name")
	logger.Info("Commands:   start, stop, add10, reset")
	logger.Info("Assets:", "count", len(assetNames))
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

// loadAssetsFromEmbedded loads files from embedded FS into the asset store
func loadAssetsFromEmbedded(embedFS embed.FS, root string, assets *sync.Map, assetsList *sync.Map) {
	err := fs.WalkDir(embedFS, root, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return nil
		}
		if d.IsDir() {
			return nil
		}
		// Skip index.html
		if filepath.Base(path) == "index.htm" {
			return nil
		}

		data, err := embedFS.ReadFile(path)
		if err != nil {
			return nil
		}

		ext := filepath.Ext(path)
		contentType := mime.TypeByExtension(ext)
		if contentType == "" {
			contentType = "application/octet-stream"
		}

		relPath, _ := filepath.Rel(root, path)
		assetID := strings.ReplaceAll(relPath, string(filepath.Separator), "/")

		assets.Store(assetID, Asset{
			ContentType: contentType,
			Data:        data,
			Filename:    filepath.Base(path),
		})
		assetsList.Store(assetID, true)
		logger.Info("Loaded asset", "id", assetID, "size", len(data))

		return nil
	})

	if err != nil {
		logger.Error("Error loading assets", "error", err)
	}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
