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
	"fmt"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Heartbeat is a periodic timer that invokes a single callback on each tick.
// It is safe for concurrent use. The callback is set via OnTick and is not
// stored internally — the caller owns handler lifecycle and panic recovery.
type Heartbeat struct {
	mu      sync.Mutex
	running bool
	stopCh  chan struct{}
	onTick  func()
	wg      sync.WaitGroup
}

// NewHeartbeat creates a new Heartbeat instance.
func NewHeartbeat() *Heartbeat {
	return &Heartbeat{}
}

// OnTick sets the callback to be invoked on each heartbeat tick.
// Only one callback is supported; subsequent calls replace the previous one.
func (h *Heartbeat) OnTick(fnTick func()) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.onTick = fnTick
}

// Start begins emitting tick events at the specified interval.
// Returns an error if the interval is invalid (zero or negative).
// Returns nil and logs if already running. Returns nil on success.
func (h *Heartbeat) Start(interval time.Duration) error {
	h.mu.Lock()
	defer h.mu.Unlock()

	if h.running {
		logger.Warning("Heartbeat already running, ignoring Start call")
		return nil
	}
	if interval <= 0 {
		return fmt.Errorf("invalid heartbeat interval: %v", interval)
	}

	h.running = true
	h.stopCh = make(chan struct{})

	h.wg.Add(1)
	go h.run(interval)
	return nil
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
			h.mu.Lock()
			fn := h.onTick
			h.mu.Unlock()
			if fn != nil {
				fn()
			}
		}
	}
}
