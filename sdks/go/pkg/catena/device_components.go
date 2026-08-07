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
 * @brief DeviceComponent chunking business logic for the Catena SDK.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-08-07
 * @file device_components.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"sort"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// DeviceComponentsForRequest streams a complete device model as DeviceComponent
// chunks: first a skeleton Device chunk (the device minus its params, commands,
// shared constraints, menus, and language packs), then one chunk per shared
// constraint, menu, top-level param, command, and language pack, each group
// sorted by oid so the stream order is deterministic. It is a business-logic
// helper a GetDevice handler can delegate to once it can produce a Device
// definition; the returned StatusResult is the terminal status a handler
// returns directly: Ok once every component has been sent, or Internal if a
// Send fails (a failed Send stops the stream immediately).
//
// Menu-group metadata (display name, order) stays on the skeleton because a
// ComponentMenu chunk carries only the menu itself; each group's menus are
// emitted as "group/menu" chunks that clients merge back into their group.
//
// The component chunks reference the device's protos rather than copying them,
// so the device must not be mutated until the call returns. Handlers that want
// to hand-pick their own chunking can call the st2138.ComponentXxx
// constructors directly instead.
func DeviceComponentsForRequest(device *st2138.Device, stream Stream[st2138.DeviceComponent]) StatusResult {
	if device == nil || device.Proto == nil {
		return StatusWithCode(StatusCodeInternal, "invalid device")
	}

	skeleton := deviceSkeleton(device.Proto)
	if err := stream.Send(st2138.ComponentDevice(&st2138.Device{Proto: skeleton})); err != nil {
		return StatusWithCode(StatusCodeInternal, err.Error())
	}

	constraints := device.Proto.GetConstraints()
	for _, oid := range sortedKeys(constraints) {
		constraint := constraints[oid]
		if constraint == nil {
			continue
		}
		if err := stream.Send(st2138.ComponentConstraint(oid, &st2138.Constraint{Proto: constraint})); err != nil {
			return StatusWithCode(StatusCodeInternal, err.Error())
		}
	}

	menuGroups := device.Proto.GetMenuGroups()
	for _, group := range sortedKeys(menuGroups) {
		menus := menuGroups[group].GetMenus()
		for _, name := range sortedKeys(menus) {
			menu := menus[name]
			if menu == nil {
				continue
			}
			if err := stream.Send(st2138.ComponentMenu(group+"/"+name, &st2138.Menu{Proto: menu})); err != nil {
				return StatusWithCode(StatusCodeInternal, err.Error())
			}
		}
	}

	params := device.Proto.GetParams()
	for _, oid := range sortedKeys(params) {
		param := params[oid]
		if param == nil {
			continue
		}
		if err := stream.Send(st2138.ComponentParam(oid, &st2138.Param{Proto: param})); err != nil {
			return StatusWithCode(StatusCodeInternal, err.Error())
		}
	}

	commands := device.Proto.GetCommands()
	for _, oid := range sortedKeys(commands) {
		command := commands[oid]
		if command == nil {
			continue
		}
		if err := stream.Send(st2138.ComponentCommand(oid, &st2138.Param{Proto: command})); err != nil {
			return StatusWithCode(StatusCodeInternal, err.Error())
		}
	}

	packs := device.Proto.GetLanguagePacks().GetPacks()
	for _, code := range sortedKeys(packs) {
		pack := packs[code]
		if pack == nil {
			continue
		}
		if err := stream.Send(st2138.ComponentLanguagePack(code, pack.GetName(), pack.GetWords())); err != nil {
			return StatusWithCode(StatusCodeInternal, err.Error())
		}
	}

	return StatusResult{Code: StatusCodeOk}
}

// deviceSkeleton snapshots the top-level device fields that are not streamed
// as separate component chunks: everything except params, commands, shared
// constraints, per-group menus, and language packs. Menu groups survive as
// empty shells so their display name and order reach the client. The skeleton
// is built on a clone so the source device is never touched, even briefly (it
// may be shared with concurrent requests).
func deviceSkeleton(device *protos.Device) *protos.Device {
	skeleton := proto.Clone(device).(*protos.Device)
	skeleton.Params = nil
	skeleton.Commands = nil
	skeleton.Constraints = nil
	skeleton.LanguagePacks = nil
	for _, group := range skeleton.MenuGroups {
		if group != nil {
			group.Menus = nil
		}
	}
	return skeleton
}

// sortedKeys returns m's keys in ascending order so component chunks are
// emitted deterministically.
func sortedKeys[V any](m map[string]V) []string {
	keys := make([]string, 0, len(m))
	for key := range m {
		keys = append(keys, key)
	}
	sort.Strings(keys)
	return keys
}
