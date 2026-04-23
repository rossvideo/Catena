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
	"bytes"
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

var deviceMarshalOpts = protojson.MarshalOptions{
	UseProtoNames:   true,
	EmitUnpopulated: true,
}

// ToJSON converts CatenaDevice to JSON bytes.
// Uses EmitUnpopulated so proto3 default values (e.g. slot:0) are visible,
// then strips fields with empty values (null, {}, [], "") at all nesting
// levels since these represent unset proto fields, not meaningful data.
// Also strips schema-forbidden fields that have default zero values and
// the "response" field from params (only valid on commands).
func (cd CatenaDevice) ToJSON() ([]byte, error) {
	if cd.device == nil {
		return nil, nil
	}
	data, err := deviceMarshalOpts.Marshal(cd.device)
	if err != nil {
		return nil, err
	}
	data = stripZeroFields(data)
	data = stripResponseFromParams(data)
	return stripEmptyValues(data), nil
}

// stripResponseFromParams removes "response":false from params but preserves
// it in commands. Proto field ordering guarantees params (field 6) appears
// before commands (field 8) in protojson output.
func stripResponseFromParams(data []byte) []byte {
	cmdKey := bytes.Index(data, []byte(`"commands":`))
	if cmdKey == -1 {
		return stripAllExactKV(data, []byte(`"response":false`))
	}
	before := make([]byte, cmdKey)
	copy(before, data[:cmdKey])
	before = stripAllExactKV(before, []byte(`"response":false`))
	return append(before, data[cmdKey:]...)
}

// Fields that the SMPTE schema forbids for most param types when at their
// proto3 default of 0. Stripping is safe because 0 means "unset/unlimited".
var zeroFieldPatterns = [][]byte{
	[]byte(`"precision":0`),
	[]byte(`"max_length":0`),
	[]byte(`"total_length":0`),
}

// stripZeroFields removes specific fields with value 0 that are forbidden
// by the SMPTE schema for most param types.
func stripZeroFields(data []byte) []byte {
	for _, pat := range zeroFieldPatterns {
		data = stripAllExactKV(data, pat)
	}
	return data
}

// stripAllExactKV removes all occurrences of an exact "key":value pattern,
// verifying the value boundary so "precision":0 doesn't match "precision":0.5.
func stripAllExactKV(data []byte, pattern []byte) []byte {
	for {
		newData := stripFirstExactKV(data, pattern)
		if len(newData) == len(data) {
			return data
		}
		data = newData
	}
}

func stripFirstExactKV(data []byte, pattern []byte) []byte {
	searchFrom := 0
	for {
		idx := bytes.Index(data[searchFrom:], pattern)
		if idx == -1 {
			return data
		}
		idx += searchFrom
		end := idx + len(pattern)

		if end < len(data) && data[end] != ',' && data[end] != '}' && data[end] != ']' && data[end] != ' ' {
			searchFrom = end
			continue
		}

		start := idx
		s := start - 1
		for s >= 0 && data[s] == ' ' {
			s--
		}
		if s >= 0 && data[s] == ',' {
			start = s
		} else {
			e := end
			for e < len(data) && data[e] == ' ' {
				e++
			}
			if e < len(data) && data[e] == ',' {
				end = e + 1
				for end < len(data) && data[end] == ' ' {
					end++
				}
			}
		}
		return append(data[:start], data[end:]...)
	}
}

var emptyValuePatterns = [][]byte{
	[]byte(`:null`),
	[]byte(`:{}`),
	[]byte(`:[]`),
	[]byte(`:""`),
}

// stripEmptyValues removes all key:value pairs from compact JSON where the
// value is null, {}, [], or "". Operates directly on the byte array.
// Loops to handle cascading (e.g. stripping all children of an object
// leaves it empty, so it gets stripped too).
func stripEmptyValues(data []byte) []byte {
	for {
		prev := len(data)
		for _, pat := range emptyValuePatterns {
			data = stripAllOfPattern(data, pat)
		}
		if len(data) == prev {
			return data
		}
	}
}

// stripAllOfPattern removes every key:value pair matching the given value
// pattern (e.g. ":null") from compact JSON bytes.
func stripAllOfPattern(data []byte, pattern []byte) []byte {
	for {
		newData := stripFirstKV(data, pattern)
		if len(newData) == len(data) {
			return data
		}
		data = newData
	}
}

// stripFirstKV finds the first occurrence of a value pattern (e.g. ":null")
// that is preceded by a JSON key ("...":), removes the entire "key":value
// pair including the adjacent comma and whitespace, and returns the result.
func stripFirstKV(data []byte, pattern []byte) []byte {
	searchFrom := 0
	for {
		idx := bytes.Index(data[searchFrom:], pattern)
		if idx == -1 {
			return data
		}
		idx += searchFrom

		if idx == 0 || data[idx-1] != '"' {
			searchFrom = idx + len(pattern)
			continue
		}

		end := idx + len(pattern)

		keyStart := idx - 2
		for keyStart >= 0 && data[keyStart] != '"' {
			keyStart--
		}

		start := keyStart

		// Walk backwards past whitespace to find a comma
		s := start - 1
		for s >= 0 && data[s] == ' ' {
			s--
		}
		if s >= 0 && data[s] == ',' {
			start = s
		} else {
			// No preceding comma; consume trailing comma + whitespace
			e := end
			for e < len(data) && data[e] == ' ' {
				e++
			}
			if e < len(data) && data[e] == ',' {
				end = e + 1
				for end < len(data) && data[end] == ' ' {
					end++
				}
			}
		}
		return append(data[:start], data[end:]...)
	}
}
