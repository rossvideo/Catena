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
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

type stubServerRuntime struct {
	tb               testing.TB
	slots            []uint16
	getDeviceFn      func(slot uint16) (catena.CatenaDevice, catena.StatusResult)
	getValueFn       func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult)
	setValueFn       func(value any, slot uint16, fqoid string) catena.StatusResult
	getAssetFn       func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult)
	commandFn        func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult)
	registerConnFn   func() (int, *catena.Connection)
	deregisterConnFn func(connID int)
	registerCalls    int
	deregisterCalls  int
	lastRegisterID   int
	lastDeregisterID int
}

func makeStubServerRuntime(tb testing.TB) *stubServerRuntime {
	return &stubServerRuntime{
		tb: tb,
	}
}

var _ catena.ServerRuntime = (*stubServerRuntime)(nil)

func (s *stubServerRuntime) GetSlots() []uint16 {
	return s.slots
}

func (s *stubServerRuntime) InvokeGetDeviceHandler(slot uint16) (catena.CatenaDevice, catena.StatusResult) {
	if s.getDeviceFn != nil {
		return s.getDeviceFn(slot)
	}
	s.tb.Fatalf("GetDevice handler not implemented in stubServerRuntime for slot %d", slot)
	return catena.CatenaDevice{}, catena.StatusResult{Code: catena.INTERNAL}
}

func (s *stubServerRuntime) InvokeGetValueHandler(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	if s.getValueFn != nil {
		return s.getValueFn(slot, fqoid)
	}
	s.tb.Fatalf("GetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.CatenaValue{}, catena.StatusResult{Code: catena.INTERNAL}
}

func (s *stubServerRuntime) InvokeSetValueHandler(value any, slot uint16, fqoid string) catena.StatusResult {
	if s.setValueFn != nil {
		return s.setValueFn(value, slot, fqoid)
	}
	s.tb.Fatalf("SetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.INTERNAL}
}

func (s *stubServerRuntime) InvokeGetAssetHandler(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
	if s.getAssetFn != nil {
		return s.getAssetFn(slot, fqoid)
	}
	s.tb.Fatalf("GetAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.CatenaAsset{}, catena.StatusResult{Code: catena.INTERNAL}
}

func (s *stubServerRuntime) InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
	if s.commandFn != nil {
		return s.commandFn(slot, commandFqoid, payload)
	}
	s.tb.Fatalf("ExecuteCommand handler not implemented in stubServerRuntime for slot %d, commandFqoid %s", slot, commandFqoid)
	return catena.CommandResult{}, catena.StatusResult{Code: catena.INTERNAL}
}

func (s *stubServerRuntime) RegisterConnection() (int, *catena.Connection) {
	if s.registerConnFn != nil {
		connID, conn := s.registerConnFn()
		s.registerCalls++
		s.lastRegisterID = connID
		return connID, conn
	}
	s.tb.Fatalf("RegisterConnection not implemented in stubServerRuntime")
	return -1, nil
}

func (s *stubServerRuntime) DeregisterConnection(connID int) {
	if s.deregisterConnFn != nil {
		s.deregisterCalls++
		s.lastDeregisterID = connID
		s.deregisterConnFn(connID)
		return
	}
	s.tb.Fatalf("DeregisterConnection not implemented in stubServerRuntime")
}

// WithConnection wires register/deregister behavior for a single fixed connection.
func (s *stubServerRuntime) WithConnection(
	connection *catena.Connection,
) {
	s.tb.Helper()

	s.registerConnFn = func() (int, *catena.Connection) {
		return connection.ID, connection
	}

	s.deregisterConnFn = func(connID int) {
		if connID != connection.ID {
			s.tb.Errorf("expected to deregister connection with id %d, got %d", connection.ID, connID)
		}
	}
}
