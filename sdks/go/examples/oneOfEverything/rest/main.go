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
 * @brief Example program containing one of everything.
 * @file main.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-13
 */

package main

import (
	"crypto/sha256"
	"embed"
	"fmt"
	"io/fs"
	"mime"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"reflect"
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

// CommandHandler processes a command and returns a response
type CommandHandler func(payload any) (catena.CatenaValue, catena.StatusResult)

// Counter state with thread-safe operations
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

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	srv          *rest.Server
)

func main() {
	// Initialize SDK with prefix and app name
	cfg, err := catena.InitOptions(catena.Options{AppName: "oneOfEverything_REST"})
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
			"slot":              uint32(0),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
		},
		1: {
			"slot":              uint32(1),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": false,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op"},
			"default_scope":     "st2138:mon",
		},
		2: {
			"slot":              uint32(2),
			"detail_level":      catena.DetailLevelFull,
			"multi_set_enabled": true,
			"subscriptions":     false,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg"},
			"default_scope":     "st2138:cfg",
		},
	}

	// ==========================================================================
	// Parameters (for GetParam/SetParam endpoints)
	// ==========================================================================
	params := &sync.Map{}

	params.Store("resolution", "1920x1080")
	params.Store("brightness", int32(50))
	params.Store("contrast", int32(50))
	params.Store("saturation", int32(50))

	params.Store("volume", int32(75))
	params.Store("muted", int32(0)) // 0 = false, 1 = true

	// System parameters
	params.Store("device_name", "Demo Device")

	// ==========================================================================
	// Commands (for ExecuteCommand endpoint)
	// ==========================================================================

	// Helper to build counter response
	buildCounterResponse := func() map[string]any {
		var runningVal int32 = 0
		if counter.IsRunning() {
			runningVal = 1
		}
		return map[string]any{
			"counter": int32(counter.GetValue()),
			"running": runningVal,
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
			val, _ := catena.ToCatenaValue(buildCounterResponse())
			return catena.Reply(val)
		},

		"stop": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
			} else {
				counter.Stop()
				logger.Info("Counter stopped", "value", counter.GetValue())
			}
			val, _ := catena.ToCatenaValue(buildCounterResponse())
			return catena.Reply(val)
		},

		"add10": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			val, _ := catena.ToCatenaValue(buildCounterResponse())
			return catena.Reply(val)
		},

		"reset": func(payload any) (catena.CatenaValue, catena.StatusResult) {
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			val, _ := catena.ToCatenaValue(buildCounterResponse())
			return catena.Reply(val)
		},
	}

	// ==========================================================================
	// Assets (for GetAsset endpoint)
	// ==========================================================================
	assets := &sync.Map{}

	// Load assets from embedded static directory
	loadAssetsFromEmbedded(staticFS, "static", assets)

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []int{0, 1, 2}
	srv = rest.NewServer(slotList)

	// --------------------------------------------------------------------------
	// Register GetDevice handler for each slot
	// --------------------------------------------------------------------------
	for _, slot := range slotList {
		slot := slot // capture loop variable
		srv.RegisterGetDeviceHandler(slot, func() (catena.CatenaDevice, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			deviceInfo, ok := devices[slot]
			if !ok {
				return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
			}
			device, err := catena.ToCatenaDevice(deviceInfo)
			if err != nil {
				return catena.ReplyError[catena.CatenaDevice](catena.INTERNAL, "failed to create device info")
			}
			return catena.Reply(device)
		})
	}

	// --------------------------------------------------------------------------
	// Register GetValue handler (GetParam)
	// --------------------------------------------------------------------------
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("GetParam", "slot", slot, "fqoid", fqoid)

		// Special case: return counter state
		if fqoid == "counter" {
			val, _ := catena.ToCatenaValue(buildCounterResponse())
			return catena.Reply(val)
		}

		// Special case: return all params as a struct
		if fqoid == "all" {
			allParams := make(map[string]any)
			params.Range(func(key, value any) bool {
				allParams[key.(string)] = value
				return true
			})
			var runningVal int32 = 0
			if counter.IsRunning() {
				runningVal = 1
			}
			allParams["counter"] = int32(counter.GetValue())
			allParams["counter_running"] = runningVal
			catenaVal, err := catena.ToCatenaValue(allParams)
			if err != nil {
				return catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to convert params")
			}
			return catena.Reply(catenaVal)
		}

		v, ok := params.Load(fqoid)
		if !ok {
			return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "parameter not found: "+fqoid)
		}

		catenaVal, err := catena.ToCatenaValue(v)
		if err != nil {
			return catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to convert value")
		}
		return catena.Reply(catenaVal)
	})

	// --------------------------------------------------------------------------
	// Register SetValue handler (SetParam)
	// --------------------------------------------------------------------------
	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("SetParam", "slot", slot, "fqoid", fqoid, "value", value)

		// Check if parameter exists
		val, ok := params.Load(fqoid)
		if !ok {
			logger.Error("SetParam param not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "param not found: "+fqoid)
		}

		// Verify type matches
		if reflect.TypeOf(val) != reflect.TypeOf(value) {
			logger.Error("SetParam type mismatch", "slot", slot, "fqoid", fqoid,
				"expected", reflect.TypeOf(val), "got", reflect.TypeOf(value))
			return catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "type mismatch")
		}

		params.Store(fqoid, value)
		logger.Info("Parameter updated", "fqoid", fqoid, "value", value)
		return catena.ReplyError[catena.CatenaValue](catena.NO_CONTENT, "")
	})

	// --------------------------------------------------------------------------
	// Register ExecuteCommand handler
	// --------------------------------------------------------------------------
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

		handler, ok := commands[commandFqoid]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			exception := map[string]any{
				"exception": map[string]any{
					"type":    "Invalid Command",
					"details": "Command not found: " + commandFqoid,
				},
			}
			val, _ := catena.ToCatenaValue(exception)
			return catena.Reply(val)
		}

		return handler(payload)
	})

	// --------------------------------------------------------------------------
	// Register GetAsset handler
	// --------------------------------------------------------------------------
	srv.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)

		val, ok := assets.Load(fqoid)
		if !ok {
			logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found: "+fqoid)
		}

		payload := val.(catena.DataPayload)

		// Convert DataPayload to CatenaAsset when returning
		catenaAsset, err := catena.ToCatenaAsset(payload, true)
		if err != nil {
			logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", err)
			return catena.ReplyError[catena.CatenaAsset](catena.INTERNAL, "failed to convert asset: "+err.Error())
		}

		logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
		return catena.Reply(catenaAsset)
	})

	// --------------------------------------------------------------------------
	// Register Fallback handler - serves index.html at root
	// --------------------------------------------------------------------------
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		if r.URL.Path == "/" {
			data, err := staticFS.ReadFile("static/index.htm")
			if err != nil {
				return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "index.html not found")
			}
			w.Header().Set("Content-Type", "text/html; charset=utf-8")
			w.Write(data)
			// Content already written to response writer
			return catena.Reply(catena.CatenaValue{})
		}
		return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found: "+r.URL.Path)
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

// loadAssetsFromEmbedded loads all files from the embedded filesystem into the asset store
func loadAssetsFromEmbedded(embedFS embed.FS, root string, assets *sync.Map) {
	err := fs.WalkDir(embedFS, root, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			logger.Warning("Error accessing path", "path", path, "error", err)
			return nil // Continue walking
		}

		if d.IsDir() {
			return nil // Skip directories
		}

		// Skip index.html (served separately via fallback handler)
		if filepath.Base(path) == "index.htm" {
			return nil
		}

		// Read file contents
		data, err := embedFS.ReadFile(path)
		if err != nil {
			logger.Warning("Failed to read file", "path", path, "error", err)
			return nil
		}

		// Determine content type from extension
		ext := filepath.Ext(path)
		contentType := mime.TypeByExtension(ext)
		if contentType == "" {
			contentType = "application/octet-stream"
		}

		// Use relative path from root as the asset ID
		relPath, err := filepath.Rel(root, path)
		if err != nil {
			relPath = filepath.Base(path)
		}
		// Normalize path separators for URL use
		assetID := strings.ReplaceAll(relPath, string(filepath.Separator), "/")

		// Build metadata
		metadata := map[string]string{
			"content-type": contentType,
			"file-name":    filepath.Base(path),
		}

		// Calculate SHA-256 digest
		hash := sha256.Sum256(data)

		// Store as DataPayload directly
		payload := catena.DataPayload{
			Payload:  data,
			Metadata: metadata,
			Digest:   hash[:],
		}

		assets.Store(assetID, payload)
		logger.Info("Loaded asset", "id", assetID, "size", len(data), "type", contentType)

		return nil
	})

	if err != nil {
		logger.Error("Error walking embedded directory", "root", root, "error", err)
	}
}
