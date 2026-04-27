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

package internal

import (
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewConnectionQueue(t *testing.T) {
	cq := newConnectionQueue(10)

	if cq == nil {
		t.Fatal("NewConnectionQueue returned nil")
	}
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_RegisterDeregister(t *testing.T) {
	cq := newConnectionQueue(0)

	connID, conn := cq.registerConnection()
	if connID <= 0 {
		t.Fatal("RegisterConnection returned negative ID")
	}
	if conn == nil {
		t.Fatal("RegisterConnection returned nil connection")
	}
	if conn.Updates == nil {
		t.Error("connection updates channel should be initialized")
	}
	if conn.Done == nil {
		t.Error("connection done channel should be initialized")
	}
	if cq.ConnectionCount() != 1 {
		t.Errorf("expected 1 connection, got %d", cq.ConnectionCount())
	}

	cq.deregisterConnection(connID)
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after deregister, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_MaxConnections(t *testing.T) {
	cq := newConnectionQueue(2)

	id1, conn1 := cq.registerConnection()
	if id1 < 0 || conn1 == nil {
		t.Fatal("first connection should succeed")
	}

	id2, conn2 := cq.registerConnection()
	if id2 < 0 || conn2 == nil {
		t.Fatal("second connection should succeed")
	}

	id3, conn3 := cq.registerConnection()
	if id3 >= 0 || conn3 != nil {
		t.Error("third connection should be rejected (max 2)")
	}

	if cq.ConnectionCount() != 2 {
		t.Errorf("expected 2 connections, got %d", cq.ConnectionCount())
	}

	cq.deregisterConnection(id1)
	id4, conn4 := cq.registerConnection()
	if id4 < 0 || conn4 == nil {
		t.Error("should be able to connect after one disconnects")
	}
}

func TestConnectionQueue_SetMaxConnections(t *testing.T) {
	cq := newConnectionQueue(1)

	cq.registerConnection()
	id2, _ := cq.registerConnection()
	if id2 >= 0 {
		t.Error("second connection should be rejected")
	}

	cq.SetMaxConnections(2)
	id3, conn3 := cq.registerConnection()
	if id3 < 0 || conn3 == nil {
		t.Error("should succeed after increasing limit")
	}
}

func TestConnectionQueue_NotifyUpdate(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn1 := cq.registerConnection()
	_, conn2 := cq.registerConnection()

	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid: "test/param",
			},
		},
	}

	cq.NotifyUpdate(update)

	checkUpdate := func(name string, conn *Connection) {
		select {
		case received := <-conn.Updates:
			pv, ok := received.Kind.(*protos.PushUpdates_Value)
			if !ok || pv.Value.GetOid() != "test/param" {
				t.Errorf("%s: expected OID 'test/param'", name)
			}
		case <-time.After(time.Second):
			t.Errorf("%s: did not receive update", name)
		}
	}

	checkUpdate("conn1", conn1)
	checkUpdate("conn2", conn2)
}

func TestConnectionQueue_Shutdown(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn1 := cq.registerConnection()
	_, conn2 := cq.registerConnection()

	go func() {
		<-conn1.Done
		cq.deregisterConnection(conn1.ID)
	}()

	go func() {
		<-conn2.Done
		cq.deregisterConnection(conn2.ID)
	}()

	cq.shutdown()

	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_Shutdown_RejectsNewConnections(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn := cq.registerConnection()

	go func() {
		<-conn.Done
		cq.deregisterConnection(conn.ID)
	}()

	cq.shutdown()

	id, c := cq.registerConnection()
	if id >= 0 || c != nil {
		t.Error("should reject new connections after shutdown")
	}
}

func TestBroadcastUpdate(t *testing.T) {
	srv := BaseServer{connectionQueue: newConnectionQueue(100)}

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
	srv := BaseServer{connectionQueue: newConnectionQueue(100)}

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
