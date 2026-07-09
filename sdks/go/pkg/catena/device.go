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
	"runtime/debug"

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

// sdkModulePath is the SDK's Go module path, used to locate the SDK's resolved
// version in the running binary's build info.
const sdkModulePath = "github.com/rossvideo/catena/sdks/go"

// SDKVersion is the Catena Go SDK version reported in the SDK-managed product
// struct (the "catena_sdk_version" field). It is resolved at startup from the
// binary's build info (the git tag or pseudo-version Go recorded for the SDK
// module), so it stays accurate without a hand-maintained constant.
var SDKVersion = sdkVersion()

// sdkVersion resolves the SDK's version from the running binary's build info.
// It returns the version Go recorded for the SDK module, falling back to
// "(devel)" for unversioned local builds and "(unknown)" when build info is
// unavailable.
func sdkVersion() string {
	bi, ok := debug.ReadBuildInfo()
	if !ok {
		return "(unknown)"
	}
	// When the SDK is imported as a dependency, its resolved version lives in
	// Deps. When the SDK module is itself the main module (e.g. running its own
	// tests or examples), the version lives on Main instead.
	if bi.Main.Path == sdkModulePath && bi.Main.Version != "" {
		return bi.Main.Version
	}
	for _, d := range bi.Deps {
		if d.Path == sdkModulePath {
			return d.Version // resolved from the git tag / pseudo-version
		}
	}
	return "(devel)"
}

// CatenaSDKURL identifies the Catena SDK in the SDK-managed product struct (the
// "catena_sdk" field).
const CatenaSDKURL = "https://github.com/rossvideo/Catena"

// Device wraps protos.Device and exposes a fluent builder API.
// Proto is the underlying proto message; it may be read or replaced directly.
type Device struct {
	Proto *protos.Device
}

// NewDevice creates an empty Device for the given slot. Params, commands,
// constraints, and menus are attached with the chainable With* methods.
//
// The mandatory "product" struct is managed by the SDK: register it once with
// Server.RegisterProductStruct and the SDK injects it into the device on
// GetDevice and serves it on GetValue / ParamInfo. NewDevice does not deal with
// the product struct.
func NewDevice(slot uint16) *Device {
	return &Device{Proto: &protos.Device{Slot: uint32(slot)}}
}

// WithParam inserts param into the device's params map, keyed by oid. The
// param's proto is deep-copied so later builder mutations on the caller's Param
// do not affect entries already added. A nil param is ignored.
func (cd *Device) WithParam(oid string, param *Param) *Device {
	if param == nil || param.Proto == nil {
		logger.Warning("Device.WithParam called with nil param; ignoring", "oid", oid)
		return cd
	}
	if cd.Proto.Params == nil {
		cd.Proto.Params = map[string]*protos.Param{}
	}
	cd.Proto.Params[oid] = proto.Clone(param.Proto).(*protos.Param)
	return cd
}

// WithCommand inserts command into the device's commands map, keyed by oid. The
// command's proto is deep-copied. A nil command is ignored.
func (cd *Device) WithCommand(oid string, command *Param) *Device {
	if command == nil || command.Proto == nil {
		logger.Warning("Device.WithCommand called with nil command; ignoring", "oid", oid)
		return cd
	}
	if cd.Proto.Commands == nil {
		cd.Proto.Commands = map[string]*protos.Param{}
	}
	cd.Proto.Commands[oid] = proto.Clone(command.Proto).(*protos.Param)
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
	if cd.Proto.Constraints == nil {
		cd.Proto.Constraints = map[string]*protos.Constraint{}
	}
	cd.Proto.Constraints[oid] = proto.Clone(constraint.Proto).(*protos.Constraint)
	return cd
}

// WithMenuGroup inserts a menu group into the device's menu_groups map, keyed
// by oid. The group's proto is deep-copied. A nil group is ignored.
func (cd *Device) WithMenuGroup(oid string, group *MenuGroup) *Device {
	if group == nil || group.Proto == nil {
		logger.Warning("Device.WithMenuGroup called with nil menu group; ignoring", "oid", oid)
		return cd
	}
	if cd.Proto.MenuGroups == nil {
		cd.Proto.MenuGroups = map[string]*protos.MenuGroup{}
	}
	cd.Proto.MenuGroups[oid] = proto.Clone(group.Proto).(*protos.MenuGroup)
	return cd
}

// WithLanguagePack inserts a language pack into the device's language_packs map,
// keyed by language code (e.g. "en"). Replaces any existing pack at that code.
func (cd *Device) WithLanguagePack(code, name string, words map[string]string) *Device {
	if cd.Proto.LanguagePacks == nil {
		cd.Proto.LanguagePacks = &protos.LanguagePacks{}
	}
	if cd.Proto.LanguagePacks.Packs == nil {
		cd.Proto.LanguagePacks.Packs = map[string]*protos.LanguagePack{}
	}
	copied := make(map[string]string, len(words))
	for k, v := range words {
		copied[k] = v
	}
	cd.Proto.LanguagePacks.Packs[code] = &protos.LanguagePack{
		Name:  name,
		Words: copied,
	}
	return cd
}

// WithDetailLevel sets how much of the device model to deliver.
func (cd *Device) WithDetailLevel(level DetailLevel) *Device {
	cd.Proto.DetailLevel = level
	return cd
}

// WithMultiSetEnabled sets whether the device supports multi-set requests.
func (cd *Device) WithMultiSetEnabled(enabled bool) *Device {
	cd.Proto.MultiSetEnabled = enabled
	return cd
}

// WithSubscriptions sets whether the device supports subscriptions.
func (cd *Device) WithSubscriptions(subscriptions bool) *Device {
	cd.Proto.Subscriptions = subscriptions
	return cd
}

// WithAccessScopes sets the device's access scopes, replacing any existing ones.
func (cd *Device) WithAccessScopes(scopes ...string) *Device {
	cd.Proto.AccessScopes = scopes
	return cd
}

// WithDefaultScope sets the device's default access scope.
func (cd *Device) WithDefaultScope(scope string) *Device {
	cd.Proto.DefaultScope = scope
	return cd
}
