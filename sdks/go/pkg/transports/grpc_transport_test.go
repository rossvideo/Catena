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
 * @brief Unit tests for gRPC server endpoints
 * @file server_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-14
 */

package transports

import (
	"context"
	"errors"
	"fmt"
	"io"
	"net"
	"testing"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/reflection/grpc_reflection_v1"
	"google.golang.org/grpc/status"
	"google.golang.org/grpc/test/bufconn"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

const bufSize = 1024 * 1024

type testGrpcTransportConfig struct {
	port         uint16
	reflection   bool
	isDev        bool
	startupDelay time.Duration
}

type testGrpcTransportOption func(*testGrpcTransportConfig)

func withTestGrpcTransportDevMode(isDev bool) testGrpcTransportOption {
	return func(cfg *testGrpcTransportConfig) {
		cfg.isDev = isDev
	}
}

func withTestGrpcTransportReflection(enabled bool) testGrpcTransportOption {
	return func(cfg *testGrpcTransportConfig) {
		cfg.reflection = enabled
	}
}

// setupTestGrpcTransport creates an in-memory gRPC test server using bufconn
func setupTestGrpcTransport(t *testing.T, slots []uint16, opts ...testGrpcTransportOption) (*GrpcTransport, *stubServerRuntime, *bufconn.Listener, func()) {
	t.Helper()

	cfg := testGrpcTransportConfig{
		port:         1234,
		reflection:   false,
		isDev:        false,
		startupDelay: 10 * time.Millisecond,
	}
	for _, opt := range opts {
		opt(&cfg)
	}

	lis := bufconn.Listen(bufSize)
	transport := NewGrpcTransport(cfg.port, cfg.reflection)
	runtime := makeStubServerRuntime(t)
	runtime.slots = slots
	transport.runtime = runtime

	go func() {
		if err := transport.grpcServer.Serve(lis); err != nil {
			t.Logf("server stopped: %v", err)
		}
	}()

	// Give server a moment to start
	if cfg.startupDelay > 0 {
		time.Sleep(cfg.startupDelay)
	}

	cleanup := func() {
		// Use Stop() instead of GracefulStop() in tests to avoid hanging
		// GracefulStop() waits for active streams which causes deadlocks in tests
		transport.grpcServer.Stop()
		lis.Close()
	}

	return transport, runtime, lis, cleanup
}

// dialTestServer creates a client connection to the test server
func dialTestServer(ctx context.Context, lis *bufconn.Listener) (*grpc.ClientConn, error) {
	return grpc.DialContext(ctx, "bufnet",
		grpc.WithContextDialer(func(context.Context, string) (net.Conn, error) {
			return lis.Dial()
		}),
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
}

// assertGRPCError checks that an error has the expected gRPC status code
func assertGRPCError(t *testing.T, err error, expectedCode codes.Code) {
	t.Helper()
	if err == nil {
		t.Fatalf("expected error with code %v, got nil", expectedCode)
	}
	st, ok := status.FromError(err)
	if !ok {
		t.Fatalf("expected gRPC status error, got %v", err)
	}
	if st.Code() != expectedCode {
		t.Errorf("expected code %v, got %v (message: %s)", expectedCode, st.Code(), st.Message())
	}
}

// =============================================================================
// Test: NewGrpcTransport
// =============================================================================

func TestNewGrpcTransport(t *testing.T) {
	transport := NewGrpcTransport(1234, false)

	if transport == nil {
		t.Fatal("NewGrpcTransport returned nil")
	}
	if transport.catenaService == nil {
		t.Error("catenaService is nil")
	}
	if transport.grpcServer == nil {
		t.Error("grpcServer is nil")
	}
	if transport.port != 1234 {
		t.Errorf("expected port 1234, got %d", transport.port)
	}
	if transport.reflection != false {
		t.Errorf("expected reflection false, got %v", transport.reflection)
	}
}

func TestNewDefaultGrpcTransport(t *testing.T) {
	transport := NewDefaultGrpcTransport()
	if transport == nil {
		t.Fatal("NewDefaultGrpcTransport returned nil")
	}
	if transport.catenaService == nil {
		t.Error("catenaService is nil")
	}
	if transport.grpcServer == nil {
		t.Error("grpcServer is nil")
	}
}

func TestGrpcTransport_PropagatesTransportContext(t *testing.T) {
	const accessToken = "Bearer grpc-token"
	const tenant = "tenant-a"

	newContext := func() (context.Context, context.CancelFunc) {
		baseCtx, cancel := context.WithCancel(context.Background())
		ctx := metadata.NewOutgoingContext(baseCtx, metadata.Pairs(
			"authorization", accessToken,
			"x-test-tenant", tenant,
		))
		return ctx, cancel
	}
	assertContext := func(t *testing.T, ctx catena.TransportContext) {
		t.Helper()
		if ctx.AccessToken != accessToken {
			t.Errorf("expected access token %q, got %q", accessToken, ctx.AccessToken)
		}
		if got := ctx.Metadata["authorization"]; len(got) != 1 || got[0] != accessToken {
			t.Errorf("expected authorization metadata %q, got %v", accessToken, got)
		}
		if got := ctx.Metadata["x-test-tenant"]; len(got) != 1 || got[0] != tenant {
			t.Errorf("expected tenant metadata %q, got %v", tenant, got)
		}
	}

	tests := []struct {
		name  string
		setup func(t *testing.T, runtime *stubServerRuntime)
		run   func(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, cancel context.CancelFunc)
	}{
		{
			name: "get populated slots",
			setup: func(t *testing.T, runtime *stubServerRuntime) {
				runtime.getSlotsFn = func(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
					assertContext(t, ctx)
					return []uint16{0}, catena.StatusWithCode(catena.OK, "")
				}
			},
			run: func(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, cancel context.CancelFunc) {
				resp, err := makeGetPopulatedSlotsRequest(t, client, ctx)
				assertNoError(t, err)
				assertSlotList(t, resp, []uint32{0})
			},
		},
		{
			name: "get value",
			setup: func(t *testing.T, runtime *stubServerRuntime) {
				runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
					assertContext(t, ctx)
					value, _ := catena.ToValue(int32(42))
					return catena.Reply(value)
				}
			},
			run: func(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, cancel context.CancelFunc) {
				_, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
				assertNoError(t, err)
			},
		},
		{
			name: "multi set value",
			setup: func(t *testing.T, runtime *stubServerRuntime) {
				runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
					assertContext(t, ctx)
					return catena.StatusWithCode(catena.OK, "")
				}
			},
			run: func(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, cancel context.CancelFunc) {
				_, err := makeMultiSetValueRequest(t, client, ctx, 0, map[string]any{
					"param1": int32(1),
					"param2": "two",
				})
				assertNoError(t, err)
			},
		},
		{
			name: "device request",
			setup: func(t *testing.T, runtime *stubServerRuntime) {
				runtime.getDeviceFn = func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
					assertContext(t, ctx)
					device, _ := catena.ToDevice(map[string]any{"slot": uint32(slot)})
					return catena.Reply(device)
				}
			},
			run: func(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, cancel context.CancelFunc) {
				stream, err := makeDeviceRequest(t, client, ctx, 0)
				assertNoError(t, err)
				_ = receiveDeviceComponent(t, stream)
			},
		},
		{
			name: "connect",
			setup: func(t *testing.T, runtime *stubServerRuntime) {
				connection := makeTestConnection(1)
				runtime.registerTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
					assertContext(t, ctx)
					return connection, catena.StatusWithCode(catena.OK, "")
				}
				runtime.deregisterConnFn = func(connID int) {}
			},
			run: func(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, cancel context.CancelFunc) {
				stream, err := makeConnectRequest(t, client, ctx)
				assertNoError(t, err)
				cancel()
				_, _ = stream.Recv()
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx, cancel := newContext()
			defer cancel()
			_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
			defer cleanup()
			tt.setup(t, runtime)

			client, clientCleanup := setupGRPCClient(t, ctx, lis)
			defer clientCleanup()
			tt.run(t, client, ctx, cancel)
		})
	}
}

// =============================================================================
// Test: GetPopulatedSlots
// =============================================================================

func TestGrpcTransport_GetPopulatedSlots(t *testing.T) {
	tests := []struct {
		name          string
		slots         []uint16
		expectedSlots []uint32
	}{
		{
			name:          "single slot",
			slots:         []uint16{0},
			expectedSlots: []uint32{0},
		},
		{
			name:          "multiple slots",
			slots:         []uint16{0, 1, 2, 5},
			expectedSlots: []uint32{0, 1, 2, 5},
		},
		{
			name:          "empty slots",
			slots:         []uint16{},
			expectedSlots: []uint32{},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx := context.Background()
			_, _, lis, cleanup := setupTestGrpcTransport(t, tt.slots)
			defer cleanup()

			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			resp, err := makeGetPopulatedSlotsRequest(t, client, ctx)
			assertNoError(t, err)
			assertSlotList(t, resp, tt.expectedSlots)
		})
	}
}

func TestGrpcTransport_GetPopulatedSlots_RuntimeError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{})
	defer cleanup()

	runtime.getSlotsFn = func(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
		return nil, catena.StatusWithCode(catena.PERMISSION_DENIED, "no slot access")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeGetPopulatedSlotsRequest(t, client, ctx)
	if err == nil {
		t.Fatal("expected GetPopulatedSlots to return an error")
	}
	st, ok := status.FromError(err)
	if !ok {
		t.Fatalf("expected gRPC status error, got %v", err)
	}
	if st.Code() != codes.PermissionDenied {
		t.Errorf("expected PermissionDenied code, got %v", st.Code())
	}
	if st.Message() != "no slot access" {
		t.Errorf("expected propagated error message, got %q", st.Message())
	}
}

// =============================================================================
// Test: DeviceRequest
// =============================================================================

func TestGrpcTransport_DeviceRequest_Success(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	// Register mock handler
	handlerCalled := false
	runtime.getDeviceFn = func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
		handlerCalled = true
		deviceMap := map[string]any{
			"slot":         uint32(slot),
			"detail_level": catena.DetailLevelFull,
		}
		device, err := catena.ToDevice(deviceMap)
		if err != nil {
			t.Fatalf("ToDevice failed: %v", err)
		}
		return catena.Reply(device)
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeDeviceRequest(t, client, ctx, 0)
	assertNoError(t, err)

	msg := receiveDeviceComponent(t, stream)
	device := msg.GetDevice()
	assertDevice(t, device, 0)

	if !handlerCalled {
		t.Error("handler was not called")
	}

	// Verify stream is closed
	_, err = stream.Recv()
	if err != io.EOF {
		t.Errorf("expected EOF, got %v", err)
	}
}

func TestGrpcTransport_DeviceRequest_InvalidSlot(t *testing.T) {
	tests := []struct {
		name         string
		slot         uint32
		expectedCode codes.Code
	}{
		// only need to test out of bounds
		{
			name:         "slot exceeds uint16 max",
			slot:         70000,
			expectedCode: codes.InvalidArgument,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx := context.Background()
			_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
			defer cleanup()

			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			stream, err := makeDeviceRequest(t, client, ctx, tt.slot)
			if err != nil {
				assertGRPCCode(t, err, tt.expectedCode, "expected error at stream creation")
				return
			}

			_, err = stream.Recv()
			assertGRPCCode(t, err, tt.expectedCode, "expected error at recv")
		})
	}
}

func TestGrpcTransport_DeviceRequest_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	// Register handler that returns error
	runtime.getDeviceFn = func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
		return catena.ReplyError[catena.Device](catena.NOT_FOUND, "device not found")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeDeviceRequest(t, client, ctx, 0)
	if err != nil {
		assertGRPCCode(t, err, codes.NotFound, "handler error at stream creation")
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.NotFound, "handler error at recv")
}

// =============================================================================
// Test: GetValue
// =============================================================================

func TestGrpcTransport_GetValue_Success(t *testing.T) {
	tests := []struct {
		name          string
		handlerValue  any
		validateValue func(*testing.T, *protos.Value)
	}{
		{
			name:         "int32 value",
			handlerValue: int32(42),
			validateValue: func(t *testing.T, v *protos.Value) {
				if v.GetInt32Value() != 42 {
					t.Errorf("expected 42, got %d", v.GetInt32Value())
				}
			},
		},
		{
			name:         "float32 value",
			handlerValue: float32(3.14),
			validateValue: func(t *testing.T, v *protos.Value) {
				if v.GetFloat32Value() != 3.14 {
					t.Errorf("expected 3.14, got %f", v.GetFloat32Value())
				}
			},
		},
		{
			name:         "string value",
			handlerValue: "hello",
			validateValue: func(t *testing.T, v *protos.Value) {
				if v.GetStringValue() != "hello" {
					t.Errorf("expected 'hello', got %s", v.GetStringValue())
				}
			},
		},
		{
			name:         "int32 array",
			handlerValue: []int32{1, 2, 3},
			validateValue: func(t *testing.T, v *protos.Value) {
				arr := v.GetInt32ArrayValues()
				if arr == nil {
					t.Fatal("expected int32 array, got nil")
				}
				if len(arr.Ints) != 3 {
					t.Errorf("expected 3 elements, got %d", len(arr.Ints))
				}
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx := context.Background()
			_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
			defer cleanup()

			runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
				if fqoid != "device.param1" {
					t.Errorf("expected fqoid 'device.param1', got %s", fqoid)
				}
				value, _ := catena.ToValue(tt.handlerValue)
				return catena.Reply(value)
			}

			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			resp, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
			assertNoError(t, err)
			tt.validateValue(t, resp)
		})
	}
}

func TestGrpcTransport_GetValue_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeGetValueRequest(t, client, ctx, 999999, "device.param1")
	assertGRPCCode(t, err, codes.InvalidArgument, "") // Note: server returns NotFound for slots without handlers
}

func TestGrpcTransport_GetValue_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
		return catena.ReplyError[catena.Value](catena.NOT_FOUND, "param not supported")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
	assertGRPCCode(t, err, codes.NotFound, "handler error")
}

// =============================================================================
// Test: SetValue
// =============================================================================

func TestGrpcTransport_SetValue_Success(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	receivedValue := any(nil)
	runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		receivedValue = value
		if fqoid != "device.param1" {
			t.Errorf("expected fqoid 'device.param1', got %s", fqoid)
		}
		return catena.StatusWithCode(catena.OK, "")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeSetValueRequest(t, client, ctx, 0, "device.param1", int32(100))
	assertNoError(t, err)

	if receivedValue == nil {
		t.Fatal("handler did not receive value")
	}
	intVal, ok := receivedValue.(int32)
	if !ok {
		t.Errorf("expected int32, got %T", receivedValue)
	}
	if intVal != 100 {
		t.Errorf("expected 100, got %d", intVal)
	}
}

func TestGrpcTransport_SetValue_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeSetValueRequest(t, client, ctx, 999999, "device.param1", int32(100))
	assertGRPCCode(t, err, codes.InvalidArgument, "")
}

func TestGrpcTransport_SetValue_NilValue(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := client.SetValue(ctx, &protos.SingleSetValuePayload{
		Slot:  0,
		Value: nil,
	})

	assertGRPCCode(t, err, codes.InvalidArgument, "nil value")
}

func TestGrpcTransport_SetValue_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		return catena.StatusWithCode(catena.INVALID_ARGUMENT, "value out of range")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeSetValueRequest(t, client, ctx, 0, "device.param1", int32(100))
	assertGRPCCode(t, err, codes.InvalidArgument, "handler error")
}

// =============================================================================
// Test: MultiSetValue
// =============================================================================

func TestGrpcTransport_MultiSetValue_Success(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	callCount := 0
	runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		callCount++
		return catena.StatusWithCode(catena.OK, "")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeMultiSetValueRequest(t, client, ctx, 0, map[string]any{
		"param1": int32(10),
		"param2": "test",
		"param3": float32(3.14),
	})
	assertNoError(t, err)

	if callCount != 3 {
		t.Errorf("expected handler to be called 3 times, got %d", callCount)
	}
}

func TestGrpcTransport_MultiSetValue_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeMultiSetValueRequest(t, client, ctx, 999999, map[string]any{
		"param1": int32(10),
	})
	assertGRPCCode(t, err, codes.InvalidArgument, "")
}

func TestGrpcTransport_MultiSetValue_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	callCount := 0
	runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		callCount++
		// Fail on second value
		if callCount == 2 {
			return catena.StatusWithCode(catena.INVALID_ARGUMENT, "second value invalid")
		}
		return catena.StatusWithCode(catena.OK, "")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeMultiSetValueRequest(t, client, ctx, 0, map[string]any{
		"param1": int32(10),
		"param2": "test",
		"param3": float32(3.14),
	})
	assertGRPCCode(t, err, codes.InvalidArgument, "handler error")

	// Verify processing stopped at second value
	if callCount != 2 {
		t.Errorf("expected handler called 2 times (stopped at error), got %d", callCount)
	}
}

func TestGrpcTransport_MultiSetValue_EmptyValues(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	callCount := 0
	runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		callCount++
		return catena.StatusWithCode(catena.OK, "")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := client.MultiSetValue(ctx, &protos.MultiSetValuePayload{
		Slot:   0,
		Values: []*protos.SetValuePayload{},
	})

	if err != nil {
		t.Fatalf("MultiSetValue with empty values failed: %v", err)
	}

	if callCount != 0 {
		t.Errorf("expected handler not to be called, got %d calls", callCount)
	}
}

// =============================================================================
// Test: ExternalObjectRequest
// =============================================================================

func TestGrpcTransport_ExternalObjectRequest_Success(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	handlerCalled := false
	runtime.getAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult) {
		handlerCalled = true
		if fqoid != "device.image1" {
			t.Errorf("expected fqoid 'device.image1', got %s", fqoid)
		}
		payload := catena.ToPayloadFromURL("http://example.com/asset.jpg")
		asset, _ := catena.ToAsset(payload, true)
		return catena.Reply(asset)
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExternalObjectRequest(t, client, ctx, 0, "device.image1")
	assertNoError(t, err)

	msg := receiveExternalObject(t, stream)
	assertExternalObject(t, msg, true)

	if !handlerCalled {
		t.Error("handler was not called")
	}

	// Verify stream is closed
	assertStreamEOF(t, stream)
}

func TestGrpcTransport_ExternalObjectRequest_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExternalObjectRequest(t, client, ctx, 999999, "device.image1")
	if err != nil {
		assertGRPCCode(t, err, codes.NotFound, "NotFound for invalid slot at stream creation") // Note: server returns NotFound for slots without handlers
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.InvalidArgument, "")
}

func TestGrpcTransport_ExternalObjectRequest_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.getAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult) {
		return catena.ReplyError[catena.Asset](catena.NOT_FOUND, "asset not found")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExternalObjectRequest(t, client, ctx, 0, "device.image1")
	if err != nil {
		assertGRPCCode(t, err, codes.NotFound, "handler error at stream creation")
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.NotFound, "handler error at recv")
}

// =============================================================================
// Test: ParamInfoRequest
// =============================================================================

func TestGrpcTransport_ParamInfoRequest_Success(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	handlerCalled := false
	runtime.paramInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
		handlerCalled = true
		if oidPrefix != "parent" {
			t.Errorf("expected oidPrefix 'parent', got %s", oidPrefix)
		}
		if !recursive {
			t.Error("expected recursive=true")
		}
		return []catena.ParamInfo{
			catena.NewParamInfo("parent", catena.NewPolyglotText("en", "Parent"), catena.ParamTypeStruct, "", 0),
			catena.NewParamInfo("parent/child", nil, catena.ParamTypeInt32, "", 0),
			catena.NewParamInfo("parent/arr", nil, catena.ParamTypeStringArray, "", 5),
		}, catena.StatusWithCode(catena.OK, "")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeParamInfoRequest(t, client, ctx, 0, "parent", true)
	assertNoError(t, err)

	count := 0
	var lastEntry *protos.ParamInfoResponse
	for {
		msg, err := stream.Recv()
		if err == io.EOF {
			break
		}
		assertNoError(t, err)
		count++
		lastEntry = msg
	}

	if count != 3 {
		t.Errorf("expected 3 entries, got %d", count)
	}
	if lastEntry == nil || lastEntry.GetInfo() == nil {
		t.Fatal("expected last entry to have populated Info")
	}
	if lastEntry.GetArrayLength() != 5 {
		t.Errorf("expected last entry array_length=5, got %d", lastEntry.GetArrayLength())
	}
	if !handlerCalled {
		t.Error("handler was not called")
	}
}

func TestGrpcTransport_ParamInfoRequest_TopLevel(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.paramInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
		if oidPrefix != "" {
			t.Errorf("expected empty oidPrefix, got %q", oidPrefix)
		}
		if recursive {
			t.Error("expected recursive=false")
		}
		return []catena.ParamInfo{
			catena.NewParamInfo("a", nil, catena.ParamTypeInt32, "", 0),
			catena.NewParamInfo("b", nil, catena.ParamTypeFloat32, "", 0),
		}, catena.StatusWithCode(catena.OK, "")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeParamInfoRequest(t, client, ctx, 0, "", false)
	assertNoError(t, err)

	count := 0
	for {
		_, err := stream.Recv()
		if err == io.EOF {
			break
		}
		assertNoError(t, err)
		count++
	}
	if count != 2 {
		t.Errorf("expected 2 entries, got %d", count)
	}
}

func TestGrpcTransport_ParamInfoRequest_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.paramInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
		return nil, catena.StatusWithCode(catena.NOT_FOUND, "param not found")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeParamInfoRequest(t, client, ctx, 0, "missing", false)
	if err != nil {
		assertGRPCCode(t, err, codes.NotFound, "handler error at stream creation")
		return
	}
	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.NotFound, "handler error at recv")
}

// TestGrpcTransport_ParamInfoRequest_EmptyResult_NotFound verifies that the gRPC
// transport promotes an OK + empty result to NOT_FOUND.
func TestGrpcTransport_ParamInfoRequest_EmptyResult_NotFound(t *testing.T) {
	t.Run("specific oid empty", func(t *testing.T) {
		ctx := context.Background()
		_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
		defer cleanup()

		runtime.paramInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.OK, "")
		}

		client, cleanup := setupGRPCClient(t, ctx, lis)
		defer cleanup()

		stream, err := makeParamInfoRequest(t, client, ctx, 0, "missing", false)
		if err != nil {
			assertGRPCCode(t, err, codes.NotFound, "empty result at stream creation")
			return
		}
		_, err = stream.Recv()
		assertGRPCCode(t, err, codes.NotFound, "empty result at recv")
	})

	t.Run("top-level empty", func(t *testing.T) {
		ctx := context.Background()
		_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
		defer cleanup()

		runtime.paramInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.OK, "")
		}

		client, cleanup := setupGRPCClient(t, ctx, lis)
		defer cleanup()

		stream, err := makeParamInfoRequest(t, client, ctx, 0, "", false)
		if err != nil {
			assertGRPCCode(t, err, codes.NotFound, "empty top-level at stream creation")
			return
		}
		_, err = stream.Recv()
		assertGRPCCode(t, err, codes.NotFound, "empty top-level at recv")
	})
}

func TestGrpcTransport_ParamInfoRequest_SlotExceedsMax(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeParamInfoRequest(t, client, ctx, 70000, "x", false)
	if err != nil {
		assertGRPCCode(t, err, codes.InvalidArgument, "slot exceeds uint16 max at stream creation")
		return
	}
	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.InvalidArgument, "slot exceeds uint16 max at recv")
}

// =============================================================================
// Test: ExecuteCommand
// =============================================================================

func TestGrpcTransport_ExecuteCommand_WithResponse(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	handlerCalled := false
	runtime.commandFn = func(slot uint16, fqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if fqoid != "device.reboot" {
			t.Errorf("expected fqoid 'device.reboot', got %s", fqoid)
		}
		if payload == nil {
			t.Error("expected non-nil payload")
		}
		value, _ := catena.ToValue(string("Command executed"))
		return catena.CommandReply(value)
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.reboot", int32(5))
	assertNoError(t, err)

	resp := receiveCommandResponse(t, stream)
	assertCommandHasResponse(t, resp)
	assertStringValue(t, resp.GetResponse(), "Command executed")

	if !handlerCalled {
		t.Error("handler was not called")
	}

	// Verify stream is closed
	assertStreamEOF(t, stream)
}

func TestGrpcTransport_ExecuteCommand_WithoutResponse(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.commandFn = func(slot uint16, fqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandNoResponse()
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.command", nil)
	assertNoError(t, err)

	resp := receiveCommandResponse(t, stream)
	assertCommandNoResponse(t, resp)
}

func TestGrpcTransport_ExecuteCommand_NilPayload(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	receivedPayload := "not nil"
	runtime.commandFn = func(slot uint16, fqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult) {
		if payload != nil {
			receivedPayload = "not nil"
		} else {
			receivedPayload = "nil"
		}
		value, _ := catena.ToValue(string("OK"))
		return catena.CommandReply(value)
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.command", nil)
	assertNoError(t, err)

	_ = receiveCommandResponse(t, stream)

	if receivedPayload != "nil" {
		t.Errorf("expected handler to receive nil payload, got %s", receivedPayload)
	}
}

func TestGrpcTransport_ExecuteCommand_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 999999, "device.command", nil)
	if err != nil {
		assertGRPCCode(t, err, codes.NotFound, "NotFound for invalid slot at stream creation") // Note: server returns NotFound for slots without handlers
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.InvalidArgument, "")
}

func TestGrpcTransport_ExecuteCommand_HandlerError(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.commandFn = func(slot uint16, fqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandError(catena.NOT_FOUND, "command not supported")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.command", nil)
	if err != nil {
		assertGRPCCode(t, err, codes.NotFound, "handler error at stream creation")
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.NotFound, "handler error at recv")
}

// =============================================================================
// Test: Connect
// =============================================================================

func TestGrpcTransport_Connect_BroadcastUpdates(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	connection := makeTestConnection(1)
	runtime.WithConnection(connection)

	stream, err := makeConnectRequest(t, client, ctx)
	assertNoError(t, err)

	// Broadcast an update
	go func() {
		time.Sleep(100 * time.Millisecond)
		connection.Updates <- &protos.PushUpdates{
			Slot: 0,
			Kind: &protos.PushUpdates_Value{
				Value: &protos.PushUpdates_PushValue{
					Oid: "device.param1",
					Value: &protos.Value{
						Kind: &protos.Value_Int32Value{
							Int32Value: 99,
						},
					},
				},
			},
		}
	}()

	// Receive broadcast update and verify
	msg := receivePushUpdate(t, stream)
	value := assertPushUpdateValue(t, msg, 0, "device.param1")
	assertInt32Value(t, value, 99)

	// Important: Cancel context to close stream before cleanup
	cancel()
}

func TestGrpcTransport_Connect_ClientDisconnect(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.WithConnection(makeTestConnection(1))

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeConnectRequest(t, client, ctx)
	assertNoError(t, err)

	// Cancel context to simulate client disconnect
	cancel()

	// Next Recv should fail with Canceled error
	_, err = stream.Recv()
	if err == nil {
		t.Fatal("expected error after context cancellation")
	}
	st, ok := status.FromError(err)
	if !ok {
		t.Fatalf("expected gRPC status error, got %v", err)
	}
	if st.Code() != codes.Canceled {
		t.Errorf("expected Canceled code, got %v", st.Code())
	}
}

func TestGrpcTransport_Connect_Rejected(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	// Register WithConnection handler that rejects connection
	runtime.registerTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
		return nil, catena.StatusResult{Code: catena.RESOURCE_EXHAUSTED, Error: "connection rejected"} // Reject connection
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeConnectRequest(t, client, ctx)
	if err != nil {
		t.Fatalf("Unexpected error making Connect request: %s", err)
	}
	_, err = stream.Recv()
	if err == nil {
		t.Fatal("expected error for rejected connection")
	}
	st, ok := status.FromError(err)
	if !ok {
		t.Fatalf("expected gRPC status error, got %v", err)
	}
	if st.Code() != codes.ResourceExhausted {
		t.Errorf("expected ResourceExhausted code, got %v", st.Code())
	}
}

func TestGrpcTransport_Connect_Shutdown(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	connection := makeTestConnection(1)
	runtime.WithConnection(connection)

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeConnectRequest(t, client, ctx)
	assertNoError(t, err)

	// Simulate server shutdown by sending the Done signal on the connection
	connection.Done <- struct{}{}

	// Next Recv should fail with Unavailable error
	_, err = stream.Recv()
	if err == nil {
		t.Fatal("expected error after server shutdown")
	}
	st, ok := status.FromError(err)
	if !ok {
		t.Fatalf("expected gRPC status error, got %v", err)
	}
	if st.Code() != codes.Unavailable {
		t.Errorf("expected Unavailable code, got %v", st.Code())
	}
}

// =============================================================================
// Test: Error Messages Dev vs Prod
// =============================================================================

func TestGrpcTransport_ErrorMessages_DevVsProd(t *testing.T) {
	type endpoint struct {
		name          string
		devMessage    string
		grpcCode      codes.Code
		setupHandlers func(*stubServerRuntime)
		callEndpoint  func(protos.CatenaServiceClient, context.Context) error
	}

	endpoints := []endpoint{
		{
			name:       "GetValue",
			devMessage: "param not supported",
			grpcCode:   codes.Unimplemented,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
					return catena.ReplyError[catena.Value](catena.UNIMPLEMENTED, "param not supported")
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.GetValue(ctx, &protos.GetValuePayload{Slot: 0, Oid: "device.param1"})
				return err
			},
		},
		{
			name:       "SetValue",
			devMessage: "value out of range",
			grpcCode:   codes.InvalidArgument,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
					return catena.StatusWithCode(catena.INVALID_ARGUMENT, "value out of range")
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				protoValue, _ := catena.ToProto(int32(100))
				_, err := client.SetValue(ctx, &protos.SingleSetValuePayload{
					Slot: 0,
					Value: &protos.SetValuePayload{
						Oid:   "device.param1",
						Value: protoValue,
					},
				})
				return err
			},
		},
		{
			name:       "MultiSetValue",
			devMessage: "multi set failed",
			grpcCode:   codes.FailedPrecondition,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.setValueFn = func(value any, slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
					return catena.StatusWithCode(catena.FAILED_PRECONDITION, "multi set failed")
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				protoValue, _ := catena.ToProto(int32(1))
				_, err := client.MultiSetValue(ctx, &protos.MultiSetValuePayload{
					Slot: 0,
					Values: []*protos.SetValuePayload{
						{Oid: "device.param1", Value: protoValue},
					},
				})
				return err
			},
		},
		{
			name:       "GetParam",
			devMessage: "GetParam not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.GetParam(ctx, &protos.GetParamPayload{Slot: 0, Oid: "device.param1"})
				return err
			},
		},
		{
			name:       "AddLanguage",
			devMessage: "AddLanguage not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.AddLanguage(ctx, &protos.AddLanguagePayload{Slot: 0})
				return err
			},
		},
		{
			name:       "LanguagePackRequest",
			devMessage: "LanguagePackRequest not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.LanguagePackRequest(ctx, &protos.LanguagePackRequestPayload{Slot: 0})
				return err
			},
		},
		{
			name:       "ListLanguages",
			devMessage: "ListLanguages not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.ListLanguages(ctx, &protos.Slot{Slot: 0})
				return err
			},
		},
		{
			name:       "RefreshToken",
			devMessage: "RefreshToken not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.RefreshToken(ctx, &protos.RefreshTokenPayload{Reason: "test"})
				return err
			},
		},
		{
			name:       "RevokeAccess",
			devMessage: "RevokeAccess not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.RevokeAccess(ctx, &protos.RevokeAccessPayload{Subject: "test-subject"})
				return err
			},
		},
	}

	for _, ep := range endpoints {
		ep := ep
		t.Run(ep.name+"/dev_shows_details", func(t *testing.T) {
			ctx := context.Background()
			_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0}, withTestGrpcTransportDevMode(true))
			defer cleanup()

			if ep.setupHandlers != nil {
				ep.setupHandlers(runtime)
			}

			client, clientCleanup := setupGRPCClient(t, ctx, lis)
			defer clientCleanup()

			err := ep.callEndpoint(client, ctx)
			if err == nil {
				t.Fatal("expected error, got nil")
			}
			st, ok := status.FromError(err)
			if !ok {
				t.Fatalf("expected gRPC status error, got %v", err)
			}
			if st.Message() != ep.devMessage {
				t.Errorf("expected message %q, got %q", ep.devMessage, st.Message())
			}
		})

		t.Run(ep.name+"/prod_hides_details", func(t *testing.T) {
			original := catena.GetEnv()
			defer catena.SetEnv(original)
			catena.SetEnv(catena.EnvProd)

			ctx := context.Background()
			_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0}, withTestGrpcTransportDevMode(false))
			defer cleanup()

			if ep.setupHandlers != nil {
				ep.setupHandlers(runtime)
			}

			client, clientCleanup := setupGRPCClient(t, ctx, lis)
			defer clientCleanup()

			err := ep.callEndpoint(client, ctx)
			if err == nil {
				t.Fatal("expected error, got nil")
			}
			st, ok := status.FromError(err)
			if !ok {
				t.Fatalf("expected gRPC status error, got %v", err)
			}
			expectedMessage := ep.grpcCode.String()
			if st.Message() != expectedMessage {
				t.Errorf("expected message %q, got %q", expectedMessage, st.Message())
			}
		})
	}
}

func TestErrorMessages_DevVsProd_Streaming(t *testing.T) {
	type endpoint struct {
		name          string
		devMessage    string
		grpcCode      codes.Code
		setupHandlers func(*stubServerRuntime)
		callEndpoint  func(protos.CatenaServiceClient, context.Context) error
	}

	endpoints := []endpoint{
		{
			name:       "DeviceRequest",
			devMessage: "device not found",
			grpcCode:   codes.NotFound,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.getDeviceFn = func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
					return catena.ReplyError[catena.Device](catena.NOT_FOUND, "device not found")
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.DeviceRequest(ctx, &protos.DeviceRequestPayload{Slot: 0})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
				return err
			},
		},
		{
			name:       "ExternalObjectRequest",
			devMessage: "asset not found",
			grpcCode:   codes.NotFound,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.getAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult) {
					return catena.ReplyError[catena.Asset](catena.NOT_FOUND, "asset not found")
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.ExternalObjectRequest(ctx, &protos.ExternalObjectRequestPayload{Slot: 0, Oid: "device.image1"})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
				return err
			},
		},
		{
			name:       "ExecuteCommand",
			devMessage: "command not supported",
			grpcCode:   codes.Unimplemented,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.commandFn = func(slot uint16, fqoid string, payload any, ctx catena.TransportContext) (catena.CommandResult, catena.StatusResult) {
					return catena.CommandError(catena.UNIMPLEMENTED, "command not supported")
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.ExecuteCommand(ctx, &protos.ExecuteCommandPayload{Slot: 0, Oid: "device.command"})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
				return err
			},
		},
		{
			name:       "ParamInfoRequest",
			devMessage: "param not found",
			grpcCode:   codes.NotFound,
			setupHandlers: func(runtime *stubServerRuntime) {
				runtime.paramInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult) {
					return nil, catena.StatusResult{Code: catena.NOT_FOUND, Error: "param not found"}
				}
			},
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.ParamInfoRequest(ctx, &protos.ParamInfoRequestPayload{Slot: 0, OidPrefix: "device"})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
				return err
			},
		},
		{
			name:       "UpdateSubscriptions",
			devMessage: "UpdateSubscriptions not implemented",
			grpcCode:   codes.Unimplemented,
			callEndpoint: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.UpdateSubscriptions(ctx, &protos.UpdateSubscriptionsPayload{Slot: 0})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
				return err
			},
		},
	}

	for _, ep := range endpoints {
		ep := ep
		t.Run(ep.name+"/dev_shows_details", func(t *testing.T) {
			original := catena.GetEnv()
			defer catena.SetEnv(original)
			catena.SetEnv(catena.EnvDev)

			ctx := context.Background()
			_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0}, withTestGrpcTransportDevMode(true))
			defer cleanup()

			if ep.setupHandlers != nil {
				ep.setupHandlers(runtime)
			}

			client, clientCleanup := setupGRPCClient(t, ctx, lis)
			defer clientCleanup()

			err := ep.callEndpoint(client, ctx)
			if err == nil {
				t.Fatal("expected error, got nil")
			}
			st, ok := status.FromError(err)
			if !ok {
				t.Fatalf("expected gRPC status error, got %v", err)
			}
			if st.Message() != ep.devMessage {
				t.Errorf("expected message %q, got %q", ep.devMessage, st.Message())
			}
		})

		t.Run(ep.name+"/prod_hides_details", func(t *testing.T) {
			original := catena.GetEnv()
			defer catena.SetEnv(original)
			catena.SetEnv(catena.EnvProd)

			ctx := context.Background()
			_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0}, withTestGrpcTransportDevMode(false))
			defer cleanup()

			if ep.setupHandlers != nil {
				ep.setupHandlers(runtime)
			}

			client, clientCleanup := setupGRPCClient(t, ctx, lis)
			defer clientCleanup()

			err := ep.callEndpoint(client, ctx)
			if err == nil {
				t.Fatal("expected error, got nil")
			}
			st, ok := status.FromError(err)
			if !ok {
				t.Fatalf("expected gRPC status error, got %v", err)
			}
			expectedMessage := ep.grpcCode.String()
			if st.Message() != expectedMessage {
				t.Errorf("expected message %q, got %q", expectedMessage, st.Message())
			}
		})
	}
}

// =============================================================================
// Test: Unimplemented Endpoints
// =============================================================================

func TestGrpcTransport_UnimplementedEndpoints(t *testing.T) {
	tests := []struct {
		name     string
		callFunc func(protos.CatenaServiceClient, context.Context) error
	}{
		{
			name: "GetParam",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.GetParam(ctx, &protos.GetParamPayload{
					Slot: 0,
					Oid:  "device.param1",
				})
				return err
			},
		},
		{
			name: "UpdateSubscriptions",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.UpdateSubscriptions(ctx, &protos.UpdateSubscriptionsPayload{
					Slot: 0,
				})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
				return err
			},
		},
		{
			name: "AddLanguage",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.AddLanguage(ctx, &protos.AddLanguagePayload{
					Slot: 0,
				})
				return err
			},
		},
		{
			name: "LanguagePackRequest",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.LanguagePackRequest(ctx, &protos.LanguagePackRequestPayload{
					Slot: 0,
				})
				return err
			},
		},
		{
			name: "ListLanguages",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.ListLanguages(ctx, &protos.Slot{
					Slot: 0,
				})
				return err
			},
		},
		{
			name: "RefreshToken",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.RefreshToken(ctx, &protos.RefreshTokenPayload{
					Reason: "test",
				})
				return err
			},
		},
		{
			name: "RevokeAccess",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				_, err := client.RevokeAccess(ctx, &protos.RevokeAccessPayload{
					Subject: "test-subject",
				})
				return err
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx := context.Background()
			_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
			defer cleanup()

			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			err := tt.callFunc(client, ctx)
			assertGRPCCode(t, err, codes.Unimplemented, "unimplemented endpoint")
		})
	}
}

// =============================================================================
// Test: gRPC Reflection
// =============================================================================

func TestGrpcTransport_Reflection_Enabled(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0}, withTestGrpcTransportReflection(true))
	defer cleanup()

	conn, err := dialTestServer(ctx, lis)
	if err != nil {
		t.Fatalf("failed to dial: %v", err)
	}
	defer conn.Close()

	// Use reflection client to list services
	refClient := grpc_reflection_v1.NewServerReflectionClient(conn)
	stream, err := refClient.ServerReflectionInfo(ctx)
	if err != nil {
		t.Fatalf("ServerReflectionInfo failed: %v", err)
	}

	// Request list of services
	err = stream.Send(&grpc_reflection_v1.ServerReflectionRequest{
		MessageRequest: &grpc_reflection_v1.ServerReflectionRequest_ListServices{
			ListServices: "",
		},
	})
	if err != nil {
		t.Fatalf("stream.Send failed: %v", err)
	}

	resp, err := stream.Recv()
	if err != nil {
		t.Fatalf("stream.Recv failed: %v", err)
	}

	listServicesResp := resp.GetListServicesResponse()
	if listServicesResp == nil {
		t.Fatal("expected ListServicesResponse, got nil")
	}

	// Verify CatenaService is listed
	found := false
	for _, svc := range listServicesResp.Service {
		if svc.Name == "st2138.CatenaService" {
			found = true
			break
		}
	}
	if !found {
		t.Error("CatenaService not found in reflection services list")
	}

	// Note: cleanup() handles shutdown automatically via defer
}

func TestGrpcTransport_Reflection_Disabled(t *testing.T) {
	ctx := context.Background()
	_, _, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	conn, err := dialTestServer(ctx, lis)
	if err != nil {
		t.Fatalf("failed to dial: %v", err)
	}
	defer conn.Close()

	// Try to use reflection client - should fail
	refClient := grpc_reflection_v1.NewServerReflectionClient(conn)
	stream, err := refClient.ServerReflectionInfo(ctx)
	if err != nil {
		// Some implementations may fail at stream creation
		assertGRPCError(t, err, codes.Unimplemented)
		return
	}

	// Request list of services
	err = stream.Send(&grpc_reflection_v1.ServerReflectionRequest{
		MessageRequest: &grpc_reflection_v1.ServerReflectionRequest_ListServices{
			ListServices: "",
		},
	})
	if err != nil {
		assertGRPCError(t, err, codes.Unimplemented)
		return
	}

	_, err = stream.Recv()
	assertGRPCError(t, err, codes.Unimplemented)
}

// =============================================================================
// Test: Concurrent Clients
// =============================================================================

func TestGrpcTransport_ConcurrentClients(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0})
	defer cleanup()

	runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
		value, _ := catena.ToValue(int32(42))
		return catena.Reply(value)
	}

	numClients := 10
	results := make(chan error, numClients)

	for i := 0; i < numClients; i++ {
		go func() {
			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			_, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
			results <- err
		}()
	}

	// Check all clients succeeded
	for i := 0; i < numClients; i++ {
		err := <-results
		if err != nil {
			t.Errorf("client %d failed: %v", i, err)
		}
	}
}

// =============================================================================
// Test: Handler per Slot
// =============================================================================

func TestGrpcTransport_DifferentHandlersPerSlot(t *testing.T) {
	ctx := context.Background()
	_, runtime, lis, cleanup := setupTestGrpcTransport(t, []uint16{0, 1})
	defer cleanup()

	// Register different handlers for each slot
	runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
		if slot == 0 {
			value, _ := catena.ToValue(int32(100))
			return catena.Reply(value)
		}
		if slot == 1 {
			value, _ := catena.ToValue(int32(200))
			return catena.Reply(value)
		}
		return catena.ReplyError[catena.Value](catena.NOT_FOUND, "slot not found")
	}

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	// Test slot 0
	resp0, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
	assertNoError(t, err)
	assertInt32Value(t, resp0, 100)

	// Test slot 1
	resp1, err := makeGetValueRequest(t, client, ctx, 1, "device.param1")
	assertNoError(t, err)
	assertInt32Value(t, resp1, 200)
}

// =============================================================================
// Integration Tests: Start() and Shutdown()
// =============================================================================

// TestServer_Start_EndpointsReachable tests that the server starts successfully
// and endpoints are reachable over real TCP connections
func TestGrpcTransport_Start_EndpointsReachable(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	shutdownCalled := false
	transport := NewGrpcTransport(0, false)
	runtime := makeStubServerRuntime(t)
	runtime.slots = []uint16{0, 1}
	runtime.shutdownTransportConnsFn = func(gotCtx context.Context, gotTransport catena.Transport) {
		shutdownCalled = true
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}

	// Register handlers
	runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
		value, _ := catena.ToValue(int32(42))
		return catena.Reply(value)
	}

	runtime.getDeviceFn = func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
		deviceMap := map[string]any{
			"slot":         uint32(slot),
			"detail_level": catena.DetailLevelFull,
		}
		device, _ := catena.ToDevice(deviceMap)
		return catena.Reply(device)
	}

	// Start server in background. Port 0 makes net.Listen pick an available
	// port; read the bound address back from the listener after Start.
	if err := transport.Start(context.Background(), runtime); err != nil {
		t.Fatalf("failed to start server: %v", err)
	}

	// Create real gRPC client connection
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	addr := transport.listener.Addr().String()
	conn, err := grpc.DialContext(ctx, addr, grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithBlock())
	if err != nil {
		transport.Shutdown(context.Background())
		t.Fatalf("failed to connect to server at %s: %v", addr, err)
	}
	defer conn.Close()

	client := protos.NewCatenaServiceClient(conn)

	// Test 1: GetPopulatedSlots
	t.Run("GetPopulatedSlots", func(t *testing.T) {
		resp, err := client.GetPopulatedSlots(ctx, &protos.Empty{})
		assertNoError(t, err)
		assertSlotList(t, resp, []uint32{0, 1})
	})

	// Test 2: GetValue
	t.Run("GetValue", func(t *testing.T) {
		resp, err := client.GetValue(ctx, &protos.GetValuePayload{
			Slot: 0,
			Oid:  "device.param1",
		})
		assertNoError(t, err)
		assertInt32Value(t, resp, 42)
	})

	// Test 3: DeviceRequest (streaming)
	t.Run("DeviceRequest", func(t *testing.T) {
		stream, err := client.DeviceRequest(ctx, &protos.DeviceRequestPayload{Slot: 1})
		assertNoError(t, err)

		msg, err := stream.Recv()
		assertNoError(t, err)

		device := msg.GetDevice()
		assertDevice(t, device, 1)
	})

	// Shutdown server gracefully
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	if err := transport.Shutdown(shutdownCtx); err != nil {
		t.Fatalf("failed to shutdown server: %v", err)
	}
	if !shutdownCalled {
		t.Error("expected shutdown transports callback to be called, but it was not")
	}
}

// TestServer_Shutdown_GracefulConnectionClose tests that Shutdown() properly
// closes active connections
func TestGrpcTransport_Shutdown_GracefulConnectionClose(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	transport := NewGrpcTransport(0, false)
	runtime := makeStubServerRuntime(t)
	runtime.WithConnection(makeTestConnection(1))
	runtime.shutdownTransportConnsFn = func(gotCtx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}

	// Start server in background. Port 0 lets net.Listen pick an available
	// port; read the bound address back from the listener after Start.
	if err := transport.Start(context.Background(), runtime); err != nil {
		t.Fatalf("failed to start server: %v", err)
	}

	// Create client and establish streaming connection.
	// grpc.WithBlock() blocks until the listener accepts, so no startup sleep
	// is required.
	ctx := context.Background()
	addr := transport.listener.Addr().String()
	conn, err := grpc.DialContext(ctx, addr, grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithBlock())
	if err != nil {
		transport.Shutdown(context.Background())
		t.Fatalf("failed to connect to server at %s: %v", addr, err)
	}
	defer conn.Close()

	client := protos.NewCatenaServiceClient(conn)

	// Establish Connect stream (long-lived connection)
	stream, err := client.Connect(ctx, &protos.ConnectPayload{})
	if err != nil {
		transport.Shutdown(context.Background())
		t.Fatalf("Connect failed: %v", err)
	}

	// Shutdown server while connection is active
	shutdownComplete := make(chan struct{})
	go func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		transport.Shutdown(ctx)
		close(shutdownComplete)
	}()
	// Verify shutdown completes
	select {
	case <-shutdownComplete:
		// Success
	case <-time.After(5 * time.Second):
		t.Error("Shutdown() did not complete within timeout")
	}

	// Verify connection gets closed
	_, err = stream.Recv()
	if err == nil {
		t.Error("expected stream to be closed after shutdown, but received message")
	}

}

func TestGrpcTransport_Shutdown_ReturnsContextErrorWhenForced(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	transport := NewGrpcTransport(0, false)
	runtime := makeStubServerRuntime(t)
	runtime.WithConnection(makeTestConnection(1))
	runtime.shutdownTransportConnsFn = func(gotCtx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}

	if err := transport.Start(context.Background(), runtime); err != nil {
		t.Fatalf("failed to start server: %v", err)
	}

	addr := transport.listener.Addr().String()
	conn, err := grpc.DialContext(context.Background(), addr, grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithBlock())
	if err != nil {
		transport.Shutdown(context.Background())
		t.Fatalf("failed to connect to server at %s: %v", addr, err)
	}
	defer conn.Close()

	client := protos.NewCatenaServiceClient(conn)
	stream, err := client.Connect(context.Background(), &protos.ConnectPayload{})
	if err != nil {
		transport.Shutdown(context.Background())
		t.Fatalf("Connect failed: %v", err)
	}

	shutdownCtx, cancel := context.WithTimeout(context.Background(), 10*time.Millisecond)
	defer cancel()

	err = transport.Shutdown(shutdownCtx)
	if !errors.Is(err, context.DeadlineExceeded) {
		t.Fatalf("expected shutdown to return deadline exceeded, got %v", err)
	}

	if _, recvErr := stream.Recv(); recvErr == nil {
		t.Fatal("expected stream to be closed after forced shutdown")
	}
}

func TestGrpcTransport_Shutdown_IgnoresClosedListener(t *testing.T) {
	transport := NewGrpcTransport(1234, false)

	listener, err := net.Listen("tcp", ":0")
	if err != nil {
		t.Fatalf("failed to create listener: %v", err)
	}
	runtime := makeStubServerRuntime(t)
	runtime.shutdownTransportConnsFn = func(gotCtx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}
	transport.listener = listener
	transport.runtime = runtime

	if err := listener.Close(); err != nil {
		t.Fatalf("failed to pre-close listener: %v", err)
	}

	if err := transport.Shutdown(context.Background()); err != nil {
		t.Fatalf("expected nil shutdown error for pre-closed listener, got %v", err)
	}

	if transport.listener != nil {
		t.Fatal("expected shutdown to clear listener reference")
	}
}

// TestServer_Start_PortAlreadyInUse tests that Start() handles port conflicts
func TestGrpcTransport_Start_PortAlreadyInUse(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	// Find an available port and create a listener to block it
	portListener, err := net.Listen("tcp", ":0")
	if err != nil {
		t.Fatalf("failed to create listener for test: %v", err)
	}
	port := portListener.Addr().(*net.TCPAddr).Port
	defer portListener.Close()

	transport := NewGrpcTransport(uint16(port), false)

	runtime := makeStubServerRuntime(t)
	runtime.shutdownTransportConnsFn = func(gotCtx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}

	// Try to start on the already-occupied port
	err = transport.Start(context.Background(), runtime)
	if err == nil {
		transport.Shutdown(context.Background())
		t.Error("expected Start() to fail on occupied port, but it succeeded")
	}
}

// TestServer_MultipleClients_RealNetwork tests concurrent clients over real TCP
func TestGrpcTransport_MultipleClients_RealNetwork(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	runtime := makeStubServerRuntime(t)
	runtime.getValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
		value, _ := catena.ToValue(int32(99))
		return catena.Reply(value)
	}

	// Use port 0 so net.Listen picks an available port; read the bound
	// address back from the listener after Start to avoid a TOCTOU race
	// between Close() and rebind.
	transport := NewGrpcTransport(0, false)

	runtime.shutdownTransportConnsFn = func(gotCtx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}

	// Start server synchronously so the listener is ready before we dial.
	if err := transport.Start(context.Background(), runtime); err != nil {
		t.Fatalf("failed to start server: %v", err)
	}
	defer transport.Shutdown(context.Background())

	addr := transport.listener.Addr().String()

	// Launch multiple concurrent clients. grpc.WithBlock() in each dial below
	// waits for the listener to be ready, so no startup sleep is required.
	numClients := 5
	results := make(chan error, numClients)
	for i := 0; i < numClients; i++ {
		go func(clientID int) {
			ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
			defer cancel()

			conn, err := grpc.DialContext(ctx, addr, grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithBlock())
			if err != nil {
				results <- fmt.Errorf("client %d: dial failed: %w", clientID, err)
				return
			}
			defer conn.Close()

			client := protos.NewCatenaServiceClient(conn)
			resp, err := client.GetValue(ctx, &protos.GetValuePayload{
				Slot: 0,
				Oid:  "test.param",
			})
			if err != nil {
				results <- fmt.Errorf("client %d: GetValue failed: %w", clientID, err)
				return
			}

			if resp.GetInt32Value() != 99 {
				results <- fmt.Errorf("client %d: expected 99, got %d", clientID, resp.GetInt32Value())
				return
			}

			results <- nil
		}(i)
	}

	// Check all clients succeeded
	for i := 0; i < numClients; i++ {
		err := <-results
		if err != nil {
			t.Errorf("client failed: %v", err)
		}
	}
}
