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
 * @date 2026-03-13
 */

package grpc

import (
	"context"
	"fmt"
	"io"
	"net"
	"os"
	"testing"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/reflection/grpc_reflection_v1"
	"google.golang.org/grpc/status"
	"google.golang.org/grpc/test/bufconn"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

const bufSize = 1024 * 1024

// TestMain sets up test environment
func TestMain(m *testing.M) {
	catena.SetEnv(catena.EnvDev)
	os.Exit(m.Run())
}

// setupTestServer creates an in-memory gRPC test server using bufconn
func setupTestServer(t *testing.T, slots []uint16, reflectionEnabled bool) (*Server, *bufconn.Listener, func()) {
	t.Helper()

	lis := bufconn.Listen(bufSize)
	cfg := catena.Config{GRPCReflection: reflectionEnabled}
	srv := NewServer(slots, 100, cfg)

	go func() {
		if err := srv.grpcServer.Serve(lis); err != nil {
			t.Logf("server stopped: %v", err)
		}
	}()

	// Give server a moment to start
	time.Sleep(10 * time.Millisecond)

	cleanup := func() {
		// Use Stop() instead of GracefulStop() in tests to avoid hanging
		// GracefulStop() waits for active streams which causes deadlocks in tests
		srv.grpcServer.Stop()
		lis.Close()
	}

	return srv, lis, cleanup
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
// Test: NewServer
// =============================================================================

func TestNewServer(t *testing.T) {
	slots := []uint16{0, 1, 2}
	cfg := catena.Config{GRPCReflection: false}
	srv := NewServer(slots, 10, cfg)

	if srv == nil {
		t.Fatal("NewServer returned nil")
	}
	if srv.baseServer == nil {
		t.Error("baseServer is nil")
	}
	if srv.grpcServer == nil {
		t.Error("grpcServer is nil")
	}
}

// =============================================================================
// Test: GetPopulatedSlots
// =============================================================================

func TestServer_GetPopulatedSlots(t *testing.T) {
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
			_, lis, cleanup := setupTestServer(t, tt.slots, false)
			defer cleanup()

			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			resp, err := makeGetPopulatedSlotsRequest(t, client, ctx)
			assertNoError(t, err)
			assertSlotList(t, resp, tt.expectedSlots)
		})
	}
}

// =============================================================================
// Test: DeviceRequest
// =============================================================================

func TestServer_DeviceRequest_Success(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	// Register mock handler
	handlerCalled := false
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		handlerCalled = true
		deviceMap := map[string]any{
			"slot":         uint32(0),
			"detail_level": catena.DetailLevelFull,
		}
		device, err := catena.ToCatenaDevice(deviceMap)
		if err.Code != catena.OK {
			t.Fatalf("ToCatenaDevice failed: %v", err.Error)
		}
		return catena.Reply(device)
	})

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

func TestServer_DeviceRequest_InvalidSlot(t *testing.T) {
	tests := []struct {
		name         string
		slot         uint32
		expectedCode codes.Code
	}{
		{
			name:         "No DeviceRequest handler for slot",
			slot:         99,
			expectedCode: codes.NotFound, // Note: server returns NotFound for slots without handlers
		},
		{
			name:         "slot exceeds uint16 max",
			slot:         70000,
			expectedCode: codes.InvalidArgument,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx := context.Background()
			_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
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

func TestServer_DeviceRequest_HandlerError(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	// Register handler that returns error
	srv.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
		return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
	})

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

func TestServer_GetValue_Success(t *testing.T) {
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
			srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
			defer cleanup()

			srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
				if fqoid != "device.param1" {
					t.Errorf("expected fqoid 'device.param1', got %s", fqoid)
				}
				value, _ := catena.ToCatenaValue(tt.handlerValue)
				return catena.Reply(value)
			})

			client, cleanup := setupGRPCClient(t, ctx, lis)
			defer cleanup()

			resp, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
			assertNoError(t, err)
			tt.validateValue(t, resp)
		})
	}
}

func TestServer_GetValue_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeGetValueRequest(t, client, ctx, 99, "device.param1")
	assertGRPCCode(t, err, codes.Unimplemented, "No GetValue handler for invalid slot") // Note: server returns Unimplemented for slots without handlers
}

func TestServer_GetValue_HandlerError(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		return catena.ReplyError[catena.CatenaValue](catena.UNIMPLEMENTED, "param not supported")
	})

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeGetValueRequest(t, client, ctx, 0, "device.param1")
	assertGRPCCode(t, err, codes.Unimplemented, "handler error")
}

// =============================================================================
// Test: SetValue
// =============================================================================

func TestServer_SetValue_Success(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	receivedValue := any(nil)
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		receivedValue = value
		if fqoid != "device.param1" {
			t.Errorf("expected fqoid 'device.param1', got %s", fqoid)
		}
		return catena.StatusWithCode(catena.OK, "")
	})

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

func TestServer_SetValue_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeSetValueRequest(t, client, ctx, 99, "device.param1", int32(100))
	assertGRPCCode(t, err, codes.Unimplemented, "No SetValue handler for invalid slot") // Note: server returns Unimplemented for slots without handlers
}

func TestServer_SetValue_NilValue(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := client.SetValue(ctx, &protos.SingleSetValuePayload{
		Slot:  0,
		Value: nil,
	})

	assertGRPCCode(t, err, codes.InvalidArgument, "nil value")
}

func TestServer_SetValue_HandlerError(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		return catena.StatusWithCode(catena.INVALID_ARGUMENT, "value out of range")
	})

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeSetValueRequest(t, client, ctx, 0, "device.param1", int32(100))
	assertGRPCCode(t, err, codes.InvalidArgument, "handler error")
}

// =============================================================================
// Test: MultiSetValue
// =============================================================================

func TestServer_MultiSetValue_Success(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	callCount := 0
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		callCount++
		return catena.StatusWithCode(catena.OK, "")
	})

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

func TestServer_MultiSetValue_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	_, err := makeMultiSetValueRequest(t, client, ctx, 99, map[string]any{
		"param1": int32(10),
	})
	assertGRPCCode(t, err, codes.Unimplemented, "No MultiSetValue handler for invalid slot") // Note: server returns Unimplemented for slots without handlers
}

func TestServer_MultiSetValue_HandlerError(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	callCount := 0
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		callCount++
		// Fail on second value
		if callCount == 2 {
			return catena.StatusWithCode(catena.INVALID_ARGUMENT, "second value invalid")
		}
		return catena.StatusWithCode(catena.OK, "")
	})

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

func TestServer_MultiSetValue_EmptyValues(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	callCount := 0
	srv.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		callCount++
		return catena.StatusWithCode(catena.OK, "")
	})

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

func TestServer_ExternalObjectRequest_Success(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	handlerCalled := false
	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		handlerCalled = true
		if fqoid != "device.image1" {
			t.Errorf("expected fqoid 'device.image1', got %s", fqoid)
		}
		payload := catena.ToPayloadFromURL("http://example.com/asset.jpg")
		asset, _ := catena.ToCatenaAsset(payload, true)
		return catena.Reply(asset)
	})

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

func TestServer_ExternalObjectRequest_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExternalObjectRequest(t, client, ctx, 99, "device.image1")
	if err != nil {
		assertGRPCCode(t, err, codes.Unimplemented, "Unimplemented for invalid slot at stream creation") // Note: server returns Unimplemented for slots without handlers
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.Unimplemented, "Unimplemented for invalid slot at recv")
}

func TestServer_ExternalObjectRequest_HandlerError(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found")
	})

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
// Test: ExecuteCommand
// =============================================================================

func TestServer_ExecuteCommand_WithResponse(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	handlerCalled := false
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		handlerCalled = true
		if fqoid != "device.reboot" {
			t.Errorf("expected fqoid 'device.reboot', got %s", fqoid)
		}
		if payload == nil {
			t.Error("expected non-nil payload")
		}
		value, _ := catena.ToCatenaValue(string("Command executed"))
		return catena.CommandReply(value)
	})

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

func TestServer_ExecuteCommand_WithoutResponse(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandNoResponse()
	})

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.command", nil)
	assertNoError(t, err)

	resp := receiveCommandResponse(t, stream)
	assertCommandNoResponse(t, resp)
}

func TestServer_ExecuteCommand_NilPayload(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	receivedPayload := "not nil"
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		if payload != nil {
			receivedPayload = "not nil"
		} else {
			receivedPayload = "nil"
		}
		value, _ := catena.ToCatenaValue(string("OK"))
		return catena.CommandReply(value)
	})

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.command", nil)
	assertNoError(t, err)

	_ = receiveCommandResponse(t, stream)

	if receivedPayload != "nil" {
		t.Errorf("expected handler to receive nil payload, got %s", receivedPayload)
	}
}

func TestServer_ExecuteCommand_InvalidSlot(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 99, "device.command", nil)
	if err != nil {
		assertGRPCCode(t, err, codes.Unimplemented, "Unimplemented for invalid slot at stream creation") // Note: server returns Unimplemented for slots without handlers
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.Unimplemented, "Unimplemented for invalid slot at recv")
}

func TestServer_ExecuteCommand_HandlerError(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	srv.RegisterExecuteCommandHandler(0, func(slot uint16, fqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		return catena.CommandError(catena.UNIMPLEMENTED, "command not supported")
	})

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeExecuteCommandRequest(t, client, ctx, 0, "device.command", nil)
	if err != nil {
		assertGRPCCode(t, err, codes.Unimplemented, "handler error at stream creation")
		return
	}

	_, err = stream.Recv()
	assertGRPCCode(t, err, codes.Unimplemented, "handler error at recv")
}

// =============================================================================
// Test: Connect
// =============================================================================

func TestServer_Connect_InitialSlots(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	_, lis, cleanup := setupTestServer(t, []uint16{0, 1, 2}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeConnectRequest(t, client, ctx)
	assertNoError(t, err)

	// Receive initial SlotsAdded update
	msg := receivePushUpdate(t, stream)
	assertPushUpdateSlotsAdded(t, msg, []uint32{0, 1, 2})

	// Important: Cancel context to close the stream before cleanup
	// Otherwise Stop() in cleanup will forcefully terminate the stream
	cancel()
}

func TestServer_Connect_BroadcastUpdates(t *testing.T) {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeConnectRequest(t, client, ctx)
	assertNoError(t, err)

	// Receive initial update
	_ = receivePushUpdate(t, stream)

	// Broadcast an update
	go func() {
		time.Sleep(100 * time.Millisecond)
		srv.BroadcastUpdate(0, "device.param1", int32(99))
	}()

	// Receive broadcast update and verify
	msg := receivePushUpdate(t, stream)
	value := assertPushUpdateValue(t, msg, 0, "device.param1")
	assertInt32Value(t, value, 99)

	// Important: Cancel context to close stream before cleanup
	cancel()
}

func TestServer_Connect_ClientDisconnect(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	client, cleanup := setupGRPCClient(t, ctx, lis)
	defer cleanup()

	stream, err := makeConnectRequest(t, client, ctx)
	assertNoError(t, err)

	// Receive initial update
	_ = receivePushUpdate(t, stream)

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

// =============================================================================
// Test: Unimplemented Endpoints
// =============================================================================

func TestServer_UnimplementedEndpoints(t *testing.T) {
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
			name: "ParamInfoRequest",
			callFunc: func(client protos.CatenaServiceClient, ctx context.Context) error {
				stream, err := client.ParamInfoRequest(ctx, &protos.ParamInfoRequestPayload{
					Slot:      0,
					OidPrefix: "device",
				})
				if err != nil {
					return err
				}
				_, err = stream.Recv()
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
			_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
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

func TestServer_Reflection_Enabled(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, true)
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

func TestServer_Reflection_Disabled(t *testing.T) {
	ctx := context.Background()
	_, lis, cleanup := setupTestServer(t, []uint16{0}, false)
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

func TestServer_ConcurrentClients(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0}, false)
	defer cleanup()

	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		value, _ := catena.ToCatenaValue(int32(42))
		return catena.Reply(value)
	})

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

func TestServer_DifferentHandlersPerSlot(t *testing.T) {
	ctx := context.Background()
	srv, lis, cleanup := setupTestServer(t, []uint16{0, 1}, false)
	defer cleanup()

	// Register different handlers for each slot
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		value, _ := catena.ToCatenaValue(int32(100))
		return catena.Reply(value)
	})

	srv.RegisterGetValueHandler(1, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		value, _ := catena.ToCatenaValue(int32(200))
		return catena.Reply(value)
	})

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
func TestServer_Start_EndpointsReachable(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	cfg := catena.Config{GRPCReflection: false}
	srv := NewServer([]uint16{0, 1}, 10, cfg)

	// Register handlers
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		value, _ := catena.ToCatenaValue(int32(42))
		return catena.Reply(value)
	})

	srv.RegisterGetDeviceHandler(1, func() (catena.CatenaDevice, catena.StatusResult) {
		deviceMap := map[string]any{
			"slot":         uint32(1),
			"detail_level": catena.DetailLevelFull,
		}
		device, _ := catena.ToCatenaDevice(deviceMap)
		return catena.Reply(device)
	})

	// Use port 0 to let OS assign available port
	port := 0
	if portListener, err := net.Listen("tcp", fmt.Sprintf(":%d", port)); err == nil {
		port = portListener.Addr().(*net.TCPAddr).Port
		portListener.Close()
	}

	// Start server in background
	serverErr := make(chan error, 1)
	go func() {
		serverErr <- srv.Start(port)
	}()

	// Give server time to start
	time.Sleep(100 * time.Millisecond)

	// Create real gRPC client connection
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	addr := fmt.Sprintf("localhost:%d", port)
	conn, err := grpc.DialContext(ctx, addr, grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithBlock())
	if err != nil {
		srv.Shutdown()
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
	srv.Shutdown()

	// Verify server stopped (Start() should return)
	select {
	case err := <-serverErr:
		// Expected: server stopped, no error or "use of closed network connection"
		if err != nil && err.Error() != grpc.ErrServerStopped.Error() {
			t.Logf("Server stopped with: %v", err)
		}
	case <-time.After(2 * time.Second):
		t.Error("server did not stop within timeout")
	}
}

// TestServer_Shutdown_GracefulConnectionClose tests that Shutdown() properly
// closes active connections
func TestServer_Shutdown_GracefulConnectionClose(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	cfg := catena.Config{GRPCReflection: false}
	srv := NewServer([]uint16{0}, 10, cfg)

	// Use port 0 to let OS assign available port
	port := 0
	if portListener, err := net.Listen("tcp", fmt.Sprintf(":%d", port)); err == nil {
		port = portListener.Addr().(*net.TCPAddr).Port
		portListener.Close()
	}

	// Start server in background
	serverErr := make(chan error, 1)
	go func() {
		serverErr <- srv.Start(port)
	}()

	// Give server time to start
	time.Sleep(100 * time.Millisecond)

	// Create client and establish streaming connection
	ctx := context.Background()
	addr := fmt.Sprintf("localhost:%d", port)
	conn, err := grpc.DialContext(ctx, addr, grpc.WithTransportCredentials(insecure.NewCredentials()), grpc.WithBlock())
	if err != nil {
		srv.Shutdown()
		t.Fatalf("failed to connect to server at %s: %v", addr, err)
	}
	defer conn.Close()

	client := protos.NewCatenaServiceClient(conn)

	// Establish Connect stream (long-lived connection)
	stream, err := client.Connect(ctx, &protos.ConnectPayload{})
	if err != nil {
		srv.Shutdown()
		t.Fatalf("Connect failed: %v", err)
	}

	// Receive initial update
	_, err = stream.Recv()
	assertNoError(t, err)

	// Shutdown server while connection is active
	shutdownComplete := make(chan struct{})
	go func() {
		srv.Shutdown()
		close(shutdownComplete)
	}()

	// Verify connection gets closed
	_, err = stream.Recv()
	if err == nil {
		t.Error("expected stream to be closed after shutdown, but received message")
	}

	// Verify shutdown completes
	select {
	case <-shutdownComplete:
		// Success
	case <-time.After(5 * time.Second):
		t.Error("Shutdown() did not complete within timeout")
	}

	// Verify server stopped
	select {
	case <-serverErr:
		// Success
	case <-time.After(1 * time.Second):
		t.Error("server did not stop after Shutdown()")
	}
}

// TestServer_Start_PortAlreadyInUse tests that Start() handles port conflicts
func TestServer_Start_PortAlreadyInUse(t *testing.T) {
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

	cfg := catena.Config{GRPCReflection: false}
	srv := NewServer([]uint16{0}, 10, cfg)

	// Try to start on the already-occupied port
	err = srv.Start(port)
	if err == nil {
		srv.Shutdown()
		t.Error("expected Start() to fail on occupied port, but it succeeded")
	}
}

// TestServer_MultipleClients_RealNetwork tests concurrent clients over real TCP
func TestServer_MultipleClients_RealNetwork(t *testing.T) {
	if testing.Short() {
		t.Skip("skipping integration test in short mode")
	}

	cfg := catena.Config{GRPCReflection: false}
	srv := NewServer([]uint16{0}, 10, cfg)

	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		value, _ := catena.ToCatenaValue(int32(99))
		return catena.Reply(value)
	})

	// Use port 0 to let OS assign available port
	port := 0
	if portListener, err := net.Listen("tcp", fmt.Sprintf(":%d", port)); err == nil {
		port = portListener.Addr().(*net.TCPAddr).Port
		portListener.Close()
	}

	// Start server
	go func() {
		srv.Start(port)
	}()
	defer srv.Shutdown()

	// Give server time to start
	time.Sleep(100 * time.Millisecond)

	// Launch multiple concurrent clients
	numClients := 5
	results := make(chan error, numClients)

	addr := fmt.Sprintf("localhost:%d", port)
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
