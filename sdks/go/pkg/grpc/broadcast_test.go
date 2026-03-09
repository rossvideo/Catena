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
	"testing"
	"time"

	"github.com/rossvideo/catena/build/go/protos"
)

func TestBroadcastUpdate(t *testing.T) {
	srv := NewServer([]int{0}, 100)

	// Register a connection
	connID, conn := srv.RegisterConnection()
	if connID < 0 {
		t.Fatal("Failed to register connection")
	}
	defer srv.DeregisterConnection(connID)

	// Broadcast an update
	srv.BroadcastUpdate(0, "test/param", int32(42))

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

func TestBroadcastUpdate_InvalidValue(t *testing.T) {
	srv := NewServer([]int{0}, 100)

	_, conn := srv.RegisterConnection()
	defer srv.DeregisterConnection(conn.ID)

	// Try to broadcast an invalid value (bool is not supported by ToProto)
	srv.BroadcastUpdate(0, "test/param", true)

	// Should not receive update (logged error instead)
	select {
	case <-conn.Updates:
		t.Error("should not have received update for invalid value")
	case <-time.After(100 * time.Millisecond):
	}
}
