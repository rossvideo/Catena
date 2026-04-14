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

// CatenaDevice wraps protos.Device for device model handling
type CatenaDevice struct {
	device *protos.Device
}

// ToCatenaDevice converts a Go map/struct to CatenaDevice
// This allows developers to work with native Go types and convert to the protobuf format
func ToCatenaDevice(m map[string]any) (CatenaDevice, error) {
	device, err := toProtoDevice(m)
	if err != nil {
		return CatenaDevice{}, fmt.Errorf("ToCatenaDevice: %w", err)
	}
	return CatenaDevice{device: device}, nil
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

	return device, nil
}

// GetProtoDevice returns the underlying protos.Device
func (cd CatenaDevice) GetProtoDevice() *protos.Device {
	return cd.device
}

// ToJSON converts CatenaDevice to JSON bytes using protojson.
// Post-processes the JSON to inject schema-required fields that proto3
// omits at their default values (slot, command response, menu_group order,
// constraint range bounds). Preserves protojson's original field ordering.
func (cd CatenaDevice) ToJSON() ([]byte, error) {
	if cd.device == nil {
		return nil, nil
	}

	b, err := (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(cd.device)
	if err != nil {
		return nil, err
	}
	return postProcessDeviceJSON(b, cd.device.GetSlot())
}

// postProcessDeviceJSON ensures all SMPTE-schema-required fields are present
// in the Device JSON even when proto3 omits them at their default values.
// Preserves the original field ordering from protojson.
func postProcessDeviceJSON(data []byte, slot uint32) ([]byte, error) {
	obj, err := parseOrderedJSON(data)
	if err != nil {
		return nil, err
	}

	obj.setFirst("slot", slot)

	if v, ok := obj.get("commands"); ok {
		if cmds, ok := v.(*orderedObj); ok {
			for _, p := range cmds.pairs {
				if cmd, ok := p.val.(*orderedObj); ok {
					cmd.setDefault("response", false)
				}
			}
		}
	}

	if v, ok := obj.get("menu_groups"); ok {
		if mgs, ok := v.(*orderedObj); ok {
			for _, p := range mgs.pairs {
				if mg, ok := p.val.(*orderedObj); ok {
					mg.setDefault("order", int64(0))
				}
			}
		}
	}

	if v, ok := obj.get("constraints"); ok {
		if cs, ok := v.(*orderedObj); ok {
			for _, p := range cs.pairs {
				if c, ok := p.val.(*orderedObj); ok {
					if r, ok := c.get("int32_range"); ok {
						if rObj, ok := r.(*orderedObj); ok {
							rObj.setDefault("min_value", int64(0))
							rObj.setDefault("max_value", int64(0))
						}
					}
					if r, ok := c.get("float_range"); ok {
						if rObj, ok := r.(*orderedObj); ok {
							rObj.setDefault("min_value", 0.0)
							rObj.setDefault("max_value", 0.0)
						}
					}
				}
			}
		}
	}

	return obj.MarshalJSON()
}
