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
 * @brief Device handling for the Catena SDK.
 * @file device.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-16
 */

package catena

import (
	"encoding/json"
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// DetailLevel represents how much of the device model to deliver
// Mirrors protos.Device_DetailLevel for convenience
type DetailLevel = protos.Device_DetailLevel

// DetailLevel constants matching the proto enum
const (
	DetailLevelFull          DetailLevel = protos.Device_FULL
	DetailLevelSubscriptions DetailLevel = protos.Device_SUBSCRIPTIONS
	DetailLevelMinimal       DetailLevel = protos.Device_MINIMAL
	DetailLevelCommands      DetailLevel = protos.Device_COMMANDS
	DetailLevelNone          DetailLevel = protos.Device_NONE
	DetailLevelUnset         DetailLevel = protos.Device_UNSET
)

// Device wraps protos.Device for device model handling
type Device struct {
	device *protos.Device
}

// ToDevice converts a Go map/struct to Device
// This allows developers to work with native Go types and convert to the protobuf format
func ToDevice(m map[string]any) (Device, error) {
	device, err := toProtoDevice(m)
	if err != nil {
		return Device{}, fmt.Errorf("ToDevice: %w", err)
	}
	return Device{device: device}, nil
}

// toProtoDevice converts native Go types to protos.Device
// For complex nested types (params, constraints, etc.), uses JSON marshaling
// to leverage protojson's automatic conversion and validation
func toProtoDevice(m map[string]any) (*protos.Device, error) {
	jsonData, err := json.Marshal(m)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal map to JSON: %w", err)
	}

	device := &protos.Device{}
	if err := protojson.Unmarshal(jsonData, device); err != nil {
		return nil, fmt.Errorf("failed to unmarshal JSON to Device proto: %w", err)
	}

	// Reserved software-update commands are auto-injected into every device so
	// upload_update / apply_update are always present and cannot be removed.
	if err := injectReservedCommands(device); err != nil {
		return nil, fmt.Errorf("failed to inject reserved commands: %w", err)
	}

	return device, nil
}

// GetProtoDevice returns the underlying protos.Device
func (cd Device) GetProtoDevice() *protos.Device {
	return cd.device
}
