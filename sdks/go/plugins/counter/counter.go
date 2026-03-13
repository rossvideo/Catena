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

// Counter is an example Catena plugin that implements a simple counter demo.
// It demonstrates how to write business logic as a plugin by exporting handler
// functions that are automatically wired up by the catena executable.
//
// Build with:
//
//	go build -buildmode=plugin -o counter.so counter.go
//
// Run with:
//
//	./catena counter.so
//
// Or with YAML config:
//
//	./catena config.yaml
package main

import (
	"embed"
	"net/http"
	"os"
	"path/filepath"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/plugin"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

//go:embed static/*
var staticFS embed.FS

// PluginInfo provides metadata about this plugin
var PluginInfo = plugin.PluginInfo{
	Name:        "Counter Demo",
	Version:     "1.0.0",
	Description: "A simple counter demonstration plugin",
}

// CounterState holds the counter value and running state
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

// Plugin state
var (
	counter      *CounterState
	shutdownChan <-chan struct{}
	assignedSlot int
	staticRoot   string // configurable via --static_root arg
	tlsEnabled   bool   // configurable via --tls arg
)

// buildResponse creates the standard response with counter state
func buildResponse() map[string]any {
	var runningVal int32 = 0
	if counter.IsRunning() {
		runningVal = 1
	}
	return map[string]any{
		"counter": int32(counter.GetValue()),
		"running": runningVal,
	}
}

// Init is called when the plugin is loaded
func Init(ctx *plugin.Context) error {
	counter = &CounterState{}
	shutdownChan = ctx.ShutdownChan
	assignedSlot = ctx.Slot

	// Process arguments from YAML config
	if ctx.Args != nil {
		if root, ok := ctx.Args["--static_root"]; ok {
			staticRoot = root
			logger.Info("Static root configured", "path", staticRoot)
		}
		if tls, ok := ctx.Args["--tls"]; ok {
			tlsEnabled = (tls == "on" || tls == "true" || tls == "1")
			logger.Info("TLS configured", "enabled", tlsEnabled)
		}
	}

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

	logger.Info("Counter plugin initialized", "slot", assignedSlot)
	return nil
}

// Shutdown is called during server shutdown
func Shutdown() error {
	logger.Info("Counter plugin shutting down")
	return nil
}

// GetValueHandler handles GET /st2138-api/v1/{slot}/value/{fqoid}
var GetValueHandler rest.GetValueHandler = func(fqoid string) (catena.CatenaValue, catena.StatusResult) {
	if fqoid == "counter" {
		val, _ := catena.ToCatenaValue(buildResponse())
		return catena.Reply(val)
	}
	return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "parameter not found: "+fqoid)
}

// ExecuteCommandHandler handles POST /st2138-api/v1/{slot}/command/{commandFqoid}
var ExecuteCommandHandler rest.ExecuteCommandHandler = func(w http.ResponseWriter, r *http.Request, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
	logger.Info("ExecuteCommand", "command", commandFqoid)

	switch commandFqoid {
	case "start":
		if counter.IsRunning() {
			logger.Info("Start command - already running")
		} else {
			counter.Start()
			logger.Info("Counter started", "value", counter.GetValue())
		}
		val, _ := catena.ToCatenaValue(buildResponse())
		return catena.Reply(val)

	case "stop":
		if !counter.IsRunning() {
			logger.Info("Stop command - already stopped")
		} else {
			counter.Stop()
			logger.Info("Counter stopped", "value", counter.GetValue())
		}
		val, _ := catena.ToCatenaValue(buildResponse())
		return catena.Reply(val)

	case "add10":
		counter.Add(10)
		logger.Info("Added 10 to counter", "value", counter.GetValue())
		val, _ := catena.ToCatenaValue(buildResponse())
		return catena.Reply(val)

	case "reset":
		counter.Reset()
		logger.Info("Counter reset", "value", counter.GetValue())
		val, _ := catena.ToCatenaValue(buildResponse())
		return catena.Reply(val)

	default:
		logger.Warning("Command not found", "command", commandFqoid)
		exception := map[string]any{
			"exception": map[string]any{
				"type":    "Invalid Command",
				"details": "Command not found: " + commandFqoid,
			},
		}
		val, _ := catena.ToCatenaValue(exception)
		return catena.Reply(val)
	}
}

// FallbackHandler serves static files and handles 404s
var FallbackHandler rest.FallbackHandler = func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
	if r.URL.Path == "/" {
		// Try external static root first (if configured), then embedded
		if staticRoot != "" {
			externalPath := filepath.Join(staticRoot, "index.htm")
			if data, err := os.ReadFile(externalPath); err == nil {
				w.Header().Set("Content-Type", "text/html; charset=utf-8")
				w.Write(data)
				return catena.Reply(catena.CatenaValue{})
			}
		}

		// Fall back to embedded static files
		data, err := staticFS.ReadFile("static/index.htm")
		if err != nil {
			return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "index.html not found")
		}
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		w.Write(data)
		return catena.Reply(catena.CatenaValue{})
	}
	return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found: "+r.URL.Path)
}
