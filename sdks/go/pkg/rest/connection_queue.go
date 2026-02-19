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
 * @brief Connection queue for managing SSE connections in the Catena REST server.
 * @file connection_queue.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-19
 */

package rest

import (
	"sync"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Connection represents an active SSE connection.
// Each connection runs in its own goroutine and receives updates via the
// updates channel as proto PushUpdates messages. The done channel is closed
// by the ConnectionQueue to signal server-initiated shutdown.
type Connection struct {
	id      int
	updates chan *protos.PushUpdates
	done    chan struct{}
}

// ConnectionQueue manages SSE connections to the service.
type ConnectionQueue struct {
	mu             sync.Mutex
	connections    map[int]*Connection
	nextConnID     int
	maxConnections int
	wg             sync.WaitGroup
	shuttingDown   bool
}

// NewConnectionQueue creates a new ConnectionQueue.
// maxConnections sets the limit on simultaneous connections (0 = unlimited).
func NewConnectionQueue(maxConnections int) *ConnectionQueue {
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

// RegisterConnection creates a new connection and returns its ID and Connection.
func (cq *ConnectionQueue) RegisterConnection() (int, *Connection) {
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
		id:      cq.nextConnID,
		updates: make(chan *protos.PushUpdates, 100),
		done:    make(chan struct{}),
	}
	cq.connections[cq.nextConnID] = conn
	cq.wg.Add(1)
	logger.Info("SSE connection registered", "connID", cq.nextConnID, "total", len(cq.connections))
	return cq.nextConnID, conn
}

// DeregisterConnection removes a connection from the queue.
// Safe to call multiple times for the same connID.
func (cq *ConnectionQueue) DeregisterConnection(connID int) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	if _, ok := cq.connections[connID]; ok {
		delete(cq.connections, connID)
		cq.wg.Done()
		logger.Info("SSE connection unregistered", "connID", connID, "remaining", len(cq.connections))
	}
}

// NotifySetValue sends a PushUpdates message to all connected clients.
// Called when a value changes (by client or server) to propagate
// the update to all SSE subscribers.
func (cq *ConnectionQueue) NotifySetValue(update *protos.PushUpdates) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	for connID, conn := range cq.connections {
		select {
		case conn.updates <- update:
		default:
			logger.Warning("SSE channel full, dropping update", "connID", connID)
		}
	}
}

// Shutdown signals all connections to stop and waits for them to deregister.
// Each connection's goroutine will receive the signal via the done channel,
// exit its event loop, and deregister itself.
func (cq *ConnectionQueue) Shutdown() {
	cq.mu.Lock()
	cq.shuttingDown = true
	for _, conn := range cq.connections {
		select {
		case <-conn.done:
		default:
			close(conn.done)
		}
	}
	cq.mu.Unlock()

	cq.wg.Wait()
	logger.Info("All SSE connections shut down")
}

// ConnectionCount returns the number of active connections.
func (cq *ConnectionQueue) ConnectionCount() int {
	cq.mu.Lock()
	defer cq.mu.Unlock()
	return len(cq.connections)
}
