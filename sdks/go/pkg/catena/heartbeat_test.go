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
	"strings"
	"sync/atomic"
	"testing"
	"time"
)

func TestNewHeartbeat(t *testing.T) {
	hb := NewHeartbeat()
	if hb == nil {
		t.Fatal("NewHeartbeat() returned nil")
	}
	if hb.IsRunning() {
		t.Error("new heartbeat should not be running")
	}
}

func TestHeartbeat_StartStop(t *testing.T) {
	hb := NewHeartbeat()

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	if !hb.IsRunning() {
		t.Error("heartbeat should be running after Start")
	}

	hb.Stop()
	if hb.IsRunning() {
		t.Error("heartbeat should not be running after Stop")
	}
}

func TestHeartbeat_DoubleStart(t *testing.T) {
	hb := NewHeartbeat()

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Error("double Start should return nil, not error")
	}
	if !hb.IsRunning() {
		t.Error("heartbeat should still be running")
	}

	hb.Stop()
}

func TestHeartbeat_DoubleStop(t *testing.T) {
	hb := NewHeartbeat()

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	hb.Stop()
	hb.Stop() // should be no-op, not panic
	if hb.IsRunning() {
		t.Error("heartbeat should not be running")
	}
}

func TestHeartbeat_StopWhenNotRunning(t *testing.T) {
	hb := NewHeartbeat()
	hb.Stop() // should be no-op, not panic
	if hb.IsRunning() {
		t.Error("heartbeat should not be running")
	}
}

func TestHeartbeat_ZeroInterval(t *testing.T) {
	hb := NewHeartbeat()
	err := hb.Start(0)
	if err == nil {
		t.Error("Start with zero interval should return an error")
	} else if !strings.Contains(err.Error(), "invalid heartbeat interval") {
		t.Errorf("unexpected error message: %v", err)
	}
	if hb.IsRunning() {
		t.Error("heartbeat should not start with zero interval")
	}
}

func TestHeartbeat_NegativeInterval(t *testing.T) {
	hb := NewHeartbeat()
	err := hb.Start(-1 * time.Second)
	if err == nil {
		t.Error("Start with negative interval should return an error")
	} else if !strings.Contains(err.Error(), "invalid heartbeat interval") {
		t.Errorf("unexpected error message: %v", err)
	}
	if hb.IsRunning() {
		t.Error("heartbeat should not start with negative interval")
	}
}

func TestHeartbeat_OnTick(t *testing.T) {
	hb := NewHeartbeat()

	var tickCount atomic.Int32
	hb.OnTick(func() {
		tickCount.Add(1)
	})

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}

	// Wait for a few ticks
	time.Sleep(55 * time.Millisecond)
	hb.Stop()

	count := tickCount.Load()
	if count < 3 {
		t.Errorf("expected at least 3 ticks, got %d", count)
	}
}

func TestHeartbeat_MultipleHandlers(t *testing.T) {
	hb := NewHeartbeat()

	var count1, count2, count3 atomic.Int32

	// Also verifies method chaining returns same instance
	result := hb.OnTick(func() {
		count1.Add(1)
	}).OnTick(func() {
		count2.Add(1)
	}).OnTick(func() {
		count3.Add(1)
	})

	if result != hb {
		t.Error("OnTick should return the same Heartbeat for chaining")
	}

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	time.Sleep(35 * time.Millisecond)
	hb.Stop()

	c1, c2, c3 := count1.Load(), count2.Load(), count3.Load()
	if c1 < 2 || c2 < 2 || c3 < 2 {
		t.Errorf("expected at least 2 ticks each, got %d, %d, %d", c1, c2, c3)
	}
	if c1 != c2 || c2 != c3 {
		t.Errorf("handlers should have same tick count, got %d, %d, %d", c1, c2, c3)
	}
}

func TestHeartbeat_HandlerPanicRecovery(t *testing.T) {
	hb := NewHeartbeat()

	var tickCount atomic.Int32

	// First handler panics
	hb.OnTick(func() {
		panic("test panic")
	})

	// Second handler should still be called
	hb.OnTick(func() {
		tickCount.Add(1)
	})

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	time.Sleep(35 * time.Millisecond)
	hb.Stop()

	count := tickCount.Load()
	if count < 2 {
		t.Errorf("expected at least 2 ticks despite panic, got %d", count)
	}
}

func TestHeartbeat_NoTicksAfterStop(t *testing.T) {
	hb := NewHeartbeat()

	var tickCount atomic.Int32
	hb.OnTick(func() {
		tickCount.Add(1)
	})

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	time.Sleep(25 * time.Millisecond)
	hb.Stop()

	countAtStop := tickCount.Load()
	time.Sleep(30 * time.Millisecond)
	countAfter := tickCount.Load()

	if countAtStop != countAfter {
		t.Errorf("ticks continued after stop: %d vs %d", countAtStop, countAfter)
	}
}

func TestHeartbeat_RestartAfterStop(t *testing.T) {
	hb := NewHeartbeat()

	var tickCount atomic.Int32
	hb.OnTick(func() {
		tickCount.Add(1)
	})

	// First run
	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	time.Sleep(25 * time.Millisecond)
	hb.Stop()
	firstRun := tickCount.Load()

	// Second run
	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Restart failed: %v", err)
	}
	time.Sleep(25 * time.Millisecond)
	hb.Stop()
	secondRun := tickCount.Load()

	if secondRun <= firstRun {
		t.Errorf("heartbeat should tick after restart: first=%d, total=%d", firstRun, secondRun)
	}
}

func TestHeartbeat_OnTickWhileRunning(t *testing.T) {
	hb := NewHeartbeat()

	var count1, count2 atomic.Int32
	hb.OnTick(func() {
		count1.Add(1)
	})

	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	time.Sleep(25 * time.Millisecond)

	// Register a new handler while already running
	hb.OnTick(func() {
		count2.Add(1)
	})

	time.Sleep(35 * time.Millisecond)
	hb.Stop()

	c1, c2 := count1.Load(), count2.Load()
	if c1 < 4 {
		t.Errorf("first handler should have ticked throughout, got %d", c1)
	}
	if c2 < 2 {
		t.Errorf("handler added while running should have ticked, got %d", c2)
	}
}

func TestHeartbeat_NoHandlers(t *testing.T) {
	// Starting with no handlers should not panic
	hb := NewHeartbeat()
	if err := hb.Start(10 * time.Millisecond); err != nil {
		t.Fatalf("Start failed: %v", err)
	}
	time.Sleep(25 * time.Millisecond)
	hb.Stop()
	// If we reach here without panic, test passes
}

func TestHeartbeat_ConcurrentStartStop(t *testing.T) {
	// Exercise the race detector — concurrent Start/Stop should not panic or deadlock
	hb := NewHeartbeat()
	var tickCount atomic.Int32
	hb.OnTick(func() {
		tickCount.Add(1)
	})

	done := make(chan struct{})
	go func() {
		defer close(done)
		for i := 0; i < 50; i++ {
			_ = hb.Start(1 * time.Millisecond)
			time.Sleep(2 * time.Millisecond)
			hb.Stop()
		}
	}()

	// Concurrently poke IsRunning
	for i := 0; i < 100; i++ {
		hb.IsRunning()
		time.Sleep(1 * time.Millisecond)
	}

	<-done
	// Success if no deadlock or race detected
}
