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
 * @brief Tests for ParamInfo handling.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-12
 * @file param_info_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"testing"
)

func TestNewParamInfo_Basic(t *testing.T) {
	pi := NewParamInfo("text_box", NewPolyglotText("en", "Text Box"), ParamTypeString, "tpl-1", 0)

	if pi.GetOid() != "text_box" {
		t.Errorf("expected oid 'text_box', got %s", pi.GetOid())
	}
	if pi.GetParamType() != ParamTypeString {
		t.Errorf("expected type STRING, got %v", pi.GetParamType())
	}
	if pi.GetTemplateOid() != "tpl-1" {
		t.Errorf("expected template_oid 'tpl-1', got %s", pi.GetTemplateOid())
	}
	if pi.GetArrayLength() != 0 {
		t.Errorf("expected array_length 0, got %d", pi.GetArrayLength())
	}
	if pi.GetProtoResponse() == nil {
		t.Fatal("expected non-nil proto response")
	}
	if pi.GetProtoInfo() == nil {
		t.Fatal("expected non-nil proto info")
	}
	name := pi.GetProtoInfo().GetName()
	if name == nil || name.GetDisplayStrings()["en"] != "Text Box" {
		t.Errorf("expected display name 'Text Box' for en, got %v", name)
	}
}

func TestNewParamInfo_NilName(t *testing.T) {
	pi := NewParamInfo("foo", nil, ParamTypeInt32, "", 0)
	if pi.GetProtoInfo().GetName() != nil {
		t.Error("expected nil name when none was provided")
	}
}

func TestNewParamInfo_ArrayLength(t *testing.T) {
	pi := NewParamInfo("arr", nil, ParamTypeStringArray, "", 7)
	if pi.GetArrayLength() != 7 {
		t.Errorf("expected array_length 7, got %d", pi.GetArrayLength())
	}
}

func TestToCatenaParamInfo_FromMap(t *testing.T) {
	m := map[string]any{
		"info": map[string]any{
			"oid":          "brightness",
			"type":         "INT32",
			"template_oid": "tpl-2",
			"name": map[string]any{
				"display_strings": map[string]string{
					"en": "Brightness",
				},
			},
		},
		"array_length": 0,
	}

	pi, err := ToCatenaParamInfo(m)
	if err != nil {
		t.Fatalf("ToCatenaParamInfo error: %v", err)
	}
	if pi.GetOid() != "brightness" {
		t.Errorf("expected oid 'brightness', got %s", pi.GetOid())
	}
	if pi.GetParamType() != ParamTypeInt32 {
		t.Errorf("expected type INT32, got %v", pi.GetParamType())
	}
	if pi.GetTemplateOid() != "tpl-2" {
		t.Errorf("expected template_oid 'tpl-2', got %s", pi.GetTemplateOid())
	}
}

func TestToCatenaParamInfo_InvalidProto(t *testing.T) {
	m := map[string]any{
		"info": map[string]any{
			"type": "NOT_A_TYPE",
		},
	}
	if _, err := ToCatenaParamInfo(m); err == nil {
		t.Error("expected error for invalid ParamType, got nil")
	}
}

func TestCatenaParamInfo_ZeroValue(t *testing.T) {
	var pi CatenaParamInfo
	if pi.GetProtoResponse() != nil {
		t.Error("expected nil proto response for zero value")
	}
	if pi.GetProtoInfo() != nil {
		t.Error("expected nil proto info for zero value")
	}
	if pi.GetOid() != "" {
		t.Errorf("expected empty oid, got %q", pi.GetOid())
	}
	if pi.GetArrayLength() != 0 {
		t.Errorf("expected array_length 0, got %d", pi.GetArrayLength())
	}
}
