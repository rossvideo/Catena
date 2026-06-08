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
 * @brief ServerRuntime stub for testing transports without a full Catena server implementation.
 * @file stub_server_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-08
 */

package transports

import (
	"context"
	"fmt"
	"sync"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

type stubServerRuntime struct {
	mu                       sync.Mutex
	tb                       testing.TB
	slots                    []uint16
	getSlotsFn               func(ctx catena.TransportContext) ([]uint16, catena.StatusResult)
	getDeviceFn              func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult)
	getValueFn               func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult)
	setValueFn               func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult
	getAssetFn               func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult)
	commandFn                func(slot uint16, commandFqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult)
	paramInfoFn              func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult)
	registerTransportConnFn  func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult)
	deregisterConnFn         func(connID int)
	shutdownTransportConnsFn func(ctx context.Context, transport catena.Transport)
	registerCalls            int
	deregisterCalls          int
	shutdownCalls            int
	lastRegisterID           int
	lastDeregisterID         int
	lastRegisterOwner        any
	lastShutdownOwner        any
}

func makeStubServerRuntime(tb testing.TB) *stubServerRuntime {
	return &stubServerRuntime{
		tb: tb,
	}
}

func (s *stubServerRuntime) panicf(format string, args ...any) {
	panic(fmt.Sprintf(format, args...))
}

var _ catena.ServerRuntime = (*stubServerRuntime)(nil)

func (s *stubServerRuntime) GetSlots(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
	if s.getSlotsFn != nil {
		return s.getSlotsFn(ctx)
	}
	return s.slots, catena.StatusResult{Code: catena.StatusCodeOk}
}

func (s *stubServerRuntime) InvokeGetDeviceHandler(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
	if s.getDeviceFn != nil {
		return s.getDeviceFn(slot, ctx)
	}
	s.panicf("GetDevice handler not implemented in stubServerRuntime for slot %d", slot)
	return catena.ReplyError[catena.Device](catena.StatusCodeInternal, "GetDevice handler not implemented")
}

func (s *stubServerRuntime) InvokeGetValueHandler(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
	if s.getValueFn != nil {
		return s.getValueFn(slot, fqoid, ctx)
	}
	s.panicf("GetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.ReplyError[catena.Value](catena.StatusCodeInternal, "GetValue handler not implemented")
}

func (s *stubServerRuntime) InvokeSetValueHandler(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
	if s.setValueFn != nil {
		return s.setValueFn(value, slot, fqoid, ctx)
	}
	s.panicf("SetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) InvokeGetAssetHandler(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult) {
	if s.getAssetFn != nil {
		return s.getAssetFn(slot, fqoid, ctx)
	}
	s.panicf("GetAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.ReplyError[catena.Asset](catena.StatusCodeInternal, "GetAsset handler not implemented")
}

func (s *stubServerRuntime) InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult) {
	if s.commandFn != nil {
		return s.commandFn(slot, commandFqoid, payload, ctx)
	}
	s.panicf("ExecuteCommand handler not implemented in stubServerRuntime for slot %d, commandFqoid %s", slot, commandFqoid)
	return catena.CommandResult{}, catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
	if s.paramInfoFn != nil {
		return s.paramInfoFn(slot, oidPrefix, recursive, ctx)
	}
	s.panicf("ParamInfo handler not implemented in stubServerRuntime for slot %d, oidPrefix %s", slot, oidPrefix)
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) RegisterTransportConnection(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.registerTransportConnFn != nil {
		conn, res := s.registerTransportConnFn(transport, ctx)
		s.registerCalls++
		if conn != nil {
			s.lastRegisterID = conn.ID
		}
		s.lastRegisterOwner = transport
		return conn, res
	}
	s.panicf("RegisterTransportConnection not implemented in stubServerRuntime")
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) DeregisterConnection(connID int) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.deregisterConnFn != nil {
		s.deregisterCalls++
		s.lastDeregisterID = connID
		s.deregisterConnFn(connID)
		return
	}
	s.panicf("DeregisterConnection not implemented in stubServerRuntime")
}

func (s *stubServerRuntime) ShutdownTransportConnections(ctx context.Context, transport catena.Transport) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.shutdownTransportConnsFn != nil {
		s.lastShutdownOwner = transport
		s.shutdownCalls++
		s.shutdownTransportConnsFn(ctx, transport)
		return
	}
	s.panicf("ShutdownTransportConnections not implemented in stubServerRuntime")
}

// WithConnection wires register/deregister behavior for a single fixed connection.
func (s *stubServerRuntime) WithConnection(
	connection *catena.Connection,
) {
	s.tb.Helper()

	s.registerTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
		return connection, catena.StatusResult{Code: catena.StatusCodeOk}
	}

	s.deregisterConnFn = func(connID int) {
		if connID != connection.ID {
			s.tb.Errorf("expected to deregister connection with id %d, got %d", connection.ID, connID)
		}
	}

	s.shutdownTransportConnsFn = func(ctx context.Context, transport catena.Transport) {
		if transport != s.lastRegisterOwner {
			s.tb.Errorf("expected to shutdown connections for transport %v, got %v", s.lastRegisterOwner, transport)
		}
		// notify any waiters that the connection has been "closed"
		connection.Done <- struct{}{}
	}
}
