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

package catena

import (
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// MenuGroup wraps a protos.MenuGroup and exposes a fluent builder API.
type MenuGroup struct {
	Oid   string
	Proto *protos.MenuGroup
}

// NewMenuGroup creates a MenuGroup with the given oid and an initialized proto.
func NewMenuGroup(oid string) *MenuGroup {
	return &MenuGroup{
		Oid: oid,
		Proto: &protos.MenuGroup{
			Menus: make(map[string]*protos.Menu),
		},
	}
}

// WithMenuGroupName sets the menu group's display name, replacing any existing name.
func (mg *MenuGroup) WithMenuGroupName(name PolyglotText) *MenuGroup {
	mg.Proto.Name = &protos.PolyglotText{DisplayStrings: name}
	return mg
}

// AddMenuGroupName merges entries from name into the menu group's existing display name.
// If no name exists yet, it is created.
func (mg *MenuGroup) AddMenuGroupName(name PolyglotText) *MenuGroup {
	if mg.Proto.Name == nil {
		mg.Proto.Name = &protos.PolyglotText{DisplayStrings: make(map[string]string)}
	}
	if mg.Proto.Name.DisplayStrings == nil {
		mg.Proto.Name.DisplayStrings = make(map[string]string)
	}
	for k, v := range name {
		mg.Proto.Name.DisplayStrings[k] = v
	}
	return mg
}

// WithMenu creates a new menu with the given name and inserts it into the
// menu group's menus map, keyed by oid. Replaces any existing menu at that oid.
func (mg *MenuGroup) WithMenu(oid string, name PolyglotText) *MenuGroup {
	mg.Proto.Menus[oid] = &protos.Menu{
		Name: &protos.PolyglotText{DisplayStrings: name},
	}
	return mg
}

// AddMenu creates a new menu with the given name and inserts it into the
// menu group's menus map, keyed by oid. Replaces any existing menu at that oid.
func (mg *MenuGroup) AddMenu(oid string, name PolyglotText) *MenuGroup {
	mg.Proto.Menus[oid] = &protos.Menu{
		Name: &protos.PolyglotText{DisplayStrings: name},
	}
	return mg
}

// Menu wraps a protos.Menu and exposes a fluent builder API.
type Menu struct {
	Oid   string
	Proto *protos.Menu
}

// NewMenu creates a Menu with the given oid and an initialized proto.
func NewMenu(oid string) *Menu {
	return &Menu{
		Oid:   oid,
		Proto: &protos.Menu{},
	}
}

// WithMenuName sets the menu's display name, replacing any existing name.
func (m *Menu) WithMenuName(name PolyglotText) *Menu {
	m.Proto.Name = &protos.PolyglotText{DisplayStrings: name}
	return m
}

// AddMenuName merges entries from name into the menu's existing display name.
// If no name exists yet, it is created.
func (m *Menu) AddMenuName(name PolyglotText) *Menu {
	if m.Proto.Name == nil {
		m.Proto.Name = &protos.PolyglotText{DisplayStrings: make(map[string]string)}
	}
	if m.Proto.Name.DisplayStrings == nil {
		m.Proto.Name.DisplayStrings = make(map[string]string)
	}
	for k, v := range name {
		m.Proto.Name.DisplayStrings[k] = v
	}
	return m
}
