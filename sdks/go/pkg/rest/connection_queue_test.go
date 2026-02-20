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
 * @brief Tests for the connectionQueue.
 * @file connection_queue_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-19
 */

package rest

import (
	"sync"
	"testing"
	"time"

	"github.com/rossvideo/catena/build/go/protos"
)

func TestNewConnectionQueue(t *testing.T) {
	cq := newConnectionQueue(10)
	if cq == nil {
		t.Fatal("newConnectionQueue returned nil")
	}
	if cq.maxConnections != 10 {
		t.Errorf("expected maxConnections 10, got %d", cq.maxConnections)
	}
	if cq.connections == nil {
		t.Error("connections map should be initialized")
	}
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_RegisterDeregister(t *testing.T) {
	cq := newConnectionQueue(0)

	connID, conn := cq.registerConnection()
	if connID < 0 {
		t.Fatal("RegisterConnection returned negative ID")
	}
	if conn == nil {
		t.Fatal("RegisterConnection returned nil connection")
	}
	if conn.updates == nil {
		t.Error("connection updates channel should be initialized")
	}
	if conn.done == nil {
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

func TestConnectionQueue_DeregisterIdempotent(t *testing.T) {
	cq := newConnectionQueue(0)

	connID, _ := cq.registerConnection()
	cq.deregisterConnection(connID)
	cq.deregisterConnection(connID) // should not panic or decrement below 0
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_MaxConnections(t *testing.T) {
	cq := newConnectionQueue(2)

	id1, conn1 := cq.registerConnection()
	if id1 < 0 || conn1 == nil {
		t.Fatal("first registration should succeed")
	}

	id2, conn2 := cq.registerConnection()
	if id2 < 0 || conn2 == nil {
		t.Fatal("second registration should succeed")
	}

	id3, conn3 := cq.registerConnection()
	if id3 != -1 || conn3 != nil {
		t.Error("third registration should be rejected when max is 2")
	}
	if cq.connectionCount() != 2 {
		t.Errorf("expected 2 connections, got %d", cq.connectionCount())
	}

	// After deregistering one, should be able to register again
	cq.deregisterConnection(id1)
	id4, conn4 := cq.registerConnection()
	if id4 < 0 || conn4 == nil {
		t.Error("registration should succeed after deregistering one")
	}
}

func TestConnectionQueue_UnlimitedConnections(t *testing.T) {
	cq := newConnectionQueue(0)

	ids := make([]int, 100)
	for i := 0; i < 100; i++ {
		id, conn := cq.registerConnection()
		if id < 0 || conn == nil {
			t.Fatalf("registration %d should succeed with unlimited connections", i)
		}
		ids[i] = id
	}
	if cq.connectionCount() != 100 {
		t.Errorf("expected 100 connections, got %d", cq.connectionCount())
	}

	for _, id := range ids {
		cq.deregisterConnection(id)
	}
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after deregistering all, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_SetMaxConnections(t *testing.T) {
	cq := newConnectionQueue(1)

	cq.registerConnection()
	id2, _ := cq.registerConnection()
	if id2 != -1 {
		t.Error("should reject when at max")
	}

	cq.setMaxConnections(2)
	id3, conn3 := cq.registerConnection()
	if id3 < 0 || conn3 == nil {
		t.Error("should succeed after increasing max")
	}
}

func TestConnectionQueue_NotifySetValue(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn1 := cq.registerConnection()
	_, conn2 := cq.registerConnection()

	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}
	cq.notifyUpdate(update)

	checkUpdate := func(name string, conn *connection) {
		select {
		case received := <-conn.updates:
			pv, ok := received.Kind.(*protos.PushUpdates_Value)
			if !ok || pv.Value.GetOid() != "test/param" {
				t.Errorf("%s: expected OID 'test/param'", name)
			}
		case <-time.After(time.Second):
			t.Errorf("%s: timed out waiting for update", name)
		}
	}
	checkUpdate("conn1", conn1)
	checkUpdate("conn2", conn2)
}

func TestConnectionQueue_NotifySetValue_ChannelFull(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn := cq.registerConnection()

	fillUpdate := &protos.PushUpdates{Kind: &protos.PushUpdates_Value{
		Value: &protos.PushUpdates_PushValue{Oid: "fill"},
	}}
	overflowUpdate := &protos.PushUpdates{Kind: &protos.PushUpdates_Value{
		Value: &protos.PushUpdates_PushValue{Oid: "overflow"},
	}}

	for i := 0; i < 100; i++ {
		cq.notifyUpdate(fillUpdate)
	}

	// This should not block (drops the update)
	cq.notifyUpdate(overflowUpdate)

	count := 0
	for len(conn.updates) > 0 {
		u := <-conn.updates
		if pv, ok := u.Kind.(*protos.PushUpdates_Value); ok && pv.Value.GetOid() == "overflow" {
			t.Error("overflow update should have been dropped")
		}
		count++
	}
	if count != 100 {
		t.Errorf("expected 100 buffered updates, got %d", count)
	}
}

func TestConnectionQueue_NotifySetValue_NoConnections(t *testing.T) {
	cq := newConnectionQueue(0)
	cq.notifyUpdate(&protos.PushUpdates{})
}

func TestConnectionQueue_Shutdown(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn1 := cq.registerConnection()
	id2, conn2 := cq.registerConnection()
	_ = id2

	// Simulate goroutines that would deregister on done signal
	var wg sync.WaitGroup
	wg.Add(2)

	go func() {
		defer wg.Done()
		<-conn1.done
		cq.deregisterConnection(conn1.id)
	}()

	go func() {
		defer wg.Done()
		<-conn2.done
		cq.deregisterConnection(conn2.id)
	}()

	done := make(chan struct{})
	go func() {
		cq.shutdown()
		close(done)
	}()

	select {
	case <-done:
		// Shutdown completed successfully
	case <-time.After(2 * time.Second):
		t.Fatal("Shutdown timed out")
	}

	wg.Wait()

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_Shutdown_RejectsNewConnections(t *testing.T) {
	cq := newConnectionQueue(0)

	_, conn := cq.registerConnection()

	// Simulate a goroutine that deregisters on done
	go func() {
		<-conn.done
		cq.deregisterConnection(conn.id)
	}()

	cq.shutdown()

	id, c := cq.registerConnection()
	if id != -1 || c != nil {
		t.Error("should reject new connections after shutdown")
	}
}

func TestConnectionQueue_Shutdown_Empty(t *testing.T) {
	cq := newConnectionQueue(0)

	done := make(chan struct{})
	go func() {
		cq.shutdown()
		close(done)
	}()

	select {
	case <-done:
		// Shutdown on empty queue should return immediately
	case <-time.After(time.Second):
		t.Fatal("Shutdown on empty queue timed out")
	}
}

func TestConnectionQueue_ConcurrentRegisterDeregister(t *testing.T) {
	cq := newConnectionQueue(0)
	var wg sync.WaitGroup

	for i := 0; i < 50; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			id, conn := cq.registerConnection()
			if id < 0 || conn == nil {
				return
			}
			time.Sleep(10 * time.Millisecond)
			cq.deregisterConnection(id)
		}()
	}

	wg.Wait()
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after concurrent test, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_ConnectionCount(t *testing.T) {
	cq := newConnectionQueue(0)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0, got %d", cq.connectionCount())
	}

	id1, _ := cq.registerConnection()
	if cq.connectionCount() != 1 {
		t.Errorf("expected 1, got %d", cq.connectionCount())
	}

	id2, _ := cq.registerConnection()
	if cq.connectionCount() != 2 {
		t.Errorf("expected 2, got %d", cq.connectionCount())
	}

	cq.deregisterConnection(id1)
	if cq.connectionCount() != 1 {
		t.Errorf("expected 1, got %d", cq.connectionCount())
	}

	cq.deregisterConnection(id2)
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0, got %d", cq.connectionCount())
	}
}
