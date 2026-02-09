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

package rest

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// PushUpdate represents a streaming update sent to connected clients via SSE
type PushUpdate struct {
	Slot            int    `json:"slot,omitempty"`
	OID             string `json:"oid,omitempty"`
	Value           any    `json:"value,omitempty"`
	SlotsAdded      []int  `json:"slots_added,omitempty"`
	SlotsRemoved    []int  `json:"slots_removed,omitempty"`
	DeviceComponent any    `json:"device_component,omitempty"`
}

// sseConnection represents an active SSE connection
type sseConnection struct {
	updates chan PushUpdate
	slots   []int // slots this connection is subscribed to
}

// SetMaxConnections sets the maximum number of SSE connections allowed (0 = unlimited)
func (s *Server) SetMaxConnections(max int) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.maxConnections = max
}

// registerConnection registers a new SSE connection and returns its ID
func (s *Server) registerConnection(slots []int) (int, chan PushUpdate) {
	s.mu.Lock()
	defer s.mu.Unlock()

	// Check connection limit
	if s.maxConnections > 0 && len(s.connections) >= s.maxConnections {
		return -1, nil
	}

	s.nextConnID++
	updates := make(chan PushUpdate, 100)
	s.connections[s.nextConnID] = &sseConnection{
		updates: updates,
		slots:   slots,
	}
	logger.Info("SSE connection registered", "connID", s.nextConnID, "total", len(s.connections))
	return s.nextConnID, updates
}

// unregisterConnection removes an SSE connection
func (s *Server) unregisterConnection(connID int) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if conn, ok := s.connections[connID]; ok {
		close(conn.updates)
		delete(s.connections, connID)
		logger.Info("SSE connection unregistered", "connID", connID, "remaining", len(s.connections))
	}
}

// BroadcastUpdate sends an update to all connected SSE clients
func (s *Server) BroadcastUpdate(update PushUpdate) {
	s.mu.Lock()
	defer s.mu.Unlock()

	for connID, conn := range s.connections {
		// Only send to connections subscribed to this slot (or all if slot is 0)
		if update.Slot == 0 || len(conn.slots) == 0 || containsSlot(conn.slots, update.Slot) {
			select {
			case conn.updates <- update:
				// Successfully sent
			default:
				// Channel full, log warning but don't block
				logger.Warning("SSE channel full, dropping update", "connID", connID, "oid", update.OID)
			}
		}
	}
}

// sendSSEEvent writes a single SSE event to the response writer
func (s *Server) sendSSEEvent(w http.ResponseWriter, flusher http.Flusher, update PushUpdate) error {
	data, err := json.Marshal(update)
	if err != nil {
		return err
	}
	_, err = fmt.Fprintf(w, "data: %s\n\n", data)
	if err != nil {
		return err
	}
	flusher.Flush()
	return nil
}

// getPopulatedSlots returns the list of registered device slots
func (s *Server) getPopulatedSlots() []int {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.slots
}

// containsSlot checks if a slot is in the list
func containsSlot(slots []int, slot int) bool {
	for _, s := range slots {
		if s == slot {
			return true
		}
	}
	return false
}

// handleConnect handles GET /st2138-api/v1/connect (SSE streaming)
func (s *Server) handleConnect(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
		writeHTTPResult(w, res, val)
		return
	}
	// Check if SSE streaming is supported
	flusher, ok := w.(http.Flusher)
	if !ok {
		val, res := catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "streaming not supported")
		writeHTTPResult(w, res, val)
		return
	}

	// Register this connection
	connID, updates := s.registerConnection(s.getPopulatedSlots())
	if connID < 0 {
		val, res := catena.ReplyError[catena.CatenaValue](catena.RESOURCE_EXHAUSTED, "too many connections")
		writeHTTPResult(w, res, val)
		return
	}
	defer s.unregisterConnection(connID)

	// Set SSE headers
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")
	origin := r.Header.Get("Origin")
	if origin != "" {
		w.Header().Set("Access-Control-Allow-Origin", origin)
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept, Origin, X-Requested-With, Language, Detail-Level")
		w.Header().Set("Access-Control-Allow-Credentials", "true")
	}
	w.WriteHeader(http.StatusOK)
	flusher.Flush()

	// Send initial message with populated slots
	initialUpdate := PushUpdate{
		SlotsAdded: s.getPopulatedSlots(),
	}
	if err := s.sendSSEEvent(w, flusher, initialUpdate); err != nil {
		logger.Error("failed to send initial SSE event", "error", err)
		return
	}

	logger.Info("SSE Connect started", "connID", connID)

	// Stream updates until client disconnects
	ctx := r.Context()
	for {
		select {
		case <-ctx.Done():
			// Client disconnected
			logger.Info("SSE client disconnected", "connID", connID)
			return
		case update, ok := <-updates:
			if !ok {
				// Channel closed (server shutdown or forced disconnect)
				logger.Info("SSE channel closed", "connID", connID)
				return
			}
			if err := s.sendSSEEvent(w, flusher, update); err != nil {
				logger.Error("failed to send SSE event", "connID", connID, "error", err)
				return
			}
		}
	}
}
