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
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
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

// SDKVersion is the Catena Go SDK version reported in the device's mandatory
// product struct (the "catena_sdk_version" field).
const SDKVersion = "1.0.0"

// CatenaSDKURL identifies the Catena SDK in the device's mandatory product
// struct (the "catena_sdk" field).
const CatenaSDKURL = "https://github.com/rossvideo/Catena"

// Device wraps protos.Device and exposes a fluent builder API.
type Device struct {
	device *protos.Device
}

// NewDevice creates a Device for the given slot and seeds the mandatory
// "product" struct param with the supplied identity fields. The product param
// is read-only and scoped to st2138:mon. Additional fields are populated with
// the chainable With* methods.
func NewDevice(slot uint16, name, vendor, version, serialNumber string) *Device {
	cd := &Device{device: &protos.Device{}}
	cd.device.Slot = uint32(slot)
	cd.WithParam("product",
		NewParamStruct().
			WithReadOnly(true).
			WithAccessScope(ScopeMon).
			WithParam("name", NewParamString(name)).
			WithParam("vendor", NewParamString(vendor)).
			WithParam("version", NewParamString(version)).
			WithParam("serial_number", NewParamString(serialNumber)).
			WithParam("catena_sdk_version", NewParamString(SDKVersion)).
			WithParam("catena_sdk", NewParamString(CatenaSDKURL)))
	return cd
}

// WithParam inserts param into the device's params map, keyed by oid. The
// param's proto is deep-copied so later builder mutations on the caller's Param
// do not affect entries already added. A nil param is ignored.
func (cd *Device) WithParam(oid string, param *Param) *Device {
	if param == nil || param.Proto == nil {
		logger.Warning("Device.WithParam called with nil param; ignoring", "oid", oid)
		return cd
	}
	if cd.device.Params == nil {
		cd.device.Params = map[string]*protos.Param{}
	}
	cd.device.Params[oid] = proto.Clone(param.Proto).(*protos.Param)
	return cd
}

// WithCommand inserts command into the device's commands map, keyed by oid. The
// command's proto is deep-copied. A nil command is ignored.
func (cd *Device) WithCommand(oid string, command *Param) *Device {
	if command == nil || command.Proto == nil {
		logger.Warning("Device.WithCommand called with nil command; ignoring", "oid", oid)
		return cd
	}
	if cd.device.Commands == nil {
		cd.device.Commands = map[string]*protos.Param{}
	}
	cd.device.Commands[oid] = proto.Clone(command.Proto).(*protos.Param)
	return cd
}

// WithConstraint inserts a shared constraint into the device's constraints map,
// keyed by oid. The constraint's proto is deep-copied. A nil constraint is
// ignored. Params can reference shared constraints via NewConstraintRefOid.
func (cd *Device) WithConstraint(oid string, constraint *Constraint) *Device {
	if constraint == nil || constraint.Proto == nil {
		logger.Warning("Device.WithConstraint called with nil constraint; ignoring", "oid", oid)
		return cd
	}
	if cd.device.Constraints == nil {
		cd.device.Constraints = map[string]*protos.Constraint{}
	}
	cd.device.Constraints[oid] = proto.Clone(constraint.Proto).(*protos.Constraint)
	return cd
}

// WithMenuGroup inserts a menu group into the device's menu_groups map, keyed
// by oid. The group's proto is deep-copied. A nil group is ignored.
func (cd *Device) WithMenuGroup(oid string, group *MenuGroup) *Device {
	if group == nil || group.Proto == nil {
		logger.Warning("Device.WithMenuGroup called with nil menu group; ignoring", "oid", oid)
		return cd
	}
	if cd.device.MenuGroups == nil {
		cd.device.MenuGroups = map[string]*protos.MenuGroup{}
	}
	cd.device.MenuGroups[oid] = proto.Clone(group.Proto).(*protos.MenuGroup)
	return cd
}

// WithLanguagePack inserts a language pack into the device's language_packs map,
// keyed by language code (e.g. "en"). Replaces any existing pack at that code.
func (cd *Device) WithLanguagePack(code, name string, words map[string]string) *Device {
	if cd.device.LanguagePacks == nil {
		cd.device.LanguagePacks = &protos.LanguagePacks{}
	}
	if cd.device.LanguagePacks.Packs == nil {
		cd.device.LanguagePacks.Packs = map[string]*protos.LanguagePack{}
	}
	copied := make(map[string]string, len(words))
	for k, v := range words {
		copied[k] = v
	}
	cd.device.LanguagePacks.Packs[code] = &protos.LanguagePack{
		Name:  name,
		Words: copied,
	}
	return cd
}

// WithDetailLevel sets how much of the device model to deliver.
func (cd *Device) WithDetailLevel(level DetailLevel) *Device {
	cd.device.DetailLevel = level
	return cd
}

// WithMultiSetEnabled sets whether the device supports multi-set requests.
func (cd *Device) WithMultiSetEnabled(enabled bool) *Device {
	cd.device.MultiSetEnabled = enabled
	return cd
}

// WithSubscriptions sets whether the device supports subscriptions.
func (cd *Device) WithSubscriptions(subscriptions bool) *Device {
	cd.device.Subscriptions = subscriptions
	return cd
}

// WithAccessScopes sets the device's access scopes, replacing any existing ones.
func (cd *Device) WithAccessScopes(scopes ...string) *Device {
	cd.device.AccessScopes = scopes
	return cd
}

// WithDefaultScope sets the device's default access scope.
func (cd *Device) WithDefaultScope(scope string) *Device {
	cd.device.DefaultScope = scope
	return cd
}

// ToProtoDevice returns the underlying protos.Device for gRPC responses.
func (cd Device) ToProtoDevice() *protos.Device {
	return cd.device
}

// ToJSON serializes the device to its REST JSON representation using proto
// field names.
func (cd Device) ToJSON() ([]byte, error) {
	return protojson.MarshalOptions{UseProtoNames: true}.Marshal(cd.device)
}
