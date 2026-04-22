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
 * @brief Unified streaming connection queue for REST (SSE) and gRPC servers.
 * @file connection_queue.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-26
 */

package internal

import (
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Connection represents an active streaming connection (SSE or gRPC).
// Each connection runs in its own goroutine and receives updates via the
// Updates channel as proto PushUpdates messages. The Done channel is closed
// by the ConnectionQueue to signal server-initiated shutdown.
type Connection struct {
	ID      int
	Updates chan *protos.PushUpdates
	Done    chan struct{}
}

// ConnectionQueue manages streaming connections for both REST (SSE) and gRPC servers.
// It provides thread-safe connection registration, broadcast updates, and graceful shutdown.
type ConnectionQueue struct {
	mu             sync.Mutex
	connections    map[int]*Connection
	nextConnID     int
	maxConnections int
	wg             sync.WaitGroup
	shuttingDown   bool
}

// NewConnectionQueue creates a new connection queue for streaming connections.
// maxConnections sets the limit on simultaneous connections (0 = unlimited).
func newConnectionQueue(maxConnections int) *ConnectionQueue {
	return &ConnectionQueue{
		connections:    make(map[int]*Connection),
		maxConnections: maxConnections,
	}
}

// SetMaxConnections updates the maximum number of connections allowed (0 = unlimited).
func (cq *ConnectionQueue) SetMaxConnections(max int) {
	cq.mu.Lock()
	defer cq.mu.Unlock()
	cq.maxConnections = max
}

// registerConnection creates a new connection and returns its ID and connection.
// Returns (-1, nil) if server is shutting down or max connections reached.
func (cq *ConnectionQueue) registerConnection() (int, *Connection) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	if cq.shuttingDown {
		return -1, nil
	}

	if cq.maxConnections > 0 && len(cq.connections) >= cq.maxConnections {
		return -1, nil
	}

	cq.nextConnID++
	conn := &Connection{
		ID:      cq.nextConnID,
		Updates: make(chan *protos.PushUpdates, 100),
		Done:    make(chan struct{}),
	}
	cq.connections[cq.nextConnID] = conn
	cq.wg.Add(1)
	logger.Info("Streaming connection registered", "connID", cq.nextConnID, "total", len(cq.connections))
	return cq.nextConnID, conn
}

// deregisterConnection removes a connection from the queue.
// Safe to call multiple times for the same connID.
func (cq *ConnectionQueue) deregisterConnection(connID int) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	if _, ok := cq.connections[connID]; ok {
		delete(cq.connections, connID)
		cq.wg.Done()
		logger.Info("Streaming connection unregistered", "connID", connID, "remaining", len(cq.connections))
	}
}

// notifyUpdate sends a PushUpdates message to all connected clients.
// Called when a value changes (by client or server) to propagate
// the update to all streaming subscribers.
func (cq *ConnectionQueue) NotifyUpdate(update *protos.PushUpdates) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	for connID, conn := range cq.connections {
		select {
		case conn.Updates <- update:
		default:
			logger.Warning("Streaming channel full, dropping update", "connID", connID)
		}
	}
}

// shutdown signals all connections to stop and waits for them to deregister.
// Each connection's goroutine will receive the signal via the Done channel,
// exit its event loop, and deregister itself.
func (cq *ConnectionQueue) shutdown() {
	cq.mu.Lock()
	cq.shuttingDown = true
	for _, conn := range cq.connections {
		select {
		case <-conn.Done:
		default:
			close(conn.Done)
		}
	}
	cq.mu.Unlock()

	cq.wg.Wait()
	logger.Info("All streaming connections shut down")
}

// ConnectionCount returns the number of active connections.
func (cq *ConnectionQueue) ConnectionCount() int {
	cq.mu.Lock()
	defer cq.mu.Unlock()
	return len(cq.connections)
}
