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
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// MenuGroup wraps a protos.MenuGroup and exposes a fluent builder API.
// The group's oid is supplied by its parent when the group is attached, so it
// is not stored here.
type MenuGroup struct {
	Proto *protos.MenuGroup
}

// NewMenuGroup creates a MenuGroup with an initialized proto.
func NewMenuGroup() *MenuGroup {
	return &MenuGroup{
		Proto: &protos.MenuGroup{
			Menus: make(map[string]*protos.Menu),
		},
	}
}

// WithName sets the menu group's display name, replacing any existing name.
func (mg *MenuGroup) WithName(name PolyglotText) *MenuGroup {
	mg.Proto.Name = &protos.PolyglotText{DisplayStrings: name}
	return mg
}

// WithMenu inserts menu into the menu group's menus map, keyed by oid.
// Replaces any existing menu at that oid. A nil menu is ignored.
func (mg *MenuGroup) WithMenu(oid string, menu *Menu) *MenuGroup {
	if menu == nil {
		logger.Warning("WithMenu called with nil menu; ignoring", "oid", oid)
		return mg
	}
	if mg.Proto.Menus == nil {
		mg.Proto.Menus = map[string]*protos.Menu{}
	}
	mg.Proto.Menus[oid] = menu.Proto
	return mg
}

// WithOrder sets the menu group's display order.
func (mg *MenuGroup) WithOrder(order uint32) *MenuGroup {
	mg.Proto.Order = order
	return mg
}

// Menu wraps a protos.Menu and exposes a fluent builder API.
// The menu's oid is supplied by its parent when the menu is attached, so it is
// not stored here.
type Menu struct {
	Proto *protos.Menu
}

// NewMenu creates a Menu with an initialized proto.
func NewMenu() *Menu {
	return &Menu{
		Proto: &protos.Menu{},
	}
}

// WithName sets the menu's display name, replacing any existing name.
func (m *Menu) WithName(name PolyglotText) *Menu {
	m.Proto.Name = &protos.PolyglotText{DisplayStrings: name}
	return m
}

// WithHidden sets whether the menu should be hidden in the client GUI.
func (m *Menu) WithHidden(hidden bool) *Menu {
	m.Proto.Hidden = hidden
	return m
}

// WithDisabled sets whether the menu should be disabled (shown as read-only)
// in the client GUI.
func (m *Menu) WithDisabled(disabled bool) *Menu {
	m.Proto.Disabled = disabled
	return m
}

// WithParamOids sets the menu's parameter members, replacing any existing oids.
func (m *Menu) WithParamOids(oids ...string) *Menu {
	m.Proto.ParamOids = oids
	return m
}

// WithCommandOids sets the menu's command members, replacing any existing oids.
func (m *Menu) WithCommandOids(oids ...string) *Menu {
	m.Proto.CommandOids = oids
	return m
}

// WithClientHint sets a single client hint key/value pair, creating the map if
// it does not yet exist.
func (m *Menu) WithClientHint(key, value string) *Menu {
	if m.Proto.ClientHints == nil {
		m.Proto.ClientHints = map[string]string{}
	}
	m.Proto.ClientHints[key] = value
	return m
}

// WithOrder sets the menu's display order.
func (m *Menu) WithOrder(order uint32) *Menu {
	m.Proto.Order = order
	return m
}
