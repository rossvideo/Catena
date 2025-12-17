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
