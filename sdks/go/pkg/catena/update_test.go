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
	"os"
	"testing"
)

func uploadPayload(data DataPayload, metadata []map[string]any) map[string]any {
	return map[string]any{
		"metadata": metadata,
		"data":     data,
	}
}

func replyInt32(t *testing.T, r CommandResult) int32 {
	t.Helper()
	proto := r.GetProtoResponse()
	if proto == nil {
		t.Fatal("expected a CommandResponse, got nil")
	}
	if r.IsException() {
		t.Fatalf("expected reply, got exception: %v", r.GetException())
	}
	return proto.GetResponse().GetInt32Value()
}

func TestToDevice_InjectsReservedCommands(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"commands": map[string]any{
			"reboot": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{"en": "Reboot Device"},
				},
				"type": ParamTypeEmpty,
			},
		},
	}

	cd, err := ToDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToDevice error: %v", err)
	}
	commands := cd.GetProtoDevice().GetCommands()

	for _, oid := range []string{"reboot", ReservedOidUploadUpdate, ReservedOidApplyUpdate} {
		if _, ok := commands[oid]; !ok {
			t.Errorf("expected command %q to be present", oid)
		}
	}

	upload := commands[ReservedOidUploadUpdate]
	if upload.GetType() != ParamTypeStruct {
		t.Errorf("upload_update: expected STRUCT, got %v", upload.GetType())
	}
	if !upload.GetResponse() {
		t.Error("upload_update: expected response=true")
	}
	dataField, ok := upload.GetParams()["data"]
	if !ok {
		t.Fatal("upload_update: expected 'data' sub-param")
	}
	if dataField.GetType() != ParamTypeData {
		t.Errorf("upload_update.data: expected DATA, got %v", dataField.GetType())
	}

	apply := commands[ReservedOidApplyUpdate]
	idField, ok := apply.GetParams()["id"]
	if !ok {
		t.Fatal("apply_update: expected 'id' sub-param")
	}
	if idField.GetType() != ParamTypeInt32 {
		t.Errorf("apply_update.id: expected INT32, got %v", idField.GetType())
	}
}

func TestToDevice_InjectsReservedCommandsWhenNoneDefined(t *testing.T) {
	cd, err := ToDevice(map[string]any{"slot": uint32(0)})
	if err != nil {
		t.Fatalf("ToDevice error: %v", err)
	}
	commands := cd.GetProtoDevice().GetCommands()
	if _, ok := commands[ReservedOidUploadUpdate]; !ok {
		t.Error("expected upload_update to be injected")
	}
	if _, ok := commands[ReservedOidApplyUpdate]; !ok {
		t.Error("expected apply_update to be injected")
	}
}

func TestUpdateManager_UploadApplyRoundTrip(t *testing.T) {
	dir := t.TempDir()
	m := newUpdateManager(dir, nil)

	metadata := []map[string]any{
		{"key": "version", "value": "1.2.3"},
		{"key": "vendor", "value": "Ross"},
	}
	payload := uploadPayload(DataPayload{Payload: []byte("firmware-bytes")}, metadata)

	uploadResult, status := m.handleUpload(payload)
	if status.Code != OK {
		t.Fatalf("handleUpload status: %v", status.Error)
	}
	id := replyInt32(t, uploadResult)
	if id != 1 {
		t.Fatalf("expected first update id 1, got %d", id)
	}

	stored := m.updates[id]
	if stored == nil {
		t.Fatal("expected update to be stored")
	}
	if stored.Metadata["version"] != "1.2.3" {
		t.Errorf("expected metadata version 1.2.3, got %q", stored.Metadata["version"])
	}
	if _, err := os.Stat(stored.DataPath); err != nil {
		t.Fatalf("expected update file to exist: %v", err)
	}

	applyResult, status := m.handleApply(map[string]any{"id": id})
	if status.Code != OK {
		t.Fatalf("handleApply status: %v", status.Error)
	}
	if progress := replyInt32(t, applyResult); progress != 100 {
		t.Errorf("expected apply progress 100, got %d", progress)
	}
}

func TestUpdateManager_UploadAssignsIncrementingIDs(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)
	payload := uploadPayload(DataPayload{Payload: []byte("x")}, nil)

	first, _ := m.handleUpload(payload)
	second, _ := m.handleUpload(payload)

	if replyInt32(t, first) != 1 || replyInt32(t, second) != 2 {
		t.Error("expected incrementing update ids 1, 2")
	}
}

func TestUpdateManager_UploadURLOnly(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)
	payload := uploadPayload(DataPayload{Url: "https://example.com/fw.bin"}, nil)

	uploadResult, status := m.handleUpload(payload)
	if status.Code != OK {
		t.Fatalf("handleUpload status: %v", status.Error)
	}
	id := replyInt32(t, uploadResult)
	if m.updates[id].URL != "https://example.com/fw.bin" {
		t.Error("expected URL to be stored")
	}

	applyResult, _ := m.handleApply(map[string]any{"id": id})
	if applyResult.IsException() {
		t.Errorf("expected URL-based apply to succeed, got exception: %v", applyResult.GetException())
	}
}

func TestUpdateManager_UploadInvalidPayload(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)

	result, _ := m.handleUpload("not-a-struct")
	if !result.IsException() {
		t.Fatal("expected exception for non-struct payload")
	}
	if result.GetException().GetType() != "InvalidPayload" {
		t.Errorf("expected InvalidPayload, got %q", result.GetException().GetType())
	}
}

func TestUpdateManager_UploadEmptyData(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)

	result, _ := m.handleUpload(uploadPayload(DataPayload{}, nil))
	if !result.IsException() {
		t.Fatal("expected exception for empty data")
	}
}

func TestUpdateManager_UploadMissingDataField(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)

	result, _ := m.handleUpload(map[string]any{"metadata": []map[string]any{}})
	if !result.IsException() {
		t.Fatal("expected exception when data field is missing")
	}
}

func TestUpdateManager_ApplyUnknownID(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)

	result, _ := m.handleApply(map[string]any{"id": int32(999)})
	if !result.IsException() {
		t.Fatal("expected exception for unknown update id")
	}
	if result.GetException().GetType() != "UnknownUpdate" {
		t.Errorf("expected UnknownUpdate, got %q", result.GetException().GetType())
	}
}

func TestUpdateManager_ApplyMissingID(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)

	result, _ := m.handleApply(map[string]any{})
	if !result.IsException() {
		t.Fatal("expected exception when id is missing")
	}
}

func TestUpdateManager_ApplyCustomFuncError(t *testing.T) {
	called := false
	m := newUpdateManager(t.TempDir(), func(u *Update) error {
		called = true
		return errors.New("bootloader rejected image")
	})

	upload, _ := m.handleUpload(uploadPayload(DataPayload{Payload: []byte("fw")}, nil))
	id := replyInt32(t, upload)

	result, _ := m.handleApply(map[string]any{"id": id})
	if !called {
		t.Error("expected custom ApplyUpdateFunc to be invoked")
	}
	if !result.IsException() {
		t.Fatal("expected exception when apply func fails")
	}
	if result.GetException().GetType() != "UpdateFailed" {
		t.Errorf("expected UpdateFailed, got %q", result.GetException().GetType())
	}
}

func TestUpdateManager_ExecuteDispatch(t *testing.T) {
	m := newUpdateManager(t.TempDir(), nil)

	// Leading-slash form (REST) should normalize to the same OID.
	upload, status := m.execute("/"+ReservedOidUploadUpdate, uploadPayload(DataPayload{Payload: []byte("fw")}, nil))
	if status.Code != OK {
		t.Fatalf("execute upload status: %v", status.Error)
	}
	id := replyInt32(t, upload)

	apply, status := m.execute(ReservedOidApplyUpdate, map[string]any{"id": id})
	if status.Code != OK {
		t.Fatalf("execute apply status: %v", status.Error)
	}
	if replyInt32(t, apply) != 100 {
		t.Error("expected apply progress 100")
	}
}

func TestIsReservedUpdateOid(t *testing.T) {
	cases := map[string]bool{
		ReservedOidUploadUpdate:       true,
		ReservedOidApplyUpdate:        true,
		"/" + ReservedOidUploadUpdate: true,
		"/" + ReservedOidApplyUpdate:  true,
		"reboot":                      false,
		"":                            false,
	}
	for oid, want := range cases {
		if got := isReservedUpdateOid(oid); got != want {
			t.Errorf("isReservedUpdateOid(%q) = %v, want %v", oid, got, want)
		}
	}
}
