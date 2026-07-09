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
 * @file device_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewDevice_Basic(t *testing.T) {
	cd := NewDevice(3)
	proto := cd.Proto
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}
	if proto.GetSlot() != 3 {
		t.Errorf("expected slot 3, got %v", proto.GetSlot())
	}

	// The product struct is managed by the SDK (RegisterProductStruct), not by
	// NewDevice, so a freshly built device has no product param.
	if _, ok := proto.GetParams()["product"]; ok {
		t.Error("did not expect NewDevice to create a 'product' param")
	}
}

func TestNewDevice_WithParams(t *testing.T) {
	cd := NewDevice(0).
		WithParam("brightness", NewParamInt32(50).
			WithName(NewPolyglotText("en", "Brightness")).
			WithWidget("SLIDER"))

	brightness, ok := cd.Proto.GetParams()["brightness"]
	if !ok {
		t.Fatal("expected 'brightness' param")
	}
	if brightness.GetWidget() != "SLIDER" {
		t.Errorf("expected widget 'SLIDER', got %v", brightness.GetWidget())
	}
	if brightness.GetValue().GetInt32Value() != 50 {
		t.Errorf("expected value 50, got %v", brightness.GetValue().GetInt32Value())
	}
}

func TestDevice_WithParam_DeepCopies(t *testing.T) {
	param := NewParamInt32(1)
	cd := NewDevice(0).
		WithParam("counter", param)

	// Mutating the original param after WithParam must not affect the device.
	param.WithValue(Value{Proto: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 999}}})

	stored := cd.Proto.GetParams()["counter"]
	if stored.GetValue().GetInt32Value() != 1 {
		t.Errorf("expected stored value to remain 1, got %v", stored.GetValue().GetInt32Value())
	}
}

func TestDevice_WithParam_Nil(t *testing.T) {
	cd := NewDevice(0).
		WithParam("nothing", nil)
	if _, ok := cd.Proto.GetParams()["nothing"]; ok {
		t.Error("expected nil param to be ignored")
	}

	cd = NewDevice(0).
		WithParam("nothing", &Param{})
	if _, ok := cd.Proto.GetParams()["nothing"]; ok {
		t.Error("expected param with nil Proto to be ignored")
	}
}

func TestDevice_WithCommand_Nil(t *testing.T) {
	cd := NewDevice(0).
		WithCommand("nothing", nil)
	if _, ok := cd.Proto.GetCommands()["nothing"]; ok {
		t.Error("expected nil command to be ignored")
	}

	cd = NewDevice(0).
		WithCommand("nothing", &Param{})
	if _, ok := cd.Proto.GetCommands()["nothing"]; ok {
		t.Error("expected command with nil Proto to be ignored")
	}
}

func TestDevice_WithConstraint_Nil(t *testing.T) {
	cd := NewDevice(0).
		WithConstraint("nothing", nil)
	if _, ok := cd.Proto.GetConstraints()["nothing"]; ok {
		t.Error("expected nil constraint to be ignored")
	}

	cd = NewDevice(0).
		WithConstraint("nothing", &Constraint{})
	if _, ok := cd.Proto.GetConstraints()["nothing"]; ok {
		t.Error("expected constraint with nil Proto to be ignored")
	}
}

func TestDevice_WithMenuGroup_Nil(t *testing.T) {
	cd := NewDevice(0).
		WithMenuGroup("nothing", nil)
	if _, ok := cd.Proto.GetMenuGroups()["nothing"]; ok {
		t.Error("expected nil menu group to be ignored")
	}

	cd = NewDevice(0).
		WithMenuGroup("nothing", &MenuGroup{})
	if _, ok := cd.Proto.GetMenuGroups()["nothing"]; ok {
		t.Error("expected menu group with nil Proto to be ignored")
	}
}

func TestDevice_WithConstraints(t *testing.T) {
	cd := NewDevice(0).
		WithConstraint("brightness_range", NewConstraintInt32Range(0, 100, 1)).
		WithConstraint("input_source_choice", NewConstraintStringChoice(false, "SDI", "HDMI", "IP"))

	constraints := cd.Proto.GetConstraints()
	if constraints == nil {
		t.Fatal("expected constraints to be set")
	}

	brightnessRange, ok := constraints["brightness_range"]
	if !ok {
		t.Fatal("expected 'brightness_range' constraint")
	}
	int32Range := brightnessRange.GetInt32Range()
	if int32Range == nil {
		t.Fatal("expected int32_range to be set")
	}
	if int32Range.GetMaxValue() != 100 {
		t.Errorf("expected max_value 100, got %v", int32Range.GetMaxValue())
	}

	inputChoice, ok := constraints["input_source_choice"]
	if !ok {
		t.Fatal("expected 'input_source_choice' constraint")
	}
	if got := len(inputChoice.GetStringChoice().GetChoices()); got != 3 {
		t.Errorf("expected 3 choices, got %d", got)
	}
}

func TestDevice_WithLanguagePacks(t *testing.T) {
	cd := NewDevice(0).
		WithLanguagePack("en", "English", map[string]string{"brightness": "Brightness"}).
		WithLanguagePack("fr", "Français", map[string]string{"brightness": "Luminosité"})

	packs := cd.Proto.GetLanguagePacks().GetPacks()
	if len(packs) != 2 {
		t.Errorf("expected 2 language packs, got %d", len(packs))
	}
	enPack, ok := packs["en"]
	if !ok {
		t.Fatal("expected 'en' language pack")
	}
	if enPack.GetName() != "English" {
		t.Errorf("expected name 'English', got %v", enPack.GetName())
	}
	if enPack.GetWords()["brightness"] != "Brightness" {
		t.Errorf("expected word 'Brightness', got %v", enPack.GetWords()["brightness"])
	}
}

func TestDevice_WithMenuGroups(t *testing.T) {
	cd := NewDevice(0).
		WithMenuGroup("video", NewMenuGroup().
			WithName(NewPolyglotText("en", "Video Settings")).
			WithMenu("basic", NewMenu().
				WithName(NewPolyglotText("en", "Basic")).
				WithParamOids("brightness", "contrast")))

	videoGroup, ok := cd.Proto.GetMenuGroups()["video"]
	if !ok {
		t.Fatal("expected 'video' menu group")
	}
	basicMenu, ok := videoGroup.GetMenus()["basic"]
	if !ok {
		t.Fatal("expected 'basic' menu")
	}
	if len(basicMenu.GetParamOids()) != 2 {
		t.Errorf("expected 2 param_oids, got %d", len(basicMenu.GetParamOids()))
	}
}

func TestDevice_WithCommands(t *testing.T) {
	cd := NewDevice(0).
		WithCommand("reboot", NewParamEmpty().
			WithName(NewPolyglotText("en", "Reboot Device")))

	reboot, ok := cd.Proto.GetCommands()["reboot"]
	if !ok {
		t.Fatal("expected 'reboot' command")
	}
	if reboot.GetType() != ParamTypeEmpty {
		t.Errorf("expected type EMPTY, got %v", reboot.GetType())
	}
}

func TestDevice_ScalarFields(t *testing.T) {
	cd := NewDevice(0).
		WithDetailLevel(DetailLevelFull).
		WithMultiSetEnabled(true).
		WithSubscriptions(true).
		WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
		WithDefaultScope("st2138:op")

	proto := cd.Proto
	if !proto.GetMultiSetEnabled() {
		t.Error("expected multi_set_enabled to be true")
	}
	if !proto.GetSubscriptions() {
		t.Error("expected subscriptions to be true")
	}
	if proto.GetDefaultScope() != "st2138:op" {
		t.Errorf("expected default_scope 'st2138:op', got %v", proto.GetDefaultScope())
	}
	if len(proto.GetAccessScopes()) != 4 {
		t.Errorf("expected 4 access_scopes, got %d", len(proto.GetAccessScopes()))
	}
}

func TestDevice_ToProtoDevice_Nil(t *testing.T) {
	cd := Device{Proto: nil}
	if cd.Proto != nil {
		t.Error("expected nil proto device")
	}
}

func TestDetailLevelConstants(t *testing.T) {
	tests := []struct {
		name     string
		level    DetailLevel
		expected int32
	}{
		{"FULL", DetailLevelFull, 0},
		{"SUBSCRIPTIONS", DetailLevelSubscriptions, 1},
		{"MINIMAL", DetailLevelMinimal, 2},
		{"COMMANDS", DetailLevelCommands, 3},
		{"NONE", DetailLevelNone, 4},
		{"UNSET", DetailLevelUnset, 5},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if int32(tt.level) != tt.expected {
				t.Errorf("DetailLevel %s = %d, want %d", tt.name, tt.level, tt.expected)
			}
		})
	}
}

func TestNewDevice_CompleteDevice(t *testing.T) {
	// This test verifies a complete device configuration like the example.
	cd := NewDevice(0).
		WithDetailLevel(DetailLevelFull).
		WithMultiSetEnabled(true).
		WithSubscriptions(true).
		WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
		WithDefaultScope("st2138:op").
		WithParam("brightness", NewParamInt32(50).
			WithName(NewPolyglotText("en", "Brightness")).
			WithConstraint(NewConstraintRefOid("brightness_range")).
			WithWidget("SLIDER")).
		WithConstraint("brightness_range", NewConstraintInt32Range(0, 100, 1)).
		WithMenuGroup("video", NewMenuGroup().
			WithName(NewPolyglotText("en", "Video Settings")).
			WithMenu("basic", NewMenu().
				WithName(NewPolyglotText("en", "Basic")).
				WithParamOids("brightness"))).
		WithCommand("reboot", NewParamEmpty().
			WithName(NewPolyglotText("en", "Reboot Device"))).
		WithLanguagePack("en", "English", map[string]string{"brightness": "Brightness"})

	proto := cd.Proto
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}
	if proto.GetSlot() != 0 {
		t.Errorf("expected slot 0, got %v", proto.GetSlot())
	}
	if !proto.GetMultiSetEnabled() {
		t.Error("expected multi_set_enabled to be true")
	}
	if !proto.GetSubscriptions() {
		t.Error("expected subscriptions to be true")
	}
	if proto.GetDefaultScope() != "st2138:op" {
		t.Errorf("expected default_scope 'st2138:op', got %v", proto.GetDefaultScope())
	}
	if len(proto.GetAccessScopes()) != 4 {
		t.Errorf("expected 4 access_scopes, got %d", len(proto.GetAccessScopes()))
	}
	if proto.GetParams()["brightness"].GetConstraint().GetRefOid() != "brightness_range" {
		t.Error("expected brightness to reference shared constraint 'brightness_range'")
	}
	if proto.GetConstraints()["brightness_range"].GetInt32Range().GetMaxValue() != 100 {
		t.Error("expected shared brightness_range constraint max 100")
	}
	if _, ok := proto.GetCommands()["reboot"]; !ok {
		t.Error("expected 'reboot' command")
	}
}

func TestDevice_WithFloatRangeConstraint(t *testing.T) {
	cd := NewDevice(0).
		WithConstraint("gain_range", NewConstraintFloatRange(-60.0, 12.0, 0.5))

	gainRange, ok := cd.Proto.GetConstraints()["gain_range"]
	if !ok {
		t.Fatal("expected 'gain_range' constraint")
	}
	floatRange := gainRange.GetFloatRange()
	if floatRange == nil {
		t.Fatal("expected float_range to be set")
	}
	if floatRange.GetMinValue() != -60.0 {
		t.Errorf("expected min_value -60.0, got %v", floatRange.GetMinValue())
	}
	if floatRange.GetMaxValue() != 12.0 {
		t.Errorf("expected max_value 12.0, got %v", floatRange.GetMaxValue())
	}
}

func TestDevice_WithSharedConstraint(t *testing.T) {
	constraint := NewConstraintStringChoice(true, "SDI", "HDMI", "IP")

	device := NewDevice(0).
		WithConstraint("input_source", constraint)

	stringChoice := device.Proto.GetConstraints()["input_source"].GetStringChoice()
	if stringChoice == nil {
		t.Fatal("expected string_choice constraint")
	}
	if !stringChoice.GetStrict() {
		t.Fatal("expected strict string choice")
	}
	if got := len(stringChoice.GetChoices()); got != 3 {
		t.Fatalf("expected 3 choices, got %d", got)
	}
}

