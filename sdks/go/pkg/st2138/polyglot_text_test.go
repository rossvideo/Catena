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
 * @brief Tests for PolyglotText type.
 * @file polyglot_text_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-10
 */

package st2138

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestPolyglotText_With(t *testing.T) {
	pt := PolyglotText{"en": "Hello"}.With("fr", "Bonjour")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
	if pt["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt["fr"])
	}
}

func TestPolyglotText_With_NilPolyglotText(t *testing.T) {
	var pt PolyglotText
	pt = pt.With("fr", "Bonjour")
	if pt["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt["fr"])
	}
}

func TestPolyglotText_Lookup(t *testing.T) {
	pt := PolyglotText{"en": "Hello", "fr": "Bonjour", "es": ""}

	if s, ok := pt.Lookup("fr"); !ok || s != "Bonjour" {
		t.Errorf("expected ('Bonjour', true), got (%q, %v)", s, ok)
	}
	if s, ok := pt.Lookup("de"); ok || s != "" {
		t.Errorf("expected ('', false) for missing entry, got (%q, %v)", s, ok)
	}
	if s, ok := pt.Lookup("es"); !ok || s != "" {
		t.Errorf("expected ('', true) for present empty entry, got (%q, %v)", s, ok)
	}
}

func TestPolyglotText_Lookup_Nil(t *testing.T) {
	var pt PolyglotText
	if s, ok := pt.Lookup("en"); ok || s != "" {
		t.Errorf("expected ('', false) for nil receiver, got (%q, %v)", s, ok)
	}
}

func TestPolyglotText_GetOrLang(t *testing.T) {
	pt := PolyglotText{"en": "Hello", "fr": "Bonjour"}
	if text := pt.GetOrLang("fr", "en"); text != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", text)
	}
	if text := pt.GetOrLang("de", "en"); text != "Hello" {
		t.Errorf("expected fallback 'Hello', got %s", text)
	}
}

func TestPolyglotText_GetOrLang_MissingFallback(t *testing.T) {
	pt := PolyglotText{"en": "Hello"}
	if text := pt.GetOrLang("de", "fr"); text != "" {
		t.Errorf("expected empty string for missing fallback, got %s", text)
	}
}

func TestPolyglotText_GetOrLang_PresentEmpty(t *testing.T) {
	pt := PolyglotText{"en": "Hello", "fr": ""}
	if text := pt.GetOrLang("fr", "en"); text != "" {
		t.Errorf("expected empty string for present empty entry, got %s", text)
	}
}

func TestPolyglotText_GetOrLang_Nil(t *testing.T) {
	var pt PolyglotText
	if text := pt.GetOrLang("de", "en"); text != "" {
		t.Errorf("expected empty string for nil receiver, got %s", text)
	}
}

func TestPolyglotText_GetOrDefault(t *testing.T) {
	pt := PolyglotText{"en": "Hello", "es": ""}
	if text := pt.GetOrDefault("en", "unused"); text != "Hello" {
		t.Errorf("expected 'Hello', got %s", text)
	}
	if text := pt.GetOrDefault("fr", "Bonjour"); text != "Bonjour" {
		t.Errorf("expected default 'Bonjour', got %s", text)
	}
	if text := pt.GetOrDefault("es", "unused"); text != "" {
		t.Errorf("expected empty string for present empty entry, got %s", text)
	}
}

func TestPolyglotText_GetOrDefault_Nil(t *testing.T) {
	var pt PolyglotText
	if text := pt.GetOrDefault("en", "Hello"); text != "Hello" {
		t.Errorf("expected default 'Hello' for nil receiver, got %s", text)
	}
}

func TestPolyglotText_Proto(t *testing.T) {
	pt := PolyglotText{"en": "Hello", "fr": "Bonjour"}
	proto := pt.Proto()
	if proto == nil {
		t.Fatal("expected non-nil proto")
	}
	if proto.DisplayStrings["en"] != "Hello" || proto.DisplayStrings["fr"] != "Bonjour" {
		t.Errorf("expected display strings [en:Hello fr:Bonjour], got %v", proto.DisplayStrings)
	}
}

func TestPolyglotText_FromProto(t *testing.T) {
	proto := &protos.PolyglotText{DisplayStrings: map[string]string{"en": "Hello", "fr": "Bonjour"}}
	pt := PolyglotTextFromProto(proto)
	if pt["en"] != "Hello" || pt["fr"] != "Bonjour" {
		t.Errorf("expected display strings [en:Hello fr:Bonjour], got %v", pt)
	}
}

func TestPolyglotText_FromProto_Empty(t *testing.T) {
	proto := &protos.PolyglotText{}
	pt := PolyglotTextFromProto(proto)
	if pt != nil {
		t.Errorf("expected empty PolyglotText, got %v", pt)
	}
}

func TestPolyglotText_FromProto_Nil(t *testing.T) {
	pt := PolyglotTextFromProto(nil)
	if pt != nil {
		t.Errorf("expected empty PolyglotText, got %v", pt)
	}
}
