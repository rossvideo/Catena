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
	"strings"
	"testing"
)

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

func TestExecuteReservedCommand_MatchesTopLevelOnly(t *testing.T) {
	reserved := []struct {
		name  string
		fqoid string
	}{
		{"upload_update", ReservedOidUploadUpdate},
		{"apply_update", ReservedOidApplyUpdate},
		{"upload_update with leading slash", "/" + ReservedOidUploadUpdate},
		{"apply_update with leading slash", "/" + ReservedOidApplyUpdate},
	}
	for _, tc := range reserved {
		t.Run("reserved/"+tc.name, func(t *testing.T) {
			if _, _, handled := executeReservedCommand(tc.fqoid, nil); !handled {
				t.Errorf("expected %q to be handled as a reserved command", tc.fqoid)
			}
		})
	}

	notReserved := []struct {
		name  string
		fqoid string
	}{
		{"nested upload_update", "custom.upload_update"},
		{"nested apply_update", "custom.apply_update"},
		{"nested upload_update with leading slash", "/custom.upload_update"},
		{"deeply nested", "device.commands.upload_update"},
		{"unrelated command", "reboot"},
	}
	for _, tc := range notReserved {
		t.Run("user/"+tc.name, func(t *testing.T) {
			if _, _, handled := executeReservedCommand(tc.fqoid, nil); handled {
				t.Errorf("expected %q NOT to be hijacked by the reserved command handler", tc.fqoid)
			}
		})
	}
}

func TestToDevice_RejectsReservedCommandDefinition(t *testing.T) {
	for _, reserved := range []string{ReservedOidUploadUpdate, ReservedOidApplyUpdate} {
		t.Run(reserved, func(t *testing.T) {
			deviceMap := map[string]any{
				"slot": uint32(0),
				"commands": map[string]any{
					reserved: map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{"en": "Custom"},
						},
						"type": ParamTypeEmpty,
					},
				},
			}

			_, err := ToDevice(deviceMap)
			if err == nil {
				t.Fatalf("expected error when device defines reserved command %q", reserved)
			}
			if !strings.Contains(err.Error(), reserved) {
				t.Errorf("expected error to name %q, got: %v", reserved, err)
			}
		})
	}
}
