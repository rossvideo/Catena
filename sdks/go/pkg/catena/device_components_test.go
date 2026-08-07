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
 * @brief Tests for DeviceComponent chunking business logic.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-08-07
 * @file device_components_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"errors"
	"fmt"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestDeviceComponentsForRequest(t *testing.T) {
	// componentTestDevice carries at least one of every chunkable piece:
	// two params, a command, a shared constraint, a menu group with two
	// menus, and a language pack.
	componentTestDevice := func() *st2138.Device {
		return st2138.NewDevice(3).
			WithDetailLevel(st2138.DetailLevelFull).
			WithMultiSetEnabled(true).
			WithAccessScopes("st2138:mon", "st2138:cfg").
			WithDefaultScope("st2138:mon").
			WithParam("alpha", st2138.NewParamInt32(1)).
			WithParam("parent", st2138.NewParamStruct().
				WithParam("child", st2138.NewParamString("x"))).
			WithCommand("go", st2138.NewParamEmpty()).
			WithConstraint("percent", st2138.NewConstraintInt32Range(0, 100, 1)).
			WithMenuGroup("status", st2138.NewMenuGroup().
				WithName(st2138.NewPolyglotText("en", "Status")).
				WithOrder(2).
				WithMenu("advanced", st2138.NewMenu().WithParamOids("parent")).
				WithMenu("overview", st2138.NewMenu().WithParamOids("alpha"))).
			WithLanguagePack("es", "Global Spanish", map[string]string{"greeting": "Hola"})
	}

	// Skeleton first, then each component kind sorted by oid.
	expectedOrder := []string{
		"device",
		"constraint:percent",
		"menu:status/advanced",
		"menu:status/overview",
		"param:alpha",
		"param:parent",
		"command:go",
		"pack:es",
	}

	t.Run("FullDecomposition", func(t *testing.T) {
		device := componentTestDevice()
		stream := &sliceStream[st2138.DeviceComponent]{}
		res := DeviceComponentsForRequest(device, stream)
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v", res)
		}
		assertComponentOrder(t, stream.Items, expectedOrder)

		skeleton := stream.Items[0].Proto.GetDevice()
		if skeleton.GetSlot() != 3 {
			t.Errorf("expected skeleton slot 3, got %d", skeleton.GetSlot())
		}
		if skeleton.GetDefaultScope() != "st2138:mon" {
			t.Errorf("expected skeleton to keep default scope, got %q", skeleton.GetDefaultScope())
		}
		if len(skeleton.GetParams()) != 0 || len(skeleton.GetCommands()) != 0 {
			t.Errorf("expected skeleton without params/commands, got %d/%d",
				len(skeleton.GetParams()), len(skeleton.GetCommands()))
		}
		if len(skeleton.GetConstraints()) != 0 || skeleton.GetLanguagePacks() != nil {
			t.Error("expected skeleton without constraints or language packs")
		}
		group := skeleton.GetMenuGroups()["status"]
		if group.GetOrder() != 2 || group.GetName().GetDisplayStrings()["en"] != "Status" {
			t.Errorf("expected menu-group shell to keep name and order, got %v", group)
		}
		if len(group.GetMenus()) != 0 {
			t.Errorf("expected menu-group shell without menus, got %d", len(group.GetMenus()))
		}

		// Component chunks reference the device's protos, not copies.
		if stream.Items[4].Proto.GetParam().GetParam() != device.Proto.GetParams()["alpha"] {
			t.Error("expected param chunk to reference the device's param proto")
		}

		// The source device is left intact, including the menus stripped from
		// the skeleton clone.
		if len(device.Proto.GetParams()) != 2 || len(device.Proto.GetCommands()) != 1 {
			t.Error("expected source device params/commands to be untouched")
		}
		if len(device.Proto.GetConstraints()) != 1 || device.Proto.GetLanguagePacks() == nil {
			t.Error("expected source device constraints/language packs to be untouched")
		}
		if len(device.Proto.GetMenuGroups()["status"].GetMenus()) != 2 {
			t.Error("expected source device menus to be untouched")
		}
	})

	t.Run("SkeletonOnly", func(t *testing.T) {
		// A device with nothing to chunk still yields its skeleton.
		stream := &sliceStream[st2138.DeviceComponent]{}
		res := DeviceComponentsForRequest(st2138.NewDevice(1), stream)
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v", res)
		}
		assertComponentOrder(t, stream.Items, []string{"device"})
	})

	t.Run("NilDevice", func(t *testing.T) {
		for _, device := range []*st2138.Device{nil, {}} {
			stream := &sliceStream[st2138.DeviceComponent]{}
			res := DeviceComponentsForRequest(device, stream)
			if res.Code != StatusCodeInternal {
				t.Fatalf("expected INTERNAL, got %v", res)
			}
			if len(stream.Items) != 0 {
				t.Fatalf("expected no chunks, got %d", len(stream.Items))
			}
		}
	})

	t.Run("NilEntriesSkipped", func(t *testing.T) {
		// Nil map values must be skipped, not sent or dereferenced.
		device := st2138.NewDevice(0)
		device.Proto.Params = map[string]*protos.Param{"ghost": nil}
		device.Proto.Commands = map[string]*protos.Param{"ghost": nil}
		device.Proto.Constraints = map[string]*protos.Constraint{"ghost": nil}
		device.Proto.MenuGroups = map[string]*protos.MenuGroup{
			"empty": nil,
			"holed": {Menus: map[string]*protos.Menu{"ghost": nil}},
		}
		device.Proto.LanguagePacks = &protos.LanguagePacks{
			Packs: map[string]*protos.LanguagePack{"ghost": nil},
		}

		stream := &sliceStream[st2138.DeviceComponent]{}
		res := DeviceComponentsForRequest(device, stream)
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v", res)
		}
		assertComponentOrder(t, stream.Items, []string{"device"})
	})

	t.Run("SendErrors", func(t *testing.T) {
		// Fail each of the eight Sends in turn: the walk must stop at the
		// failure and report INTERNAL, so every send site's error return is
		// exercised.
		for failAfter := range expectedOrder {
			stream := &sliceStream[st2138.DeviceComponent]{Err: errors.New("boom"), FailAfter: failAfter}
			res := DeviceComponentsForRequest(componentTestDevice(), stream)
			if res.Code != StatusCodeInternal {
				t.Fatalf("FailAfter %d: expected INTERNAL from a failed Send, got %v", failAfter, res)
			}
			if len(stream.Items) != failAfter {
				t.Errorf("FailAfter %d: expected %d chunks before the failure, got %d",
					failAfter, failAfter, len(stream.Items))
			}
		}
	})
}

// assertComponentOrder checks that chunks arrive as the expected kind:oid
// descriptions, in order.
func assertComponentOrder(t *testing.T, chunks []st2138.DeviceComponent, expected []string) {
	t.Helper()

	if len(chunks) != len(expected) {
		t.Fatalf("expected %d chunks, got %d", len(expected), len(chunks))
	}
	for i, want := range expected {
		if got := describeComponent(chunks[i]); got != want {
			t.Errorf("expected chunk[%d] %q, got %q", i, want, got)
		}
	}
}

// describeComponent renders a chunk as "kind" or "kind:oid" for compact
// order assertions.
func describeComponent(chunk st2138.DeviceComponent) string {
	switch kind := chunk.Proto.GetKind().(type) {
	case *protos.DeviceComponent_Device:
		return "device"
	case *protos.DeviceComponent_SharedConstraint:
		return "constraint:" + kind.SharedConstraint.GetOid()
	case *protos.DeviceComponent_Menu:
		return "menu:" + kind.Menu.GetOid()
	case *protos.DeviceComponent_Param:
		return "param:" + kind.Param.GetOid()
	case *protos.DeviceComponent_Command:
		return "command:" + kind.Command.GetOid()
	case *protos.DeviceComponent_LanguagePack:
		return "pack:" + kind.LanguagePack.GetLanguage()
	default:
		return fmt.Sprintf("unknown:%T", kind)
	}
}
