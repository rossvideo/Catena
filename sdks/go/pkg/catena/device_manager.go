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
	"errors"
	"strconv"
	"sync"
)

// DeviceManager manages multiple devices keyed by slot.
type DeviceManager struct {
	mu      sync.RWMutex
	devices map[int]Device
}

func NewDeviceManager() *DeviceManager {
	return &DeviceManager{
		devices: make(map[int]Device),
	}
}

func (m *DeviceManager) Register(slot int, d Device) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.devices[slot] = d
}

func (m *DeviceManager) Get(slot int) (Device, bool) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	d, ok := m.devices[slot]
	return d, ok
}

func (m *DeviceManager) MustGet(slot int) (Device, error) {
	if d, ok := m.Get(slot); ok {
		return d, nil
	}
	return nil, errors.New("device not found for slot: " + strconv.Itoa(slot))
}

func (m *DeviceManager) ListSlots() []int {
	m.mu.RLock()
	defer m.mu.RUnlock()
	out := make([]int, 0, len(m.devices))
	for k := range m.devices {
		out = append(out, k)
	}
	return out
}
