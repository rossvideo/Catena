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
 * @brief Server interface and handler types for the Catena SDK.
 * @file server.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package catena

import (
	"context"
	"errors"
	"sync"
)

// Handler function types used by both REST and gRPC servers.
type DeviceHandler func() (CatenaDevice, StatusResult)
type GetValueHandler func(slot uint16, fqoid string) (CatenaValue, StatusResult)
type SetValueHandler func(value any, slot uint16, fqoid string) StatusResult
type GetAssetHandler func(slot uint16, fqoid string) (CatenaAsset, StatusResult)
type ExecuteCommandHandler func(slot uint16, commandFqoid string, payload any) (CommandResult, StatusResult)

var ErrServerStopped = errors.New("server is stopped")

// CatenaServer is the transport-agnostic interface satisfied by both
// rest.Server and grpc.Server, enabling shared handler registration code.
type CatenaServer interface {
	RegisterGetDeviceHandler(slot uint16, handler DeviceHandler)
	RegisterGetValueHandler(slot uint16, handler GetValueHandler)
	RegisterSetValueHandler(slot uint16, handler SetValueHandler)
	RegisterGetAssetHandler(slot uint16, handler GetAssetHandler)
	RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler)
	BroadcastUpdate(slot uint16, fqoid string, value any)
	Start(port int) error
	Shutdown()
}

type Transport interface {
	// Start begins transport operation with the given context.
	// The context signals when the transport should gracefully exit.
	// Start should return quickly and may spawn background goroutines.
	// Transports should monitor ctx.Done() and exit cleanly when signaled.
	Start(ctx context.Context)

	// Shutdown closes the transport and waits for cleanup.
	// The context provides a deadline; Shutdown must respect it and return
	// promptly even if cleanup is incomplete.
	Shutdown(ctx context.Context)
}

type Server struct {
	mu                     sync.Mutex
	ctx                    context.Context
	shutdown               bool
	stopped                chan struct{}
	Slots                  []uint32
	getDeviceHandlers      map[uint16]DeviceHandler
	getValueHandlers       map[uint16]GetValueHandler
	setValueHandlers       map[uint16]SetValueHandler
	getAssetHandlers       map[uint16]GetAssetHandler
	executeCommandHandlers map[uint16]ExecuteCommandHandler
	// connectionQueue        *ConnectionQueue
	transports []Transport
}

func NewServer() *Server {
	return &Server{
		ctx:        context.Background(),
		shutdown:   false,
		stopped:    make(chan struct{}),
		transports: []Transport{},
	}
}

func (s *Server) RegisterTransport(transport Transport) error {
	s.mu.Lock()
	if s.shutdown {
		s.mu.Unlock()
		return ErrServerStopped
	}
	s.transports = append(s.transports, transport)
	ctx := s.ctx
	s.mu.Unlock()

	// Transport startup should be non-blocking and must not happen under the server lock.
	// Pass server context so transport can derive its own child contexts if needed.
	transport.Start(ctx)

	return nil
}

func (s *Server) DeregisterTransport(transport Transport) {
	s.mu.Lock()
	idx := -1
	for i, t := range s.transports {
		if t == transport {
			idx = i
			break
		}
	}

	if idx == -1 {
		s.mu.Unlock()
		return
	}

	s.transports = append(s.transports[:idx], s.transports[idx+1:]...)
	s.mu.Unlock()

	// Shutdown may block while draining work; call it outside the server lock.
	transport.Shutdown(context.Background())
}

func (s *Server) RegisterGetDeviceHandler(slot uint16, handler DeviceHandler)              {}
func (s *Server) RegisterGetValueHandler(slot uint16, handler GetValueHandler)             {}
func (s *Server) RegisterSetValueHandler(slot uint16, handler SetValueHandler)             {}
func (s *Server) RegisterGetAssetHandler(slot uint16, handler GetAssetHandler)             {}
func (s *Server) RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler) {}
func (s *Server) BroadcastUpdate(slot uint16, fqoid string, value any)                     {}
func (s *Server) Start(port int) error

func (s *Server) Wait() {
	<-s.stopped
}

func (s *Server) Shutdown() {
	s.mu.Lock()
	if s.shutdown {
		s.mu.Unlock()
		return
	}
	s.shutdown = true
	transports := s.transports
	s.transports = nil
	s.mu.Unlock()

	// Shutdown all transports outside the lock.
	for _, t := range transports {
		t.Shutdown(context.Background())
	}

	close(s.stopped)
}
