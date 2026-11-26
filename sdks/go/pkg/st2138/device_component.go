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
 * @brief DeviceComponent chunk type for streamed DeviceRequest responses.
 * @file device_component.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package st2138

import (
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// DeviceComponent wraps protos.DeviceComponent, one chunk of a streamed
// DeviceRequest response. A small device model is typically sent as a single
// ComponentDevice chunk; a large one may be broken into a ComponentDevice
// skeleton followed by ComponentParam / ComponentConstraint / ComponentMenu /
// ComponentCommand / ComponentLanguagePack chunks that fill in the pieces.
// Proto is the underlying proto message; it may be read or replaced directly.
type DeviceComponent struct {
	Proto *protos.DeviceComponent
}

// Ensure DeviceComponent can be streamed as a Message.
var _ Message = DeviceComponent{}

// Wire returns the underlying protos.DeviceComponent as the streamed chunk's
// wire representation.
func (c DeviceComponent) Wire() proto.Message {
	return c.Proto
}

// ComponentKind identifies which device component GetComponent should return.
// Mirrors protos.GetComponentPayload_ComponentKind for convenience.
type ComponentKind protos.GetComponentPayload_ComponentKind

// ComponentKind constants matching the proto enum.
// ComponentKindUnused is not a valid request.
const (
	ComponentKindUnused       ComponentKind = ComponentKind(protos.GetComponentPayload_UNUSED)
	ComponentKindParam        ComponentKind = ComponentKind(protos.GetComponentPayload_PARAM)
	ComponentKindConstraint   ComponentKind = ComponentKind(protos.GetComponentPayload_CONSTRAINT)
	ComponentKindMenu         ComponentKind = ComponentKind(protos.GetComponentPayload_MENU)
	ComponentKindCommand      ComponentKind = ComponentKind(protos.GetComponentPayload_COMMAND)
	ComponentKindLanguagePack ComponentKind = ComponentKind(protos.GetComponentPayload_LANGUAGE_PACK)
)

// IsValid reports whether kind is one of the five addressable
// component kinds. UNUSED and any other value are not valid kinds.
func (ck ComponentKind) IsValid() bool {
	switch ck {
	case ComponentKindParam,
		ComponentKindConstraint,
		ComponentKindMenu,
		ComponentKindCommand,
		ComponentKindLanguagePack:
		return true
	default:
		return false
	}
}

// String returns the string representation of the component kind.
func (ck ComponentKind) String() string {
	return protos.GetComponentPayload_ComponentKind(ck).String()
}

// ComponentDevice wraps a whole device model (or the top-level skeleton of
// one) as a DeviceComponent chunk. The device's proto is referenced, not
// copied, so it must not be mutated after being sent.
func ComponentDevice(device *Device) DeviceComponent {
	var protoDevice *protos.Device
	if device != nil {
		protoDevice = device.Proto
	}
	return DeviceComponent{
		Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_Device{Device: protoDevice},
		},
	}
}

// ComponentParam wraps one parameter descriptor as a DeviceComponent chunk.
// oid locates the param within device["params"] (RFC 6901 JSON pointer, e.g.
// "monitor" or "monitor/eq").
func ComponentParam(oid string, param *Param) DeviceComponent {
	return DeviceComponent{
		Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_Param{Param: componentParam(oid, param)},
		},
	}
}

// ComponentConstraint wraps one shared constraint as a DeviceComponent chunk.
// oid is relative to the device's top-level constraints object.
func ComponentConstraint(oid string, constraint *Constraint) DeviceComponent {
	return DeviceComponent{
		Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_SharedConstraint{SharedConstraint: componentConstraint(oid, constraint)},
		},
	}
}

// ComponentMenu wraps one menu as a DeviceComponent chunk. oid identifies the
// menu relative to the device's top-level menu-groups object as
// "menu-group-name/menu-name", e.g. "status/vendor_info".
func ComponentMenu(oid string, menu *Menu) DeviceComponent {
	return DeviceComponent{
		Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_Menu{Menu: componentMenu(oid, menu)},
		},
	}
}

// ComponentCommand wraps one command descriptor as a DeviceComponent chunk.
// oid locates the command within device["commands"].
func ComponentCommand(oid string, command *Param) DeviceComponent {
	return DeviceComponent{
		Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_Command{Command: componentCommand(oid, command)},
		},
	}
}

// ComponentLanguagePack wraps one language pack as a DeviceComponent chunk.
// language is the language code that identifies the pack (e.g. "en"), name is
// the pack's display name (e.g. "Global Spanish"), and words maps word keys to
// display strings. The words map is referenced, not copied.
func ComponentLanguagePack(language, name string, words map[string]string) DeviceComponent {
	return DeviceComponent{
		Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_LanguagePack{LanguagePack: componentLanguagePack(language, name, words)},
		},
	}
}

// ComponentResponse wraps protos.ComponentResponse, the unary GetComponent
// result. It is the addressable subset of DeviceComponent: a param, shared
// constraint, menu, command, or language pack. Proto is the underlying proto
// message; it may be read or replaced directly.
type ComponentResponse struct {
	Proto *protos.ComponentResponse
}

// ComponentResponseParam wraps one parameter descriptor as a ComponentResponse.
// oid locates the param within device["params"].
func ComponentResponseParam(oid string, param *Param) ComponentResponse {
	return ComponentResponse{
		Proto: &protos.ComponentResponse{
			Kind: &protos.ComponentResponse_Param{Param: componentParam(oid, param)},
		},
	}
}

// ComponentResponseConstraint wraps one shared constraint as a ComponentResponse.
// oid is relative to the device's top-level constraints object.
func ComponentResponseConstraint(oid string, constraint *Constraint) ComponentResponse {
	return ComponentResponse{
		Proto: &protos.ComponentResponse{
			Kind: &protos.ComponentResponse_SharedConstraint{SharedConstraint: componentConstraint(oid, constraint)},
		},
	}
}

// ComponentResponseMenu wraps one menu as a ComponentResponse. oid identifies
// the menu relative to the device's top-level menu-groups object as
// "menu-group-name/menu-name".
func ComponentResponseMenu(oid string, menu *Menu) ComponentResponse {
	return ComponentResponse{
		Proto: &protos.ComponentResponse{
			Kind: &protos.ComponentResponse_Menu{Menu: componentMenu(oid, menu)},
		},
	}
}

// ComponentResponseCommand wraps one command descriptor as a ComponentResponse.
// oid locates the command within device["commands"].
func ComponentResponseCommand(oid string, command *Param) ComponentResponse {
	return ComponentResponse{
		Proto: &protos.ComponentResponse{
			Kind: &protos.ComponentResponse_Command{Command: componentCommand(oid, command)},
		},
	}
}

// ComponentResponseLanguagePack wraps one language pack as a ComponentResponse.
// language is the language code that identifies the pack (e.g. "en"), name is
// the pack's display name, and words maps word keys to display strings. The
// words map is referenced, not copied.
func ComponentResponseLanguagePack(language, name string, words map[string]string) ComponentResponse {
	return ComponentResponse{
		Proto: &protos.ComponentResponse{
			Kind: &protos.ComponentResponse_LanguagePack{LanguagePack: componentLanguagePack(language, name, words)},
		},
	}
}

// componentParam builds a DeviceComponent_ComponentParam from an oid and param.
func componentParam(oid string, param *Param) *protos.DeviceComponent_ComponentParam {
	var protoParam *protos.Param
	if param != nil {
		protoParam = param.Proto
	}
	return &protos.DeviceComponent_ComponentParam{Oid: oid, Param: protoParam}
}

// componentConstraint builds a DeviceComponent_ComponentConstraint from an oid and constraint.
func componentConstraint(oid string, constraint *Constraint) *protos.DeviceComponent_ComponentConstraint {
	var protoConstraint *protos.Constraint
	if constraint != nil {
		protoConstraint = constraint.Proto
	}
	return &protos.DeviceComponent_ComponentConstraint{Oid: oid, Constraint: protoConstraint}
}

// componentMenu builds a DeviceComponent_ComponentMenu from an oid and menu.
func componentMenu(oid string, menu *Menu) *protos.DeviceComponent_ComponentMenu {
	var protoMenu *protos.Menu
	if menu != nil {
		protoMenu = menu.Proto
	}
	return &protos.DeviceComponent_ComponentMenu{Oid: oid, Menu: protoMenu}
}

// componentCommand builds a DeviceComponent_ComponentCommand from an oid and command.
func componentCommand(oid string, command *Param) *protos.DeviceComponent_ComponentCommand {
	var protoCommand *protos.Param
	if command != nil {
		protoCommand = command.Proto
	}
	return &protos.DeviceComponent_ComponentCommand{Oid: oid, Command: protoCommand}
}

// componentLanguagePack builds a DeviceComponent_ComponentLanguagePack from a language, name, and words map.
func componentLanguagePack(language, name string, words map[string]string) *protos.DeviceComponent_ComponentLanguagePack {
	return &protos.DeviceComponent_ComponentLanguagePack{
		Language:     language,
		LanguagePack: &protos.LanguagePack{Name: name, Words: words},
	}
}
