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

package grpc

import (
	"context"
	"testing"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func TestNewServer(t *testing.T) {
	slots := []int{0, 1, 2}
	srv := NewServer(slots)

	if srv == nil {
		t.Fatal("NewServer returned nil")
	}

	if len(srv.BaseServer.Slots) != len(slots) {
		t.Errorf("expected %d slots, got %d", len(slots), len(srv.BaseServer.Slots))
	}

	if srv.grpcServer == nil {
		t.Error("grpcServer should not be nil")
	}
}

func TestGetPopulatedSlots(t *testing.T) {
	slots := []int{0, 1, 2}
	srv := NewServer(slots)

	req := &protos.Empty{}
	resp, err := srv.GetPopulatedSlots(context.Background(), req)

	if err != nil {
		t.Fatalf("GetPopulatedSlots failed: %v", err)
	}

	if len(resp.Slots) != len(slots) {
		t.Errorf("expected %d slots, got %d", len(slots), len(resp.Slots))
	}

	for i, slot := range resp.Slots {
		if int(slot) != slots[i] {
			t.Errorf("slot %d: expected %d, got %d", i, slots[i], slot)
		}
	}
}

func TestGetValue(t *testing.T) {
	srv := NewServer([]int{0})

	// Register a test handler
	srv.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		val, _ := catena.ToProto(int32(42))
		return catena.CatenaValue{Value: val}, catena.StatusWithCode(catena.OK, "")
	})

	req := &protos.GetValuePayload{
		Slot: 0,
		Oid:  "test.param",
	}

	resp, err := srv.GetValue(context.Background(), req)
	if err != nil {
		t.Fatalf("GetValue failed: %v", err)
	}

	if resp == nil {
		t.Fatal("GetValue returned nil response")
	}

	// Verify it's an Int32 value
	if resp.GetInt32Value() != 42 {
		t.Errorf("expected value 42, got %v", resp.GetInt32Value())
	}
}

func TestSetValue(t *testing.T) {
	srv := NewServer([]int{0})

	var receivedValue any
	var receivedSlot int
	var receivedOid string

	srv.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
		receivedValue = value
		receivedSlot = slot
		receivedOid = fqoid
		return catena.StatusWithCode(catena.OK, "")
	})

	protoValue, _ := catena.ToProto(int32(123))
	req := &protos.SingleSetValuePayload{
		Slot: 0,
		Value: &protos.SetValuePayload{
			Oid:   "test.param",
			Value: protoValue,
		},
	}

	_, err := srv.SetValue(context.Background(), req)
	if err != nil {
		t.Fatalf("SetValue failed: %v", err)
	}

	if receivedSlot != 0 {
		t.Errorf("expected slot 0, got %d", receivedSlot)
	}

	if receivedOid != "test.param" {
		t.Errorf("expected oid 'test.param', got '%s'", receivedOid)
	}

	if val, ok := receivedValue.(int32); !ok || val != 123 {
		t.Errorf("expected value 123, got %v", receivedValue)
	}
}

