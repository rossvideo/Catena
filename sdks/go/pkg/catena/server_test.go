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
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/golang-jwt/jwt/v5"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"google.golang.org/protobuf/proto"
)

// make sure all endpoint types are covered by the String() method,
// and that none of them return "Unknown" which is the default for unrecognized values.
func TestEndpointType_String(t *testing.T) {
	for e := range endpointTypeMax {
		name := e.String()
		if name == "Unknown" {
			t.Errorf("found endpoint type not covered by String(): %d", e)
		}
	}
}

// just to fill in coverage
func TestEndpointType_String_Unknown(t *testing.T) {
	endpoint := EndpointType(9999) // deliberately invalid
	if endpoint.String() != "Unknown" {
		t.Errorf("expected String() to return 'Unknown' for invalid endpoint type, got %q", endpoint.String())
	}
}

// assertAllEndpointsCovered fails if any EndpointType is neither exercised by
// the test (covered) nor deliberately declared out of scope (excluded). This
// forces a new endpoint to be consciously handled: either add it to the table
// or exclude it with a reason at the call site. The helper stays generic - it
// has no knowledge of which endpoints are special.
func assertAllEndpointsCovered(t *testing.T, covered []EndpointType, excluded ...EndpointType) {
	t.Helper()
	for e := range endpointTypeMax {
		if slices.Contains(covered, e) || slices.Contains(excluded, e) {
			continue
		}
		t.Errorf("endpoint %v (%d) is neither covered nor explicitly excluded by this test", e, int(e))
	}
	// Guard against stale exclusions: an endpoint that is both covered and
	// excluded, or excluded but no longer a real endpoint, signals the call
	// site drifted and should be cleaned up.
	for _, e := range excluded {
		if slices.Contains(covered, e) {
			t.Errorf("endpoint %v (%d) is both covered and excluded; drop the exclusion", e, int(e))
		}
		if e >= endpointTypeMax {
			t.Errorf("excluded endpoint %d is not a valid EndpointType", int(e))
		}
	}
}

func makeTestJwtToken(t *testing.T, scopes []string) string {
	t.Helper()
	token := jwt.NewWithClaims(jwt.SigningMethodNone, jwt.MapClaims{
		"scope": strings.Join(scopes, " "),
	})
	signedToken, err := token.SignedString(jwt.UnsafeAllowNoneSignatureType)
	if err != nil {
		t.Fatalf("Failed to sign JWT token: %v", err)
	}
	return signedToken
}

const validTestJWT = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJzY29wZSI6InJlYWQgd3JpdGUgYWxsIHN0MjEzODpvcDp3In0."
const validTestJWTWithoutExecuteCommandScope = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJzY29wZSI6InJlYWQgd3JpdGUgYWxsIn0."
const validTestJWTWithCfgScope = "eyJhbGciOiJub25lIiwidHlwIjoiSldUIn0.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJzY29wZSI6InN0MjEzODpjZmcifQ."

func newTestServer(t *testing.T, authzEnabled bool) *server {
	t.Helper()

	srv, err := NewServer(ServerOptions{
		MaxConnections: 100,
		AuthzEnabled:   authzEnabled,
		JwtOptions: JwtValidationOptions{
			// leave everything else unset for testing which will disable all claims checking
			ValidateSignature: false, // skip signature validation for testing
		},
	})
	if err != nil {
		t.Fatalf("NewServer() error = %v", err)
	}
	return srv.(*server)
}

func validTestTransportContext(metadata map[string][]string) TransportContext {
	return TransportContext{
		AccessToken: validTestJWT,
		Metadata:    metadata,
	}
}

func TestServer_IsDev(t *testing.T) {
	for _, isDev := range []bool{true, false} {
		t.Run(fmt.Sprintf("IsDev=%v", isDev), func(t *testing.T) {
			srv, err := NewServer(ServerOptions{IsDev: isDev})
			if err != nil {
				t.Fatalf("NewServer() error = %v", err)
			}
			// cast srv to *server to access IsDev() method
			s := srv.(*server)
			if s.IsDev() != isDev {
				t.Errorf("expected IsDev() to return %v for server created with IsDev: %v", isDev, isDev)
			}
		})
	}
}

func TestServer_ParseTransportContext(t *testing.T) {
	t.Run("ParsesTokenAndMetadata", func(t *testing.T) {
		srv := newTestServer(t, true)
		ctx, status := srv.parseTransportContext(TransportContext{
			AccessToken: "Bearer " + validTestJWT,
			Metadata:    map[string][]string{"scope": {"read"}},
		})

		if status.Code != StatusCodeOk {
			t.Fatalf("expected OK status, got %v", status)
		}
		if ctx.Token == nil {
			t.Fatal("expected parsed JWT token")
		}
		if ctx.Token.Raw != validTestJWT {
			t.Errorf("expected token raw value %q, got %q", validTestJWT, ctx.Token.Raw)
		}
		if !ctx.HasReadScope("all") {
			t.Errorf("expected parsed token read scopes to include all, got %v", ctx.readScopes)
		}
		if !ctx.HasReadScope("read") || !ctx.HasReadScope("write") {
			t.Errorf("expected parsed token read scopes to include read and write, got %v", ctx.readScopes)
		}
		if !ctx.HasReadScope(ScopeOp) {
			t.Errorf("expected parsed token read scopes to include %s, got %v", ScopeOp, ctx.readScopes)
		}
		if !ctx.HasWriteScope(ScopeOp) {
			t.Errorf("expected parsed token write scopes to include %s, got %v", ScopeOp, ctx.writeScopes)
		}
		if ctx.HasWriteScope("write") {
			t.Errorf("expected non-:w scope to be read-only, got %v", ctx.writeScopes)
		}
		if !ctx.HasAnyWriteScope() {
			t.Errorf("expected parsed token scopes to satisfy write access, got %v", ctx.writeScopes)
		}
		if !ctx.HasAnyReadScope() {
			t.Errorf("expected parsed token scopes to satisfy read access, got %v", ctx.readScopes)
		}
		if len(ctx.Metadata["scope"]) != 1 || ctx.Metadata["scope"][0] != "read" {
			t.Errorf("expected metadata to be preserved, got %v", ctx.Metadata)
		}
	})

	// just to fill in coverage
	t.Run("NilValidator", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.jwtValidator = nil // simulate a nil validator
		ctx := validTestTransportContext(map[string][]string{})
		_, status := srv.parseTransportContext(ctx)
		if status.Code != StatusCodeUnauthenticated {
			t.Fatalf("expected UNAUTHENTICATED status, got %v", status)
		}
	})

	t.Run("InvalidAccessToken", func(t *testing.T) {
		srv := newTestServer(t, true)

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
				if status.Code != StatusCodeUnauthenticated {
					t.Fatalf("expected UNAUTHENTICATED, got %v", status)
				}
				if ctx.Token != nil {
					t.Fatalf("expected no token on parse failure, got %v", ctx.Token)
				}
			})
		}
	})
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
			if err.Code != StatusCodeOk {
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
			if err.Code != StatusCodeInvalidArgument {
				t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
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
			if err.Code != StatusCodeOk {
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
			if err.Code != StatusCodeInvalidArgument {
				t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
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

func TestServer_BoundedShutdownContext_NilParent(t *testing.T) {
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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

	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)

	err := srv.RegisterTransport(nil)

	if err == nil {
		t.Error("expected error when registering nil transport, got nil")
	}
}

func TestServer_RegisterTransport_StartupError(t *testing.T) {
	expectedError := fmt.Errorf("expected startup error")
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)

	// Simulate deregistering a transport that was never registered
	err := srv.DeregisterTransport(context.Background(), &stubTransport{tb: t})

	if err != nil {
		t.Errorf("expected no error when deregistering non-existent transport, got %v", err)
	}
}

func TestServer_DeregisterTransport_Shutdown(t *testing.T) {
	srv := newTestServer(t, true)

	// Simulate server shutdown
	srv.shutdown = true

	err := srv.DeregisterTransport(context.Background(), &stubTransport{tb: t})

	// this isn't an error
	if err != nil {
		t.Errorf("expected no error when deregistering transport during shutdown, got %v", err)
	}
}

func TestServer_Wait(t *testing.T) {
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)

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

func TestServer_RealRegisterHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		notifyCalled := 0
		srv := newTestServer(t, true)
		srv.connectionQueue = &stubConnectionQueue{
			tb: t,
			notifyFn: func(update *protos.PushUpdates, scope string) {
				notifyCalled++
				if !proto.Equal(update, &protos.PushUpdates{
					Kind: &protos.PushUpdates_SlotsAdded{
						SlotsAdded: &protos.SlotList{
							Slots: []uint32{22},
						},
					},
				}) {
					t.Errorf("expected slots_added update for slot 22, got %v", update)
				}
				if scope != "" {
					// scope should be empty for slots_added notification
					// because slots_added is not a scope-based notification
					t.Errorf("expected empty scope for slots_added notification, got %q", scope)
				}
			}}
		storeCalled := 0
		srv.realRegisterHandler(22, func() {
			storeCalled++
			if _, exists := srv.slots[22]; exists {
				t.Error("expected store to run before slot was registered in server slots")
			}
		})
		if notifyCalled != 1 {
			t.Errorf("expected notify function to be called once during registration, got %d", notifyCalled)
		}
		if storeCalled != 1 {
			t.Errorf("expected store function to be called once during registration, got %d", storeCalled)
		}
		if _, exists := srv.slots[22]; !exists {
			t.Error("expected slot 22 to be registered in server slots")
		}
	})

	t.Run("DuplicateSlot", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.connectionQueue = &stubConnectionQueue{
			tb: t,
			notifyFn: func(update *protos.PushUpdates, scope string) {
				t.Error("expected no notify function to be called for duplicate slot registration")
			},
		}
		srv.slots[22] = struct{}{} // pre-register slot 22 to simulate duplicate
		storeCalled := 0
		srv.realRegisterHandler(22, func() {
			storeCalled++
		})
		// make sure store is still called
		if storeCalled != 1 {
			t.Errorf("expected store function to be called once during registration, got %d", storeCalled)
		}
	})
}

func TestServer_RegisterEndpoint(t *testing.T) {
	// Each Register* method delegates to s.registerHandlerFn and stores its
	// handler in a specific map. register calls the method under test with a
	// stub handler (never invoked); has reports whether that handler landed in
	// the correct map for the slot. endpoint records which EndpointType the
	// handler serves so assertAllEndpointsCovered can prove every endpoint has a
	// registration path; notAnEndpoint opts a case out of that accounting for
	// handlers (like heartbeat) that are registered per-slot but are not
	// EndpointTypes.
	tests := []struct {
		name          string
		endpoint      EndpointType
		notAnEndpoint bool
		register      func(srv *server, slot uint16)
		has           func(srv *server, slot uint16) bool
	}{
		{
			endpoint: EndpointGetDevice,
			register: func(srv *server, slot uint16) {
				srv.RegisterGetDeviceHandler(slot, func(uint16, HandlerContext) (Device, StatusResult) {
					return Reply(Device{})
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.getDeviceHandlers[slot] != nil },
		},
		{
			endpoint: EndpointGetValue,
			register: func(srv *server, slot uint16) {
				srv.RegisterGetValueHandler(slot, func(uint16, string, HandlerContext) (Value, StatusResult) {
					return Reply(Value{})
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.getValueHandlers[slot] != nil },
		},
		{
			endpoint: EndpointSetValue,
			register: func(srv *server, slot uint16) {
				srv.RegisterSetValueHandler(slot, func(uint16, []SetValueEntry, HandlerContext) StatusResult {
					return StatusWithCode(StatusCodeOk, "")
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.setValueHandlers[slot] != nil },
		},
		{
			endpoint: EndpointGetParam,
			register: func(srv *server, slot uint16) {
				srv.RegisterGetParamHandler(slot, func(uint16, string, HandlerContext) (Param, StatusResult) {
					return Reply(Param{})
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.getParamHandlers[slot] != nil },
		},
		{
			endpoint: EndpointGetAsset,
			register: func(srv *server, slot uint16) {
				srv.RegisterGetAssetHandler(slot, func(uint16, string, HandlerContext) (Asset, StatusResult) {
					return Reply(Asset{})
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.getAssetHandlers[slot] != nil },
		},
		{
			endpoint: EndpointExecuteCommand,
			register: func(srv *server, slot uint16) {
				srv.RegisterExecuteCommandHandler(slot, func(uint16, string, any, HandlerContext) (CommandResult, StatusResult) {
					return CommandNoResponse()
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.executeCommandHandlers[slot] != nil },
		},
		{
			endpoint: EndpointParamInfo,
			register: func(srv *server, slot uint16) {
				srv.RegisterParamInfoHandler(slot, func(uint16, string, bool, HandlerContext, Stream[ParamInfo]) StatusResult {
					return StatusWithCode(StatusCodeOk, "")
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.paramInfoHandlers[slot] != nil },
		},
		{
			endpoint: EndpointListLanguages,
			register: func(srv *server, slot uint16) {
				srv.RegisterListLanguagesHandler(slot, func(uint16, HandlerContext) ([]string, StatusResult) {
					return []string{}, StatusWithCode(StatusCodeOk, "")
				})
			},
			has: func(srv *server, slot uint16) bool { return srv.listLanguagesHandlers[slot] != nil },
		},
		{
			name:          "HeartbeatHandler",
			notAnEndpoint: true, // heartbeat is a server-driven timer, not an EndpointType
			register: func(srv *server, slot uint16) {
				srv.RegisterHeartbeatHandler(slot, func(uint16) {})
			},
			has: func(srv *server, slot uint16) bool { return srv.heartbeatHandlers[slot] != nil },
		},
	}

	// Every EndpointType must have a Register* path exercised above.
	// EndpointGetSlots and EndpointConnect are server-driven endpoints with no
	// per-slot Register* method, so they are explicitly out of scope here.
	covered := make([]EndpointType, 0, len(tests))
	for _, tt := range tests {
		if tt.notAnEndpoint {
			continue
		}
		covered = append(covered, tt.endpoint)
	}
	assertAllEndpointsCovered(t, covered, EndpointGetSlots, EndpointConnect)

	for _, tt := range tests {
		var name string
		if tt.notAnEndpoint {
			name = tt.name
		} else {
			name = tt.endpoint.String()
		}
		t.Run(name, func(t *testing.T) {
			srv := newTestServer(t, true)
			var gotSlot uint16
			called := 0
			srv.registerHandlerFn = func(slot uint16, store func()) {
				called++
				gotSlot = slot
				// call the provided store to simulate storing the handler
				store()
			}

			const slot = 42
			tt.register(srv, slot)

			if called != 1 {
				t.Fatalf("expected registerHandlerFn to be called once, but it was called %d times", called)
			}
			if gotSlot != slot {
				t.Errorf("expected slot %d, got %d", slot, gotSlot)
			}
			if !tt.has(srv, slot) {
				t.Errorf("expected handler to be registered for slot %d in the correct map", slot)
			}
		})
	}
}

// TestServer_RequestContext proves the merged request context is done when
// either parent finishes (server shutdown or the transport's request context)
// or when the returned stop func is called - and that cancelling the derived
// context never cancels either parent.
func TestServer_RequestContext(t *testing.T) {

	t.Run("ServerShutdownCancels", func(t *testing.T) {
		srv := newTestServer(t, true)
		reqCtx := t.Context()

		ctx, stop := srv.requestContext(reqCtx)
		defer stop()

		assertContextNotDone(t, ctx)

		// server shutdown must propagate to the derived context via AfterFunc
		srv.ctxCancel()

		assertContextDone(t, ctx, context.Canceled)
		// the request context is a parent, so it must be untouched
		assertContextNotDone(t, reqCtx)
	})

	t.Run("RequestContextCancels", func(t *testing.T) {
		srv := newTestServer(t, true)
		reqCtx, reqCancel := context.WithCancel(context.Background())

		ctx, stop := srv.requestContext(reqCtx)
		defer stop()

		assertContextNotDone(t, ctx)

		// cancelling the transport's request context must done the derived one
		reqCancel()

		assertContextDone(t, ctx, context.Canceled)
		// the server context is a parent, so it must be untouched
		assertContextNotDone(t, srv.ctx)
	})

	t.Run("StopCancels", func(t *testing.T) {
		srv := newTestServer(t, true)
		reqCtx := t.Context()

		ctx, stop := srv.requestContext(reqCtx)

		assertContextNotDone(t, ctx)

		// the returned stop func must done the derived context without
		// touching either parent
		stop()

		assertContextDone(t, ctx, context.Canceled)
		assertContextNotDone(t, reqCtx)
		assertContextNotDone(t, srv.ctx)
	})

	t.Run("NilRequestContextStillCancelsOnShutdown", func(t *testing.T) {
		srv := newTestServer(t, true)

		// a nil request context falls back to Background but must still be
		// cancelled by server shutdown
		nilContext := (context.Context)(nil)
		ctx, stop := srv.requestContext(nilContext)
		defer stop()

		assertContextNotDone(t, ctx)

		srv.ctxCancel()

		assertContextDone(t, ctx, context.Canceled)
	})
}

func TestServer_RealInvokeGate(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		transportContext := validTestTransportContext(nil)

		// just make a basic call with valid everything
		ctx, res := srv.realInvokeGate(transportContext, EndpointGetDevice, false)
		if res.IsError() {
			t.Errorf("expected no error from realInvokeGate, got %v", res)
		}
		// on success the gate must hand back a populated HandlerContext
		if ctx.Token == nil {
			t.Error("expected populated HandlerContext on success, got empty token")
		}
	})

	t.Run("SuccessWrite", func(t *testing.T) {
		srv := newTestServer(t, true)
		// validTestJWT carries st2138:op:w, so it satisfies write access
		transportContext := validTestTransportContext(nil)

		ctx, res := srv.realInvokeGate(transportContext, EndpointSetValue, true)
		if res.IsError() {
			t.Errorf("expected no error from realInvokeGate with write scope, got %v", res)
		}
		if ctx.Token == nil {
			t.Error("expected populated HandlerContext on success, got empty token")
		}
	})

	t.Run("InvalidTransportContext", func(t *testing.T) {
		srv := newTestServer(t, true)
		transportContext := validTestTransportContext(nil)

		// simulate an invalid transport context
		transportContext.AccessToken = "invalid"

		_, res := srv.realInvokeGate(transportContext, EndpointGetDevice, false)
		if res.IsOk() {
			t.Errorf("expected error from realInvokeGate with invalid transport context, got %v", res)
		}
	})

	t.Run("NoReadScope", func(t *testing.T) {
		srv := newTestServer(t, true)
		transportContext := validTestTransportContext(nil)

		transportContext.AccessToken = makeTestJwtToken(t, []string{}) // no scopes

		_, res := srv.realInvokeGate(transportContext, EndpointGetDevice, false)
		if res.IsOk() {
			t.Errorf("expected error from realInvokeGate with no read scope, got %v", res)
		}
	})

	t.Run("NoWriteScope", func(t *testing.T) {
		srv := newTestServer(t, true)
		transportContext := validTestTransportContext(nil)

		// read scope only (st2138:op without :w), so write access must be denied
		transportContext.AccessToken = makeTestJwtToken(t, []string{ScopeOp})

		_, res := srv.realInvokeGate(transportContext, EndpointSetValue, true)
		if res.IsOk() {
			t.Errorf("expected error from realInvokeGate with no write scope, got %v", res)
		}
	})

	t.Run("DeniedFromAccessAllowed", func(t *testing.T) {
		srv := newTestServer(t, true)
		transportContext := validTestTransportContext(nil)

		handlerCalled := 0
		var observed context.Context
		srv.RegisterAccessHandler(func(endpoint EndpointType, ctx HandlerContext) bool {
			if endpoint != EndpointGetDevice {
				t.Errorf("expected endpoint %v, got %v", EndpointGetDevice, endpoint)
			}
			if ctx.Token == nil {
				t.Errorf("expected parsed token from access token")
			}
			observed = ctx.Context()
			// make sure the ctx is not done yet when the access handler is called
			assertContextNotDone(t, observed)
			handlerCalled++
			return false // deny access
		})

		_, res := srv.realInvokeGate(transportContext, EndpointGetDevice, false)
		if res.IsOk() {
			t.Errorf("expected error from realInvokeGate with access denied, got %v", res)
		}
		if handlerCalled != 1 {
			t.Errorf("expected access handler to be called once, got %d", handlerCalled)
		}
		// the gate owns cleanup on the denial path: the context it built must be
		// released before returning, so no caller is left holding it. This also
		// covers the scope-denial branch, which shares the same release call.
		if observed == nil {
			t.Fatal("expected access handler to observe a non-nil request context")
		}
		assertContextDone(t, observed, context.Canceled)
	})

	t.Run("AccessHandlerObservesRequestContext", func(t *testing.T) {
		srv := newTestServer(t, true)
		transportContext := validTestTransportContext(nil)

		// the gate must build the request context BEFORE running the access
		// handler, so a handler that inspects ctx.Context() sees a live context.
		var observed context.Context
		srv.RegisterAccessHandler(func(endpoint EndpointType, ctx HandlerContext) bool {
			observed = ctx.Context()
			return true
		})

		handlerContext, res := srv.realInvokeGate(transportContext, EndpointGetDevice, false)
		if res.IsError() {
			t.Fatalf("expected OK from realInvokeGate, got %v", res)
		}
		defer handlerContext.release()

		if observed == nil {
			t.Fatal("expected access handler to observe a non-nil request context")
		}
		assertContextNotDone(t, observed)
		// the gate hands that same live context back to the caller
		if handlerContext.Context() != observed {
			t.Error("expected the gate to return the context the access handler saw")
		}
		// release must cancel it
		handlerContext.release()
		assertContextDone(t, observed, context.Canceled)
	})

	t.Run("AuthzDisabled", func(t *testing.T) {
		srv := newTestServer(t, false) // authz disabled
		transportContext := validTestTransportContext(nil)

		// with authz disabled, any token should be accepted
		transportContext.AccessToken = "invalid"

		srv.accessHandler = func(endpoint EndpointType, ctx HandlerContext) bool {
			t.Error("expected access handler to not be called when authz is disabled")
			return false
		}

		_, res := srv.realInvokeGate(transportContext, EndpointGetDevice, false)
		if res.IsError() {
			t.Errorf("expected no error from realInvokeGate with authz disabled, got %v", res)
		}
	})
}

// mockInvokeGateFn replaces srv.invokeGateFn with a stub that asserts the wrapper
// passed the expected endpoint and writeAccess, then returns the given ctx and
// res. Pass an OK res for the normal path or an error res to exercise a gate
// failure.
func mockInvokeGateFn(t *testing.T, srv *server, expectedEndpoint EndpointType,
	expectedWrite bool, ctx HandlerContext, res StatusResult,
) {
	srv.invokeGateFn = func(_ TransportContext, endpoint EndpointType, writeAccess bool) (HandlerContext, StatusResult) {
		if endpoint != expectedEndpoint {
			t.Errorf("expected gate endpoint %v, got %v", expectedEndpoint, endpoint)
		}
		if writeAccess != expectedWrite {
			t.Errorf("expected gate writeAccess %v, got %v", expectedWrite, writeAccess)
		}
		return ctx, res
	}
}

func TestServer_InvokeHandler(t *testing.T) {
	// just an empty token ref, so we can compare by reference later
	testToken := &jwt.Token{}
	handlerContext := HandlerContext{
		Token: testToken,
	}

	// basic run through of the invokeHandler helper function
	t.Run("FindsHandler", func(t *testing.T) {
		srv := newTestServer(t, true)
		mockInvokeGateFn(t, srv, EndpointGetDevice, false, handlerContext, StatusWithCode(StatusCodeOk, ""))
		transportContext := validTestTransportContext(nil)

		handlerCalled := 0
		_, res := invokeHandler(srv, transportContext, EndpointGetDevice, false,
			map[uint16]func(){
				11: func() {
					// since we can't == funcs, make a func we can test
					handlerCalled++
				},
			}, 11, "", func(call func(), ctx HandlerContext) (struct{}, StatusResult) {
				// call the call func to test if its the right one
				call()
				if ctx.Token != testToken {
					t.Errorf("expected handler context to be passed through, got %v", ctx.Token)
				}
				return struct{}{}, StatusResult{Code: StatusCodeOk}
			},
		)
		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if res.IsError() {
			t.Errorf("expected no error from invokeHandler, got %v", res)
		}
	})

	t.Run("GateError", func(t *testing.T) {
		srv := newTestServer(t, true)
		mockInvokeGateFn(t, srv, EndpointGetDevice, false, HandlerContext{},
			StatusResult{Code: StatusCodeInvalidArgument, Error: "TEST_ERROR"})
		transportContext := validTestTransportContext(nil)
		_, res := invokeHandler(srv, transportContext, EndpointGetDevice, false, map[uint16]func(){},
			11, "", func(call func(), ctx HandlerContext) (struct{}, StatusResult) {
				t.Error("expected gate error to short-circuit handler invocation, but handler was called")
				return struct{}{}, StatusResult{Code: StatusCodeOk}
			})
		if res.IsOk() {
			t.Error("expected error from invokeHandler due to gate error, got OK")
		}
		if res.Error != "TEST_ERROR" {
			t.Errorf("expected error message 'TEST_ERROR', got %q", res.Error)
		}
	})

	t.Run("HandlerNotFound", func(t *testing.T) {
		srv := newTestServer(t, true)
		mockInvokeGateFn(t, srv, EndpointGetDevice, false, handlerContext, StatusWithCode(StatusCodeOk, ""))
		transportContext := validTestTransportContext(nil)
		_, res := invokeHandler(srv, transportContext, EndpointGetDevice, false, map[uint16]func(){},
			11, "", func(call func(), ctx HandlerContext) (struct{}, StatusResult) {
				t.Error("expected handler not found to short-circuit handler invocation, but handler was called")
				return struct{}{}, StatusResult{Code: StatusCodeOk}
			})
		if res.IsOk() {
			t.Error("expected error from invokeHandler due to handler not found, got OK")
		}
		if res.Code != StatusCodeNotFound {
			t.Errorf("expected error code %v for handler not found, got %v", StatusCodeNotFound, res.Code)
		}
	})
}

func TestServer_InvokeGetDeviceHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointGetDevice, false, knownCtx, StatusWithCode(StatusCodeOk, ""))

		expected := Device{Proto: &protos.Device{}}
		handlerCalled := 0
		srv.getDeviceHandlers[11] = func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
			handlerCalled++
			if slot != 11 {
				t.Errorf("expected slot 11, got %d", slot)
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			return Reply(expected)
		}

		actual, status := srv.InvokeGetDeviceHandler(11, validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if actual.Proto != expected.Proto {
			t.Errorf("expected device %v, got %v", expected, actual)
		}
	})
}

func TestServer_InvokeGetValueHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointGetValue, false, knownCtx, StatusWithCode(StatusCodeOk, ""))

		expected := Value{}
		handlerCalled := 0
		srv.getValueHandlers[7] = func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
			handlerCalled++
			if slot != 7 {
				t.Errorf("expected slot 7, got %d", slot)
			}
			if fqoid != "test/param" {
				t.Errorf("expected fqoid 'test/param', got %s", fqoid)
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			return Reply(expected)
		}

		actual, status := srv.InvokeGetValueHandler(7, "test/param", validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if !proto.Equal(actual.Proto, expected.Proto) {
			t.Errorf("expected value %v, got %v", expected, actual)
		}
	})
}

func TestServer_InvokeSetValueHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointSetValue, true, knownCtx, StatusWithCode(StatusCodeOk, ""))

		handlerCalled := 0
		var got []SetValueEntry
		srv.setValueHandlers[3] = func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
			handlerCalled++
			got = entries
			if slot != 3 {
				t.Errorf("expected slot 3, got %d", slot)
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			return StatusWithCode(StatusCodeOk, "")
		}

		status := srv.InvokeSetValueHandler(3, []SetValueEntry{{Fqoid: "test/param", Value: int32(42)}}, validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if len(got) != 1 || got[0].Value != int32(42) {
			t.Errorf("expected the entry batch to reach the handler unchanged, got %v", got)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
	})

	t.Run("MultipleEntries", func(t *testing.T) {
		srv := newTestServer(t, true)
		mockInvokeGateFn(t, srv, EndpointSetValue, true, HandlerContext{}, StatusWithCode(StatusCodeOk, ""))

		callCount := 0
		var got []SetValueEntry
		srv.setValueHandlers[0] = func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
			callCount++
			got = entries
			return StatusWithCode(StatusCodeOk, "")
		}

		entries := []SetValueEntry{
			{Fqoid: "a", Value: int32(1)},
			{Fqoid: "b", Value: int32(2)},
		}
		status := srv.InvokeSetValueHandler(0, entries, validTestTransportContext(nil))

		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if callCount != 1 {
			t.Errorf("expected handler called once with the full batch, got %d", callCount)
		}
		if len(got) != 2 {
			t.Errorf("expected 2 entries delivered to handler, got %d", len(got))
		}
	})

	t.Run("HandlerError", func(t *testing.T) {
		srv := newTestServer(t, true)
		mockInvokeGateFn(t, srv, EndpointSetValue, true, HandlerContext{}, StatusWithCode(StatusCodeOk, ""))

		srv.setValueHandlers[0] = func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
			return StatusWithCode(StatusCodeInvalidArgument, "bad value")
		}

		status := srv.InvokeSetValueHandler(0, []SetValueEntry{{Fqoid: "a", Value: int32(1)}}, validTestTransportContext(nil))
		if status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument status, got %v", status.Code)
		}
	})
}

func TestServer_InvokeGetParamHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointGetParam, false, knownCtx, StatusWithCode(StatusCodeOk, ""))

		expected := Param{}
		handlerCalled := 0
		srv.getParamHandlers[22] = func(slot uint16, fqoid string, ctx HandlerContext) (Param, StatusResult) {
			handlerCalled++
			if slot != 22 {
				t.Errorf("expected slot 22, got %d", slot)
			}
			if fqoid != "test/param" {
				t.Errorf("expected fqoid 'test/param', got %s", fqoid)
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			return Reply(expected)
		}

		actual, status := srv.InvokeGetParamHandler(22, "test/param", validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if !proto.Equal(actual.Proto, expected.Proto) {
			t.Errorf("expected value %v, got %v", expected, actual)
		}
	})
}

func TestServer_InvokeGetAssetHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointGetAsset, false, knownCtx, StatusWithCode(StatusCodeOk, ""))

		dp := DataPayload{
			Metadata: map[string]string{"content-type": "image/png"},
			Payload:  []byte("fake image"),
		}
		expected, _ := ToAsset(dp, false)

		handlerCalled := 0
		srv.getAssetHandlers[5] = func(slot uint16, fqoid string, ctx HandlerContext) (Asset, StatusResult) {
			handlerCalled++
			if slot != 5 {
				t.Errorf("expected slot 5, got %d", slot)
			}
			if fqoid != "test/asset" {
				t.Errorf("expected fqoid 'test/asset', got %s", fqoid)
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			return Reply(expected)
		}

		actual, status := srv.InvokeGetAssetHandler(5, "test/asset", validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if !proto.Equal(actual.Proto, expected.Proto) {
			t.Errorf("expected asset %v, got %v", expected, actual)
		}
	})
}

func TestServer_InvokeExecuteCommandHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointExecuteCommand, true, knownCtx, StatusWithCode(StatusCodeOk, ""))

		payload := "test-payload"
		handlerCalled := 0
		srv.executeCommandHandlers[9] = func(slot uint16, commandFqoid string, gotPayload any, ctx HandlerContext) (CommandResult, StatusResult) {
			handlerCalled++
			if slot != 9 {
				t.Errorf("expected slot 9, got %d", slot)
			}
			if commandFqoid != "test/command" {
				t.Errorf("expected commandFqoid 'test/command', got %s", commandFqoid)
			}
			if gotPayload != payload {
				t.Errorf("expected payload %v, got %v", payload, gotPayload)
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			return CommandNoResponse()
		}

		result, status := srv.InvokeExecuteCommandHandler(9, "test/command", payload, validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if !result.IsEmpty() {
			t.Error("expected CommandNoResponse (empty) result")
		}
	})
}

func TestServer_InvokeParamInfoHandler(t *testing.T) {
	t.Run("Success", func(t *testing.T) {
		srv := newTestServer(t, true)
		knownCtx := HandlerContext{Token: &jwt.Token{Raw: "known"}}
		mockInvokeGateFn(t, srv, EndpointParamInfo, false, knownCtx, StatusWithCode(StatusCodeOk, ""))

		handlerCalled := 0
		srv.paramInfoHandlers[4] = func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext, stream Stream[ParamInfo]) StatusResult {
			handlerCalled++
			if slot != 4 {
				t.Errorf("expected slot 4, got %d", slot)
			}
			if oidPrefix != "test/param" {
				t.Errorf("expected oidPrefix 'test/param', got %s", oidPrefix)
			}
			if !recursive {
				t.Error("expected recursive flag to be passed through as true")
			}
			if ctx.Token != knownCtx.Token {
				t.Error("expected the gate's handler context to be passed through to the handler")
			}
			if err := stream.Send(NewParamInfo("test/param", nil, ParamTypeString, "", 0)); err != nil {
				t.Errorf("unexpected Send error: %v", err)
			}
			return StatusWithCode(StatusCodeOk, "")
		}

		stream := &sliceStream[ParamInfo]{}
		status := srv.InvokeParamInfoHandler(4, "test/param", true, stream, validTestTransportContext(nil))

		if handlerCalled != 1 {
			t.Errorf("expected handler to be called once, got %d", handlerCalled)
		}
		if status.IsError() {
			t.Errorf("expected OK status, got %v", status)
		}
		if len(stream.Items) != 1 {
			t.Fatalf("expected 1 streamed param info entry, got %d", len(stream.Items))
		}
		if got := stream.Items[0].Proto.GetInfo().GetOid(); got != "test/param" {
			t.Errorf("streamed param info oid = %q, want %q", got, "test/param")
		}
	})
}

// TestServer_InvokeEndpointsRouteThroughGate proves that every endpoint actually
// runs through the authorization gate instead of bypassing it: with an invalid
// token the real gate must reject the call, so the endpoint handler and the
// access handler must not run. The gate's own branches (bad token, missing
// scope, access-handler denial) are exercised in detail by
// TestServer_RealInvokeGate; this table only guards the wiring so that a new
// endpoint which forgets to call the gate is caught by assertAllEndpointsCovered.
func TestServer_InvokeEndpointsRouteThroughGate(t *testing.T) {
	invalidContext := TransportContext{AccessToken: "not-a-jwt"}

	tests := []struct {
		endpoint EndpointType
		invoke   func(srv *server, handlerCalled *bool) StatusResult
	}{
		{
			endpoint: EndpointGetSlots,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.slots[0] = struct{}{}
				_, status := srv.GetSlots(invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointGetDevice,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetDeviceHandler(0, func(uint16, HandlerContext) (Device, StatusResult) {
					*handlerCalled = true
					return Reply(Device{})
				})
				_, status := srv.InvokeGetDeviceHandler(0, invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointGetValue,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetValueHandler(0, func(uint16, string, HandlerContext) (Value, StatusResult) {
					*handlerCalled = true
					return Reply(Value{})
				})
				_, status := srv.InvokeGetValueHandler(0, "test/param", invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointSetValue,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterSetValueHandler(0, func(uint16, []SetValueEntry, HandlerContext) StatusResult {
					*handlerCalled = true
					return StatusWithCode(StatusCodeOk, "")
				})
				return srv.InvokeSetValueHandler(0, []SetValueEntry{{Fqoid: "test/param", Value: int32(42)}}, invalidContext)
			},
		},
		{
			endpoint: EndpointGetParam,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetParamHandler(0, func(uint16, string, HandlerContext) (Param, StatusResult) {
					*handlerCalled = true
					return Reply(Param{})
				})
				_, status := srv.InvokeGetParamHandler(0, "test/param", invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointGetAsset,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetAssetHandler(0, func(uint16, string, HandlerContext) (Asset, StatusResult) {
					*handlerCalled = true
					return Reply(Asset{})
				})
				_, status := srv.InvokeGetAssetHandler(0, "test/asset", invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointExecuteCommand,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterExecuteCommandHandler(0, func(uint16, string, any, HandlerContext) (CommandResult, StatusResult) {
					*handlerCalled = true
					return CommandNoResponse()
				})
				_, status := srv.InvokeExecuteCommandHandler(0, "test/command", nil, invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointParamInfo,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterParamInfoHandler(0, func(uint16, string, bool, HandlerContext, Stream[ParamInfo]) StatusResult {
					*handlerCalled = true
					return StatusWithCode(StatusCodeOk, "")
				})
				return srv.InvokeParamInfoHandler(0, "test/param", false, &sliceStream[ParamInfo]{}, invalidContext)
			},
		},
		{
			endpoint: EndpointListLanguages,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterListLanguagesHandler(0, func(uint16, HandlerContext) ([]string, StatusResult) {
					*handlerCalled = true
					return []string{}, StatusWithCode(StatusCodeOk, "")
				})
				_, status := srv.InvokeListLanguagesHandler(0, invalidContext)
				return status
			},
		},
		{
			endpoint: EndpointConnect,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				conn, status := srv.RegisterTransportConnection(nil, invalidContext)
				if conn != nil {
					t.Errorf("expected no connection when the gate rejects, got %+v", conn)
				}
				return status
			},
		},
	}

	covered := make([]EndpointType, 0, len(tests))
	for _, tt := range tests {
		covered = append(covered, tt.endpoint)
	}
	assertAllEndpointsCovered(t, covered)

	for _, tt := range tests {
		t.Run(tt.endpoint.String(), func(t *testing.T) {
			srv := newTestServer(t, true)
			handlerCalled := false
			accessCalled := false
			srv.RegisterAccessHandler(func(EndpointType, HandlerContext) bool {
				accessCalled = true
				return true
			})

			status := tt.invoke(srv, &handlerCalled)

			if status.IsOk() {
				t.Fatal("expected the gate to reject an invalid token, got OK")
			}
			if handlerCalled {
				t.Fatal("endpoint handler must not run when the gate rejects")
			}
			if accessCalled {
				t.Fatal("access handler must not run when the token cannot be resolved")
			}
		})
	}
}

func TestServer_RegisterHeartbeatHandler(t *testing.T) {
	srv := newTestServer(t, true)

	srv.RegisterHeartbeatHandler(0, func(slot uint16) {})

	if srv.heartbeatHandlers[0] == nil {
		t.Error("expected heartbeat handler to be registered for slot 0")
	}
	// there's no invoke function for heartbeat handlers since they're just called by the server on a timer, so we'll just call it directly to test that it works
}

func TestServer_RegisterAccessHandlerNilResetsToAllowAll(t *testing.T) {
	srv := newTestServer(t, true)
	srv.slots[0] = struct{}{}

	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		return false
	})

	_, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != StatusCodePermissionDenied {
		t.Fatalf("expected StatusCodePermissionDenied with deny handler, got %v", status.Code)
	}

	srv.RegisterAccessHandler(nil)

	slots, status := srv.GetSlots(validTestTransportContext(nil))
	if status.Code != StatusCodeOk {
		t.Fatalf("expected OK after nil reset to default handler, got %v", status.Code)
	}
	if len(slots) != 1 || slots[0] != 0 {
		t.Errorf("expected slot [0], got %v", slots)
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
			srv := newTestServer(t, true)
			srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
				t.Fatal("access handler should not run when token parsing fails")
				return true
			})

			slots, status := srv.GetSlots(TransportContext{AccessToken: tt.accessToken})

			if status.Code != StatusCodeUnauthenticated {
				t.Errorf("expected UNAUTHENTICATED, got %v", status.Code)
			}
			if slots != nil {
				t.Errorf("expected no slots when token parsing fails, got %v", slots)
			}
		})
	}
}

func TestServer_AuthzDisabledSkipsAccessHandlerAndAllowsScopedHandlerChecks(t *testing.T) {
	srv := newTestServer(t, false)
	handlerCalled := false
	accessCalled := false

	srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
		accessCalled = true
		return false
	})
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		handlerCalled = true
		if !ctx.HasReadScope(ScopeCfg) {
			return ReplyError[Value](StatusCodePermissionDenied, "configuration scope required")
		}
		return Reply(Value{})
	})

	_, status := srv.InvokeGetValueHandler(0, "test/param", TransportContext{})

	if accessCalled {
		t.Fatal("access handler should not run when authz is disabled")
	}
	if !handlerCalled {
		t.Fatal("endpoint handler should run when authz is disabled")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK, got %v", status.Code)
	}
}

func TestServer_AuthzDisabledHandlerContextGrantsAllScopes(t *testing.T) {
	srv := newTestServer(t, false)

	ctx, status := srv.resolveHandlerContext(TransportContext{})
	if status.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v", status)
	}
	if !ctx.HasReadScope(ScopeCfg) {
		t.Fatal("expected HasReadScope to succeed when authz is disabled")
	}
	if !ctx.HasWriteScope(ScopeCfg) {
		t.Fatal("expected HasWriteScope to succeed when authz is disabled")
	}
	if !ctx.HasAnyReadScope() || !ctx.HasAnyWriteScope() {
		t.Fatal("expected HasAnyReadScope and HasAnyWriteScope to succeed when authz is disabled")
	}

	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		if !ctx.HasReadScope(ScopeCfg) {
			return ReplyError[Value](StatusCodePermissionDenied, "configuration scope required")
		}
		return Reply(Value{})
	})

	_, status = srv.InvokeGetValueHandler(0, "test/param", TransportContext{})
	if status.Code != StatusCodeOk {
		t.Errorf("expected handler scope check to pass when authz is disabled, got %v", status.Code)
	}
}

func TestServer_AuthzDisabledAllowsRequestsWithoutToken(t *testing.T) {
	tests := []struct {
		endpoint          EndpointType
		invoke            func(*server, *bool) StatusResult
		expectHandlerCall bool
	}{
		{
			endpoint: EndpointGetSlots,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.slots[0] = struct{}{}
				slots, status := srv.GetSlots(TransportContext{})
				if status.Code == StatusCodeOk && (len(slots) != 1 || slots[0] != 0) {
					t.Fatalf("GetSlots: expected [0], got %v", slots)
				}
				return status
			},
		},
		{
			endpoint:          EndpointGetDevice,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
					*handlerCalled = true
					return Reply(Device{})
				})
				_, status := srv.InvokeGetDeviceHandler(0, TransportContext{})
				return status
			},
		},
		{
			endpoint:          EndpointGetValue,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
					*handlerCalled = true
					return Reply(Value{})
				})
				_, status := srv.InvokeGetValueHandler(0, "test/param", TransportContext{})
				return status
			},
		},
		{
			endpoint:          EndpointSetValue,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterSetValueHandler(0, func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
					*handlerCalled = true
					return StatusWithCode(StatusCodeOk, "")
				})
				return srv.InvokeSetValueHandler(0, []SetValueEntry{{Fqoid: "test/param", Value: int32(42)}}, TransportContext{})
			},
		},
		{
			endpoint:          EndpointGetParam,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetParamHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Param, StatusResult) {
					*handlerCalled = true
					return Reply(Param{})
				})
				_, status := srv.InvokeGetParamHandler(0, "test/param", TransportContext{})
				return status
			},
		},
		{
			endpoint:          EndpointGetAsset,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Asset, StatusResult) {
					*handlerCalled = true
					return Reply(Asset{})
				})
				_, status := srv.InvokeGetAssetHandler(0, "test/asset", TransportContext{})
				return status
			},
		},
		{
			endpoint:          EndpointExecuteCommand,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any, ctx HandlerContext) (CommandResult, StatusResult) {
					*handlerCalled = true
					return CommandNoResponse()
				})
				_, status := srv.InvokeExecuteCommandHandler(0, "test/command", nil, TransportContext{})
				return status
			},
		},
		{
			endpoint:          EndpointParamInfo,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext, stream Stream[ParamInfo]) StatusResult {
					*handlerCalled = true
					return StatusWithCode(StatusCodeOk, "")
				})
				return srv.InvokeParamInfoHandler(0, "test/param", false, &sliceStream[ParamInfo]{}, TransportContext{})
			},
		},
		{
			endpoint:          EndpointListLanguages,
			expectHandlerCall: true,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.RegisterListLanguagesHandler(0, func(slot uint16, ctx HandlerContext) ([]string, StatusResult) {
					*handlerCalled = true
					return []string{}, StatusWithCode(StatusCodeOk, "")
				})
				_, status := srv.InvokeListLanguagesHandler(0, TransportContext{})
				return status
			},
		},
		{
			endpoint: EndpointConnect,
			invoke: func(srv *server, handlerCalled *bool) StatusResult {
				srv.connectionQueue = &stubConnectionQueue{
					tb: t,
					registerOwnedFn: func(owner any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult) {
						return &Connection{ID: 1}, StatusWithCode(StatusCodeOk, "")
					},
				}
				conn, status := srv.RegisterTransportConnection(nil, TransportContext{})
				if status.Code == StatusCodeOk && conn == nil {
					t.Fatal("RegisterTransportConnection: expected connection without token")
				}
				return status
			},
		},
	}

	endpoints := []EndpointType{}
	for _, tt := range tests {
		endpoints = append(endpoints, tt.endpoint)
	}
	assertAllEndpointsCovered(t, endpoints)

	for _, tt := range tests {
		t.Run(tt.endpoint.String(), func(t *testing.T) {
			srv := newTestServer(t, false)
			handlerCalled := false
			accessCalled := false
			srv.RegisterAccessHandler(func(endpointType EndpointType, ctx HandlerContext) bool {
				accessCalled = true
				return false
			})

			status := tt.invoke(srv, &handlerCalled)

			if accessCalled {
				t.Fatal("access handler should not run when authz is disabled")
			}
			if tt.expectHandlerCall && !handlerCalled {
				t.Fatal("endpoint handler should run when authz is disabled")
			}
			if !tt.expectHandlerCall && handlerCalled {
				t.Fatal("endpoint handler unexpectedly ran")
			}
			if status.Code != StatusCodeOk {
				t.Fatalf("expected OK without token, got %v (%s)", status.Code, status.Error)
			}
		})
	}
}

var _ connectionQueueInterface = (*stubConnectionQueue)(nil)

type stubConnectionQueue struct {
	tb              testing.TB
	setMaxFn        func(max int)
	registerOwnedFn func(owner any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult)
	deregisterFn    func(connID int)
	notifyFn        func(update *protos.PushUpdates, scope string)
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
	return nil, StatusResult{Code: StatusCodeInternal}
}

func (s *stubConnectionQueue) deregisterConnection(connID int) {
	if s.deregisterFn != nil {
		s.deregisterFn(connID)
	} else {
		s.tb.Fatalf("deregisterConnection called on stubConnectionQueue without deregisterFn defined")
	}
}

func (s *stubConnectionQueue) notifyUpdate(update *protos.PushUpdates, scope string) {
	if s.notifyFn != nil {
		s.notifyFn(update, scope)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
	srv.slots[0] = struct{}{}
	srv.slots[5] = struct{}{}
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		registerOwnedFn: func(gotTransport any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult) {
			called = true
			if gotTransport != transport {
				t.Error("expected transport owner to be passed through")
			}
			if !handlerContext.HasWriteScope(ScopeOp) {
				t.Errorf("expected handler context to be passed through, got %v", handlerContext.writeScopes)
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
			}, StatusResult{Code: StatusCodeOk}
		},
	}

	conn, status := srv.RegisterTransportConnection(transport, validTestTransportContext(nil))

	if !called {
		t.Error("expected registerOwnedConnection to be called on connection queue")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if conn == nil || conn.ID != 78 {
		t.Errorf("expected connection ID 78, got %+v", conn)
	}
}

func TestServer_RegisterTransportConnection_Failed(t *testing.T) {
	srv := newTestServer(t, true)
	srv.connectionQueue = &stubConnectionQueue{
		tb: t,
		registerOwnedFn: func(gotOwner any, handlerContext HandlerContext, initialUpdate *protos.PushUpdates) (*Connection, StatusResult) {
			return nil, StatusResult{Code: StatusCodeResourceExhausted, Error: "queue full"}
		},
	}

	conn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))

	if status.Code != StatusCodeResourceExhausted {
		t.Errorf("expected StatusCodeResourceExhausted on registration failure, got %v", status.Code)
	}
	if conn != nil {
		t.Errorf("expected nil connection on registration failure, got %+v", conn)
	}
}

func TestServer_SetMaxConnections_Passthrough(t *testing.T) {
	called := false
	lastMax := 0
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)

	// Register a connection
	conn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != StatusCodeOk {
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
	srv.BroadcastUpdate(0, "test/param", int32(42), ScopeOp)

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
	srv := newTestServer(t, true)

	matchingConn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != StatusCodeOk {
		t.Fatalf("Failed to register matching connection: %v", status)
	}
	defer srv.DeregisterConnection(matchingConn.ID)

	nonMatchingConn, status := srv.RegisterTransportConnection(nil, TransportContext{
		AccessToken: validTestJWTWithCfgScope,
	})
	if status.Code != StatusCodeOk {
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

	srv.BroadcastUpdate(0, "test/param", int32(42), ScopeOp)

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
	srv := newTestServer(t, false)

	matchingConn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != StatusCodeOk {
		t.Fatalf("Failed to register matching connection: %v", status)
	}
	defer srv.DeregisterConnection(matchingConn.ID)

	noScopeConn, status := srv.RegisterTransportConnection(nil, TransportContext{
		AccessToken: validTestJWTWithoutExecuteCommandScope,
	})
	if status.Code != StatusCodeOk {
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

	srv.BroadcastUpdate(0, "test/param", int32(42), ScopeOp)

	for _, conn := range []*Connection{matchingConn, noScopeConn} {
		select {
		case <-conn.Updates:
		case <-time.After(time.Second):
			t.Fatal("connection did not receive broadcast update")
		}
	}
}

func TestServer_BroadcastUpdate_InvalidValue(t *testing.T) {
	srv := newTestServer(t, true)

	conn, status := srv.RegisterTransportConnection(nil, validTestTransportContext(nil))
	if status.Code != StatusCodeOk {
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
	srv.BroadcastUpdate(0, "test/param", true, ScopeOp)

	// Should not receive update (logged error instead)
	select {
	case <-conn.Updates:
		t.Error("should not have received update for invalid value")
	case <-time.After(100 * time.Millisecond):
	}
}

func TestServer_StartHeartbeat_InvalidInterval(t *testing.T) {
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)
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
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)

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
	srv := newTestServer(t, true)

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
