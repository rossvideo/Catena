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

// Package plugin defines the interface for Catena business logic plugins.
//
// Plugins are compiled as Go shared objects (.so files) and loaded at runtime
// by the catena executable. Each plugin is automatically assigned a slot by
// catena, and requests to that slot are routed to the plugin's handlers.
//
// To create a plugin:
//  1. Create a Go file with package main
//  2. Export handler variables using the rest.* types
//  3. Optionally export Init and Shutdown functions for lifecycle hooks
//  4. Build with: go build -buildmode=plugin -o myplugin.so myplugin.go
//
// Example plugin:
//
//	package main
//
//	import (
//	    "github.com/rossvideo/catena/sdks/go/pkg/catena"
//	    "github.com/rossvideo/catena/sdks/go/pkg/plugin"
//	    "github.com/rossvideo/catena/sdks/go/pkg/rest"
//	)
//
//	var PluginInfo = plugin.PluginInfo{Name: "MyPlugin", Version: "1.0.0"}
//
//	func Init(ctx *plugin.Context) error { return nil }
//
//	var GetValueHandler rest.GetValueHandler = func(fqoid string) (catena.CatenaValue, catena.StatusResult) {
//	    // Handle get value requests
//	}
//
//	var SetValueHandler rest.SetValueHandler = func(value any, fqoid string) catena.StatusResult {
//	    // Handle set value requests
//	}
package plugin

import (
	"fmt"
	"plugin"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// Context provides runtime context to plugins during initialization.
type Context struct {
	// Config is the parsed Catena configuration
	Config catena.Config

	// ShutdownChan is closed when the server is shutting down.
	// Plugins can use this to clean up background goroutines.
	ShutdownChan <-chan struct{}

	// Slot is the slot number assigned to this plugin by catena
	Slot int

	// Args contains the arguments passed to this plugin from the YAML config.
	// Keys are argument names (e.g., "--tls"), values are argument values.
	Args map[string]string
}

// InitFunc is called when the plugin is loaded, before any handlers are invoked.
type InitFunc func(ctx *Context) error

// ShutdownFunc is called during server shutdown to allow cleanup.
type ShutdownFunc func() error

// PluginInfo contains optional metadata about the plugin.
type PluginInfo struct {
	Name        string
	Version     string
	Description string
}

// LoadedPlugin represents a plugin that has been loaded and is ready for use.
type LoadedPlugin struct {
	Path string
	Info PluginInfo
	Slot int // Assigned slot number

	// Lifecycle hooks
	Init     InitFunc
	Shutdown ShutdownFunc

	// Handlers (nil if not exported by the plugin)
	GetDevice      rest.DeviceHandler
	GetValue       rest.GetValueHandler
	SetValue       rest.SetValueHandler
	GetAsset       rest.GetAssetHandler
	ExecuteCommand rest.ExecuteCommandHandler
	Fallback       rest.FallbackHandler
}

// Load loads a plugin from the specified .so file path.
// It looks up all exported handler symbols (exported as variables with rest.* types).
func Load(path string) (*LoadedPlugin, error) {
	p, err := plugin.Open(path)
	if err != nil {
		return nil, fmt.Errorf("failed to open plugin %s: %w", path, err)
	}

	loaded := &LoadedPlugin{Path: path}

	// Look up optional PluginInfo
	if infoSym, err := p.Lookup("PluginInfo"); err == nil {
		if info, ok := infoSym.(*PluginInfo); ok {
			loaded.Info = *info
		}
	}

	// Look up optional Init function
	if sym, err := p.Lookup("Init"); err == nil {
		if fn, ok := sym.(func(*Context) error); ok {
			loaded.Init = fn
		}
	}

	// Look up optional Shutdown function
	if sym, err := p.Lookup("Shutdown"); err == nil {
		if fn, ok := sym.(func() error); ok {
			loaded.Shutdown = fn
		}
	}

	// Look up handler variables (exported as var with rest.* types)

	if sym, err := p.Lookup("GetDeviceHandler"); err == nil {
		if fn, ok := sym.(*rest.DeviceHandler); ok {
			loaded.GetDevice = *fn
		}
	}

	if sym, err := p.Lookup("GetValueHandler"); err == nil {
		if fn, ok := sym.(*rest.GetValueHandler); ok {
			loaded.GetValue = *fn
		}
	}

	if sym, err := p.Lookup("SetValueHandler"); err == nil {
		if fn, ok := sym.(*rest.SetValueHandler); ok {
			loaded.SetValue = *fn
		}
	}

	if sym, err := p.Lookup("GetAssetHandler"); err == nil {
		if fn, ok := sym.(*rest.GetAssetHandler); ok {
			loaded.GetAsset = *fn
		}
	}

	if sym, err := p.Lookup("ExecuteCommandHandler"); err == nil {
		if fn, ok := sym.(*rest.ExecuteCommandHandler); ok {
			loaded.ExecuteCommand = *fn
		}
	}

	if sym, err := p.Lookup("FallbackHandler"); err == nil {
		if fn, ok := sym.(*rest.FallbackHandler); ok {
			loaded.Fallback = *fn
		}
	}

	return loaded, nil
}

// HasAnyHandler returns true if the plugin exports at least one handler.
func (p *LoadedPlugin) HasAnyHandler() bool {
	return p.GetDevice != nil ||
		p.GetValue != nil ||
		p.SetValue != nil ||
		p.GetAsset != nil ||
		p.ExecuteCommand != nil ||
		p.Fallback != nil
}
