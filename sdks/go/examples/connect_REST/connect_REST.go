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
 * @brief Connect REST example.
 * @file connect_REST.go
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
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// SSEClient represents a connected SSE client
type SSEClient struct {
	ID       string
	Channels chan SSEMessage
	Done     chan struct{}
}

// SSEMessage represents a message to send via SSE
type SSEMessage struct {
	Event string
	Data  string
}

// SSEHub manages all connected SSE clients
type SSEHub struct {
	clients    map[string]*SSEClient
	register   chan *SSEClient
	unregister chan string
	broadcast  chan SSEMessage
	mu         sync.RWMutex
}

func NewSSEHub() *SSEHub {
	hub := &SSEHub{
		clients:    make(map[string]*SSEClient),
		register:   make(chan *SSEClient),
		unregister: make(chan string),
		broadcast:  make(chan SSEMessage, 100),
	}
	go hub.run()
	return hub
}

func (h *SSEHub) run() {
	for {
		select {
		case client := <-h.register:
			h.mu.Lock()
			h.clients[client.ID] = client
			h.mu.Unlock()
			logger.Info("SSE client connected", "id", client.ID, "total_clients", len(h.clients))

		case clientID := <-h.unregister:
			h.mu.Lock()
			if client, ok := h.clients[clientID]; ok {
				close(client.Done)
				close(client.Channels)
				delete(h.clients, clientID)
			}
			h.mu.Unlock()
			logger.Info("SSE client disconnected", "id", clientID, "total_clients", len(h.clients))

		case msg := <-h.broadcast:
			h.mu.RLock()
			for _, client := range h.clients {
				select {
				case client.Channels <- msg:
				default:
					// Client buffer full, skip
				}
			}
			h.mu.RUnlock()
		}
	}
}

func (h *SSEHub) Broadcast(event, data string) {
	h.broadcast <- SSEMessage{Event: event, Data: data}
}

var clientCounter int
var counterMu sync.Mutex

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "connect_sse_example"

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

	// Create SSE hub for managing client connections
	hub := NewSSEHub()

	// Start a goroutine that simulates device parameter changes
	go simulateDeviceUpdates(hub)

	slotList := []int{0}
	srv := rest.NewServer(slotList)

	// Register the Connect handler for SSE streaming
	srv.RegisterConnectHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		logger.Info("SSE Connect request received")

		// Check if the client supports SSE
		flusher, ok := w.(http.Flusher)
		if !ok {
			logger.Error("SSE not supported - ResponseWriter doesn't implement Flusher")
			return catena.InternalServerError("SSE not supported")
		}

		// Set SSE headers
		w.Header().Set("Content-Type", "text/event-stream")
		w.Header().Set("Cache-Control", "no-cache")
		w.Header().Set("Connection", "keep-alive")
		w.Header().Set("Access-Control-Allow-Origin", "*")

		// Generate unique client ID
		counterMu.Lock()
		clientCounter++
		clientID := fmt.Sprintf("client-%d", clientCounter)
		counterMu.Unlock()

		// Create client
		client := &SSEClient{
			ID:       clientID,
			Channels: make(chan SSEMessage, 10),
			Done:     make(chan struct{}),
		}

		// Register client with hub
		hub.register <- client

		// Send initial connection confirmation
		fmt.Fprintf(w, "event: connected\n")
		fmt.Fprintf(w, "data: {\"client_id\":\"%s\",\"message\":\"Connected to Catena SSE stream\"}\n\n", clientID)
		flusher.Flush()

		// Handle client messages until disconnect
		ctx := r.Context()
		for {
			select {
			case <-ctx.Done():
				// Client disconnected
				hub.unregister <- clientID
				return catena.StatusResult{Status: http.StatusOK}

			case msg, ok := <-client.Channels:
				if !ok {
					// Channel closed
					return catena.StatusResult{Status: http.StatusOK}
				}
				// Send SSE message
				if msg.Event != "" {
					fmt.Fprintf(w, "event: %s\n", msg.Event)
				}
				fmt.Fprintf(w, "data: %s\n\n", msg.Data)
				flusher.Flush()

			case <-client.Done:
				return catena.StatusResult{Status: http.StatusOK}
			}
		}
	})

	// Not found handler
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	logger.Info("Connect SSE Example listening", "port", port)
	logger.Info("SSE Endpoint: GET /st2138-api/v1/connect")
	logger.Info("")
	logger.Info("Test with curl:")
	logger.Info("  curl -N http://localhost:" + portStr + "/st2138-api/v1/connect")
	logger.Info("")
	logger.Info("The server will broadcast simulated parameter updates every 2 seconds")

	srv.StartHTTPServer(port)
	select {}
}

// simulateDeviceUpdates broadcasts fake device updates for demonstration
func simulateDeviceUpdates(hub *SSEHub) {
	counter := 0
	ticker := time.NewTicker(2 * time.Second)
	defer ticker.Stop()

	// Different types of events to simulate
	events := []struct {
		event string
		data  func(int) string
	}{
		{
			event: "param_change",
			data: func(c int) string {
				return fmt.Sprintf(`{"slot":0,"fqoid":"counter","value":%d}`, c)
			},
		},
		{
			event: "param_change",
			data: func(c int) string {
				status := []string{"idle", "running", "paused", "error"}[c%4]
				return fmt.Sprintf(`{"slot":0,"fqoid":"status","value":"%s"}`, status)
			},
		},
		{
			event: "device_event",
			data: func(c int) string {
				return fmt.Sprintf(`{"type":"heartbeat","timestamp":%d}`, time.Now().Unix())
			},
		},
		{
			event: "param_change",
			data: func(c int) string {
				temp := 35.0 + float64(c%20)*0.5
				return fmt.Sprintf(`{"slot":0,"fqoid":"temperature","value":%.1f}`, temp)
			},
		},
	}

	for range ticker.C {
		counter++
		evt := events[counter%len(events)]
		hub.Broadcast(evt.event, evt.data(counter))
		logger.Debug("Broadcast SSE update", "event", evt.event, "counter", counter)
	}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

