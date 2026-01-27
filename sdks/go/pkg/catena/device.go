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
	"encoding/json"
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/build/go/protos"
)

// CatenaDevice wraps protos.Device for device model handling
type CatenaDevice struct {
	device   *protos.Device
	jsonData []byte // Cached JSON representation
}

// NewCatenaDevice creates a CatenaDevice from a protos.Device
func NewCatenaDevice(newDevice *protos.Device) CatenaDevice {
	return CatenaDevice{device: newDevice}
}

// ToCatenaDevice converts a Go map/struct to CatenaDevice
// This allows developers to work with native Go types and convert to the protobuf format
func ToCatenaDevice(m map[string]any) (CatenaDevice, error) {
	device, jsonData, err := toProtoDevice(m)
	if err != nil {
		return CatenaDevice{jsonData: nil}, fmt.Errorf("ToCatenaDevice: %w", err)
	}
	return CatenaDevice{device: device, jsonData: jsonData}, nil
}

// toProtoDevice converts native Go types to protos.Device
// For complex nested types (params, constraints, etc.), uses JSON marshaling
// to leverage protojson's automatic conversion and validation
// Returns the device, cached JSON data, and any error
func toProtoDevice(m map[string]any) (*protos.Device, []byte, error) {
	// For complex types, use JSON marshaling to handle nested structures
	jsonData, err := json.Marshal(m)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to marshal map to JSON: %w", err)
	}

	device := &protos.Device{}
	if err := protojson.Unmarshal(jsonData, device); err != nil {
		return nil, nil, fmt.Errorf("failed to unmarshal JSON to Device proto: %w", err)
	}

	return device, jsonData, nil
}

// GetProtoDevice returns the underlying protos.Device
func (cd CatenaDevice) GetProtoDevice() *protos.Device {
	return cd.device
}

// GetSlot returns the device slot
func (cd CatenaDevice) GetSlot() uint32 {
	if cd.device == nil {
		return 0
	}
	return cd.device.Slot
}

// GetDetailLevel returns the detail level
func (cd CatenaDevice) GetDetailLevel() protos.Device_DetailLevel {
	if cd.device == nil {
		return protos.Device_UNSET
	}
	return cd.device.DetailLevel
}

// IsMultiSetEnabled returns whether multi-set is enabled
func (cd CatenaDevice) IsMultiSetEnabled() bool {
	if cd.device == nil {
		return false
	}
	return cd.device.MultiSetEnabled
}

// SupportsSubscriptions returns whether subscriptions are supported
func (cd CatenaDevice) SupportsSubscriptions() bool {
	if cd.device == nil {
		return false
	}
	return cd.device.Subscriptions
}

// GetAccessScopes returns the device access scopes
func (cd CatenaDevice) GetAccessScopes() []string {
	if cd.device == nil {
		return nil
	}
	return cd.device.AccessScopes
}

// GetDefaultScope returns the default access scope
func (cd CatenaDevice) GetDefaultScope() string {
	if cd.device == nil {
		return ""
	}
	return cd.device.DefaultScope
}

// ToJSON converts CatenaDevice to JSON bytes using protojson
// Uses cached JSON if available, otherwise generates new JSON
func (cd CatenaDevice) ToJSON() ([]byte, error) {
	if cd.device == nil {
		return nil, nil
	}

	// Return cached JSON if available
	if cd.jsonData != nil {
		return cd.jsonData, nil
	}

	// Generate JSON from protobuf if not cached
	return (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(cd.device)
}
