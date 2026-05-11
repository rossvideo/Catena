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
 * @brief Test for the Catena SDK server.
 * @file server_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"context"
	"errors"
	"fmt"
	"slices"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"google.golang.org/protobuf/proto"
)

func TestValidateSlot_Valid(t *testing.T) {
	tests := []struct {
		name string
		in   uint32
	}{
		{name: "zero", in: 0},
		{name: "one", in: 1},
		{name: "typical", in: 42},
		{name: "max uint16", in: 65535},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlot(tt.in)
			if err.Code != OK {
				t.Errorf("expected no error, got %v", err)
			}
			if result != uint16(tt.in) {
				t.Errorf("expected result %v, got %v", tt.in, result)
			}
		})
	}
}

func TestValidateSlot_Invalid(t *testing.T) {
	tests := []struct {
		name string
		in   uint32
	}{
		{name: "negative (uint32 underflow)", in: ^uint32(0)}, // 0xFFFFFFFF = 4294967295, which is -1 if interpreted as signed
		{name: "too large", in: 65536},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlot(tt.in)
			if err.Code != INVALID_ARGUMENT {
				t.Errorf("expected INVALID_ARGUMENT error, got %v", err)
			}
			if result != 0 {
				t.Errorf("expected result 0 on error, got %v", result)
			}
		})
	}
}

func TestValidateSlotString_Valid(t *testing.T) {
	tests := []struct {
		name string
		in   string
	}{
		{name: "zero", in: "0"},
		{name: "one", in: "1"},
		{name: "typical", in: "42"},
		{name: "max uint16", in: "65535"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlotString(tt.in)
			if err.Code != OK {
				t.Errorf("expected no error, got %v", err)
			}
			expected := uint16(0)
			fmt.Sscanf(tt.in, "%d", &expected)
			if result != expected {
				t.Errorf("expected result %v, got %v", expected, result)
			}
		})
	}
}

func TestValidateSlotString_Invalid(t *testing.T) {
	tests := []struct {
		name string
		in   string
	}{
		{name: "negative", in: "-1"},
		{name: "too large", in: "65536"},
		{name: "non-numeric", in: "abc"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlotString(tt.in)
			if err.Code != INVALID_ARGUMENT {
				t.Errorf("expected INVALID_ARGUMENT error, got %v", err)
			}
			if result != 0 {
				t.Errorf("expected result 0 on error, got %v", result)
			}
		})
	}
}

type stubTransport struct {
	tb         testing.TB
	startFn    func(context.Context, ServerRuntime) error
	shutdownFn func(context.Context) error
}

var _ Transport = (*stubTransport)(nil)

func (s *stubTransport) Start(ctx context.Context, runtime ServerRuntime) error {
	if s.startFn != nil {
		return s.startFn(ctx, runtime)
	}
	s.tb.Fatalf("Start called on stubTransport without startFn defined")
	return nil
}

func (s *stubTransport) Shutdown(ctx context.Context) error {
	if s.shutdownFn != nil {
		return s.shutdownFn(ctx)
	}
	s.tb.Fatalf("Shutdown called on stubTransport without shutdownFn defined")
	return nil
}

func TestServer_RegisterTransport_Normal(t *testing.T) {
	called := false
	// not really an error just testing that the Start function is called and
	// its return value is passed through correctly
	expectedError := fmt.Errorf("expected error")
	srv := NewServer(100)

	// server starts with new transports
	if len(srv.transports) != 0 {
		t.Errorf("expected 0 transports, got %d", len(srv.transports))
	}

	actual := srv.RegisterTransport(&stubTransport{
		tb: t,
		startFn: func(ctx context.Context, runtime ServerRuntime) error {
			called = true
			return expectedError
		},
	})

	// now there should be one transport registered
	if len(srv.transports) != 1 {
		t.Errorf("expected 1 transport, got %d", len(srv.transports))
	}

	if !called {
		t.Error("expected Start to be called on transport")
	}
	if actual != expectedError {
		t.Errorf("expected error %v, got %v", expectedError, actual)
	}
}

func TestServer_RegisterTransport_Nil(t *testing.T) {
	srv := NewServer(100)

	err := srv.RegisterTransport(nil)

	if err == nil {
		t.Error("expected error when registering nil transport, got nil")
	}
}

func TestServer_RegisterTransport_Shutdown(t *testing.T) {
	srv := NewServer(100)

	// Simulate server shutdown
	srv.shutdown = true

	err := srv.RegisterTransport(&stubTransport{})

	if !errors.Is(err, ErrServerStopped) {
		t.Errorf("expected ErrServerStopped, got %v", err)
	}
}

func TestServer_DeregisterTransport_Normal(t *testing.T) {
	called := false
	// not really an error just testing that the Start function is called and
	// its return value is passed through correctly
	expectedError := fmt.Errorf("expected error")
	srv := NewServer(100)
	transport := &stubTransport{
		tb: t,
		shutdownFn: func(ctx context.Context) error {
			called = true
			return expectedError
		},
	}

	srv.transports = append(srv.transports, transport)

	err := srv.DeregisterTransport(transport)

	if err != expectedError {
		t.Errorf("expected error %v from transport shutdown, got %v", expectedError, err)
	}
	if !called {
		t.Error("expected Shutdown to be called on transport")
	}
	if len(srv.transports) != 0 {
		t.Errorf("expected 0 transports after deregistration, got %d", len(srv.transports))
	}
}

func TestServer_DeregisterTransport_NotFound(t *testing.T) {
	srv := NewServer(100)

	// Simulate deregistering a transport that was never registered
	err := srv.DeregisterTransport(&stubTransport{tb: t})

	if err != nil {
		t.Errorf("expected no error when deregistering non-existent transport, got %v", err)
	}
}

func TestServer_DeregisterTransport_Shutdown(t *testing.T) {
	srv := NewServer(100)

	// Simulate server shutdown
	srv.shutdown = true

	err := srv.DeregisterTransport(&stubTransport{tb: t})

	// this isn't an error
	if err != nil {
		t.Errorf("expected no error when deregistering transport during shutdown, got %v", err)
	}
}

func TestServer_Wait(t *testing.T) {
	srv := NewServer(100)

	// Wait should block until shutdown is called
	done := make(chan struct{})
	go func() {
		srv.Wait()
		close(done)
	}()

	// Wait should not return until shutdown is called
	select {
	case <-done:
		t.Error("expected Wait to block until shutdown, but it returned early")
	default:
	}

	srv.Shutdown()

	select {
	case <-done:
		// success
	case <-time.After(time.Second):
		t.Error("expected Wait to return after shutdown, but it timed out")
	}
}

func TestServer_Wait_MultipleCalls(t *testing.T) {
	srv := NewServer(100)

	// Call Wait multiple times; they should all return after shutdown
	done1 := make(chan struct{})
	done2 := make(chan struct{})
	go func() {
		srv.Wait()
		close(done1)
	}()
	go func() {
		srv.Wait()
		close(done2)
	}()

	srv.Shutdown()

	select {
	case <-done1:
		// success
	case <-time.After(time.Second):
		t.Error("expected first Wait to return after shutdown, but it timed out")
	}

	select {
	case <-done2:
		// success
	case <-time.After(time.Second):
		t.Error("expected second Wait to return after shutdown, but it timed out")
	}
}

func TestServer_Shutdown_Idempotent(t *testing.T) {
	srv := NewServer(100)

	// register a transport that simulates a long shutdown to test that multiple calls to Shutdown don't cause issues
	srv.RegisterTransport(&stubTransport{
		startFn: func(ctx context.Context, runtime ServerRuntime) error {
			return nil
		},
		shutdownFn: func(ctx context.Context) error {
			return fmt.Errorf("expected error")
		},
	})

	// Call Shutdown multiple times; it should not cause errors
	srv.Shutdown()
	srv.Shutdown()

	// If we reach this point without panicking or erroring, the test passes
}

func TestServer_RegisterGetDeviceHandler(t *testing.T) {
	srv := NewServer(100)

	expected := CatenaDevice{
		device: &protos.Device{},
	}

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (CatenaDevice, StatusResult) {
		handlerCalled = true
		return expected, StatusResult{Code: OK}
	})

	// Call the registered handler
	actual, _ := srv.InvokeGetDeviceHandler(0)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if !proto.Equal(actual.GetProtoDevice(), expected.GetProtoDevice()) {
		t.Errorf("expected device %v, got %v", expected, actual)
	}

	// check that the slot is registered
	if !slices.Contains(srv.GetSlots(), uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterGetValueHandler(t *testing.T) {
	srv := NewServer(100)

	expected := CatenaValue{}

	handlerCalled := false
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (CatenaValue, StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		if fqoid != "test/param" {
			t.Errorf("expected fqoid 'test/param', got %s", fqoid)
		}
		return expected, StatusResult{Code: OK}
	})

	actual, _ := srv.InvokeGetValueHandler(0, "test/param")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if !proto.Equal(actual.Value, expected.Value) {
		t.Errorf("expected value %v, got %v", expected, actual)
	}

	// check that the slot is registered
	if !slices.Contains(srv.GetSlots(), uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterSetValueHandler(t *testing.T) {
	srv := NewServer(100)

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) StatusResult {
		handlerCalled = true
		if value != int32(42) {
			t.Errorf("expected value int32(42), got %v", value)
		}
		return StatusResult{Code: OK}
	})

	status := srv.InvokeSetValueHandler(int32(42), 0, "test/param")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}

	// check that the slot is registered
	if !slices.Contains(srv.GetSlots(), uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterGetAssetHandler(t *testing.T) {
	srv := NewServer(100)

	dp := DataPayload{
		Metadata: map[string]string{"content-type": "image/png"},
		Payload:  []byte("fake image"),
	}
	expected, _ := ToCatenaAsset(dp, false)

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (CatenaAsset, StatusResult) {
		handlerCalled = true
		return expected, StatusResult{Code: OK}
	})

	actual, _ := srv.InvokeGetAssetHandler(0, "test/asset")

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if !proto.Equal(actual.GetProtoAsset(), expected.GetProtoAsset()) {
		t.Errorf("expected asset %v, got %v", expected, actual)
	}

	// check that the slot is registered
	if !slices.Contains(srv.GetSlots(), uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterExecuteCommandHandler(t *testing.T) {
	srv := NewServer(100)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (CommandResult, StatusResult) {
		handlerCalled = true
		if commandFqoid != "test/command" {
			t.Errorf("expected commandFqoid 'test/command', got %s", commandFqoid)
		}
		return CommandNoResponse()
	})

	result, _ := srv.InvokeExecuteCommandHandler(0, "test/command", nil)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if !result.IsEmpty() {
		t.Error("expected CommandNoResponse (empty) result")
	}

	// check that the slot is registered
	if !slices.Contains(srv.GetSlots(), uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_InvokeGetDeviceHandler_NoHandler(t *testing.T) {
	srv := NewServer(100)

	if len(srv.GetSlots()) != 0 {
		t.Errorf("expected 0 slots, got %d", len(srv.GetSlots()))
	}

	_, status := srv.InvokeGetDeviceHandler(0)

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_InvokeGetValueHandler_NoHandler(t *testing.T) {
	srv := NewServer(100)

	if len(srv.GetSlots()) != 0 {
		t.Errorf("expected 0 slots, got %d", len(srv.GetSlots()))
	}

	_, status := srv.InvokeGetValueHandler(0, "test/param")

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_InvokeSetValueHandler_NoHandler(t *testing.T) {
	srv := NewServer(100)

	if len(srv.GetSlots()) != 0 {
		t.Errorf("expected 0 slots, got %d", len(srv.GetSlots()))
	}

	status := srv.InvokeSetValueHandler(42, 0, "test/param")

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_InvokeGetAsset_NoHandler(t *testing.T) {
	srv := NewServer(100)

	if len(srv.GetSlots()) != 0 {
		t.Errorf("expected 0 slots, got %d", len(srv.GetSlots()))
	}

	_, status := srv.InvokeGetAssetHandler(0, "test/asset")

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_ExecuteCommand_Route(t *testing.T) {
	srv := NewServer(100)

	if len(srv.GetSlots()) != 0 {
		t.Errorf("expected 0 slots, got %d", len(srv.GetSlots()))
	}

	_, status := srv.InvokeExecuteCommandHandler(0, "test/command", nil)

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

var _ connectionQueueInterface = (*stubConnectionQueue)(nil)

type stubConnectionQueue struct {
	tb           testing.TB
	setMaxFn     func(max int)
	registerFn   func() (int, *Connection)
	deregisterFn func(connID int)
	notifyFn     func(update *protos.PushUpdates)
	shutdownFn   func()
	countFn      func() int
}

func (s *stubConnectionQueue) setMaxConnections(max int) {
	if s.setMaxFn != nil {
		s.setMaxFn(max)
	} else {
		s.tb.Fatalf("setMaxConnections called on stubConnectionQueue without setMaxFn defined")
	}
}

func (s *stubConnectionQueue) registerConnection() (int, *Connection) {
	if s.registerFn != nil {
		return s.registerFn()
	}
	s.tb.Fatalf("registerConnection called on stubConnectionQueue without registerFn defined")
	return 0, nil
}

func (s *stubConnectionQueue) deregisterConnection(connID int) {
	if s.deregisterFn != nil {
		s.deregisterFn(connID)
	} else {
		s.tb.Fatalf("deregisterConnection called on stubConnectionQueue without deregisterFn defined")
	}
}

func (s *stubConnectionQueue) notifyUpdate(update *protos.PushUpdates) {
	if s.notifyFn != nil {
		s.notifyFn(update)
	} else {
		s.tb.Fatalf("notifyUpdate called on stubConnectionQueue without notifyFn defined")
	}
}

func (s *stubConnectionQueue) shutdown() {
	if s.shutdownFn != nil {
		s.shutdownFn()
	} else {
		s.tb.Fatalf("shutdown called on stubConnectionQueue without shutdownFn defined")
	}
}

func (s *stubConnectionQueue) connectionCount() int {
	if s.countFn != nil {
		return s.countFn()
	}
	s.tb.Fatalf("connectionCount called on stubConnectionQueue without countFn defined")
	return 0
}

func TestServer_RegisterConnection_Passthrough(t *testing.T) {
	called := false
	srv := NewServer(100)
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		registerFn: func() (int, *Connection) {
			called = true
			return 77, &Connection{ID: 77}
		},
	}

	connID, conn := srv.RegisterConnection()

	if !called {
		t.Error("expected registerConnection to be called on connection queue")
	}
	if connID != 77 {
		t.Errorf("expected connID 77, got %d", connID)
	}
	if conn == nil || conn.ID != 77 {
		t.Errorf("expected connection ID 77, got %+v", conn)
	}
}

func TestServer_DeregisterConnection_Passthrough(t *testing.T) {
	called := false
	lastConnID := 0
	srv := NewServer(100)
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		deregisterFn: func(connID int) {
			called = true
			lastConnID = connID
		},
	}

	srv.DeregisterConnection(42)

	if !called {
		t.Error("expected deregisterConnection to be called on connection queue")
	}
	if lastConnID != 42 {
		t.Errorf("expected connID 42, got %d", lastConnID)
	}
}

func TestServer_SetMaxConnections_Passthrough(t *testing.T) {
	called := false
	lastMax := 0
	srv := NewServer(100)
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		setMaxFn: func(max int) {
			called = true
			lastMax = max
		},
	}

	srv.SetMaxConnections(321)

	if !called {
		t.Error("expected setMaxConnections to be called on connection queue")
	}
	if lastMax != 321 {
		t.Errorf("expected max 321, got %d", lastMax)
	}
}

func TestServer_ConnectionCount_Passthrough(t *testing.T) {
	srv := NewServer(100)
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		countFn: func() int {
			return 5
		},
	}

	count := srv.ConnectionCount()

	if count != 5 {
		t.Errorf("expected connection count 5, got %d", count)
	}
}
