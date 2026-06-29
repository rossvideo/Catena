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

package catena

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewMenuGroup(t *testing.T) {
	mg := NewMenuGroup()
	if mg.Proto == nil {
		t.Fatal("expected non-nil proto")
	}
	if mg.Proto.Menus == nil {
		t.Fatal("expected initialized menus map")
	}
	if len(mg.Proto.Menus) != 0 {
		t.Errorf("expected empty menus map, got %d entries", len(mg.Proto.Menus))
	}
}

func TestMenuGroup_WithName(t *testing.T) {
	mg := NewMenuGroup().WithName(NewPolyglotText("en", "Settings"))
	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
}

func TestMenuGroup_WithName_Replaces(t *testing.T) {
	mg := NewMenuGroup().
		WithName(NewPolyglotText("en", "Old").With("fr", "Ancien")).
		WithName(NewPolyglotText("en", "New"))

	ds := mg.Proto.Name.GetDisplayStrings()
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

	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
	if ds["fr"] != "Paramètres" {
		t.Errorf("expected 'Paramètres', got %s", ds["fr"])
	}
}

func TestMenuGroup_WithMenu(t *testing.T) {
	mg := NewMenuGroup().WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video")))

	menu, ok := mg.Proto.Menus["video"]
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

	ds := mg.Proto.Menus["video"].Name.GetDisplayStrings()
	if ds["en"] != "New Video" {
		t.Errorf("expected 'New Video', got %s", ds["en"])
	}
}

func TestMenuGroup_WithMenu_NilMapInitialized(t *testing.T) {
	mg := &MenuGroup{Proto: &protos.MenuGroup{}}
	if mg.Proto.Menus != nil {
		t.Fatal("expected nil menus map before WithMenu")
	}

	mg.WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video")))

	if mg.Proto.Menus == nil {
		t.Fatal("expected menus map to be initialized by WithMenu")
	}
	if _, ok := mg.Proto.Menus["video"]; !ok {
		t.Error("expected menu 'video' to exist")
	}
}

func TestMenuGroup_WithMenu_NilIgnored(t *testing.T) {
	mg := NewMenuGroup().WithMenu("video", nil)
	if len(mg.Proto.Menus) != 0 {
		t.Errorf("expected nil menu to be ignored, got %d entries", len(mg.Proto.Menus))
	}
}

func TestMenuGroup_MultipleMenus(t *testing.T) {
	mg := NewMenuGroup().
		WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video"))).
		WithMenu("audio", NewMenu().WithName(NewPolyglotText("en", "Audio")))

	if len(mg.Proto.Menus) != 2 {
		t.Errorf("expected 2 menus, got %d", len(mg.Proto.Menus))
	}
	if mg.Proto.Menus["video"].Name.GetDisplayStrings()["en"] != "Video" {
		t.Error("expected video menu name 'Video'")
	}
	if mg.Proto.Menus["audio"].Name.GetDisplayStrings()["en"] != "Audio" {
		t.Error("expected audio menu name 'Audio'")
	}
}

func TestMenuGroup_WithOrder(t *testing.T) {
	mg := NewMenuGroup().WithOrder(3)
	if mg.Proto.GetOrder() != 3 {
		t.Errorf("expected order 3, got %d", mg.Proto.GetOrder())
	}
}

func TestMenuGroup_FullChaining(t *testing.T) {
	mg := NewMenuGroup().
		WithName(NewPolyglotText("en", "Main Group").With("fr", "Groupe Principal")).
		WithOrder(1).
		WithMenu("video", NewMenu().WithName(NewPolyglotText("en", "Video"))).
		WithMenu("audio", NewMenu().WithName(NewPolyglotText("en", "Audio").With("fr", "Son")))

	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Main Group" {
		t.Errorf("expected 'Main Group', got %s", ds["en"])
	}
	if ds["fr"] != "Groupe Principal" {
		t.Errorf("expected 'Groupe Principal', got %s", ds["fr"])
	}
	if mg.Proto.GetOrder() != 1 {
		t.Errorf("expected order 1, got %d", mg.Proto.GetOrder())
	}

	if len(mg.Proto.Menus) != 2 {
		t.Fatalf("expected 2 menus, got %d", len(mg.Proto.Menus))
	}
	if mg.Proto.Menus["audio"].Name.GetDisplayStrings()["fr"] != "Son" {
		t.Errorf("expected audio fr='Son', got %s", mg.Proto.Menus["audio"].Name.GetDisplayStrings()["fr"])
	}
}

func TestNewMenu(t *testing.T) {
	m := NewMenu()
	if m.Proto == nil {
		t.Fatal("expected non-nil proto")
	}
}

func TestMenu_WithName(t *testing.T) {
	m := NewMenu().WithName(NewPolyglotText("en", "Video Settings"))
	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Video Settings" {
		t.Errorf("expected 'Video Settings', got %s", ds["en"])
	}
}

func TestMenu_WithName_Replaces(t *testing.T) {
	m := NewMenu().
		WithName(NewPolyglotText("en", "Old").With("fr", "Ancien")).
		WithName(NewPolyglotText("en", "New"))

	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
	if _, ok := ds["fr"]; ok {
		t.Errorf("expected fr to be absent after replacement, got %s", ds["fr"])
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

	if !m.Proto.GetHidden() {
		t.Error("expected hidden to be true")
	}
	if !m.Proto.GetDisabled() {
		t.Error("expected disabled to be true")
	}
	if got := m.Proto.GetParamOids(); len(got) != 2 || got[0] != "brightness" || got[1] != "contrast" {
		t.Errorf("expected param_oids [brightness contrast], got %v", got)
	}
	if got := m.Proto.GetCommandOids(); len(got) != 1 || got[0] != "reboot" {
		t.Errorf("expected command_oids [reboot], got %v", got)
	}
	if m.Proto.GetClientHints()["ui-url"] != "https://example.com" {
		t.Errorf("expected client hint ui-url, got %v", m.Proto.GetClientHints())
	}
	if m.Proto.GetOrder() != 2 {
		t.Errorf("expected order 2, got %d", m.Proto.GetOrder())
	}
}

func TestMenu_WithClientHint_Multiple(t *testing.T) {
	m := NewMenu().
		WithClientHint("a", "1").
		WithClientHint("b", "2")

	hints := m.Proto.GetClientHints()
	if hints["a"] != "1" || hints["b"] != "2" {
		t.Errorf("expected both client hints, got %v", hints)
	}
}
