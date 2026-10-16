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
 * @brief Tests for Menu and MenuGroup builder types.
 * @file menu_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-06-01
 */

package st2138

import (
	"errors"
	"testing"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewMenuGroup(t *testing.T) {
	mg := NewMenuGroup()
	if mg.proto == nil {
		t.Fatal("expected non-nil proto")
	}
	if mg.proto.Menus == nil {
		t.Fatal("expected initialized menus map")
	}
	if len(mg.proto.Menus) != 0 {
		t.Errorf("expected empty menus map, got %d entries", len(mg.proto.Menus))
	}
}

func TestMenuGroup_WithName(t *testing.T) {
	mg := NewMenuGroup().WithName(NewPolyglotText("en", "Settings"))
	ds := mg.proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
}

func TestMenuGroup_WithName_Replaces(t *testing.T) {
	mg := NewMenuGroup().
		WithName(NewPolyglotText("en", "Old").With("fr", "Ancien")).
		WithName(NewPolyglotText("en", "New"))

	ds := mg.proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
	if _, ok := ds["fr"]; ok {
		t.Errorf("expected fr to be absent after replacement, got %s", ds["fr"])
	}
}

// TestMenuGroup_WithName_LoopBuiltName demonstrates building a name up in a
// loop via an empty PolyglotText and passing it in, replacing the old
// AddMenuGroupName merge helper.
func TestMenuGroup_WithName_LoopBuiltName(t *testing.T) {
	name := NewPolyglotText()
	for lang, text := range map[string]string{"en": "Settings", "fr": "Paramètres"} {
		name.With(lang, text)
	}
	mg := NewMenuGroup().WithName(name)

	ds := mg.proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
	if ds["fr"] != "Paramètres" {
		t.Errorf("expected 'Paramètres', got %s", ds["fr"])
	}
}

func TestMenuGroup_WithName_NestedErrors(t *testing.T) {
	mg, err := NewMenuGroup().WithName(NewPolyglotText("en", "test", "fail")).Build()
	if mg != (MenuGroup{}) {
		t.Errorf("expected nil menu group, got %v", mg)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}

func TestMenuGroup_WithName_EmptyPolyglotText(t *testing.T) {
	mg, err := NewMenuGroup().WithName(NewPolyglotText()).Build()
	if mg != (MenuGroup{}) {
		t.Errorf("expected nil menu group, got %v", mg)
	}
	if err == nil {
		t.Error("expected error for builder with empty polyglot text")
	}
}

func TestMenuGroup_WithMenu(t *testing.T) {
	mg := NewMenuGroup().WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video")))

	menu, ok := mg.proto.Menus["video"]
	if !ok {
		t.Fatal("expected menu 'video' to exist")
	}
	ds := menu.Name.GetDisplayStrings()
	if ds["en"] != "Video" {
		t.Errorf("expected 'Video', got %s", ds["en"])
	}
}

func TestMenuGroup_WithMenu_Replaces(t *testing.T) {
	mg := NewMenuGroup().
		WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Old Video"))).
		WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "New Video")))

	ds := mg.proto.Menus["video"].Name.GetDisplayStrings()
	if ds["en"] != "New Video" {
		t.Errorf("expected 'New Video', got %s", ds["en"])
	}
}

func TestMenuGroup_WithMenu_NilMapInitialized(t *testing.T) {
	mg := &MenuGroupBuilder{proto: &protos.MenuGroup{}}
	if mg.proto.Menus != nil {
		t.Fatal("expected nil menus map before WithMenu")
	}

	mg.WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video")))

	if mg.proto.Menus == nil {
		t.Fatal("expected menus map to be initialized by WithMenu")
	}
	if _, ok := mg.proto.Menus["video"]; !ok {
		t.Error("expected menu 'video' to exist")
	}
}

func TestMenuGroup_WithMenu_Nil(t *testing.T) {
	mg, err := NewMenuGroup().WithMenu("video", nil).Build()
	if err == nil {
		t.Fatal("expected error, got nil")
	}
	if mg != (MenuGroup{}) {
		t.Fatal("expected nil menu group")
	}
}

func TestMenuGroup_WithMenu_NestedErrors(t *testing.T) {
	mg, err := NewMenuGroup().WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "test", "fail"))).Build()
	if mg != (MenuGroup{}) {
		t.Errorf("expected nil menu group, got %v", mg)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}

func TestMenuGroup_WithMenu_EmptyMenu(t *testing.T) {
	mg, err := NewMenuGroup().WithMenu("video", &MenuBuilder{}).Build()
	if mg != (MenuGroup{}) {
		t.Errorf("expected nil menu group, got %v", mg)
	}
	if err == nil {
		t.Error("expected error for builder with empty menu")
	}
}

func TestMenuGroup_MultipleMenus(t *testing.T) {
	mg := NewMenuGroup().
		WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video"))).
		WithMenu("audio", NewMenu().WithName(NewPolyglotText("en", "Audio")))

	if len(mg.proto.Menus) != 2 {
		t.Errorf("expected 2 menus, got %d", len(mg.proto.Menus))
	}
	if mg.proto.Menus["video"].Name.GetDisplayStrings()["en"] != "Video" {
		t.Error("expected video menu name 'Video'")
	}
	if mg.proto.Menus["audio"].Name.GetDisplayStrings()["en"] != "Audio" {
		t.Error("expected audio menu name 'Audio'")
	}
}

func TestMenuGroup_WithOrder(t *testing.T) {
	mg := NewMenuGroup().WithOrder(3)
	if mg.proto.GetOrder() != 3 {
		t.Errorf("expected order 3, got %d", mg.proto.GetOrder())
	}
}

func TestMenuGroup_FullChaining(t *testing.T) {
	mg := NewMenuGroup().
		WithName(NewPolyglotText("en", "Main Group").With("fr", "Groupe Principal")).
		WithOrder(1).
		WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video"))).
		WithMenu("audio", NewMenu().WithName(NewPolyglotText("en", "Audio").With("fr", "Son")))

	ds := mg.proto.Name.GetDisplayStrings()
	if ds["en"] != "Main Group" {
		t.Errorf("expected 'Main Group', got %s", ds["en"])
	}
	if ds["fr"] != "Groupe Principal" {
		t.Errorf("expected 'Groupe Principal', got %s", ds["fr"])
	}
	if mg.proto.GetOrder() != 1 {
		t.Errorf("expected order 1, got %d", mg.proto.GetOrder())
	}

	if len(mg.proto.Menus) != 2 {
		t.Fatalf("expected 2 menus, got %d", len(mg.proto.Menus))
	}
	if mg.proto.Menus["audio"].Name.GetDisplayStrings()["fr"] != "Son" {
		t.Errorf("expected audio fr='Son', got %s", mg.proto.Menus["audio"].Name.GetDisplayStrings()["fr"])
	}
}

func TestMenuGroup_Build(t *testing.T) {
	m, err := NewMenuGroup().WithName(NewPolyglotText("en", "Video")).Build()
	if err != nil {
		t.Fatalf("NewMenuGroup().WithName(NewPolyglotText('en', 'Video')).Build() failed: %v", err)
	}
	if m == (MenuGroup{}) {
		t.Fatal("expected non-nil menu group")
	}
	if m.Proto == nil {
		t.Fatal("expected non-nil menu group proto")
	}
}

func TestMenuGroup_BuildSurfacesError(t *testing.T) {
	m, err := NewMenuGroup().WithName(nil).Build()
	if err == nil {
		t.Fatal("expected error, got nil")
	}
	if m != (MenuGroup{}) {
		t.Fatal("expected nil menu group")
	}
}

func TestMenuGroupBuilder_WithShortCircuitErrors(t *testing.T) {
	mg := NewMenuGroup()
	mg.err = errors.New("test error")
	before := proto.Clone(mg.proto)

	group, err := mg.
		WithName(NewPolyglotText("en", "Status")).
		WithMenu("overview", NewMenu().WithParamOids("alpha")).
		WithOrder(2).
		Build()
	if err == nil || err.Error() != "test error" {
		t.Fatalf("expected 'test error', got %v", err)
	}
	if group != (MenuGroup{}) {
		t.Fatal("expected nil menu group for builder with error")
	}
	if !proto.Equal(before, mg.proto) {
		t.Fatal("expected With* to leave the menu group unchanged after an error")
	}
}

func TestMenuGroupBuilder_Err(t *testing.T) {
	mg := NewMenuGroup()
	mg.err = errors.New("test error")
	if mg.Err() == nil {
		t.Fatal("expected error for builder with error")
	}
	if mg.Err().Error() != "test error" {
		t.Fatal("expected error message 'test error', got", mg.Err())
	}
}

func TestMenuGroup_WrapMenuGroup(t *testing.T) {
	proto := &protos.MenuGroup{
		Name: &protos.PolyglotText{
			DisplayStrings: map[string]string{"en": "Video"},
		},
	}
	mg := &MenuGroupBuilder{proto: proto}
	if mg.proto == nil {
		t.Fatal("expected non-nil proto")
	}
	if mg.proto.Name.GetDisplayStrings()["en"] != "Video" {
		t.Errorf("expected 'Video', got %s", mg.proto.Name.GetDisplayStrings()["en"])
	}
}

func TestNewMenu(t *testing.T) {
	m := NewMenu()
	if m.proto == nil {
		t.Fatal("expected non-nil proto")
	}
}

func TestMenu_WithName(t *testing.T) {
	m := NewMenu().WithName(NewPolyglotText("en", "Video Settings"))
	ds := m.proto.Name.GetDisplayStrings()
	if ds["en"] != "Video Settings" {
		t.Errorf("expected 'Video Settings', got %s", ds["en"])
	}
}

func TestMenu_WithName_Replaces(t *testing.T) {
	m := NewMenu().
		WithName(NewPolyglotText("en", "Old").With("fr", "Ancien")).
		WithName(NewPolyglotText("en", "New"))

	ds := m.proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
	if _, ok := ds["fr"]; ok {
		t.Errorf("expected fr to be absent after replacement, got %s", ds["fr"])
	}
}

func TestMenu_WithName_NestedErrors(t *testing.T) {
	m, err := NewMenu().WithName(NewPolyglotText("en", "test", "fail")).Build()
	if m != (Menu{}) {
		t.Errorf("expected nil menu, got %v", m)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}
func TestMenu_WithName_EmptyPolyglotText(t *testing.T) {
	m, err := NewMenu().WithName(NewPolyglotText()).Build()
	if m != (Menu{}) {
		t.Errorf("expected nil menu, got %v", m)
	}
	if err == nil {
		t.Error("expected error for builder with empty polyglot text")
	}
}

func TestMenu_WithFields(t *testing.T) {
	m := NewMenu().
		WithName(NewPolyglotText("en", "Video")).
		WithHidden(true).
		WithDisabled(true).
		WithParamOids("brightness", "contrast").
		WithCommandOids("reboot").
		WithClientHint("ui-url", "https://example.com").
		WithOrder(2)

	if !m.proto.GetHidden() {
		t.Error("expected hidden to be true")
	}
	if !m.proto.GetDisabled() {
		t.Error("expected disabled to be true")
	}
	if got := m.proto.GetParamOids(); len(got) != 2 || got[0] != "brightness" || got[1] != "contrast" {
		t.Errorf("expected param_oids [brightness contrast], got %v", got)
	}
	if got := m.proto.GetCommandOids(); len(got) != 1 || got[0] != "reboot" {
		t.Errorf("expected command_oids [reboot], got %v", got)
	}
	if m.proto.GetClientHints()["ui-url"] != "https://example.com" {
		t.Errorf("expected client hint ui-url, got %v", m.proto.GetClientHints())
	}
	if m.proto.GetOrder() != 2 {
		t.Errorf("expected order 2, got %d", m.proto.GetOrder())
	}
}

func TestMenu_WithClientHint_Multiple(t *testing.T) {
	m := NewMenu().
		WithClientHint("a", "1").
		WithClientHint("b", "2")

	hints := m.proto.GetClientHints()
	if hints["a"] != "1" || hints["b"] != "2" {
		t.Errorf("expected both client hints, got %v", hints)
	}
}

func TestMenu_Build(t *testing.T) {
	m, err := NewMenu().WithName(NewPolyglotText("en", "Video")).Build()
	if err != nil {
		t.Fatalf("NewMenu().WithName(NewPolyglotText('en', 'Video')).Build() failed: %v", err)
	}
	if m == (Menu{}) {
		t.Fatal("expected non-nil menu")
	}
	if m.Proto == nil {
		t.Fatal("expected non-nil proto")
	}
}

func TestMenu_BuildSurfacesError(t *testing.T) {
	m, err := NewMenu().WithName(nil).Build()
	if err == nil {
		t.Fatal("expected error, got nil")
	}
	if m != (Menu{}) {
		t.Fatal("expected nil menu")
	}
}

func TestMenuBuilder_WithShortCircuitErrors(t *testing.T) {
	m := NewMenu()
	m.err = errors.New("test error")
	before := proto.Clone(m.proto)

	menu, err := m.
		WithName(NewPolyglotText("en", "Video")).
		WithHidden(true).
		WithDisabled(true).
		WithParamOids("brightness").
		WithCommandOids("reboot").
		WithClientHint("ui-url", "https://example.com").
		WithOrder(2).
		Build()
	if err == nil || err.Error() != "test error" {
		t.Fatalf("expected 'test error', got %v", err)
	}
	if menu != (Menu{}) {
		t.Fatal("expected nil menu for builder with error")
	}
	if !proto.Equal(before, m.proto) {
		t.Fatal("expected With* to leave the menu unchanged after an error")
	}
}

func TestMenuBuilder_Err(t *testing.T) {
	m := NewMenu()
	m.err = errors.New("test error")
	if m.Err() == nil {
		t.Fatal("expected error for builder with error")
	}
	if m.Err().Error() != "test error" {
		t.Fatal("expected error message 'test error', got", m.Err())
	}
}
