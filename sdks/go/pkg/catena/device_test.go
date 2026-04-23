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
	"encoding/json"
	"strings"
	"testing"
)

func TestToCatenaDevice_Basic(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	if cd.GetProtoDevice() == nil {
		t.Error("expected non-nil proto device")
	}
}

func TestToCatenaDevice_WithParams(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"params": map[string]any{
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type":      ParamTypeInt32,
				"read_only": false,
				"widget":    "SLIDER",
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	// Check that params were converted
	params := proto.GetParams()
	if params == nil {
		t.Fatal("expected params to be set")
	}
	brightness, ok := params["brightness"]
	if !ok {
		t.Fatal("expected 'brightness' param")
	}
	if brightness.GetWidget() != "SLIDER" {
		t.Errorf("expected widget 'SLIDER', got %v", brightness.GetWidget())
	}
}

func TestToCatenaDevice_WithConstraints(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"constraints": map[string]any{
			"brightness_range": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(0),
					"max_value": int32(100),
				},
			},
			"input_source_choice": map[string]any{
				"string_choice": map[string]any{
					"choices": []string{"SDI", "HDMI", "IP"},
				},
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	constraints := proto.GetConstraints()
	if constraints == nil {
		t.Fatal("expected constraints to be set")
	}

	// Check int32_range constraint
	brightnessRange, ok := constraints["brightness_range"]
	if !ok {
		t.Fatal("expected 'brightness_range' constraint")
	}
	int32Range := brightnessRange.GetInt32Range()
	if int32Range == nil {
		t.Fatal("expected int32_range to be set")
	}
	if int32Range.GetMinValue() != 0 {
		t.Errorf("expected min_value 0, got %v", int32Range.GetMinValue())
	}
	if int32Range.GetMaxValue() != 100 {
		t.Errorf("expected max_value 100, got %v", int32Range.GetMaxValue())
	}

	// Check string_choice constraint
	inputChoice, ok := constraints["input_source_choice"]
	if !ok {
		t.Fatal("expected 'input_source_choice' constraint")
	}
	stringChoice := inputChoice.GetStringChoice()
	if stringChoice == nil {
		t.Fatal("expected string_choice to be set")
	}
	choices := stringChoice.GetChoices()
	if len(choices) != 3 {
		t.Errorf("expected 3 choices, got %d", len(choices))
	}
}

func TestToCatenaDevice_WithLanguagePacks(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"language_packs": map[string]any{
			"packs": map[string]any{
				"en": map[string]any{
					"name": "English",
					"words": map[string]string{
						"brightness": "Brightness",
						"contrast":   "Contrast",
					},
				},
				"fr": map[string]any{
					"name": "Français",
					"words": map[string]string{
						"brightness": "Luminosité",
						"contrast":   "Contraste",
					},
				},
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	langPacks := proto.GetLanguagePacks()
	if langPacks == nil {
		t.Fatal("expected language_packs to be set")
	}
	packs := langPacks.GetPacks()
	if packs == nil {
		t.Fatal("expected packs to be set")
	}
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
}

func TestToCatenaDevice_WithMenuGroups(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"menu_groups": map[string]any{
			"video": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Video Settings",
					},
				},
				"menus": map[string]any{
					"basic": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Basic",
							},
						},
						"param_oids": []string{"brightness", "contrast"},
					},
				},
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	menuGroups := proto.GetMenuGroups()
	if menuGroups == nil {
		t.Fatal("expected menu_groups to be set")
	}

	videoGroup, ok := menuGroups["video"]
	if !ok {
		t.Fatal("expected 'video' menu group")
	}

	menus := videoGroup.GetMenus()
	if menus == nil {
		t.Fatal("expected menus to be set")
	}

	basicMenu, ok := menus["basic"]
	if !ok {
		t.Fatal("expected 'basic' menu")
	}

	paramOids := basicMenu.GetParamOids()
	if len(paramOids) != 2 {
		t.Errorf("expected 2 param_oids, got %d", len(paramOids))
	}
}

func TestToCatenaDevice_WithCommands(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"commands": map[string]any{
			"reboot": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Reboot Device",
					},
				},
				"type": ParamTypeEmpty,
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	commands := proto.GetCommands()
	if commands == nil {
		t.Fatal("expected commands to be set")
	}

	reboot, ok := commands["reboot"]
	if !ok {
		t.Fatal("expected 'reboot' command")
	}
	if reboot.GetType() != ParamTypeEmpty {
		t.Errorf("expected type EMPTY, got %v", reboot.GetType())
	}
}

func TestCatenaDevice_ToJSON(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"params": map[string]any{
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type": ParamTypeInt32,
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}

	jsonData, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON error: %v", err)
	}
	if jsonData == nil {
		t.Fatal("expected non-nil JSON data")
	}

	// Verify it's valid JSON
	var result map[string]any
	if err := json.Unmarshal(jsonData, &result); err != nil {
		t.Fatalf("invalid JSON output: %v", err)
	}

	// Check some fields exist
	if _, ok := result["params"]; !ok {
		t.Error("expected 'params' in JSON output")
	}
}

func TestCatenaDevice_ToJSON_SlotZeroPresent(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"params": map[string]any{
			"volume": map[string]any{
				"type": ParamTypeInt32,
				"value": map[string]any{
					"int32_value": 0,
				},
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}

	jsonData, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON error: %v", err)
	}

	body := string(jsonData)

	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0 in JSON output, got %s", body)
	}

	if !strings.Contains(body, `"int32_value":0`) {
		t.Errorf("expected int32_value:0 in JSON output, got %s", body)
	}

	// Non-struct params must not have empty params map
	if strings.Contains(body, `"params":{}`) {
		t.Errorf("empty params map should not appear in output, got %s", body)
	}
}

func TestCatenaDevice_ToJSON_Nil(t *testing.T) {
	cd := CatenaDevice{device: nil}
	jsonData, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON error: %v", err)
	}
	if jsonData != nil {
		t.Error("expected nil JSON data for nil device")
	}
}

func TestCatenaDevice_GetProtoDevice_Nil(t *testing.T) {
	cd := CatenaDevice{device: nil}
	if cd.GetProtoDevice() != nil {
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

func TestToCatenaDevice_CompleteDevice(t *testing.T) {
	// This test verifies a complete device configuration like the example
	deviceMap := map[string]any{
		"slot":              uint32(0),
		"detail_level":      DetailLevelFull,
		"multi_set_enabled": true,
		"subscriptions":     true,
		"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
		"default_scope":     "st2138:op",
		"params": map[string]any{
			"brightness": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Brightness",
					},
				},
				"type": ParamTypeInt32,
				"constraint": map[string]any{
					"ref_oid": "brightness_range",
				},
				"read_only": false,
				"widget":    "SLIDER",
			},
		},
		"constraints": map[string]any{
			"brightness_range": map[string]any{
				"int32_range": map[string]any{
					"min_value": int32(0),
					"max_value": int32(100),
				},
			},
		},
		"menu_groups": map[string]any{
			"video": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Video Settings",
					},
				},
				"menus": map[string]any{
					"basic": map[string]any{
						"name": map[string]any{
							"display_strings": map[string]string{
								"en": "Basic",
							},
						},
						"param_oids": []string{"brightness"},
					},
				},
			},
		},
		"commands": map[string]any{
			"reboot": map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Reboot Device",
					},
				},
				"type": ParamTypeEmpty,
			},
		},
		"language_packs": map[string]any{
			"packs": map[string]any{
				"en": map[string]any{
					"name": "English",
					"words": map[string]string{
						"brightness": "Brightness",
					},
				},
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}

	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	// Verify key fields
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

	// Verify access scopes
	scopes := proto.GetAccessScopes()
	if len(scopes) != 4 {
		t.Errorf("expected 4 access_scopes, got %d", len(scopes))
	}

	// Verify JSON output works
	jsonData, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON error: %v", err)
	}
	if len(jsonData) == 0 {
		t.Error("expected non-empty JSON")
	}
}

func TestToCatenaDevice_FloatRangeConstraint(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
		"constraints": map[string]any{
			"gain_range": map[string]any{
				"float_range": map[string]any{
					"min_value": float32(-60.0),
					"max_value": float32(12.0),
				},
			},
		},
	}

	cd, err := ToCatenaDevice(deviceMap)
	if err != nil {
		t.Fatalf("ToCatenaDevice error: %v", err)
	}
	proto := cd.GetProtoDevice()
	if proto == nil {
		t.Fatal("expected non-nil proto device")
	}

	constraints := proto.GetConstraints()
	if constraints == nil {
		t.Fatal("expected constraints to be set")
	}

	gainRange, ok := constraints["gain_range"]
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

// --- stripEmptyFields + EmitUnpopulated coverage via ToJSON ---

func TestToJSON_SlotZeroPresent(t *testing.T) {
	cd, err := ToCatenaDevice(map[string]any{
		"slot": uint32(0),
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON: %v", err)
	}
	body := string(b)
	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0 in output; got %s", body)
	}
}

func TestToJSON_SlotNonZero(t *testing.T) {
	cd, err := ToCatenaDevice(map[string]any{
		"slot": uint32(5),
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON: %v", err)
	}
	body := string(b)
	if !strings.Contains(body, `"slot":5`) {
		t.Errorf("expected slot:5; got %s", body)
	}
}

func TestToJSON_EmptyMapsStripped(t *testing.T) {
	cd, err := ToCatenaDevice(map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON: %v", err)
	}
	body := string(b)

	if !strings.Contains(body, `"slot":0`) {
		t.Errorf("expected slot:0; got %s", body)
	}
	for _, field := range []string{"params", "constraints", "commands", "menu_groups", "language_packs"} {
		if strings.Contains(body, `"`+field+`":{}`) {
			t.Errorf("empty %s should be stripped; got %s", field, body)
		}
	}
}

func TestToJSON_PopulatedMapsKept(t *testing.T) {
	cd, err := ToCatenaDevice(map[string]any{
		"slot": uint32(0),
		"params": map[string]any{
			"brightness": map[string]any{
				"type": ParamTypeInt32,
			},
		},
		"commands": map[string]any{
			"reboot": map[string]any{
				"type": ParamTypeEmpty,
			},
		},
	})
	if err != nil {
		t.Fatalf("ToCatenaDevice: %v", err)
	}
	b, err := cd.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON: %v", err)
	}
	body := string(b)

	if !strings.Contains(body, `"params"`) {
		t.Errorf("expected params in output; got %s", body)
	}
	if !strings.Contains(body, `"commands"`) {
		t.Errorf("expected commands in output; got %s", body)
	}
}

func TestStripEmptyValues(t *testing.T) {
	tests := []struct {
		name  string
		input string
		want  string
	}{
		{
			name:  "strip null compact",
			input: `{"slot":0,"value":null,"type":"INT32"}`,
			want:  `{"slot":0,"type":"INT32"}`,
		},
		{
			name:  "strip null spaced",
			input: `{"slot":0, "value":null, "type":"INT32"}`,
			want:  `{"slot":0, "type":"INT32"}`,
		},
		{
			name:  "strip empty object",
			input: `{"slot":0, "params":{}, "detail_level":"FULL"}`,
			want:  `{"slot":0, "detail_level":"FULL"}`,
		},
		{
			name:  "strip empty array",
			input: `{"slot":0, "oid_aliases":[], "type":"INT32"}`,
			want:  `{"slot":0, "type":"INT32"}`,
		},
		{
			name:  "strip empty string",
			input: `{"slot":0, "widget":"", "type":"INT32"}`,
			want:  `{"slot":0, "type":"INT32"}`,
		},
		{
			name:  "keep populated values",
			input: `{"slot":0, "params":{"a":1}, "name":"test"}`,
			want:  `{"slot":0, "params":{"a":1}, "name":"test"}`,
		},
		{
			name:  "cascading strip",
			input: `{"outer":{"inner":null}}`,
			want:  `{}`,
		},
		{
			name:  "strip first field spaced",
			input: `{"value":null, "slot":0}`,
			want:  `{"slot":0}`,
		},
		{
			name:  "strip last field spaced",
			input: `{"slot":0, "value":null}`,
			want:  `{"slot":0}`,
		},
		{
			name:  "strip multiple patterns",
			input: `{"a":null, "b":{}, "c":[], "d":"", "e":1}`,
			want:  `{"e":1}`,
		},
		{
			name:  "nested null stripped",
			input: `{"params":{"brightness":{"value":null, "type":"INT32"}}}`,
			want:  `{"params":{"brightness":{"type":"INT32"}}}`,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := string(stripEmptyValues([]byte(tt.input)))
			if got != tt.want {
				t.Errorf("stripEmptyValues() = %s, want %s", got, tt.want)
			}
		})
	}
}
