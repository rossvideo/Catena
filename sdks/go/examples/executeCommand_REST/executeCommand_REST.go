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
 * @brief ExecuteCommand REST example - Counter Demo.
 * @file executeCommand_REST.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-12
 */

package main

import (
	"embed"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"strconv"
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
type CommandHandler func(payload any) catena.StatusResult

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	srv          *rest.Server
)

// Counter state
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

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "executeCommand_REST"

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

	// Initialize counter
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

	// Helper to build response with current state
	buildResponse := func() map[string]any {
		return map[string]any{
			"counter": counter.GetValue(),
			"running": counter.IsRunning(),
		}
	}

	// ==========================================================================
	// Commands
	// ==========================================================================
	commands := map[string]CommandHandler{
		// Start command
		"start": func(payload any) catena.StatusResult {
			if counter.IsRunning() {
				logger.Info("Start command - already running")
				return catena.OK(buildResponse())
			}
			counter.Start()
			logger.Info("Counter started", "value", counter.GetValue())
			return catena.OK(buildResponse())
		},

		// Stop command
		"stop": func(payload any) catena.StatusResult {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
				return catena.OK(buildResponse())
			}
			counter.Stop()
			logger.Info("Counter stopped", "value", counter.GetValue())
			return catena.OK(buildResponse())
		},

		// Add10 command
		"add10": func(payload any) catena.StatusResult {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			return catena.OK(buildResponse())
		},

		// Reset command
		"reset": func(payload any) catena.StatusResult {
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			return catena.OK(buildResponse())
		},
	}

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []int{0}
	srv = rest.NewServer(slotList)

	// Register GetValue handler to retrieve counter state
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) catena.StatusResult {
		if fqoid == "counter" {
			return catena.OK(buildResponse())
		}
		return catena.NotFound("parameter not found: " + fqoid)
	})

	// Register ExecuteCommand handler
	srv.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) catena.StatusResult {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

		handler, ok := commands[commandFqoid]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"type":    "Invalid Command",
					"details": "Command not found: " + commandFqoid,
				},
			})
		}

		return handler(payload)
	})

	// Serve static files at root (including index.html)
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		// Serve index.html for root path
		if r.URL.Path == "/" {
			data, err := staticFS.ReadFile("static/index.htm")
			if err != nil {
				return catena.NotFound("index.html not found")
			}
			w.Header().Set("Content-Type", "text/html; charset=utf-8")
			w.Write(data)
			return catena.StatusResult{Status: http.StatusOK}
		}
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	// ==========================================================================
	// Start Server
	// ==========================================================================
	logger.Info("=======================================================")
	logger.Info("ExecuteCommand REST Example")
	logger.Info("=======================================================")
	logger.Info("REST server starting", "port", port)
	logger.Info("")
	logger.Info("Web UI available at:")
	logger.Info(fmt.Sprintf("  http://localhost:%d/", port))
	logger.Info("")
	logger.Info("=======================================================")

	srv.StartHTTPServer(port)

	// Wait for shutdown signal
	<-shutdownChan
	logger.Info("Server shutdown complete")
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
