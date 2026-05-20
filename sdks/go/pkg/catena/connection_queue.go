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
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-14
 */

package catena

import (
	"context"
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
	owner   Transport
}

// connectionQueue manages streaming connections for both REST (SSE) and gRPC servers.
// It provides thread-safe connection registration, broadcast updates, and graceful shutdown.
type connectionQueue struct {
	mu             sync.Mutex
	connections    map[int]*Connection
	nextConnID     int
	maxConnections int
	shuttingDown   bool
	cond           *sync.Cond
}

type connectionQueueInterface interface {
	setMaxConnections(max int)
	registerOwnedConnection(owner Transport, initialUpdate *protos.PushUpdates) (int, *Connection)
	deregisterConnection(connID int)
	notifyUpdate(update *protos.PushUpdates)
	shutdown(ctx context.Context)
	shutdownOwner(ctx context.Context, owner Transport)
	shutdownConnection(ctx context.Context, connection *Connection)
	connectionCount() int
}

var _ connectionQueueInterface = (*connectionQueue)(nil)

// newConnectionQueue creates a new connection queue for streaming connections.
// maxConnections sets the limit on simultaneous connections (0 = unlimited).
func newConnectionQueue(maxConnections int) connectionQueueInterface {
	cq := &connectionQueue{
		connections:    make(map[int]*Connection),
		maxConnections: maxConnections,
	}
	cq.cond = sync.NewCond(&cq.mu)
	return cq
}

// setMaxConnections updates the maximum number of connections allowed (0 = unlimited).
func (cq *connectionQueue) setMaxConnections(max int) {
	cq.mu.Lock()
	defer cq.mu.Unlock()
	cq.maxConnections = max
}

// registerConnection creates a new connection and returns its ID and connection.
// Returns (-1, nil) if server is shutting down or max connections reached.
func (cq *connectionQueue) registerOwnedConnection(owner Transport, initialUpdate *protos.PushUpdates) (int, *Connection) {
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
		owner:   owner,
	}
	cq.connections[cq.nextConnID] = conn

	// send the initial update if provided
	if initialUpdate != nil {
		select {
		case conn.Updates <- initialUpdate:
		default:
			logger.Warning("Streaming channel full, dropping initial update", "connID", cq.nextConnID)
		}
	}
	logger.Info("Streaming connection registered", "connID", cq.nextConnID, "total", len(cq.connections))
	return cq.nextConnID, conn
}

// deregisterConnection removes a connection from the queue.
// Safe to call multiple times for the same connID.
func (cq *connectionQueue) deregisterConnection(connID int) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	if _, ok := cq.connections[connID]; ok {
		delete(cq.connections, connID)
		cq.cond.Broadcast()
		logger.Info("Streaming connection unregistered", "connID", connID, "remaining", len(cq.connections))
	}
}

// notifyUpdate sends a PushUpdates message to all connected clients.
// Called when a value changes (by client or server) to propagate
// the update to all streaming subscribers.
func (cq *connectionQueue) notifyUpdate(update *protos.PushUpdates) {
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
func (cq *connectionQueue) shutdown(ctx context.Context) {
	cq.mu.Lock()
	cq.shuttingDown = true
	conns := make([]*Connection, 0, len(cq.connections))
	for _, conn := range cq.connections {
		conns = append(conns, conn)
	}
	cq.mu.Unlock()

	var wg sync.WaitGroup
	for _, conn := range conns {
		wg.Add(1)
		go func(c *Connection) {
			defer wg.Done()
			cq.shutdownConnection(ctx, c)
		}(conn)
	}
	wg.Wait()

	logger.Info("All streaming connections shut down")
}

// shutdownOwner signals all connections owned by the specified owner to stop and waits for them to deregister.
func (cq *connectionQueue) shutdownOwner(ctx context.Context, owner Transport) {
	cq.mu.Lock()
	var ownerConns []*Connection
	for _, conn := range cq.connections {
		if conn.owner == owner {
			ownerConns = append(ownerConns, conn)
		}
	}
	cq.mu.Unlock()

	var wg sync.WaitGroup
	for _, conn := range ownerConns {
		wg.Add(1)
		go func(c *Connection) {
			defer wg.Done()
			cq.shutdownConnection(ctx, c)
		}(conn)
	}
	wg.Wait()
}

func (cq *connectionQueue) shutdownConnection(ctx context.Context, connection *Connection) {
	cq.mu.Lock()
	defer cq.mu.Unlock()

	// notify the connection to shut down by closing its Done channel
	select {
	case <-connection.Done:
	default:
		close(connection.Done)
	}

	// make a watchdog goroutine to force close the connection if it doesn't shut down gracefully within the context deadline
	done := make(chan struct{})
	go func() {
		select {
		case <-ctx.Done():
			cq.mu.Lock()
			defer cq.mu.Unlock()
			logger.Warning("Connection did not shut down gracefully, forcing close", "connID", connection.ID)
			delete(cq.connections, connection.ID)
			cq.cond.Broadcast()
		case <-done:
		}
	}()

	// wait for the connection to deregister itself
	for {
		if _, ok := cq.connections[connection.ID]; !ok {
			close(done)
			break
		}
		// friendly reminder that cond.Wait releases the lock while waiting and re-acquires it when signaled, so this loop will wake up whenever a connection is deregistered and check if it's this one
		cq.cond.Wait()
	}
}

// connectionCount returns the number of active connections.
func (cq *connectionQueue) connectionCount() int {
	cq.mu.Lock()
	defer cq.mu.Unlock()
	return len(cq.connections)
}
