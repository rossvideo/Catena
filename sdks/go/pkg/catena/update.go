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
 * @brief Reserved software-update commands for the Catena SDK.
 * @file update.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-06-02
 */

package catena

import (
	"encoding/json"
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Reserved command OIDs owned by the SDK for software updates. They are
// auto-injected into every device model and cannot be defined by a device.
const (
	ReservedOidUploadUpdate = "upload_update"
	ReservedOidApplyUpdate  = "apply_update"
)

// reservedUpdateCommands holds the descriptor layout for the reserved
// software-update commands. The data field uses ParamType "DATA" (not the
// non-existent "DATA_PAYLOAD") so protojson can unmarshal it.
var reservedUpdateCommands = map[string]any{
	ReservedOidUploadUpdate: map[string]any{
		"name": map[string]any{
			"display_strings": map[string]string{"en": "Update Software"},
		},
		"type": "STRUCT",
		"params": map[string]any{
			"metadata": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{"en": "Metadata"},
				},
				"type": "STRUCT_ARRAY",
				"params": map[string]any{
					"key": map[string]any{
						"type": "STRING",
						"name": map[string]any{
							"display_strings": map[string]string{"en": "Key"},
						},
					},
					"value": map[string]any{
						"type": "STRING",
						"name": map[string]any{
							"display_strings": map[string]string{"en": "Value"},
						},
					},
				},
			},
			"data": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{"en": "Update Data"},
				},
				"type": "DATA",
			},
		},
		"response": true,
	},
	ReservedOidApplyUpdate: map[string]any{
		"name": map[string]any{
			"display_strings": map[string]string{"en": "Update Software"},
		},
		"type": "STRUCT",
		"params": map[string]any{
			"id": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{"en": "Update ID"},
				},
				"type": "INT32",
			},
		},
		"response": true,
	},
}

// injectReservedCommands adds the reserved software-update commands to the
// device's command map. A device may not define these OIDs itself; doing so is
// an error. The reserved commands are otherwise present in every device by
// default.
func injectReservedCommands(device *protos.Device) error {
	if device.Commands == nil {
		device.Commands = make(map[string]*protos.Param)
	}
	for oid := range reservedUpdateCommands {
		if _, exists := device.Commands[oid]; exists {
			return fmt.Errorf("command %q is reserved by the SDK and cannot be defined by a device", oid)
		}
	}
	for oid, descriptor := range reservedUpdateCommands {
		param, err := descriptorToParam(descriptor)
		if err != nil {
			return fmt.Errorf("inject reserved command %q: %w", oid, err)
		}
		device.Commands[oid] = param
	}
	return nil
}

// descriptorToParam converts a Go descriptor map into a protos.Param using the
// same JSON path that toProtoDevice relies on.
func descriptorToParam(descriptor any) (*protos.Param, error) {
	jsonData, err := json.Marshal(descriptor)
	if err != nil {
		return nil, fmt.Errorf("marshal descriptor: %w", err)
	}
	param := &protos.Param{}
	if err := protojson.Unmarshal(jsonData, param); err != nil {
		return nil, fmt.Errorf("unmarshal descriptor: %w", err)
	}
	return param, nil
}
