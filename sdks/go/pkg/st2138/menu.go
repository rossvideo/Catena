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
 * @brief Menu and MenuGroup builder types for the Catena SDK.
 * @file menu.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-06-01
 */

package st2138

import (
	"errors"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// MenuGroup wraps a protos.MenuGroup and exposes a fluent builder API.
// The group's oid is supplied by its parent when the group is attached, so it
// is not stored here.
type MenuGroup struct {
	Proto *protos.MenuGroup
}

type MenuGroupBuilder struct {
	proto *protos.MenuGroup
	err   error
}

// NewMenuGroup creates a MenuGroup with an initialized proto.
func NewMenuGroup() *MenuGroupBuilder {
	return &MenuGroupBuilder{
		proto: &protos.MenuGroup{
			Menus: make(map[string]*protos.Menu),
		},
		err: nil,
	}
}

// Wraps a protos.MenuGroup in a MenuGroupBuilder,
// giving access to the .With* builders.
func WrapMenuGroup(proto *protos.MenuGroup) *MenuGroupBuilder {
	return &MenuGroupBuilder{
		proto: proto,
		err:   nil,
	}
}

// WithName sets the menu group's display name, replacing any existing name.
func (mg *MenuGroupBuilder) WithName(name *PolyglotTextBuilder) *MenuGroupBuilder {
	if mg.err != nil {
		return mg
	}
	if name == nil {
		mg.err = errors.New("name cannot be nil")
		return mg
	}

	p, err := name.Build()
	if err != nil {
		mg.err = err
		return mg
	}
	if p == nil {
		mg.err = errors.New("polyglot text builder returned nil polyglot")
		return mg
	}
	mg.proto.Name = &protos.PolyglotText{DisplayStrings: p}
	return mg
}

// WithMenu inserts menu into the menu group's menus map, keyed by oid.
// Replaces any existing menu at that oid. A nil menu errors and short-circuits further method calls.
// The menu's proto is deep-copied so that later builder mutations on the
// caller's Menu (or reusing it under another oid) do not affect entries that
// were already added.
func (mg *MenuGroupBuilder) WithMenu(oid string, menu *MenuBuilder) *MenuGroupBuilder {
	if mg.err != nil {
		return mg
	}
	if menu == nil {
		mg.err = errors.New("menu cannot be nil")
		return mg
	}
	m, err := menu.Build()
	if err != nil {
		mg.err = err
		return mg
	}
	if m == nil || m.Proto == nil {
		mg.err = errors.New("menu builder returned nil menu")
		return mg
	}
	if mg.proto.Menus == nil {
		mg.proto.Menus = map[string]*protos.Menu{}
	}
	mg.proto.Menus[oid] = proto.Clone(m.Proto).(*protos.Menu)
	return mg
}

// WithOrder sets the menu group's display order.
func (mg *MenuGroupBuilder) WithOrder(order uint32) *MenuGroupBuilder {
	if mg.err != nil {
		return mg
	}
	mg.proto.Order = order
	return mg
}

// Build returns a usable MenuGroup from the builder. If the builder has an error, it
// returns nil and the error. Must be called after all builder methods have been called.
func (mg *MenuGroupBuilder) Build() (*MenuGroup, error) {
	if mg.err != nil {
		return nil, mg.err
	}
	return &MenuGroup{Proto: mg.proto}, nil
}

// Err returns the stored error of the builder.
func (mg *MenuGroupBuilder) Err() error {
	return mg.err
}

// Menu wraps a protos.Menu and exposes a fluent builder API.
// The menu's oid is supplied by its parent when the menu is attached, so it is
// not stored here.
type Menu struct {
	Proto *protos.Menu
}

type MenuBuilder struct {
	proto *protos.Menu
	err   error
}

// NewMenu creates a Menu with an initialized proto.
func NewMenu() *MenuBuilder {
	return &MenuBuilder{
		proto: &protos.Menu{},
		err:   nil,
	}
}

// WithName sets the menu's display name, replacing any existing name.
func (m *MenuBuilder) WithName(name *PolyglotTextBuilder) *MenuBuilder {
	if m.err != nil {
		return m
	}
	if name == nil {
		m.err = errors.New("name is required")
		return m
	}

	p, err := name.Build()
	if err != nil {
		m.err = err
		return m
	}
	if p == nil {
		m.err = errors.New("polyglot text builder returned nil polyglot text")
		return m
	}
	m.proto.Name = &protos.PolyglotText{DisplayStrings: p}
	return m
}

// WithHidden sets whether the menu should be hidden in the client GUI.
func (m *MenuBuilder) WithHidden(hidden bool) *MenuBuilder {
	if m.err != nil {
		return m
	}
	m.proto.Hidden = hidden
	return m
}

// WithDisabled sets whether the menu should be disabled (shown as read-only)
// in the client GUI.
func (m *MenuBuilder) WithDisabled(disabled bool) *MenuBuilder {
	if m.err != nil {
		return m
	}
	m.proto.Disabled = disabled
	return m
}

// WithParamOids sets the menu's parameter members, replacing any existing oids.
func (m *MenuBuilder) WithParamOids(oids ...string) *MenuBuilder {
	if m.err != nil {
		return m
	}
	m.proto.ParamOids = oids
	return m
}

// WithCommandOids sets the menu's command members, replacing any existing oids.
func (m *MenuBuilder) WithCommandOids(oids ...string) *MenuBuilder {
	if m.err != nil {
		return m
	}
	m.proto.CommandOids = oids
	return m
}

// WithClientHint sets a single client hint key/value pair, creating the map if
// it does not yet exist.
func (m *MenuBuilder) WithClientHint(key, value string) *MenuBuilder {
	if m.err != nil {
		return m
	}
	if m.proto.ClientHints == nil {
		m.proto.ClientHints = map[string]string{}
	}
	m.proto.ClientHints[key] = value
	return m
}

// WithOrder sets the menu's display order.
func (m *MenuBuilder) WithOrder(order uint32) *MenuBuilder {
	if m.err != nil {
		return m
	}
	m.proto.Order = order
	return m
}

// Build returns a usable Menu from the builder. If the builder has an error, it
// returns nil and the error. Must be called after all builder methods have been called.
func (m *MenuBuilder) Build() (*Menu, error) {
	if m.err != nil {
		return nil, m.err
	}
	return &Menu{Proto: m.proto}, nil
}

// Err returns the stored error of the builder.
func (m *MenuBuilder) Err() error {
	return m.err
}

// Wrapper around Build that panics if the builder has an error.
func (m *MenuBuilder) MakeMenu() *Menu {
	p, err := m.Build()
	if err != nil {
		panic(err)
	}
	return p
}
