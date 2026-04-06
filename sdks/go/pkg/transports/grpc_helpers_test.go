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
 * @brief Test helpers for gRPC server tests.
 * @file test_helpers_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-03-15
 */

package transports

import (
	"context"
	"io"
	"testing"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/grpc/test/bufconn"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func boolPtr(b bool) *bool { return &b }

// --- Client setup helpers ---

// setupGRPCClient creates a gRPC client connected to the test server.
// Returns the client and a cleanup function that should be deferred.
func setupGRPCClient(t *testing.T, ctx context.Context, lis *bufconn.Listener) (protos.CatenaServiceClient, func()) {
	t.Helper()

	conn, err := dialTestServer(ctx, lis)
	if err != nil {
		t.Fatalf("failed to dial test server: %v", err)
	}

	client := protos.NewCatenaServiceClient(conn)

	cleanup := func() {
		conn.Close()
	}

	return client, cleanup
}

// --- Unary RPC helpers ---

// makeGetPopulatedSlotsRequest makes a GetPopulatedSlots RPC call.
func makeGetPopulatedSlotsRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context) (*protos.SlotList, error) {
	t.Helper()
	return client.GetPopulatedSlots(ctx, &protos.Empty{})
}

// makeGetValueRequest makes a GetValue RPC call with the specified slot and OID.
func makeGetValueRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32, oid string) (*protos.Value, error) {
	t.Helper()
	return client.GetValue(ctx, &protos.GetValuePayload{
		Slot: slot,
		Oid:  oid,
	})
}

// makeSetValueRequest makes a SetValue RPC call with the specified slot, OID, and value.
func makeSetValueRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32, oid string, value any) (*protos.Empty, error) {
	t.Helper()

	protoValue, err := catena.ToProto(value)
	if err.Code != catena.OK {
		t.Fatalf("failed to convert value to proto: %v", err.Error)
	}

	return client.SetValue(ctx, &protos.SingleSetValuePayload{
		Slot: slot,
		Value: &protos.SetValuePayload{
			Oid:   oid,
			Value: protoValue,
		},
	})
}

// makeMultiSetValueRequest makes a MultiSetValue RPC call with multiple OID/value pairs.
func makeMultiSetValueRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32, values map[string]any) (*protos.Empty, error) {
	t.Helper()

	setValues := make([]*protos.SetValuePayload, 0, len(values))
	for oid, value := range values {
		protoValue, err := catena.ToProto(value)
		if err.Code != catena.OK {
			t.Fatalf("failed to convert value for %s to proto: %v", oid, err.Error)
		}
		setValues = append(setValues, &protos.SetValuePayload{
			Oid:   oid,
			Value: protoValue,
		})
	}

	return client.MultiSetValue(ctx, &protos.MultiSetValuePayload{
		Slot:   slot,
		Values: setValues,
	})
}

// --- Streaming RPC helpers ---

// makeDeviceRequest initiates a DeviceRequest stream and returns the stream handle.
func makeDeviceRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32) (grpc.ServerStreamingClient[protos.DeviceComponent], error) {
	t.Helper()
	return client.DeviceRequest(ctx, &protos.DeviceRequestPayload{Slot: slot})
}

// makeExternalObjectRequest initiates an ExternalObjectRequest stream.
func makeExternalObjectRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32, oid string) (grpc.ServerStreamingClient[protos.ExternalObjectPayload], error) {
	t.Helper()
	return client.ExternalObjectRequest(ctx, &protos.ExternalObjectRequestPayload{
		Slot: slot,
		Oid:  oid,
	})
}

// makeExecuteCommandRequest initiates an ExecuteCommand stream with optional payload.
// Pass nil for respond to leave it unset (server defaults to responding).
func makeExecuteCommandRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32, oid string, payload any, respond *bool) (grpc.ServerStreamingClient[protos.CommandResponse], error) {
	t.Helper()

	req := &protos.ExecuteCommandPayload{
		Slot: slot,
		Oid:  oid,
	}

	if respond != nil {
		req.Respond = *respond
	} else {
		req.Respond = true
	}

	if payload != nil {
		protoValue, err := catena.ToProto(payload)
		if err.Code != catena.OK {
			t.Fatalf("failed to convert command payload to proto: %v", err.Error)
		}
		req.Value = protoValue
	}

	return client.ExecuteCommand(ctx, req)
}

// makeConnectRequest initiates a Connect stream for push updates.
func makeConnectRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context) (grpc.ServerStreamingClient[protos.PushUpdates], error) {
	t.Helper()
	return client.Connect(ctx, &protos.ConnectPayload{})
}

// makeParamInfoRequest initiates a ParamInfoRequest stream for parameter info entries.
func makeParamInfoRequest(t *testing.T, client protos.CatenaServiceClient, ctx context.Context, slot uint32, oidPrefix string, recursive bool) (grpc.ServerStreamingClient[protos.ParamInfoResponse], error) {
	t.Helper()
	return client.ParamInfoRequest(ctx, &protos.ParamInfoRequestPayload{
		Slot:      slot,
		OidPrefix: oidPrefix,
		Recursive: recursive,
	})
}

// --- Stream reading helpers ---

// receiveDeviceComponent reads one DeviceComponent from a stream and returns it.
// Fails the test if an error occurs.
func receiveDeviceComponent(t *testing.T, stream grpc.ServerStreamingClient[protos.DeviceComponent]) *protos.DeviceComponent {
	t.Helper()
	msg, err := stream.Recv()
	if err != nil {
		t.Fatalf("failed to receive device component: %v", err)
	}
	return msg
}

// receiveExternalObject reads one ExternalObjectPayload from a stream.
func receiveExternalObject(t *testing.T, stream grpc.ServerStreamingClient[protos.ExternalObjectPayload]) *protos.ExternalObjectPayload {
	t.Helper()
	msg, err := stream.Recv()
	if err != nil {
		t.Fatalf("failed to receive external object: %v", err)
	}
	return msg
}

// receiveCommandResponse reads one CommandResponse from a stream.
func receiveCommandResponse(t *testing.T, stream grpc.ServerStreamingClient[protos.CommandResponse]) *protos.CommandResponse {
	t.Helper()
	msg, err := stream.Recv()
	if err != nil {
		t.Fatalf("failed to receive command response: %v", err)
	}
	return msg
}

// receivePushUpdate reads one PushUpdates from a stream.
func receivePushUpdate(t *testing.T, stream grpc.ServerStreamingClient[protos.PushUpdates]) *protos.PushUpdates {
	t.Helper()
	msg, err := stream.Recv()
	if err != nil {
		t.Fatalf("failed to receive push update: %v", err)
	}
	return msg
}

// assertStreamEOF verifies that the stream has ended by checking for EOF error.
func assertStreamEOF[T any](t *testing.T, stream grpc.ServerStreamingClient[T]) {
	t.Helper()
	_, err := stream.Recv()
	if err != io.EOF {
		t.Errorf("expected EOF, got %v", err)
	}
}

// --- Assertion helpers ---

// assertNoError fails the test if err is not nil.
func assertNoError(t *testing.T, err error) {
	t.Helper()
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
}

// assertGRPCCode verifies that the error has the expected gRPC status code.
// This is a more descriptive version of assertGRPCError.
func assertGRPCCode(t *testing.T, err error, expectedCode codes.Code, msgContext string) {
	t.Helper()
	if err == nil {
		t.Fatalf("%s: expected error with code %v, got nil", msgContext, expectedCode)
	}
	st, ok := status.FromError(err)
	if !ok {
		t.Fatalf("%s: expected gRPC status error, got %v", msgContext, err)
	}
	if st.Code() != expectedCode {
		t.Errorf("%s: expected code %v, got %v (message: %s)", msgContext, expectedCode, st.Code(), st.Message())
	}
}

// assertSlotList verifies that the SlotList contains the expected slots.
func assertSlotList(t *testing.T, slotList *protos.SlotList, expectedSlots []uint32) {
	t.Helper()
	if slotList == nil {
		t.Fatal("slot list is nil")
	}
	if len(slotList.Slots) != len(expectedSlots) {
		t.Fatalf("expected %d slots, got %d", len(expectedSlots), len(slotList.Slots))
	}
	for i, slot := range slotList.Slots {
		if slot != expectedSlots[i] {
			t.Errorf("slot[%d]: expected %d, got %d", i, expectedSlots[i], slot)
		}
	}
}

// assertDevice verifies basic properties of a Device message.
func assertDevice(t *testing.T, device *protos.Device, expectedSlot uint32) {
	t.Helper()
	if device == nil {
		t.Fatal("device is nil")
	}
	if device.GetSlot() != expectedSlot {
		t.Errorf("expected slot %d, got %d", expectedSlot, device.GetSlot())
	}
}

// assertValue verifies that a Value message matches the expected type and value.
func assertValue(t *testing.T, value *protos.Value, validator func(*testing.T, *protos.Value)) {
	t.Helper()
	if value == nil {
		t.Fatal("value is nil")
	}
	validator(t, value)
}

// assertInt32Value checks that a Value contains the expected int32.
func assertInt32Value(t *testing.T, value *protos.Value, expected int32) {
	t.Helper()
	if value == nil {
		t.Fatal("value is nil")
	}
	if value.GetInt32Value() != expected {
		t.Errorf("expected int32 value %d, got %d", expected, value.GetInt32Value())
	}
}

// assertFloat32Value checks that a Value contains the expected float32.
func assertFloat32Value(t *testing.T, value *protos.Value, expected float32) {
	t.Helper()
	if value == nil {
		t.Fatal("value is nil")
	}
	if value.GetFloat32Value() != expected {
		t.Errorf("expected float32 value %f, got %f", expected, value.GetFloat32Value())
	}
}

// assertStringValue checks that a Value contains the expected string.
func assertStringValue(t *testing.T, value *protos.Value, expected string) {
	t.Helper()
	if value == nil {
		t.Fatal("value is nil")
	}
	if value.GetStringValue() != expected {
		t.Errorf("expected string value %q, got %q", expected, value.GetStringValue())
	}
}

// assertExternalObject verifies basic properties of an ExternalObjectPayload.
func assertExternalObject(t *testing.T, obj *protos.ExternalObjectPayload, expectedCachable bool) {
	t.Helper()
	if obj == nil {
		t.Fatal("external object is nil")
	}
	if obj.GetCachable() != expectedCachable {
		t.Errorf("expected cachable=%v, got %v", expectedCachable, obj.GetCachable())
	}
}

// assertCommandResponse verifies that a command response is not empty.
func assertCommandResponse(t *testing.T, resp *protos.CommandResponse) {
	t.Helper()
	if resp == nil {
		t.Fatal("command response is nil")
	}
}

// assertCommandHasResponse verifies the command returned a response value.
func assertCommandHasResponse(t *testing.T, resp *protos.CommandResponse) {
	t.Helper()
	if resp.GetResponse() == nil {
		t.Fatal("expected command response value, got nil")
	}
}

// assertCommandNoResponse verifies the command returned no response.
func assertCommandNoResponse(t *testing.T, resp *protos.CommandResponse) {
	t.Helper()
	if resp.GetNoResponse() == nil {
		t.Fatal("expected command no_response, got response value")
	}
}

// assertPushUpdateSlotsAdded verifies a PushUpdate contains SlotsAdded.
func assertPushUpdateSlotsAdded(t *testing.T, update *protos.PushUpdates, expectedSlots []uint32) {
	t.Helper()
	if update == nil {
		t.Fatal("push update is nil")
	}
	slotsAdded := update.GetSlotsAdded()
	if slotsAdded == nil {
		t.Fatal("expected SlotsAdded, got nil")
	}
	assertSlotList(t, slotsAdded, expectedSlots)
}

// assertPushUpdateValue verifies a PushUpdate contains a value update.
func assertPushUpdateValue(t *testing.T, update *protos.PushUpdates, expectedSlot uint32, expectedOid string) *protos.Value {
	t.Helper()
	if update == nil {
		t.Fatal("push update is nil")
	}
	pushValue := update.GetValue()
	if pushValue == nil {
		t.Fatal("expected PushValue, got nil")
	}
	if update.GetSlot() != expectedSlot {
		t.Errorf("expected slot %d, got %d", expectedSlot, update.GetSlot())
	}
	if pushValue.GetOid() != expectedOid {
		t.Errorf("expected oid %q, got %q", expectedOid, pushValue.GetOid())
	}
	return pushValue.GetValue()
}
