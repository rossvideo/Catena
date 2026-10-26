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

package st2138

import (
	"google.golang.org/protobuf/proto"

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

// Device wraps protos.Device and exposes a fluent builder API.
// Proto is the underlying proto message; it may be read or replaced directly.
type Device struct {
	Proto *protos.Device
}

type DeviceBuilder struct {
	proto *protos.Device
	err   error
}

// NewDevice creates an empty Device for the given slot. Params, commands,
// constraints, and menus are attached with the chainable With* methods.
//
// The mandatory "product" struct is managed by the SDK: register it once with
// Server.RegisterProductStruct and the SDK injects it into the device on
// GetDevice and serves it on GetValue / ParamInfo. NewDevice does not deal with
// the product struct.
func NewDevice(slot uint16) *DeviceBuilder {
	return &DeviceBuilder{
		proto: &protos.Device{Slot: uint32(slot)},
		err:   nil,
	}
}

// Build returns a usable Device from the builder. If the builder has an error, it
// returns an empty Device and the error. Must be called after all builder methods have been called.
func (db *DeviceBuilder) Build() (Device, error) {
	if db.err != nil {
		return Device{}, db.err
	}
	db.err = ErrAlreadyBuilt
	return Device{Proto: db.proto}, nil
}

// Clone returns a deep copy of the DeviceBuilder.
// Inherits the builder's error state, including ErrAlreadyBuilt.
// Must be called before Build(), or the clone will be unusable.
func (db *DeviceBuilder) Clone() *DeviceBuilder {
	return &DeviceBuilder{proto: proto.Clone(db.proto).(*protos.Device), err: db.err}
}

// WithParam inserts param into the device's params map, keyed by oid. The
// param's proto is deep-copied so later builder mutations on the caller's Param
// do not affect entries already added. A nil param errors and short-circuits further method calls.
func (db *DeviceBuilder) WithParam(oid string, paramBuilder *ParamBuilder) *DeviceBuilder {
	param, ok := buildChild(&db.err, paramBuilder, (*ParamBuilder).Build, "WithParam")
	if !ok {
		return db
	}
	put(&db.proto.Params, oid, param.Proto)
	return db
}

// WithCommand inserts command into the device's commands map, keyed by oid. The
// command's proto is deep-copied. A nil command errors and short-circuits further method calls.
func (db *DeviceBuilder) WithCommand(oid string, commandBuilder *ParamBuilder) *DeviceBuilder {
	command, ok := buildChild(&db.err, commandBuilder, (*ParamBuilder).Build, "WithCommand")
	if !ok {
		return db
	}
	put(&db.proto.Commands, oid, command.Proto)
	return db
}

// WithConstraint inserts a shared constraint into the device's constraints map,
// keyed by oid. The constraint's proto is deep-copied. A nil constraint
// errors and short-circuits further method calls. Params can reference shared constraints via NewConstraintRefOid.
func (db *DeviceBuilder) WithConstraint(oid string, constraint *ConstraintBuilder) *DeviceBuilder {
	c, ok := buildChild(&db.err, constraint, (*ConstraintBuilder).Build, "WithConstraint")
	if !ok {
		return db
	}
	put(&db.proto.Constraints, oid, c.Proto)
	return db
}

// WithMenuGroup inserts a menu group into the device's menu_groups map, keyed
// by oid. The group's proto is deep-copied. A nil group errors and short-circuits further method calls.
func (db *DeviceBuilder) WithMenuGroup(oid string, groupBuilder *MenuGroupBuilder) *DeviceBuilder {
	group, ok := buildChild(&db.err, groupBuilder, (*MenuGroupBuilder).Build, "WithMenuGroup")
	if !ok {
		return db
	}
	put(&db.proto.MenuGroups, oid, group.Proto)
	return db
}

// WithLanguagePack inserts a language pack into the device's language_packs map,
// keyed by language code (e.g. "en"). Replaces any existing pack at that code.
func (db *DeviceBuilder) WithLanguagePack(code, name string, words map[string]string) *DeviceBuilder {
	if db.err != nil {
		return db
	}
	if db.proto.LanguagePacks == nil {
		db.proto.LanguagePacks = &protos.LanguagePacks{}
	}
	if db.proto.LanguagePacks.Packs == nil {
		db.proto.LanguagePacks.Packs = map[string]*protos.LanguagePack{}
	}
	copied := make(map[string]string, len(words))
	for k, v := range words {
		copied[k] = v
	}
	db.proto.LanguagePacks.Packs[code] = &protos.LanguagePack{
		Name:  name,
		Words: copied,
	}
	return db
}

// WithDetailLevel sets how much of the device model to deliver.
func (db *DeviceBuilder) WithDetailLevel(level DetailLevel) *DeviceBuilder {
	if db.err != nil {
		return db
	}
	db.proto.DetailLevel = level
	return db
}

// WithMultiSetEnabled sets whether the device supports multi-set requests.
func (db *DeviceBuilder) WithMultiSetEnabled(enabled bool) *DeviceBuilder {
	if db.err != nil {
		return db
	}
	db.proto.MultiSetEnabled = enabled
	return db
}

// WithSubscriptions sets whether the device supports subscriptions.
func (db *DeviceBuilder) WithSubscriptions(subscriptions bool) *DeviceBuilder {
	if db.err != nil {
		return db
	}
	db.proto.Subscriptions = subscriptions
	return db
}

// WithAccessScopes sets the device's access scopes, replacing any existing ones.
func (db *DeviceBuilder) WithAccessScopes(scopes ...string) *DeviceBuilder {
	if db.err != nil {
		return db
	}
	db.proto.AccessScopes = scopes
	return db
}

// WithDefaultScope sets the device's default access scope.
func (db *DeviceBuilder) WithDefaultScope(scope string) *DeviceBuilder {
	if db.err != nil {
		return db
	}
	db.proto.DefaultScope = scope
	return db
}
