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
	"os"
	"path/filepath"
	"strings"
	"sync"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Reserved command OIDs owned by the SDK for software updates. They are
// auto-injected into every device model and intercepted by the built-in
// update manager, so applications do not register handlers for them.
const (
	ReservedOidUploadUpdate = "upload_update"
	ReservedOidApplyUpdate  = "apply_update"
)

// Update describes an uploaded software update awaiting application.
type Update struct {
	ID       int32
	Metadata map[string]string
	DataPath string // local path to the uploaded bytes (empty when URL-based)
	URL      string // external URL reference (empty when bytes were uploaded)
}

// ApplyUpdateFunc applies a previously uploaded update. The default reference
// implementation only validates that the uploaded data is reachable; real
// bootloader/firmware apply logic is device-specific and should be supplied
// via ServerOptions.ApplyUpdateFunc.
type ApplyUpdateFunc func(update *Update) error

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
// device's command map, creating the map when necessary. The reserved
// commands always take precedence so they cannot be removed by applications.
func injectReservedCommands(device *protos.Device) error {
	if device.Commands == nil {
		device.Commands = make(map[string]*protos.Param)
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

// normalizeReservedOid strips a leading slash so REST (/upload_update) and
// gRPC (upload_update) OID forms compare equal.
func normalizeReservedOid(fqoid string) string {
	return strings.TrimPrefix(fqoid, "/")
}

// isReservedUpdateOid reports whether fqoid targets one of the reserved
// software-update commands.
func isReservedUpdateOid(fqoid string) bool {
	switch normalizeReservedOid(fqoid) {
	case ReservedOidUploadUpdate, ReservedOidApplyUpdate:
		return true
	default:
		return false
	}
}

// updateManager owns the lifecycle of uploaded software updates.
type updateManager struct {
	mu      sync.Mutex
	dir     string
	nextID  int32
	updates map[int32]*Update
	applyFn ApplyUpdateFunc
}

func newUpdateManager(dir string, applyFn ApplyUpdateFunc) *updateManager {
	if dir == "" {
		dir = filepath.Join(os.TempDir(), "catena-updates")
	}
	if applyFn == nil {
		applyFn = defaultApplyUpdate
	}
	return &updateManager{
		dir:     dir,
		updates: make(map[int32]*Update),
		applyFn: applyFn,
	}
}

// defaultApplyUpdate is the reference implementation: it validates that the
// uploaded update data is reachable. Device-specific apply logic should be
// supplied via ServerOptions.ApplyUpdateFunc.
func defaultApplyUpdate(update *Update) error {
	if update == nil {
		return fmt.Errorf("nil update")
	}
	if update.DataPath != "" {
		if _, err := os.Stat(update.DataPath); err != nil {
			return fmt.Errorf("update data not found: %w", err)
		}
		return nil
	}
	if update.URL != "" {
		return nil
	}
	return fmt.Errorf("update %d has no data", update.ID)
}

// execute dispatches a reserved update command to its handler.
func (m *updateManager) execute(fqoid string, payload any) (CommandResult, StatusResult) {
	switch normalizeReservedOid(fqoid) {
	case ReservedOidUploadUpdate:
		return m.handleUpload(payload)
	case ReservedOidApplyUpdate:
		return m.handleApply(payload)
	default:
		return CommandError(NOT_FOUND, "unknown reserved command: "+fqoid)
	}
}

// handleUpload stores the uploaded update data and metadata, returning the
// newly assigned update ID so apply_update can reference it.
func (m *updateManager) handleUpload(payload any) (CommandResult, StatusResult) {
	fields, ok := payload.(map[string]any)
	if !ok {
		return CommandExceptionResult("InvalidPayload", "upload_update expects a STRUCT payload",
			NewPolyglotText("en", "Invalid update payload"))
	}

	data, ok := fields["data"].(DataPayload)
	if !ok {
		return CommandExceptionResult("InvalidPayload", "upload_update requires a 'data' DATA field",
			NewPolyglotText("en", "Missing update data"))
	}
	if len(data.Payload) == 0 && data.Url == "" {
		return CommandExceptionResult("InvalidPayload", "upload_update 'data' has neither payload nor url",
			NewPolyglotText("en", "Empty update data"))
	}

	metadata := parseUpdateMetadata(fields["metadata"])

	m.mu.Lock()
	defer m.mu.Unlock()

	m.nextID++
	id := m.nextID
	update := &Update{ID: id, Metadata: metadata}

	if len(data.Payload) > 0 {
		if err := os.MkdirAll(m.dir, 0o755); err != nil {
			m.nextID--
			return CommandExceptionResult("StorageError", err.Error(),
				NewPolyglotText("en", "Failed to store update"))
		}
		dataPath := filepath.Join(m.dir, fmt.Sprintf("%d.bin", id))
		if err := os.WriteFile(dataPath, data.Payload, 0o644); err != nil {
			m.nextID--
			return CommandExceptionResult("StorageError", err.Error(),
				NewPolyglotText("en", "Failed to store update"))
		}
		update.DataPath = dataPath
	} else {
		update.URL = data.Url
	}

	m.updates[id] = update
	logger.Info("upload_update stored", "id", id, "path", update.DataPath, "url", update.URL, "bytes", len(data.Payload))

	val, res := ToValue(id)
	if res.Code != OK {
		return CommandError(res.Code, res.Error)
	}
	return CommandReply(val)
}

// handleApply applies a previously uploaded update identified by its ID.
func (m *updateManager) handleApply(payload any) (CommandResult, StatusResult) {
	fields, ok := payload.(map[string]any)
	if !ok {
		return CommandExceptionResult("InvalidPayload", "apply_update expects a STRUCT payload",
			NewPolyglotText("en", "Invalid update payload"))
	}

	id, ok := toInt32(fields["id"])
	if !ok {
		return CommandExceptionResult("InvalidPayload", "apply_update requires an 'id' INT32 field",
			NewPolyglotText("en", "Missing update id"))
	}

	m.mu.Lock()
	update, found := m.updates[id]
	applyFn := m.applyFn
	m.mu.Unlock()

	if !found {
		return CommandExceptionResult("UnknownUpdate", fmt.Sprintf("no update with id %d", id),
			NewPolyglotText("en", "Unknown update id"))
	}

	if err := applyFn(update); err != nil {
		return CommandExceptionResult("UpdateFailed", err.Error(),
			NewPolyglotText("en", "Update failed"))
	}

	logger.Info("apply_update applied", "id", id)

	// Single-response progress: report completion. True streamed 0-100 progress
	// is a follow-up that requires a streaming command handler + transport.
	val, res := ToValue(int32(100))
	if res.Code != OK {
		return CommandError(res.Code, res.Error)
	}
	return CommandReply(val)
}

// parseUpdateMetadata flattens the STRUCT_ARRAY of {key, value} entries into a
// plain string map. Unrecognized shapes yield an empty map.
func parseUpdateMetadata(v any) map[string]string {
	out := make(map[string]string)
	arr, ok := v.([]map[string]any)
	if !ok {
		return out
	}
	for _, entry := range arr {
		key, _ := entry["key"].(string)
		value, _ := entry["value"].(string)
		if key != "" {
			out[key] = value
		}
	}
	return out
}

// toInt32 coerces the numeric forms FromProto / JSON decoding may produce.
func toInt32(v any) (int32, bool) {
	switch n := v.(type) {
	case int32:
		return n, true
	case int:
		return int32(n), true
	case int64:
		return int32(n), true
	case float64:
		return int32(n), true
	case float32:
		return int32(n), true
	default:
		return 0, false
	}
}
