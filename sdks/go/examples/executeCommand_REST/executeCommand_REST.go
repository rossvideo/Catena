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
	"encoding/json"
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

// ==========================================================================
// SSE Hub for broadcasting command events to connected clients
// ==========================================================================

// CommandEvent represents a command execution event sent via SSE
type CommandEvent struct {
	Command string `json:"command"`
	Counter int64  `json:"counter"`
	Running bool   `json:"running"`
}

// SSEHub manages SSE client connections
type SSEHub struct {
	mu      sync.RWMutex
	clients map[chan CommandEvent]struct{}
}

func NewSSEHub() *SSEHub {
	return &SSEHub{
		clients: make(map[chan CommandEvent]struct{}),
	}
}

func (h *SSEHub) Subscribe() chan CommandEvent {
	ch := make(chan CommandEvent, 10)
	h.mu.Lock()
	h.clients[ch] = struct{}{}
	h.mu.Unlock()
	logger.Info("SSE client connected", "total_clients", len(h.clients))
	return ch
}

func (h *SSEHub) Unsubscribe(ch chan CommandEvent) {
	h.mu.Lock()
	delete(h.clients, ch)
	close(ch)
	h.mu.Unlock()
	logger.Info("SSE client disconnected", "total_clients", len(h.clients))
}

func (h *SSEHub) Broadcast(event CommandEvent) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	for ch := range h.clients {
		select {
		case ch <- event:
		default:
			// Client too slow, skip this event
		}
	}
}

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
	cfg.AppName = "counter_commands_REST"

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

	// Initialize SSE hub for broadcasting command events
	sseHub := NewSSEHub()

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

		result := handler(payload)

		// Broadcast command event to all connected SSE clients
		sseHub.Broadcast(CommandEvent{
			Command: commandFqoid,
			Counter: counter.GetValue(),
			Running: counter.IsRunning(),
		})

		return result
	})

	// Register SSE Connect handler for real-time command notifications
	srv.RegisterConnectHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		// Set SSE headers
		w.Header().Set("Content-Type", "text/event-stream")
		w.Header().Set("Cache-Control", "no-cache")
		w.Header().Set("Connection", "keep-alive")
		w.Header().Set("Access-Control-Allow-Origin", "*")

		// Subscribe to command events
		eventChan := sseHub.Subscribe()
		defer sseHub.Unsubscribe(eventChan)

		// Get flusher for streaming
		flusher, ok := w.(http.Flusher)
		if !ok {
			logger.Error("SSE: ResponseWriter does not support flushing")
			return catena.InternalServerError("streaming not supported")
		}

		// Send initial connection event
		fmt.Fprintf(w, "event: connected\ndata: {\"status\":\"connected\"}\n\n")
		flusher.Flush()

		// Stream events until client disconnects
		for {
			select {
			case event := <-eventChan:
				data, _ := json.Marshal(event)
				fmt.Fprintf(w, "event: command\ndata: %s\n\n", data)
				flusher.Flush()
			case <-r.Context().Done():
				logger.Info("SSE client disconnected (context done)")
				return catena.StatusResult{Status: http.StatusOK}
			case <-shutdownChan:
				return catena.StatusResult{Status: http.StatusOK}
			}
		}
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
	logger.Info("Counter Commands REST Example")
	logger.Info("=======================================================")
	logger.Info("REST server starting", "port", port)
	logger.Info("")
	logger.Info("Web UI available at:")
	logger.Info(fmt.Sprintf("  http://localhost:%d/", port))
	logger.Info("")
	logger.Info("API endpoints:")
	logger.Info("  GET  /st2138-api/v1/0/value/counter    - Get counter state")
	logger.Info("  POST /st2138-api/v1/0/command/{cmd}    - Execute command")
	logger.Info("  GET  /st2138-api/v1/connect            - SSE stream for events")
	logger.Info("")
	logger.Info("Available commands:")
	logger.Info("  start  - Start auto-incrementing (1/sec)")
	logger.Info("  stop   - Stop auto-incrementing")
	logger.Info("  add10  - Add 10 to counter")
	logger.Info("  reset  - Reset counter to 0")
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
