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
 * @brief Tests for the ConnectionQueue.
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
)

func TestNewConnectionQueue(t *testing.T) {
	cq := NewConnectionQueue(10)
	if cq == nil {
		t.Fatal("NewConnectionQueue returned nil")
	}
	if cq.maxConnections != 10 {
		t.Errorf("expected maxConnections 10, got %d", cq.maxConnections)
	}
	if cq.connections == nil {
		t.Error("connections map should be initialized")
	}
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_RegisterDeregister(t *testing.T) {
	cq := NewConnectionQueue(0)

	connID, conn := cq.RegisterConnection()
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
	if cq.ConnectionCount() != 1 {
		t.Errorf("expected 1 connection, got %d", cq.ConnectionCount())
	}

	cq.DeregisterConnection(connID)
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after deregister, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_DeregisterIdempotent(t *testing.T) {
	cq := NewConnectionQueue(0)

	connID, _ := cq.RegisterConnection()
	cq.DeregisterConnection(connID)
	cq.DeregisterConnection(connID) // should not panic or decrement below 0
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_MaxConnections(t *testing.T) {
	cq := NewConnectionQueue(2)

	id1, conn1 := cq.RegisterConnection()
	if id1 < 0 || conn1 == nil {
		t.Fatal("first registration should succeed")
	}

	id2, conn2 := cq.RegisterConnection()
	if id2 < 0 || conn2 == nil {
		t.Fatal("second registration should succeed")
	}

	id3, conn3 := cq.RegisterConnection()
	if id3 != -1 || conn3 != nil {
		t.Error("third registration should be rejected when max is 2")
	}
	if cq.ConnectionCount() != 2 {
		t.Errorf("expected 2 connections, got %d", cq.ConnectionCount())
	}

	// After deregistering one, should be able to register again
	cq.DeregisterConnection(id1)
	id4, conn4 := cq.RegisterConnection()
	if id4 < 0 || conn4 == nil {
		t.Error("registration should succeed after deregistering one")
	}
}

func TestConnectionQueue_UnlimitedConnections(t *testing.T) {
	cq := NewConnectionQueue(0)

	ids := make([]int, 100)
	for i := 0; i < 100; i++ {
		id, conn := cq.RegisterConnection()
		if id < 0 || conn == nil {
			t.Fatalf("registration %d should succeed with unlimited connections", i)
		}
		ids[i] = id
	}
	if cq.ConnectionCount() != 100 {
		t.Errorf("expected 100 connections, got %d", cq.ConnectionCount())
	}

	for _, id := range ids {
		cq.DeregisterConnection(id)
	}
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after deregistering all, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_SetMaxConnections(t *testing.T) {
	cq := NewConnectionQueue(1)

	cq.RegisterConnection()
	id2, _ := cq.RegisterConnection()
	if id2 != -1 {
		t.Error("should reject when at max")
	}

	cq.SetMaxConnections(2)
	id3, conn3 := cq.RegisterConnection()
	if id3 < 0 || conn3 == nil {
		t.Error("should succeed after increasing max")
	}
}

func TestConnectionQueue_NotifySetValue(t *testing.T) {
	cq := NewConnectionQueue(0)

	_, conn1 := cq.RegisterConnection()
	_, conn2 := cq.RegisterConnection()

	update := PushUpdate{Slot: 0, OID: "test/param", Value: int32(42)}
	cq.NotifySetValue(update)

	select {
	case received := <-conn1.updates:
		if received.OID != "test/param" {
			t.Errorf("conn1: expected OID 'test/param', got %s", received.OID)
		}
	case <-time.After(time.Second):
		t.Error("conn1: timed out waiting for update")
	}

	select {
	case received := <-conn2.updates:
		if received.OID != "test/param" {
			t.Errorf("conn2: expected OID 'test/param', got %s", received.OID)
		}
	case <-time.After(time.Second):
		t.Error("conn2: timed out waiting for update")
	}
}

func TestConnectionQueue_NotifySetValue_ChannelFull(t *testing.T) {
	cq := NewConnectionQueue(0)

	_, conn := cq.RegisterConnection()

	// Fill the channel buffer (100)
	for i := 0; i < 100; i++ {
		cq.NotifySetValue(PushUpdate{OID: "fill"})
	}

	// This should not block (drops the update)
	cq.NotifySetValue(PushUpdate{OID: "overflow"})

	// Drain and verify no "overflow" OID made it through
	count := 0
	for len(conn.updates) > 0 {
		u := <-conn.updates
		if u.OID == "overflow" {
			t.Error("overflow update should have been dropped")
		}
		count++
	}
	if count != 100 {
		t.Errorf("expected 100 buffered updates, got %d", count)
	}
}

func TestConnectionQueue_NotifySetValue_NoConnections(t *testing.T) {
	cq := NewConnectionQueue(0)
	// Should not panic with no connections
	cq.NotifySetValue(PushUpdate{OID: "test"})
}

func TestConnectionQueue_Shutdown(t *testing.T) {
	cq := NewConnectionQueue(0)

	_, conn1 := cq.RegisterConnection()
	id2, conn2 := cq.RegisterConnection()
	_ = id2

	// Simulate goroutines that would deregister on done signal
	var wg sync.WaitGroup
	wg.Add(2)

	go func() {
		defer wg.Done()
		<-conn1.done
		cq.DeregisterConnection(conn1.id)
	}()

	go func() {
		defer wg.Done()
		<-conn2.done
		cq.DeregisterConnection(conn2.id)
	}()

	done := make(chan struct{})
	go func() {
		cq.Shutdown()
		close(done)
	}()

	select {
	case <-done:
		// Shutdown completed successfully
	case <-time.After(2 * time.Second):
		t.Fatal("Shutdown timed out")
	}

	wg.Wait()

	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_Shutdown_RejectsNewConnections(t *testing.T) {
	cq := NewConnectionQueue(0)

	_, conn := cq.RegisterConnection()

	// Simulate a goroutine that deregisters on done
	go func() {
		<-conn.done
		cq.DeregisterConnection(conn.id)
	}()

	cq.Shutdown()

	id, c := cq.RegisterConnection()
	if id != -1 || c != nil {
		t.Error("should reject new connections after shutdown")
	}
}

func TestConnectionQueue_Shutdown_Empty(t *testing.T) {
	cq := NewConnectionQueue(0)

	done := make(chan struct{})
	go func() {
		cq.Shutdown()
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
	cq := NewConnectionQueue(0)
	var wg sync.WaitGroup

	for i := 0; i < 50; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			id, conn := cq.RegisterConnection()
			if id < 0 || conn == nil {
				return
			}
			time.Sleep(10 * time.Millisecond)
			cq.DeregisterConnection(id)
		}()
	}

	wg.Wait()
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0 connections after concurrent test, got %d", cq.ConnectionCount())
	}
}

func TestConnectionQueue_ConnectionCount(t *testing.T) {
	cq := NewConnectionQueue(0)

	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0, got %d", cq.ConnectionCount())
	}

	id1, _ := cq.RegisterConnection()
	if cq.ConnectionCount() != 1 {
		t.Errorf("expected 1, got %d", cq.ConnectionCount())
	}

	id2, _ := cq.RegisterConnection()
	if cq.ConnectionCount() != 2 {
		t.Errorf("expected 2, got %d", cq.ConnectionCount())
	}

	cq.DeregisterConnection(id1)
	if cq.ConnectionCount() != 1 {
		t.Errorf("expected 1, got %d", cq.ConnectionCount())
	}

	cq.DeregisterConnection(id2)
	if cq.ConnectionCount() != 0 {
		t.Errorf("expected 0, got %d", cq.ConnectionCount())
	}
}
