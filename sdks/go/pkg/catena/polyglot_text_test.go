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

package catena

import (
	"testing"
)

func TestNewPolyglotText(t *testing.T) {
	pt := NewPolyglotText("en", "Hello")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
}

func TestPolyglotText_With(t *testing.T) {
	pt := NewPolyglotText("en", "Hello").With("fr", "Bonjour")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
	if pt["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt["fr"])
	}
}

func TestNewPolyglotText_Empty(t *testing.T) {
	pt := NewPolyglotText()
	if pt == nil {
		t.Fatal("expected non-nil PolyglotText")
	}
	if len(pt) != 0 {
		t.Errorf("expected empty PolyglotText, got %d entries", len(pt))
	}

	pt.With("en", "Hello").With("fr", "Bonjour")
	if pt["en"] != "Hello" || pt["fr"] != "Bonjour" {
		t.Errorf("expected entries to be added, got %v", pt)
	}
}

func TestNewPolyglotText_MultiplePairs(t *testing.T) {
	pt := NewPolyglotText("en", "Hello", "fr", "Bonjour")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
	if pt["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt["fr"])
	}
}

func TestNewPolyglotText_OddArgs(t *testing.T) {
	pt := NewPolyglotText("en", "Hello", "fr")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
	if _, ok := pt["fr"]; ok {
		t.Errorf("expected unpaired trailing lang to be ignored, got %v", pt)
	}
}

func TestPolyglotText_Get(t *testing.T) {
	pt := NewPolyglotText("en", "Hello").With("fr", "Bonjour")
	if pt.Get("fr", "en") != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt.Get("fr", "en"))
	}
	if pt.Get("de", "en") != "Hello" {
		t.Errorf("expected fallback 'Hello', got %s", pt.Get("de", "en"))
	}
}

func TestPolyglotText_Get_MissingFallback(t *testing.T) {
	pt := NewPolyglotText("en", "Hello")
	if pt.Get("de", "fr") != "" {
		t.Errorf("expected empty string for missing fallback, got %s", pt.Get("de", "fr"))
	}
}
