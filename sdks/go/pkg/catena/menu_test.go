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
)

func TestNewMenuGroup(t *testing.T) {
	mg := NewMenuGroup("group1")
	if mg.Oid != "group1" {
		t.Errorf("expected oid 'group1', got %s", mg.Oid)
	}
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

func TestMenuGroup_WithMenuGroupName(t *testing.T) {
	mg := NewMenuGroup("g1").WithMenuGroupName(NewPolyglotText("en", "Settings"))
	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
}

func TestMenuGroup_WithMenuGroupName_Replaces(t *testing.T) {
	mg := NewMenuGroup("g1").
		WithMenuGroupName(NewPolyglotText("en", "Old").With("fr", "Ancien")).
		WithMenuGroupName(NewPolyglotText("en", "New"))

	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
	if _, ok := ds["fr"]; ok {
		t.Errorf("expected fr to be absent after replacement, got %s", ds["fr"])
	}
}

func TestMenuGroup_AddMenuGroupName_CreatesIfNil(t *testing.T) {
	mg := NewMenuGroup("g1").AddMenuGroupName(NewPolyglotText("en", "Settings"))
	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
}

func TestMenuGroup_AddMenuGroupName_Merges(t *testing.T) {
	mg := NewMenuGroup("g1").
		WithMenuGroupName(NewPolyglotText("en", "Settings")).
		AddMenuGroupName(NewPolyglotText("fr", "Paramètres"))

	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Settings" {
		t.Errorf("expected 'Settings', got %s", ds["en"])
	}
	if ds["fr"] != "Paramètres" {
		t.Errorf("expected 'Paramètres', got %s", ds["fr"])
	}
}

func TestMenuGroup_AddMenuGroupName_OverwritesExisting(t *testing.T) {
	mg := NewMenuGroup("g1").
		WithMenuGroupName(NewPolyglotText("en", "Old")).
		AddMenuGroupName(NewPolyglotText("en", "New"))

	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
}

func TestMenuGroup_WithMenu(t *testing.T) {
	mg := NewMenuGroup("g1").WithMenu("video", NewPolyglotText("en", "Video"))

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
	mg := NewMenuGroup("g1").
		WithMenu("video", NewPolyglotText("en", "Old Video")).
		WithMenu("video", NewPolyglotText("en", "New Video"))

	ds := mg.Proto.Menus["video"].Name.GetDisplayStrings()
	if ds["en"] != "New Video" {
		t.Errorf("expected 'New Video', got %s", ds["en"])
	}
}

func TestMenuGroup_AddMenu(t *testing.T) {
	mg := NewMenuGroup("g1").AddMenu("audio", NewPolyglotText("en", "Audio"))

	menu, ok := mg.Proto.Menus["audio"]
	if !ok {
		t.Fatal("expected menu 'audio' to exist")
	}
	ds := menu.Name.GetDisplayStrings()
	if ds["en"] != "Audio" {
		t.Errorf("expected 'Audio', got %s", ds["en"])
	}
}

func TestMenuGroup_MultipleMenus(t *testing.T) {
	mg := NewMenuGroup("g1").
		WithMenu("video", NewPolyglotText("en", "Video")).
		AddMenu("audio", NewPolyglotText("en", "Audio"))

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

func TestMenuGroup_FullChaining(t *testing.T) {
	mg := NewMenuGroup("main").
		WithMenuGroupName(NewPolyglotText("en", "Main Group")).
		AddMenuGroupName(NewPolyglotText("fr", "Groupe Principal")).
		WithMenu("video", NewPolyglotText("en", "Video")).
		AddMenu("audio", NewPolyglotText("en", "Audio").With("fr", "Son"))

	if mg.Oid != "main" {
		t.Errorf("expected oid 'main', got %s", mg.Oid)
	}

	ds := mg.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Main Group" {
		t.Errorf("expected 'Main Group', got %s", ds["en"])
	}
	if ds["fr"] != "Groupe Principal" {
		t.Errorf("expected 'Groupe Principal', got %s", ds["fr"])
	}

	if len(mg.Proto.Menus) != 2 {
		t.Fatalf("expected 2 menus, got %d", len(mg.Proto.Menus))
	}
	if mg.Proto.Menus["audio"].Name.GetDisplayStrings()["fr"] != "Son" {
		t.Errorf("expected audio fr='Son', got %s", mg.Proto.Menus["audio"].Name.GetDisplayStrings()["fr"])
	}
}

func TestNewMenu(t *testing.T) {
	m := NewMenu("video")
	if m.Oid != "video" {
		t.Errorf("expected oid 'video', got %s", m.Oid)
	}
	if m.Proto == nil {
		t.Fatal("expected non-nil proto")
	}
}

func TestMenu_WithMenuName(t *testing.T) {
	m := NewMenu("video").WithMenuName(NewPolyglotText("en", "Video Settings"))
	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Video Settings" {
		t.Errorf("expected 'Video Settings', got %s", ds["en"])
	}
}

func TestMenu_WithMenuName_Replaces(t *testing.T) {
	m := NewMenu("video").
		WithMenuName(NewPolyglotText("en", "Old").With("fr", "Ancien")).
		WithMenuName(NewPolyglotText("en", "New"))

	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
	if _, ok := ds["fr"]; ok {
		t.Errorf("expected fr to be absent after replacement, got %s", ds["fr"])
	}
}

func TestMenu_AddMenuName_CreatesIfNil(t *testing.T) {
	m := NewMenu("video").AddMenuName(NewPolyglotText("en", "Video"))
	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Video" {
		t.Errorf("expected 'Video', got %s", ds["en"])
	}
}

func TestMenu_AddMenuName_Merges(t *testing.T) {
	m := NewMenu("video").
		WithMenuName(NewPolyglotText("en", "Video")).
		AddMenuName(NewPolyglotText("fr", "Vidéo"))

	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "Video" {
		t.Errorf("expected 'Video', got %s", ds["en"])
	}
	if ds["fr"] != "Vidéo" {
		t.Errorf("expected 'Vidéo', got %s", ds["fr"])
	}
}

func TestMenu_AddMenuName_OverwritesExisting(t *testing.T) {
	m := NewMenu("video").
		WithMenuName(NewPolyglotText("en", "Old")).
		AddMenuName(NewPolyglotText("en", "New"))

	ds := m.Proto.Name.GetDisplayStrings()
	if ds["en"] != "New" {
		t.Errorf("expected 'New', got %s", ds["en"])
	}
}
