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

package catena

import (
	"context"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewConnectionQueue(t *testing.T) {
	cq := newConnectionQueue(10)

	if cq == nil {
		t.Fatal("NewConnectionQueue returned nil")
	}
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_RegisterDeregister(t *testing.T) {
	cq := newConnectionQueue(0)
	owner := &stubTransport{tb: t}

	connID, conn := cq.registerOwnedConnection(owner, nil)
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
	if cq.connectionCount() != 1 {
		t.Errorf("expected 1 connection, got %d", cq.connectionCount())
	}

	cq.deregisterConnection(connID)
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after deregister, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_RegisterConnection_InitialUpdate(t *testing.T) {
	cq := newConnectionQueue(0)
	owner := &stubTransport{tb: t}

	initialUpdate := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid: "initial/param",
			},
		},
	}

	connID, conn := cq.registerOwnedConnection(owner, initialUpdate)
	if connID <= 0 {
		t.Fatal("RegisterConnection returned negative ID")
	}
	if conn == nil {
		t.Fatal("RegisterConnection returned nil connection")
	}

	select {
	case received := <-conn.Updates:
		pv, ok := received.Kind.(*protos.PushUpdates_Value)
		if !ok || pv.Value.GetOid() != "initial/param" {
			t.Errorf("expected OID 'initial/param' in initial update")
		}
	case <-time.After(time.Second):
		t.Error("did not receive initial update")
	}
}

func TestConnectionQueue_MaxConnections(t *testing.T) {
	cq := newConnectionQueue(2)
	owner := &stubTransport{tb: t}

	id1, conn1 := cq.registerOwnedConnection(owner, nil)
	if id1 < 0 || conn1 == nil {
		t.Fatal("first connection should succeed")
	}

	id2, conn2 := cq.registerOwnedConnection(owner, nil)
	if id2 < 0 || conn2 == nil {
		t.Fatal("second connection should succeed")
	}

	id3, conn3 := cq.registerOwnedConnection(owner, nil)
	if id3 >= 0 || conn3 != nil {
		t.Error("third connection should be rejected (max 2)")
	}

	if cq.connectionCount() != 2 {
		t.Errorf("expected 2 connections, got %d", cq.connectionCount())
	}

	cq.deregisterConnection(id1)
	id4, conn4 := cq.registerOwnedConnection(owner, nil)
	if id4 < 0 || conn4 == nil {
		t.Error("should be able to connect after one disconnects")
	}
}

func TestConnectionQueue_SetMaxConnections(t *testing.T) {
	cq := newConnectionQueue(1)
	owner := &stubTransport{tb: t}

	cq.registerOwnedConnection(owner, nil)
	id2, _ := cq.registerOwnedConnection(owner, nil)
	if id2 >= 0 {
		t.Error("second connection should be rejected")
	}

	cq.setMaxConnections(2)
	id3, conn3 := cq.registerOwnedConnection(owner, nil)
	if id3 < 0 || conn3 == nil {
		t.Error("should succeed after increasing limit")
	}
}

func TestConnectionQueue_NotifyUpdate(t *testing.T) {
	cq := newConnectionQueue(0)

	owner := &stubTransport{tb: t}

	_, conn1 := cq.registerOwnedConnection(owner, nil)
	_, conn2 := cq.registerOwnedConnection(owner, nil)

	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid: "test/param",
			},
		},
	}

	cq.notifyUpdate(update)

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
	owner := &stubTransport{tb: t}

	_, conn1 := cq.registerOwnedConnection(owner, nil)
	_, conn2 := cq.registerOwnedConnection(owner, nil)

	go func() {
		<-conn1.Done
		cq.deregisterConnection(conn1.ID)
	}()

	go func() {
		<-conn2.Done
		cq.deregisterConnection(conn2.ID)
	}()

	cq.shutdown(context.Background())

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_Shutdown_RejectsNewConnections(t *testing.T) {
	cq := newConnectionQueue(0)
	owner := &stubTransport{tb: t}

	_, conn := cq.registerOwnedConnection(owner, nil)

	go func() {
		<-conn.Done
		cq.deregisterConnection(conn.ID)
	}()

	cq.shutdown(context.Background())

	id, c := cq.registerOwnedConnection(owner, nil)
	if id >= 0 || c != nil {
		t.Error("should reject new connections after shutdown")
	}
}

func TestConnectionQueue_ShutdownOwner(t *testing.T) {
	cq := newConnectionQueue(0)
	ownerA := &stubTransport{tb: t}
	ownerB := &stubTransport{tb: t}

	_, connA := cq.registerOwnedConnection(ownerA, nil)
	_, connB := cq.registerOwnedConnection(ownerB, nil)

	go func() {
		<-connA.Done
		cq.deregisterConnection(connA.ID)
	}()

	select {
	case <-connB.Done:
		t.Fatal("unexpected shutdown signal for unrelated owner")
	default:
	}

	cq.shutdownOwner(context.Background(), ownerA)

	if cq.connectionCount() != 1 {
		t.Fatalf("expected one remaining connection after owner shutdown, got %d", cq.connectionCount())
	}

	select {
	case <-connB.Done:
		t.Fatal("unexpected shutdown signal for unrelated owner")
	default:
	}

	cq.deregisterConnection(connB.ID)
}

func TestConnectionQueue_ShutdownConnection_Graceful(t *testing.T) {
	cq := newConnectionQueue(0)
	owner := &stubTransport{tb: t}

	_, conn := cq.registerOwnedConnection(owner, nil)

	go func() {
		<-conn.Done
		cq.deregisterConnection(conn.ID)
	}()

	cq.shutdownConnection(context.Background(), conn)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_ShutdownConnection_AlreadyClosed(t *testing.T) {
	cq := newConnectionQueue(0)
	owner := &stubTransport{tb: t}

	_, conn := cq.registerOwnedConnection(owner, nil)

	go func() {
		<-conn.Done
		cq.deregisterConnection(conn.ID)
	}()

	close(conn.Done) // simulate connection already closed

	cq.shutdownConnection(context.Background(), conn)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConectionQueue_ShutdownConnection_Deadline(t *testing.T) {
	cq := newConnectionQueue(0)
	owner := &stubTransport{tb: t}

	_, conn := cq.registerOwnedConnection(owner, nil)

	// don't deregister

	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()
	cq.shutdownConnection(ctx, conn)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_ShutdownConnection_OneDeadline(t *testing.T) {
	cq := newConnectionQueue(0)
	ownerA := &stubTransport{tb: t}
	ownerB := &stubTransport{tb: t}

	_, connA := cq.registerOwnedConnection(ownerA, nil)
	_, connB := cq.registerOwnedConnection(ownerB, nil)

	// only properly deregsiter connA, leave connB hanging

	go func() {
		<-connA.Done
		cq.deregisterConnection(connA.ID)
	}()

	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()
	cq.shutdownConnection(ctx, connA)
	cq.shutdownConnection(ctx, connB)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}
