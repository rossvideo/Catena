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

package st2138

import (
	"errors"
	"testing"

	"google.golang.org/protobuf/proto"
)

func TestNewDevice_Basic(t *testing.T) {
	cd, err := NewDevice(3).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cd.Proto == nil {
		t.Fatal("expected non-nil proto device")
	}
	if cd.Proto.GetSlot() != 3 {
		t.Errorf("expected slot 3, got %v", cd.Proto.GetSlot())
	}

	// The product struct is managed by the SDK (RegisterProductStruct), not by
	// NewDevice, so a freshly built device has no product param.
	if _, ok := cd.Proto.GetParams()["product"]; ok {
		t.Error("did not expect NewDevice to create a 'product' param")
	}
}

func TestNewDevice_WithParams(t *testing.T) {
	cd, err := NewDevice(0).
		WithParam("brightness", NewParamInt32(50).
			WithName(PolyglotText{"en": "Brightness"}).
			WithWidget("SLIDER")).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

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

func TestDevice_WithParam_Nil(t *testing.T) {
	builder := NewDevice(0).WithParam("nothing", nil)
	if _, ok := builder.proto.GetParams()["nothing"]; ok {
		t.Error("expected nil param to short-circuit")
	}
	cd, err := builder.Build()
	if err == nil {
		t.Error("expected error for builder with nil param")
	}
	if cd != (Device{}) {
		t.Errorf("expected empty Device, got %v", cd)
	}
}

func TestDevice_WithParam_NestedErrors(t *testing.T) {
	builder := NewDevice(0).WithParam("nothing", NewParamInt32(1, 2))
	if _, ok := builder.proto.GetParams()["nothing"]; ok {
		t.Error("expected nested builder error to leave params unchanged")
	}
	cd, err := builder.Build()
	if cd != (Device{}) {
		t.Errorf("expected nil device, got %v", cd)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}

func TestDevice_WithCommand_Nil(t *testing.T) {
	builder := NewDevice(0).WithCommand("nothing", nil)
	if _, ok := builder.proto.GetCommands()["nothing"]; ok {
		t.Error("expected nil command to leave commands unchanged")
	}
	cd, err := builder.Build()
	if err == nil {
		t.Fatal("expected error for builder with nil command")
	}
	if cd != (Device{}) {
		t.Fatal("expected empty device for builder with nil command")
	}

	cd, err = NewDevice(0).WithCommand("nothing", NewParamEmpty()).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cd.Proto.GetCommands()["nothing"] == nil {
		t.Fatal("expected command to be set")
	}
}

func TestDevice_WithCommand_NestedErrors(t *testing.T) {
	builder := NewDevice(0).WithCommand("nothing", NewParamInt32(1, 2))
	if _, ok := builder.proto.GetCommands()["nothing"]; ok {
		t.Error("expected nested builder error to leave commands unchanged")
	}
	cd, err := builder.Build()
	if cd != (Device{}) {
		t.Errorf("expected nil device, got %v", cd)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}

func TestDevice_WithConstraint_Nil(t *testing.T) {
	builder := NewDevice(0).WithConstraint("nothing", nil)
	if _, ok := builder.proto.GetConstraints()["nothing"]; ok {
		t.Error("expected nil constraint builder to short-circuit")
	}
	cd, err := builder.Build()
	if err == nil {
		t.Error("expected error for builder with nil constraint")
	}
	if cd != (Device{}) {
		t.Errorf("expected empty Device, got %v", cd)
	}
}

func TestDevice_WithConstraint_NestedErrors(t *testing.T) {
	constraint := NewConstraintInt32Range(0, 10, 1)
	constraint.err = errors.New("test error")
	builder := NewDevice(0).WithConstraint("nothing", constraint)
	if _, ok := builder.proto.GetConstraints()["nothing"]; ok {
		t.Error("expected nested builder error to leave constraints unchanged")
	}
	cd, err := builder.Build()
	if cd != (Device{}) {
		t.Errorf("expected empty device, got %v", cd)
	}
	if err == nil {
		t.Error("expected error for builder with constraint builder error, got nil")
	}
}

func TestDevice_WithMenuGroup_Nil(t *testing.T) {
	builder := NewDevice(0).WithMenuGroup("nothing", nil)
	if _, ok := builder.proto.GetMenuGroups()["nothing"]; ok {
		t.Error("expected nil menu group to short-circuit")
	}
	cd, err := builder.Build()
	if err == nil {
		t.Error("expected error for builder with nil menu group")
	}
	if cd != (Device{}) {
		t.Errorf("expected empty Device, got %v", cd)
	}
}

func TestDevice_WithMenuGroup_NestedErrors(t *testing.T) {
	builder := NewDevice(0).WithMenuGroup("nothing", NewMenuGroup().WithName(nil))
	if _, ok := builder.proto.GetMenuGroups()["nothing"]; ok {
		t.Error("expected nested builder error to leave menu groups unchanged")
	}
	cd, err := builder.Build()
	if cd != (Device{}) {
		t.Errorf("expected nil device, got %v", cd)
	}
	if err == nil {
		t.Error("expected error for builder with nested errors")
	}
}

func TestDevice_WithConstraints(t *testing.T) {
	cd, err := NewDevice(0).
		WithConstraint("brightness_range", NewConstraintInt32Range(0, 100, 1)).
		WithConstraint("input_source_choice", NewConstraintStringChoice(false, "SDI", "HDMI", "IP")).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

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
	cd, err := NewDevice(0).
		WithLanguagePack("en", "English", map[string]string{"brightness": "Brightness"}).
		WithLanguagePack("fr", "Français", map[string]string{"brightness": "Luminosité"}).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

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
	cd, err := NewDevice(0).
		WithMenuGroup("video", NewMenuGroup().
			WithName(PolyglotText{"en": "Video Settings"}).
			WithMenu("basic", NewMenu().
				WithName(PolyglotText{"en": "Basic"}).
				WithParamOids("brightness", "contrast"))).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

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
	cd, err := NewDevice(0).
		WithCommand("reboot", NewParamEmpty().
			WithName(PolyglotText{"en": "Reboot Device"})).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	reboot, ok := cd.Proto.GetCommands()["reboot"]
	if !ok {
		t.Fatal("expected 'reboot' command")
	}
	if reboot.GetType() != ParamTypeEmpty {
		t.Errorf("expected type EMPTY, got %v", reboot.GetType())
	}
}

func TestDevice_ScalarFields(t *testing.T) {
	cd, err := NewDevice(0).
		WithDetailLevel(DetailLevelFull).
		WithMultiSetEnabled(true).
		WithSubscriptions(true).
		WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
		WithDefaultScope("st2138:op").
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if !cd.Proto.GetMultiSetEnabled() {
		t.Error("expected multi_set_enabled to be true")
	}
	if !cd.Proto.GetSubscriptions() {
		t.Error("expected subscriptions to be true")
	}
	if cd.Proto.GetDefaultScope() != "st2138:op" {
		t.Errorf("expected default_scope 'st2138:op', got %v", cd.Proto.GetDefaultScope())
	}
	if len(cd.Proto.GetAccessScopes()) != 4 {
		t.Errorf("expected 4 access_scopes, got %d", len(cd.Proto.GetAccessScopes()))
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
	cd, err := NewDevice(0).
		WithDetailLevel(DetailLevelFull).
		WithMultiSetEnabled(true).
		WithSubscriptions(true).
		WithAccessScopes("st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm").
		WithDefaultScope("st2138:op").
		WithParam("brightness", NewParamInt32(50).
			WithName(PolyglotText{"en": "Brightness"}).
			WithConstraint(NewConstraintRefOid("brightness_range")).
			WithWidget("SLIDER")).
		WithConstraint("brightness_range", NewConstraintInt32Range(0, 100, 1)).
		WithMenuGroup("video", NewMenuGroup().
			WithName(PolyglotText{"en": "Video Settings"}).
			WithMenu("basic", NewMenu().
				WithName(PolyglotText{"en": "Basic"}).
				WithParamOids("brightness"))).
		WithCommand("reboot", NewParamEmpty().
			WithName(PolyglotText{"en": "Reboot Device"})).
		WithLanguagePack("en", "English", map[string]string{"brightness": "Brightness"}).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

	if cd.Proto == nil {
		t.Fatal("expected non-nil proto device")
	}
	if cd.Proto.GetSlot() != 0 {
		t.Errorf("expected slot 0, got %v", cd.Proto.GetSlot())
	}
	if !cd.Proto.GetMultiSetEnabled() {
		t.Error("expected multi_set_enabled to be true")
	}
	if !cd.Proto.GetSubscriptions() {
		t.Error("expected subscriptions to be true")
	}
	if cd.Proto.GetDefaultScope() != "st2138:op" {
		t.Errorf("expected default_scope 'st2138:op', got %v", cd.Proto.GetDefaultScope())
	}
	if len(cd.Proto.GetAccessScopes()) != 4 {
		t.Errorf("expected 4 access_scopes, got %d", len(cd.Proto.GetAccessScopes()))
	}
	if cd.Proto.GetParams()["brightness"].GetConstraint().GetRefOid() != "brightness_range" {
		t.Error("expected brightness to reference shared constraint 'brightness_range'")
	}
	if cd.Proto.GetConstraints()["brightness_range"].GetInt32Range().GetMaxValue() != 100 {
		t.Error("expected shared brightness_range constraint max 100")
	}
	if _, ok := cd.Proto.GetCommands()["reboot"]; !ok {
		t.Error("expected 'reboot' command")
	}
}

func TestDevice_WithFloatRangeConstraint(t *testing.T) {
	cd, err := NewDevice(0).
		WithConstraint("gain_range", NewConstraintFloatRange(-60.0, 12.0, 0.5)).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

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

	device, err := NewDevice(0).
		WithConstraint("input_source", constraint).
		Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}

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

func TestDevice_Build(t *testing.T) {
	cd, err := NewDevice(0).Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if cd.Proto == nil {
		t.Fatal("expected non-nil proto device")
	}
	if cd.Proto.GetSlot() != 0 {
		t.Errorf("expected slot 0, got %v", cd.Proto.GetSlot())
	}
}

func TestDevice_BuildSurfacesErrors(t *testing.T) {
	cd := NewDevice(0)
	cd.err = errors.New("test error")
	if _, err := cd.Build(); err == nil {
		t.Fatal("expected error for builder with error:", err)
	}
}

func TestDeviceBuilder_WithShortCircuitErrors(t *testing.T) {
	cd := NewDevice(0)
	cd.err = errors.New("test error")
	before := proto.Clone(cd.proto)

	device, err := cd.
		WithParam("brightness", NewParamInt32(1)).
		WithCommand("reboot", NewParamEmpty()).
		WithConstraint("range", NewConstraintInt32Range(0, 10, 1)).
		WithMenuGroup("status", NewMenuGroup().WithName(PolyglotText{"en": "Status"})).
		WithLanguagePack("en", "English", map[string]string{"greeting": "Hello"}).
		WithDetailLevel(DetailLevelFull).
		WithMultiSetEnabled(true).
		WithSubscriptions(true).
		WithAccessScopes("st2138:mon").
		WithDefaultScope("st2138:mon").
		Build()
	if err == nil || err.Error() != "test error" {
		t.Fatalf("expected 'test error', got %v", err)
	}
	if device != (Device{}) {
		t.Fatal("expected nil device for builder with error")
	}
	if !proto.Equal(before, cd.proto) {
		t.Fatal("expected With* to leave the device unchanged after an error")
	}
}

func TestDevice_Clone(t *testing.T) {
	cd := NewDevice(0)
	clone := cd.Clone()
	if !proto.Equal(cd.proto, clone.proto) {
		t.Fatal("expected clone to be equal to original")
	}
	clone.WithDetailLevel(DetailLevelSubscriptions)
	if proto.Equal(cd.proto, clone.proto) {
		t.Fatal("expected original device to be unchanged")
	}
}

func TestDeviceBuilder_CloneInheritsError(t *testing.T) {
	cd := NewDevice(0)
	cd.err = errors.New("test error")
	clone := cd.Clone()
	if clone.err == nil || clone.err.Error() != "test error" {
		t.Fatal("expected clone to inherit error")
	}
}

func TestDevice_AlreadyBuilt(t *testing.T) {
	cd := NewDevice(0)
	p, err := cd.Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetSlot() != 0 {
		t.Errorf("expected slot 0, got %v", p.Proto.GetSlot())
	}
	p, err = cd.WithDetailLevel(DetailLevelFull).Build()
	if err == nil || !errors.Is(err, ErrAlreadyBuilt) {
		t.Fatal("expected error for builder after Build(), got", err)
	}
	if p != (Device{}) {
		t.Fatal("expected empty device for building already built device, got", p)
	}
}

func TestDeviceBuilder_CloneAlreadyBuilt(t *testing.T) {
	cd := NewDevice(0)
	p, err := cd.Build()
	if err != nil {
		t.Fatal("expected no error for builder with valid value", err)
	}
	if p.Proto.GetSlot() != 0 {
		t.Errorf("expected slot 0, got %v", p.Proto.GetSlot())
	}

	clone := cd.Clone()
	c, err := clone.Build()
	if err == nil || !errors.Is(err, ErrAlreadyBuilt) {
		t.Fatal("expected error for clone after Build(), got", err)
	}
	if c != (Device{}) {
		t.Fatal("expected empty device for clone after Build(), got", c)
	}
}
