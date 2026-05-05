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
	"log/slog"
	"sync"
	"time"
)

//server.go has an api to control its instance of heartbeat
//for more manual control, the user can interact with it directly via GetHeartbeat() and call Start/Stop/OnTick as needed
/*
hb := catena.NewHeartbeat()
hb.OnTick(func() {
    srv.BroadcastUpdate(0, "/product/version", "1.0.0")
})
srv.SetHeartbeat(hb)
hb.Start(5 * time.Second)
// ... later ...
hb.Stop()
*/

// HeartbeatHandler is called on each heartbeat tick.
// Handlers should return quickly to avoid blocking the heartbeat goroutine.
type HeartbeatHandler func()

// Heartbeat emits periodic tick events at a configurable interval.
// It is safe for concurrent use; multiple handlers can be registered
// and will be called sequentially on each tick.
type Heartbeat struct {
	mu       sync.Mutex
	running  bool
	stopCh   chan struct{}
	handlers []HeartbeatHandler
	wg       sync.WaitGroup
}

// NewHeartbeat creates a new Heartbeat instance.
func NewHeartbeat() *Heartbeat {
	return &Heartbeat{
		handlers: make([]HeartbeatHandler, 0),
	}
}

// OnTick registers a handler to be called on each heartbeat tick.
// Multiple handlers can be registered and will be called in order.
// Handlers should not block for long periods.
// Returns the Heartbeat for method chaining.
func (h *Heartbeat) OnTick(handler HeartbeatHandler) *Heartbeat {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.handlers = append(h.handlers, handler)
	return h
}

// Start begins emitting tick events at the specified interval.
// If the heartbeat is already running, this is a no-op.
// The interval must be positive; zero or negative values are ignored.
func (h *Heartbeat) Start(interval time.Duration) {
	h.mu.Lock()
	defer h.mu.Unlock()

	if h.running {
		return
	}
	if interval <= 0 {
		return
	}

	h.running = true
	h.stopCh = make(chan struct{})

	h.wg.Add(1)
	go h.run(interval)
}

// Stop halts the heartbeat. No further tick events will be emitted.
// Blocks until the heartbeat goroutine has exited.
// Safe to call multiple times or when not running.
func (h *Heartbeat) Stop() {
	h.mu.Lock()
	if !h.running {
		h.mu.Unlock()
		return
	}
	h.running = false
	close(h.stopCh)
	h.mu.Unlock()

	// Wait for goroutine to exit
	h.wg.Wait()
}

// IsRunning returns true if the heartbeat is currently active.
func (h *Heartbeat) IsRunning() bool {
	h.mu.Lock()
	defer h.mu.Unlock()
	return h.running
}

// run is the internal goroutine that emits tick events.
func (h *Heartbeat) run(interval time.Duration) {
	defer h.wg.Done()

	ticker := time.NewTicker(interval)
	defer ticker.Stop()

	for {
		select {
		case <-h.stopCh:
			return
		case <-ticker.C:
			h.emitTick()
		}
	}
}

// emitTick calls all registered handlers, recovering from panics.
func (h *Heartbeat) emitTick() {
	h.mu.Lock()
	handlers := make([]HeartbeatHandler, len(h.handlers))
	copy(handlers, h.handlers)
	h.mu.Unlock()

	for _, handler := range handlers {
		func() {
			defer func() {
				if r := recover(); r != nil {
					slog.Error("panic in heartbeat handler", "error", r)
				}
			}()
			handler()
		}()
	}
}
