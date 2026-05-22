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
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-14
 */

package catena

import (
	"context"
	"errors"
	"fmt"
	"slices"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/golang-jwt/jwt/v5"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"google.golang.org/protobuf/proto"
)

const validTestJWT = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJzY29wZSI6InJlYWQgd3JpdGUgYWxsIHN0MjEzODpvcDp3In0."
const validTestJWTWithoutExecuteCommandScope = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJzY29wZSI6InJlYWQgd3JpdGUgYWxsIn0."
const validTestJWTWithCfgScope = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJzY29wZSI6InN0MjEzODpjZmcifQ."

func validTestTransportContext(metadata map[string][]string) TransportContext {
	return TransportContext{
		AccessToken: validTestJWT,
		Metadata:    metadata,
	}
}

func TestServer_ParseTransportContext_ParsesTokenAndMetadata(t *testing.T) {
	srv := NewServer(100, true).(*server)
	ctx, status := srv.parseTransportContext(TransportContext{
		AccessToken: "Bearer " + validTestJWT,
		Metadata:    map[string][]string{"scope": {"read"}},
	})

	if status.Code != OK {
		t.Fatalf("expected OK status, got %v", status)
	}
	if ctx.Token == nil {
		t.Fatal("expected parsed JWT token")
	}
	if ctx.Token.Raw != validTestJWT {
		t.Errorf("expected token raw value %q, got %q", validTestJWT, ctx.Token.Raw)
	}
	if !ctx.HasScope("all") {
		t.Errorf("expected parsed token scopes to include all, got %v", ctx.scopes)
	}
	if !ctx.HasScope("read") || !ctx.HasScope("write") {
		t.Errorf("expected parsed token scopes to include read and write, got %v", ctx.scopes)
	}
	if !ctx.HasScope("st2138:op:w") {
		t.Errorf("expected parsed token scopes to include st2138:op:w, got %v", ctx.scopes)
	}
	if !ctx.HasAnyWriteScope() {
		t.Errorf("expected parsed token scopes to satisfy write access, got %v", ctx.scopes)
	}
	if !ctx.HasAnyReadScope() {
		t.Errorf("expected parsed token scopes to satisfy read access, got %v", ctx.scopes)
	}
	if len(ctx.Metadata["scope"]) != 1 || ctx.Metadata["scope"][0] != "read" {
		t.Errorf("expected metadata to be preserved, got %v", ctx.Metadata)
	}
}

func TestHandlerContext_HasAnyReadScope(t *testing.T) {
	readOnly := HandlerContext{scopes: map[string]struct{}{"st2138:mon": {}}}
	if !readOnly.HasAnyReadScope() {
		t.Fatal("expected read scope to satisfy read access")
	}
	if readOnly.HasAnyWriteScope() {
		t.Fatal("read scope should not satisfy write access")
	}

	writeOnly := HandlerContext{scopes: map[string]struct{}{"st2138:cfg:w": {}}}
	if !writeOnly.HasAnyReadScope() {
		t.Fatal("write scope should satisfy read access")
	}
	if !writeOnly.HasAnyWriteScope() {
		t.Fatal("expected write scope to satisfy write access")
	}
}

func TestServer_ParseTransportContext_InvalidAccessToken(t *testing.T) {
	srv := NewServer(100, true).(*server)

	tests := []struct {
		name        string
		accessToken string
	}{
		{name: "missing", accessToken: ""},
		{name: "malformed", accessToken: "not-a-jwt"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx, status := srv.parseTransportContext(TransportContext{AccessToken: tt.accessToken})
			if status.Code != UNAUTHENTICATED {
				t.Fatalf("expected UNAUTHENTICATED, got %v", status)
			}
			if ctx.Token != nil {
				t.Fatalf("expected no token on parse failure, got %v", ctx.Token)
			}
		})
	}
}

func TestExtractTokenScopes_NonMapClaims(t *testing.T) {
	token := &jwt.Token{
		Claims: &jwt.RegisteredClaims{
			Subject: "test-user",
		},
	}

	scopes, status := extractTokenScopes(token)

	if status.Code != OK {
		t.Fatalf("expected OK status, got %v", status)
	}
	if len(scopes) != 0 {
		t.Errorf("expected no scopes for non-map claims, got %v", scopes)
	}
}

func TestExtractTokenScopes_MissingScopeClaim(t *testing.T) {
	token := &jwt.Token{
		Claims: jwt.MapClaims{
			"sub": "test-user",
		},
	}

	scopes, status := extractTokenScopes(token)

	if status.Code != OK {
		t.Fatalf("expected OK status, got %v", status)
	}
	if len(scopes) != 0 {
		t.Errorf("expected no scopes when scope claim is absent, got %v", scopes)
	}
}

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

func assertContextDeadlineWithin(t *testing.T, ctx context.Context, maxWait time.Duration) {
	t.Helper()

	deadline, ok := ctx.Deadline()
	if !ok {
		t.Fatal("expected shutdown context with deadline")
	}

	remaining := time.Until(deadline)
	if remaining <= 0 {
		t.Fatalf("expected shutdown context deadline in the future, got %v", remaining)
	}
	if remaining > maxWait+250*time.Millisecond {
		t.Fatalf("expected shutdown deadline within %v, got %v remaining", maxWait, remaining)
	}
}

func TestServer_BoundedShutdownContext_NilParent(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.maxShutdownWait = 100 * time.Millisecond

	// make a nil context so we don't get warnings from editors about passing nil
	// contexts. The server should handle this gracefully and not panic.
	// Very cool that editors can do that, but we want to test it here.
	var nilCtx context.Context
	ctx, cancel := srv.boundedShutdownContext(nilCtx)
	defer cancel()

	if ctx == nil {
		t.Fatal("expected non-nil shutdown context")
	}

	assertContextDeadlineWithin(t, ctx, srv.maxShutdownWait)
	if err := ctx.Err(); err != nil {
		t.Fatalf("expected active shutdown context, got %v", err)
	}
}

func TestServer_BoundedShutdownContext_NoWait(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.maxShutdownWait = 0 // disabled

	parent := context.Background()
	ctx, cancel := srv.boundedShutdownContext(parent)
	defer cancel()

	if ctx != parent {
		t.Fatal("expected parent context to be returned unchanged when maxShutdownWait <= 0")
	}
	if _, ok := ctx.Deadline(); ok {
		t.Fatal("expected no deadline on returned context when maxShutdownWait <= 0")
	}
}

func TestServer_Shutdown_CallsShutdown(t *testing.T) {
	called := false

	srv := NewServer(100, true).(*server)
	srv.maxShutdownWait = 100 * time.Millisecond
	srv.connectionQueue = &stubConnectionQueue{
		shutdownFn: func(ctx context.Context) {
			called = true
			assertContextDeadlineWithin(t, ctx, srv.maxShutdownWait)
		},
	}

	srv.Shutdown(context.Background())
	if !called {
		t.Error("expected shutdown to call shutdown on connection queue")
	}
}

func TestServer_RegisterTransport_Normal(t *testing.T) {
	called := false
	srv := NewServer(100, true).(*server)

	// server starts with new transports
	if len(srv.transports) != 0 {
		t.Errorf("expected 0 transports, got %d", len(srv.transports))
	}

	err := srv.RegisterTransport(&stubTransport{
		tb: t,
		startFn: func(ctx context.Context, runtime ServerRuntime) error {
			called = true
			if ctx == nil {
				t.Error("expected non-nil context")
			}
			if runtime != srv {
				t.Errorf("expected runtime to be the server, got %v", runtime)
			}
			return nil
		},
	})

	// now there should be one transport registered
	if len(srv.transports) != 1 {
		t.Errorf("expected 1 transport, got %d", len(srv.transports))
	}

	if !called {
		t.Error("expected Start to be called on transport")
	}
	if err != nil {
		t.Errorf("expected no error from RegisterTransport, got %v", err)
	}
}

func TestServer_RegisterTransport_Nil(t *testing.T) {
	srv := NewServer(100, true).(*server)

	err := srv.RegisterTransport(nil)

	if err == nil {
		t.Error("expected error when registering nil transport, got nil")
	}
}

func TestServer_RegisterTransport_StartupError(t *testing.T) {
	expectedError := fmt.Errorf("expected startup error")
	srv := NewServer(100, true).(*server)
	err := srv.RegisterTransport(&stubTransport{
		tb: t,
		startFn: func(ctx context.Context, runtime ServerRuntime) error {
			return expectedError
		},
	})

	if err != expectedError {
		t.Errorf("expected error %v from transport startup, got %v", expectedError, err)
	}
	if len(srv.transports) != 0 {
		t.Errorf("expected 0 transports to be registered on startup error, got %d", len(srv.transports))
	}
}

func TestServer_RegisterTransport_Shutdown(t *testing.T) {
	srv := NewServer(100, true).(*server)

	// Simulate server shutdown
	srv.shutdown = true

	err := srv.RegisterTransport(&stubTransport{})

	if !errors.Is(err, ErrServerStopped) {
		t.Errorf("expected ErrServerStopped, got %v", err)
	}
}

func TestServer_DeregisterTransport_Normal(t *testing.T) {
	called := false
	shutdownCalled := false
	// not really an error just testing that the Start function is called and
	// its return value is passed through correctly
	expectedError := fmt.Errorf("expected error")
	srv := NewServer(100, true).(*server)
	srv.maxShutdownWait = 100 * time.Millisecond
	transport := &stubTransport{
		tb: t,
		shutdownFn: func(ctx context.Context) error {
			called = true
			assertContextDeadlineWithin(t, ctx, srv.maxShutdownWait)
			return expectedError
		},
	}
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		shutdownOwnerFn: func(ctx context.Context, gotOwner any) {
			shutdownCalled = true
			if gotOwner != transport {
				t.Errorf("expected shutdownOwner to be called with the transport as owner, got %v", gotOwner)
			}
			assertContextDeadlineWithin(t, ctx, srv.maxShutdownWait)
		},
	}

	srv.transports = append(srv.transports, transport)

	err := srv.DeregisterTransport(context.Background(), transport)

	if err != expectedError {
		t.Errorf("expected error %v from transport shutdown, got %v", expectedError, err)
	}
	if !called {
		t.Error("expected Shutdown to be called on transport")
	}
	if len(srv.transports) != 0 {
		t.Errorf("expected 0 transports after deregistration, got %d", len(srv.transports))
	}
	if !shutdownCalled {
		t.Error("expected shutdownOwner to be called on connection queue")
	}
}

func TestServer_DeregisterTransport_NotFound(t *testing.T) {
	srv := NewServer(100, true).(*server)

	// Simulate deregistering a transport that was never registered
	err := srv.DeregisterTransport(context.Background(), &stubTransport{tb: t})

	if err != nil {
		t.Errorf("expected no error when deregistering non-existent transport, got %v", err)
	}
}

func TestServer_DeregisterTransport_Shutdown(t *testing.T) {
	srv := NewServer(100, true).(*server)

	// Simulate server shutdown
	srv.shutdown = true

	err := srv.DeregisterTransport(context.Background(), &stubTransport{tb: t})

	// this isn't an error
	if err != nil {
		t.Errorf("expected no error when deregistering transport during shutdown, got %v", err)
	}
}

func TestServer_Wait(t *testing.T) {
	srv := NewServer(100, true).(*server)

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

	srv.Shutdown(context.Background())

	select {
	case <-done:
		// success
	case <-time.After(time.Second):
		t.Error("expected Wait to return after shutdown, but it timed out")
	}
}

func TestServer_Wait_MultipleCalls(t *testing.T) {
	srv := NewServer(100, true).(*server)

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

	srv.Shutdown(context.Background())

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
	srv := NewServer(100, true).(*server)

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
	srv.Shutdown(context.Background())
	srv.Shutdown(context.Background())

	// If we reach this point without panicking or erroring, the test passes
}

func TestServer_RegisterGetDeviceHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)
	transportContext := validTestTransportContext(map[string][]string{"tenant": {"test"}})

	expected := Device{
		device: &protos.Device{},
	}

	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		if ctx.Token == nil || ctx.Token.Raw != transportContext.AccessToken {
			t.Errorf("expected parsed token from access token")
		}
		if !ctx.HasScope("all") {
			t.Errorf("expected parsed token scopes to include all, got %v", ctx.scopes)
		}
		if len(ctx.Metadata["tenant"]) != 1 || ctx.Metadata["tenant"][0] != "test" {
			t.Errorf("expected tenant metadata to be passed through, got %v", ctx.Metadata)
		}
		return Reply(expected)
	})

	// Call the registered handler
	actual, status := srv.InvokeGetDeviceHandler(0, transportContext)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if !proto.Equal(actual.GetProtoDevice(), expected.GetProtoDevice()) {
		t.Errorf("expected device %v, got %v", expected, actual)
	}

	// check that the slot is registered
	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if !slices.Contains(slots, uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterGetDeviceHandler_SendsSlotsAdded(t *testing.T) {
	srv := NewServer(100, true).(*server)
	var updates []*protos.PushUpdates
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		notifyFn: func(update *protos.PushUpdates, scopes []string) {
			updates = append(updates, update)
		},
	}

	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
		return Reply(Device{})
	})

	if len(updates) != 1 {
		t.Fatalf("expected 1 slots_added update, got %d", len(updates))
	}
	expected := &protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: []uint32{0},
			},
		},
	}
	if !proto.Equal(updates[0], expected) {
		t.Errorf("expected slots_added update %v, got %v", expected, updates[0])
	}
}

func TestServer_RegisterGetValueHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)
	transportContext := validTestTransportContext(nil)

	expected := Value{}

	handlerCalled := false
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		if fqoid != "test/param" {
			t.Errorf("expected fqoid 'test/param', got %s", fqoid)
		}
		if ctx.Token == nil || ctx.Token.Raw != transportContext.AccessToken {
			t.Errorf("expected parsed token from access token")
		}
		if !ctx.HasScope("all") {
			t.Errorf("expected parsed token scopes to include all, got %v", ctx.scopes)
		}
		return Reply(expected)
	})

	actual, status := srv.InvokeGetValueHandler(0, "test/param", transportContext)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if !proto.Equal(actual.Value, expected.Value) {
		t.Errorf("expected value %v, got %v", expected, actual)
	}

	// check that the slot is registered
	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if !slices.Contains(slots, uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterSetValueHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)
	transportContext := validTestTransportContext(nil)

	handlerCalled := false
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string, ctx HandlerContext) StatusResult {
		handlerCalled = true
		if value != int32(42) {
			t.Errorf("expected value int32(42), got %v", value)
		}
		if ctx.Token == nil || ctx.Token.Raw != transportContext.AccessToken {
			t.Errorf("expected parsed token from access token")
		}
		if !ctx.HasScope("all") {
			t.Errorf("expected parsed token scopes to include all, got %v", ctx.scopes)
		}
		return StatusResult{Code: OK}
	})

	status := srv.InvokeSetValueHandler(int32(42), 0, "test/param", transportContext)

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}

	// check that the slot is registered
	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if !slices.Contains(slots, uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterGetAssetHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	dp := DataPayload{
		Metadata: map[string]string{"content-type": "image/png"},
		Payload:  []byte("fake image"),
	}
	expected, _ := ToAsset(dp, false)

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Asset, StatusResult) {
		handlerCalled = true
		return Reply(expected)
	})

	actual, status := srv.InvokeGetAssetHandler(0, "test/asset", validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if !proto.Equal(actual.GetProtoAsset(), expected.GetProtoAsset()) {
		t.Errorf("expected asset %v, got %v", expected, actual)
	}

	// check that the slot is registered
	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if !slices.Contains(slots, uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterExecuteCommandHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any, ctx HandlerContext) (CommandResult, StatusResult) {
		handlerCalled = true
		if commandFqoid != "test/command" {
			t.Errorf("expected commandFqoid 'test/command', got %s", commandFqoid)
		}
		return CommandNoResponse()
	})

	result, status := srv.InvokeExecuteCommandHandler(0, "test/command", nil, validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if !result.IsEmpty() {
		t.Error("expected CommandNoResponse (empty) result")
	}

	// check that the slot is registered
	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if !slices.Contains(slots, uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_InvokeExecuteCommandHandler_RequiresWriteScope(t *testing.T) {
	srv := NewServer(100, true).(*server)
	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any, ctx HandlerContext) (CommandResult, StatusResult) {
		handlerCalled = true
		return CommandNoResponse()
	})

	_, status := srv.InvokeExecuteCommandHandler(0, "test/command", nil, TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	})

	if handlerCalled {
		t.Fatal("execute command handler should not run without a write scope")
	}
	if status.Code != PERMISSION_DENIED {
		t.Errorf("expected PERMISSION_DENIED, got %v", status.Code)
	}
}

func TestServer_ReadEndpointsRequireReadScope(t *testing.T) {
	transportContext := TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	}

	tests := []struct {
		name   string
		invoke func(*server, *bool) StatusResult
	}{
		{
			name: "get device",
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
					*handlerCalled = true
					return Reply(Device{})
				})
				_, status := srv.InvokeGetDeviceHandler(0, transportContext)
				return status
			},
		},
		{
			name: "get value",
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
					*handlerCalled = true
					return Reply(Value{})
				})
				_, status := srv.InvokeGetValueHandler(0, "test/param", transportContext)
				return status
			},
		},
		{
			name: "get asset",
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Asset, StatusResult) {
					*handlerCalled = true
					return Reply(Asset{})
				})
				_, status := srv.InvokeGetAssetHandler(0, "test/asset", transportContext)
				return status
			},
		},
		{
			name: "param info",
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext) ([]ParamInfo, StatusResult) {
					*handlerCalled = true
					return []ParamInfo{}, StatusWithCode(OK, "")
				})
				_, status := srv.InvokeParamInfoHandler(0, "test/param", false, transportContext)
				return status
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := NewServer(100, true).(*server)
			handlerCalled := false

			status := tt.invoke(srv, &handlerCalled)

			if handlerCalled {
				t.Fatal("endpoint handler should not run without a read scope")
			}
			if status.Code != PERMISSION_DENIED {
				t.Errorf("expected PERMISSION_DENIED, got %v", status.Code)
			}
		})
	}
}

func TestServer_WriteEndpointsRequireWriteScope(t *testing.T) {
	transportContext := TransportContext{
		AccessToken: validTestJWTWithCfgScope,
	}

	tests := []struct {
		name   string
		invoke func(*server, *bool) StatusResult
	}{
		{
			name: "set value",
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string, ctx HandlerContext) StatusResult {
					*handlerCalled = true
					return StatusWithCode(OK, "")
				})
				return srv.InvokeSetValueHandler(int32(42), 0, "test/param", transportContext)
			},
		},
		{
			name: "execute command",
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any, ctx HandlerContext) (CommandResult, StatusResult) {
					*handlerCalled = true
					return CommandNoResponse()
				})
				_, status := srv.InvokeExecuteCommandHandler(0, "test/command", nil, transportContext)
				return status
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := NewServer(100, true).(*server)
			handlerCalled := false

			status := tt.invoke(srv, &handlerCalled)

			if handlerCalled {
				t.Fatal("endpoint handler should not run without a write scope")
			}
			if status.Code != PERMISSION_DENIED {
				t.Errorf("expected PERMISSION_DENIED, got %v", status.Code)
			}
		})
	}
}

func TestServer_RegisterParamInfoHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	handlerCalled := false
	srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext) ([]ParamInfo, StatusResult) {
		handlerCalled = true
		if oidPrefix != "test/param" {
			t.Errorf("expected oidPrefix 'test/param', got %s", oidPrefix)
		}
		return []ParamInfo{}, StatusResult{Code: OK}
	})

	actual, status := srv.InvokeParamInfoHandler(0, "test/param", false, validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if len(actual) != 0 {
		t.Errorf("expected 0 param info entries, got %d", len(actual))
	}

	// check that the slot is registered
	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if !slices.Contains(slots, uint16(0)) {
		t.Error("slot 0 should be registered in server slots")
	}
}

func TestServer_RegisterHeartbeatHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	srv.RegisterHeartbeatHandler(0, func(slot uint16) {})

	if srv.heartbeatHandlers[0] == nil {
		t.Error("expected heartbeat handler to be registered for slot 0")
	}
	// there's no invoke function for heartbeat handlers since they're just called by the server on a timer, so we'll just call it directly to test that it works
}

func TestServer_InvokeGetDeviceHandler_NoHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if len(slots) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slots))
	}

	_, status = srv.InvokeGetDeviceHandler(0, validTestTransportContext(nil))

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_InvokeGetValueHandler_NoHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if len(slots) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slots))
	}

	_, status = srv.InvokeGetValueHandler(0, "test/param", validTestTransportContext(nil))

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_InvokeSetValueHandler_NoHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if len(slots) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slots))
	}

	status = srv.InvokeSetValueHandler(42, 0, "test/param", validTestTransportContext(nil))

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_InvokeGetAsset_NoHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if len(slots) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slots))
	}

	_, status = srv.InvokeGetAssetHandler(0, "test/asset", validTestTransportContext(nil))

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_ExecuteCommand_Route(t *testing.T) {
	srv := NewServer(100, true).(*server)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if len(slots) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slots))
	}

	_, status = srv.InvokeExecuteCommandHandler(0, "test/command", nil, validTestTransportContext(nil))

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_ParamInfo_NoHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Errorf("expected OK status from GetSlots, got %v", status.Code)
	}
	if len(slots) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slots))
	}

	_, status = srv.InvokeParamInfoHandler(0, "test/param", false, validTestTransportContext(nil))

	// no handler should return NOT_FOUND status
	if status.Code != NOT_FOUND {
		t.Errorf("expected NOT_FOUND status, got %v", status.Code)
	}
}

func TestServer_RegisterAccessHandlerNilResetsToAllowAll(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.slots[0] = struct{}{}

	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		return false
	})

	_, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != PERMISSION_DENIED {
		t.Fatalf("expected PERMISSION_DENIED with deny handler, got %v", status.Code)
	}

	srv.RegisterAccessHandler(nil)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != OK {
		t.Fatalf("expected OK after nil reset to default handler, got %v", status.Code)
	}
	if len(slots) != 1 || slots[0] != 0 {
		t.Errorf("expected slot [0], got %v", slots)
	}
}

func TestServer_AccessHandlerDeniesEndpoint(t *testing.T) {
	srv := NewServer(100, true).(*server)
	handlerCalled := false
	accessCalled := false
	transportContext := validTestTransportContext(map[string][]string{"scope": {"none"}})

	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		accessCalled = true
		if endpointType != EndpointGetValue {
			t.Errorf("expected EndpointGetValue, got %v", endpointType)
		}
		if ctx.Token == nil || ctx.Token.Raw != transportContext.AccessToken {
			t.Errorf("expected parsed token from access token")
		}
		if !ctx.HasScope("all") {
			t.Errorf("expected parsed token scopes to include all, got %v", ctx.scopes)
		}
		if len(ctx.Metadata["scope"]) != 1 || ctx.Metadata["scope"][0] != "none" {
			t.Errorf("expected scope metadata to be passed through, got %v", ctx.Metadata)
		}
		return false
	})
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		handlerCalled = true
		return Reply(Value{})
	})

	_, status := srv.InvokeGetValueHandler(0, "test/param", transportContext)

	if !accessCalled {
		t.Fatal("expected access handler to be called")
	}
	if handlerCalled {
		t.Fatal("endpoint handler should not run when access is denied")
	}
	if status.Code != PERMISSION_DENIED {
		t.Errorf("expected PERMISSION_DENIED, got %v", status.Code)
	}
}

func TestServer_GetSlots_AllowsNoCatenaScopeWhenAccessHandlerAllows(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.slots[0] = struct{}{}
	srv.slots[5] = struct{}{}

	accessCalled := false
	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		accessCalled = true
		if endpointType != EndpointGetSlots {
			t.Errorf("expected EndpointGetSlots, got %v", endpointType)
		}
		if ctx.HasAnyReadScope() {
			t.Errorf("expected no Catena read scope, got %v", ctx.scopes)
		}
		if ctx.HasAnyWriteScope() {
			t.Errorf("expected no Catena write scope, got %v", ctx.scopes)
		}
		return true
	})

	slots, status := srv.GetSlots(TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	})

	if !accessCalled {
		t.Fatal("expected access handler to be called")
	}
	if status.Code != OK {
		t.Fatalf("expected OK, got %v", status.Code)
	}
	slices.Sort(slots)
	if !slices.Equal(slots, []uint16{0, 5}) {
		t.Errorf("expected slots [0, 5], got %v", slots)
	}
}

func TestServer_GetSlots_DeniedByAccessHandler(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.slots[0] = struct{}{}

	accessCalled := false
	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		accessCalled = true
		if endpointType != EndpointGetSlots {
			t.Errorf("expected EndpointGetSlots, got %v", endpointType)
		}
		return false
	})

	slots, status := srv.GetSlots(validTestTransportContext(nil))

	if !accessCalled {
		t.Fatal("expected access handler to be called")
	}
	if status.Code != PERMISSION_DENIED {
		t.Errorf("expected PERMISSION_DENIED, got %v", status.Code)
	}
	if slots != nil {
		t.Errorf("expected no slots when access is denied, got %v", slots)
	}
}

func TestServer_GetSlots_InvalidTokenSkipsAccessHandler(t *testing.T) {
	tests := []struct {
		name        string
		accessToken string
	}{
		{name: "missing", accessToken: ""},
		{name: "malformed", accessToken: "not-a-jwt"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := NewServer(100, true).(*server)
			srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
				t.Fatal("access handler should not run when token parsing fails")
				return true
			})

			slots, status := srv.GetSlots(TransportContext{AccessToken: tt.accessToken})

			if status.Code != UNAUTHENTICATED {
				t.Errorf("expected UNAUTHENTICATED, got %v", status.Code)
			}
			if slots != nil {
				t.Errorf("expected no slots when token parsing fails, got %v", slots)
			}
		})
	}
}

func TestServer_GetSlotsAndConnect_HaveDifferentScopeRequirements(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.slots[0] = struct{}{}

	endpoints := []EndpointType{}
	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		endpoints = append(endpoints, endpointType)
		return true
	})
	transportContext := TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	}

	slots, status := srv.GetSlots(transportContext)
	if status.Code != OK {
		t.Fatalf("expected GetSlots to succeed with access handler approval, got %v", status.Code)
	}
	if !slices.Equal(slots, []uint16{0}) {
		t.Errorf("expected slot [0], got %v", slots)
	}

	conn, status := srv.RegisterTransportConnection(nil, transportContext)
	if status.Code != PERMISSION_DENIED {
		t.Errorf("expected Connect to require read scope, got %v", status.Code)
	}
	if conn != nil {
		t.Errorf("expected no connection without read scope, got %+v", conn)
	}
	if !slices.Contains(endpoints, EndpointGetSlots) {
		t.Errorf("expected EndpointGetSlots access check, got %v", endpoints)
	}
	if !slices.Contains(endpoints, EndpointConnect) {
		t.Errorf("expected EndpointConnect access check, got %v", endpoints)
	}
}

func TestServer_AuthzDisabledBypassesAccessAndScopeChecks(t *testing.T) {
	srv := NewServer(100, false).(*server)
	handlerCalled := false
	accessCalled := false

	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		accessCalled = true
		return false
	})
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		handlerCalled = true
		return Reply(Value{})
	})

	_, status := srv.InvokeGetValueHandler(0, "test/param", TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	})

	if accessCalled {
		t.Fatal("access handler should not run when authz is disabled")
	}
	if !handlerCalled {
		t.Fatal("endpoint handler should run when authz is disabled")
	}
	if status.Code != OK {
		t.Errorf("expected OK, got %v", status.Code)
	}
}

var _ connectionQueueInterface = (*stubConnectionQueue)(nil)

type stubConnectionQueue struct {
	tb              testing.TB
	setMaxFn        func(max int)
	registerOwnedFn func(owner any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult)
	deregisterFn    func(connID int)
	notifyFn        func(update *protos.PushUpdates, scopes []string)
	shutdownFn      func(ctx context.Context)
	shutdownOwnerFn func(ctx context.Context, owner any)
	shutdownConnFn  func(ctx context.Context, conn *Connection)
	countFn         func() int
}

func (s *stubConnectionQueue) setMaxConnections(max int) {
	if s.setMaxFn != nil {
		s.setMaxFn(max)
	} else {
		s.tb.Fatalf("setMaxConnections called on stubConnectionQueue without setMaxFn defined")
	}
}

func (s *stubConnectionQueue) registerOwnedConnection(owner Transport, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult) {
	if s.registerOwnedFn != nil {
		return s.registerOwnedFn(owner, handlerContext, initialUpdate)
	}
	s.tb.Fatalf("registerOwnedConnection called on stubConnectionQueue without registerOwnedFn defined")
	return nil, StatusResult{Code: INTERNAL}
}

func (s *stubConnectionQueue) deregisterConnection(connID int) {
	if s.deregisterFn != nil {
		s.deregisterFn(connID)
	} else {
		s.tb.Fatalf("deregisterConnection called on stubConnectionQueue without deregisterFn defined")
	}
}

func (s *stubConnectionQueue) notifyUpdate(update *protos.PushUpdates, scopes []string) {
	if s.notifyFn != nil {
		s.notifyFn(update, scopes)
	} else {
		s.tb.Fatalf("notifyUpdate called on stubConnectionQueue without notifyFn defined")
	}
}

func (s *stubConnectionQueue) shutdown(ctx context.Context) {
	if s.shutdownFn != nil {
		s.shutdownFn(ctx)
	} else {
		s.tb.Fatalf("shutdown called on stubConnectionQueue without shutdownFn defined")
	}
}

func (s *stubConnectionQueue) shutdownOwner(ctx context.Context, owner Transport) {
	if s.shutdownOwnerFn != nil {
		s.shutdownOwnerFn(ctx, owner)
	} else {
		s.tb.Fatalf("shutdownOwner called on stubConnectionQueue without shutdownOwnerFn defined")
	}
}

func (s *stubConnectionQueue) shutdownConnection(ctx context.Context, conn *Connection) {
	if s.shutdownConnFn != nil {
		s.shutdownConnFn(ctx, conn)
	} else {
		s.tb.Fatalf("shutdownConnection called on stubConnectionQueue without shutdownConnFn defined")
	}
}

func (s *stubConnectionQueue) connectionCount() int {
	if s.countFn != nil {
		return s.countFn()
	}
	s.tb.Fatalf("connectionCount called on stubConnectionQueue without countFn defined")
	return 0
}

func TestServer_DeregisterConnection_Passthrough(t *testing.T) {
	called := false
	lastConnID := 0
	srv := NewServer(100, true).(*server)
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

func TestServer_RegisterTransportConnection_Passthrough(t *testing.T) {
	called := false
	transport := &stubTransport{tb: t}
	srv := NewServer(100, true).(*server)
	srv.slots[0] = struct{}{}
	srv.slots[5] = struct{}{}
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		registerOwnedFn: func(gotTransport any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult) {
			called = true
			if gotTransport != transport {
				t.Error("expected transport owner to be passed through")
			}
			if !handlerContext.HasScope(ScopeOpWrite) {
				t.Errorf("expected handler context to be passed through, got %v", handlerContext.scopes)
			}
			if initialUpdate == nil {
				t.Fatal("expected initial update, got nil")
			}
			if initialUpdate.GetSlotsAdded() == nil {
				t.Fatal("expected SlotsAdded update to have SlotsAdded field, got nil")
			}
			slots := initialUpdate.GetSlotsAdded().GetSlots()
			slices.Sort(slots)
			if !slices.Equal(slots, []uint32{0, 5}) {
				t.Errorf("expected slots [0, 5], got %v", slots)
			}
			return &Connection{
				ID:      78,
				Updates: make(chan *protos.PushUpdates, 10),
				Done:    make(chan struct{}),
			}, StatusResult{Code: OK}
		},
	}

	conn, status := srv.RegisterTransportConnection(transport, validTestTransportContext(nil))

	if !called {
		t.Error("expected registerOwnedConnection to be called on connection queue")
	}
	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if conn == nil || conn.ID != 78 {
		t.Errorf("expected connection ID 78, got %+v", conn)
	}
}

func TestServer_RegisterTransportConnection_Failed(t *testing.T) {
	srv := NewServer(100, true).(*server)
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		registerOwnedFn: func(gotOwner any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult) {
			return nil, StatusResult{Code: RESOURCE_EXHAUSTED, Error: "queue full"}
		},
	}

	conn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))

	if status.Code != RESOURCE_EXHAUSTED {
		t.Errorf("expected RESOURCE_EXHAUSTED on registration failure, got %v", status.Code)
	}
	if conn != nil {
		t.Errorf("expected nil connection on registration failure, got %+v", conn)
	}
}

func TestServer_SetMaxConnections_Passthrough(t *testing.T) {
	called := false
	lastMax := 0
	srv := NewServer(100, true).(*server)
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

func TestServer_ShutdownTransportConnections_Passthrough(t *testing.T) {
	called := false
	transport := &stubTransport{tb: t}
	srv := NewServer(100, true).(*server)
	srv.maxShutdownWait = 100 * time.Millisecond
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		shutdownOwnerFn: func(ctx context.Context, gotOwner any) {
			called = true
			assertContextDeadlineWithin(t, ctx, srv.maxShutdownWait)
			if gotOwner != transport {
				t.Errorf("expected transport %v, got %v", transport, gotOwner)
			}
		},
	}

	srv.ShutdownTransportConnections(context.Background(), transport)

	if !called {
		t.Error("expected shutdown to be called on connection queue")
	}
}

func TestServer_ConnectionCount_Passthrough(t *testing.T) {
	srv := NewServer(100, true).(*server)
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

func TestServer_BroadcastUpdate_Normal(t *testing.T) {
	srv := NewServer(100, true).(*server)

	// Register a connection
	conn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != OK {
		t.Fatalf("Failed to register connection: %v", status)
	}
	defer srv.DeregisterConnection(conn.ID)

	select {
	// read off the initial update
	case _ = <-conn.Updates:
	default:
		t.Fatal("expected initial update on new connection")
	}

	// Broadcast an update
	srv.BroadcastUpdate(0, "test/param", int32(42), []string{ScopeOpWrite})

	// Verify the update was received
	select {
	case update := <-conn.Updates:
		if update.Slot != 0 {
			t.Errorf("expected slot 0, got %d", update.Slot)
		}
		pv, ok := update.Kind.(*protos.PushUpdates_Value)
		if !ok {
			t.Fatal("expected PushUpdates_Value")
		}
		if pv.Value.GetOid() != "test/param" {
			t.Errorf("expected oid 'test/param', got '%s'", pv.Value.GetOid())
		}
		if pv.Value.GetValue().GetInt32Value() != 42 {
			t.Errorf("expected value 42, got %d", pv.Value.GetValue().GetInt32Value())
		}
	case <-time.After(time.Second):
		t.Fatal("did not receive broadcast update")
	}
}

func TestServer_BroadcastUpdate_FiltersConnectionsByScope(t *testing.T) {
	srv := NewServer(100, true).(*server)

	matchingConn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != OK {
		t.Fatalf("Failed to register matching connection: %v", status)
	}
	defer srv.DeregisterConnection(matchingConn.ID)

	nonMatchingConn, status := srv.RegisterTransportConnection(nil, TransportContext{
		AccessToken: validTestJWTWithCfgScope,
	})
	if status.Code != OK {
		t.Fatalf("Failed to register non-matching connection: %v", status)
	}
	defer srv.DeregisterConnection(nonMatchingConn.ID)

	for _, conn := range []*Connection{matchingConn, nonMatchingConn} {
		select {
		case <-conn.Updates:
		default:
			t.Fatal("expected initial update on new connection")
		}
	}

	srv.BroadcastUpdate(0, "test/param", int32(42), []string{ScopeOpWrite})

	select {
	case <-matchingConn.Updates:
	case <-time.After(time.Second):
		t.Fatal("matching connection did not receive broadcast update")
	}

	select {
	case <-nonMatchingConn.Updates:
		t.Fatal("non-matching connection should not receive broadcast update")
	default:
	}
}

func TestServer_BroadcastUpdate_AuthzDisabledBypassesScopeFilter(t *testing.T) {
	srv := NewServer(100, false).(*server)

	matchingConn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != OK {
		t.Fatalf("Failed to register matching connection: %v", status)
	}
	defer srv.DeregisterConnection(matchingConn.ID)

	noScopeConn, status := srv.RegisterTransportConnection(nil, TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	})
	if status.Code != OK {
		t.Fatalf("Failed to register no-scope connection: %v", status)
	}
	defer srv.DeregisterConnection(noScopeConn.ID)

	for _, conn := range []*Connection{matchingConn, noScopeConn} {
		select {
		case <-conn.Updates:
		default:
			t.Fatal("expected initial update on new connection")
		}
	}

	srv.BroadcastUpdate(0, "test/param", int32(42), []string{ScopeOpWrite})

	for _, conn := range []*Connection{matchingConn, noScopeConn} {
		select {
		case <-conn.Updates:
		case <-time.After(time.Second):
			t.Fatal("connection did not receive broadcast update")
		}
	}
}

func TestServer_BroadcastUpdate_InvalidValue(t *testing.T) {
	srv := NewServer(100, true).(*server)

	conn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != OK {
		t.Fatalf("Failed to register connection: %v", status)
	}
	defer srv.DeregisterConnection(conn.ID)

	select {
	// read off the initial update
	case _ = <-conn.Updates:
	default:
		t.Fatal("expected initial update on new connection")
	}

	// Try to broadcast an invalid value (bool is not supported by ToProto)
	srv.BroadcastUpdate(0, "test/param", true, []string{ScopeOpWrite})

	// Should not receive update (logged error instead)
	select {
	case <-conn.Updates:
		t.Error("should not have received update for invalid value")
	case <-time.After(100 * time.Millisecond):
	}
}

func TestServer_StartHeartbeat_InvalidInterval(t *testing.T) {
	srv := NewServer(100, true).(*server)

	// Zero interval should be rejected and heartbeat should remain nil
	srv.StartHeartbeat(0)
	if srv.heartbeat != nil {
		t.Error("heartbeat should remain nil after invalid interval")
	}

	// Negative interval should also be rejected
	srv.StartHeartbeat(-1 * time.Second)
	if srv.heartbeat != nil {
		t.Error("heartbeat should remain nil after negative interval")
	}
}

func TestServer_StartHeartbeat_StartsHeartbeat(t *testing.T) {
	srv := NewServer(100, true).(*server)
	defer srv.StopHeartbeat()

	srv.StartHeartbeat(10 * time.Millisecond)

	srv.mu.Lock()
	hb := srv.heartbeat
	srv.mu.Unlock()

	if hb == nil {
		t.Fatal("heartbeat should be set after StartHeartbeat")
	}
	if !hb.IsRunning() {
		t.Error("heartbeat should be running after StartHeartbeat")
	}
}

func TestServer_StartHeartbeat_InvokesHandlers(t *testing.T) {
	srv := NewServer(100, true).(*server)
	defer srv.StopHeartbeat()

	var slot0Called, slot1Called atomic.Int32
	srv.RegisterHeartbeatHandler(0, func(slot uint16) { slot0Called.Add(1) })
	srv.RegisterHeartbeatHandler(1, func(slot uint16) { slot1Called.Add(1) })

	srv.StartHeartbeat(10 * time.Millisecond)
	time.Sleep(55 * time.Millisecond)
	srv.StopHeartbeat()

	if slot0Called.Load() < 3 {
		t.Errorf("expected slot 0 handler to be called at least 3 times, got %d", slot0Called.Load())
	}
	if slot1Called.Load() < 3 {
		t.Errorf("expected slot 1 handler to be called at least 3 times, got %d", slot1Called.Load())
	}
}

func TestServer_StartHeartbeat_HandlerReceivesCorrectSlot(t *testing.T) {
	srv := NewServer(100, true).(*server)
	defer srv.StopHeartbeat()

	var receivedSlots sync.Map
	srv.RegisterHeartbeatHandler(5, func(slot uint16) {
		receivedSlots.Store(slot, true)
	})
	srv.RegisterHeartbeatHandler(7, func(slot uint16) {
		receivedSlots.Store(slot, true)
	})

	srv.StartHeartbeat(10 * time.Millisecond)
	time.Sleep(35 * time.Millisecond)
	srv.StopHeartbeat()

	if _, ok := receivedSlots.Load(uint16(5)); !ok {
		t.Error("slot 5 handler should have been called with slot 5")
	}
	if _, ok := receivedSlots.Load(uint16(7)); !ok {
		t.Error("slot 7 handler should have been called with slot 7")
	}
}

func TestServer_StartHeartbeat_ReplacesExisting(t *testing.T) {
	srv := NewServer(100, true).(*server)
	defer srv.StopHeartbeat()

	srv.StartHeartbeat(10 * time.Millisecond)

	srv.mu.Lock()
	first := srv.heartbeat
	srv.mu.Unlock()

	srv.StartHeartbeat(20 * time.Millisecond)

	srv.mu.Lock()
	second := srv.heartbeat
	srv.mu.Unlock()

	if second == first {
		t.Error("StartHeartbeat should replace the existing heartbeat with a new instance")
	}
	if !second.IsRunning() {
		t.Error("new heartbeat should be running")
	}
	if first.IsRunning() {
		t.Error("old heartbeat should have been stopped")
	}
}

func TestServer_StartHeartbeat_InvalidIntervalPreservesExisting(t *testing.T) {
	srv := NewServer(100, true).(*server)
	defer srv.StopHeartbeat()

	srv.StartHeartbeat(10 * time.Millisecond)

	srv.mu.Lock()
	original := srv.heartbeat
	srv.mu.Unlock()

	// Invalid interval should not replace the running heartbeat
	srv.StartHeartbeat(0)

	srv.mu.Lock()
	current := srv.heartbeat
	srv.mu.Unlock()

	if current != original {
		t.Error("invalid interval should not replace the existing heartbeat")
	}
	if !current.IsRunning() {
		t.Error("existing heartbeat should still be running")
	}
}

func TestServer_StartHeartbeat_HandlerPanicRecovered(t *testing.T) {
	srv := NewServer(100, true).(*server)
	defer srv.StopHeartbeat()

	var afterPanic atomic.Int32
	srv.RegisterHeartbeatHandler(0, func(slot uint16) {
		afterPanic.Add(1)
		panic("test panic in heartbeat handler")
	})

	srv.StartHeartbeat(10 * time.Millisecond)
	time.Sleep(55 * time.Millisecond)
	srv.StopHeartbeat()

	// Handler should have been called multiple times; panics should be recovered
	if afterPanic.Load() < 2 {
		t.Errorf("expected multiple calls despite panics, got %d", afterPanic.Load())
	}
}

func TestServer_StopHeartbeat_WhenRunning(t *testing.T) {
	srv := NewServer(100, true).(*server)

	srv.StartHeartbeat(10 * time.Millisecond)

	srv.mu.Lock()
	hb := srv.heartbeat
	srv.mu.Unlock()

	srv.StopHeartbeat()

	srv.mu.Lock()
	current := srv.heartbeat
	srv.mu.Unlock()

	if current != nil {
		t.Error("heartbeat field should be nil after StopHeartbeat")
	}
	if hb.IsRunning() {
		t.Error("heartbeat should not be running after StopHeartbeat")
	}
}

func TestServer_StopHeartbeat_WhenNotRunning(t *testing.T) {
	srv := NewServer(100, true).(*server)

	// Should not panic or error when no heartbeat is running
	srv.StopHeartbeat()

	srv.mu.Lock()
	hb := srv.heartbeat
	srv.mu.Unlock()

	if hb != nil {
		t.Error("heartbeat should remain nil")
	}
}

func TestServer_StopHeartbeat_NoTicksAfterStop(t *testing.T) {
	srv := NewServer(100, true).(*server)

	var tickCount atomic.Int32
	srv.RegisterHeartbeatHandler(0, func(slot uint16) { tickCount.Add(1) })

	srv.StartHeartbeat(10 * time.Millisecond)
	time.Sleep(35 * time.Millisecond)
	srv.StopHeartbeat()

	countAtStop := tickCount.Load()
	time.Sleep(30 * time.Millisecond)

	if tickCount.Load() != countAtStop {
		t.Errorf("ticks continued after StopHeartbeat: %d vs %d", countAtStop, tickCount.Load())
	}
}

func TestServer_StartHeartbeat_RestartAfterStop(t *testing.T) {
	srv := NewServer(100, true).(*server)

	var tickCount atomic.Int32
	srv.RegisterHeartbeatHandler(0, func(slot uint16) { tickCount.Add(1) })

	srv.StartHeartbeat(10 * time.Millisecond)
	time.Sleep(25 * time.Millisecond)
	srv.StopHeartbeat()

	tickCount.Store(0)

	srv.StartHeartbeat(10 * time.Millisecond)
	time.Sleep(35 * time.Millisecond)
	srv.StopHeartbeat()

	if tickCount.Load() < 2 {
		t.Errorf("expected ticks after restart, got %d", tickCount.Load())
	}
}
