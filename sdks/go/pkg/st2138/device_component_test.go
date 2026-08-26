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
 * @brief Tests for DeviceComponent chunk constructors.
 * @file device_component_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package st2138

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestComponentDevice(t *testing.T) {
	device := NewDevice(3)
	chunk := ComponentDevice(device)

	got := chunk.Proto.GetDevice()
	if got != device.Proto {
		t.Errorf("expected the chunk to reference the device's proto, got %v", got)
	}
	if got.GetSlot() != 3 {
		t.Errorf("expected slot 3, got %d", got.GetSlot())
	}
}

func TestComponentDevice_Nil(t *testing.T) {
	chunk := ComponentDevice(nil)
	if _, ok := chunk.Proto.GetKind().(*protos.DeviceComponent_Device); !ok {
		t.Errorf("expected a Device-kind chunk, got %v", chunk.Proto.GetKind())
	}
	if chunk.Proto.GetDevice() != nil {
		t.Errorf("expected nil device, got %v", chunk.Proto.GetDevice())
	}
}

func TestComponentParam(t *testing.T) {
	param := NewParamInt32(42)
	chunk := ComponentParam("brightness", param)

	component := chunk.Proto.GetParam()
	if component.GetOid() != "brightness" {
		t.Errorf("expected oid 'brightness', got %q", component.GetOid())
	}
	if component.GetParam() != param.Proto {
		t.Errorf("expected the chunk to reference the param's proto, got %v", component.GetParam())
	}
}

func TestComponentConstraint(t *testing.T) {
	constraint := NewConstraintInt32Range(0, 100, 1)
	chunk := ComponentConstraint("range", constraint)

	component := chunk.Proto.GetSharedConstraint()
	if component.GetOid() != "range" {
		t.Errorf("expected oid 'range', got %q", component.GetOid())
	}
	if component.GetConstraint() != constraint.Proto {
		t.Errorf("expected the chunk to reference the constraint's proto, got %v", component.GetConstraint())
	}
}

func TestComponentMenu(t *testing.T) {
	menu := NewMenu().WithParamOids("brightness")
	chunk := ComponentMenu("status/main", menu)

	component := chunk.Proto.GetMenu()
	if component.GetOid() != "status/main" {
		t.Errorf("expected oid 'status/main', got %q", component.GetOid())
	}
	if component.GetMenu() != menu.Proto {
		t.Errorf("expected the chunk to reference the menu's proto, got %v", component.GetMenu())
	}
}

func TestComponentCommand(t *testing.T) {
	command := NewParamEmpty()
	chunk := ComponentCommand("reset", command)

	component := chunk.Proto.GetCommand()
	if component.GetOid() != "reset" {
		t.Errorf("expected oid 'reset', got %q", component.GetOid())
	}
	if component.GetCommand() != command.Proto {
		t.Errorf("expected the chunk to reference the command's proto, got %v", component.GetCommand())
	}
}

func TestComponentLanguagePack(t *testing.T) {
	chunk := ComponentLanguagePack("en", "English", map[string]string{"greeting": "Hello"})

	component := chunk.Proto.GetLanguagePack()
	if component.GetLanguage() != "en" {
		t.Errorf("expected language 'en', got %q", component.GetLanguage())
	}
	pack := component.GetLanguagePack()
	if pack.GetName() != "English" {
		t.Errorf("expected pack name 'English', got %q", pack.GetName())
	}
	if pack.GetWords()["greeting"] != "Hello" {
		t.Errorf("expected greeting 'Hello', got %q", pack.GetWords()["greeting"])
	}
}

func TestDeviceComponent_Wire(t *testing.T) {
	chunk := ComponentDevice(NewDevice(1))
	wire := chunk.Wire()
	if wire != chunk.Proto {
		t.Errorf("expected Wire() to return the underlying proto, got %v", wire)
	}
}
