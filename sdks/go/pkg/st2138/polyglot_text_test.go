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
	"errors"
	"testing"
)

func TestNewPolyglotText(t *testing.T) {
	pt := NewPolyglotText("en", "Hello")
	if pt.polyglot["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt.polyglot["en"])
	}
}

func TestPolyglotText_With(t *testing.T) {
	pt := NewPolyglotText("en", "Hello").With("fr", "Bonjour")
	if pt.polyglot["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt.polyglot["en"])
	}
	if pt.polyglot["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt.polyglot["fr"])
	}
}

func TestNewPolyglotText_Empty(t *testing.T) {
	pt := NewPolyglotText()
	if pt.polyglot == nil {
		t.Fatal("expected non-nil PolyglotText")
	}
	if len(pt.polyglot) != 0 {
		t.Errorf("expected empty PolyglotText, got %d entries", len(pt.polyglot))
	}

	pt.With("en", "Hello").With("fr", "Bonjour")
	if pt.polyglot["en"] != "Hello" || pt.polyglot["fr"] != "Bonjour" {
		t.Errorf("expected entries to be added, got %v", pt.polyglot)
	}
}

func TestNewPolyglotText_MultiplePairs(t *testing.T) {
	pt := NewPolyglotText("en", "Hello", "fr", "Bonjour")
	if pt.polyglot["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt.polyglot["en"])
	}
	if pt.polyglot["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt.polyglot["fr"])
	}
}

func TestNewPolyglotText_OddArgs(t *testing.T) {
	pt, err := NewPolyglotText("en", "Hello", "fr").Build()
	if err == nil {
		t.Fatalf("expected error, got nil")
	}
	if len(pt) != 0 {
		t.Fatal("expected empty polyglot text")
	}
}

func TestPolyglotText_Get(t *testing.T) {
	pt := NewPolyglotText("en", "Hello").With("fr", "Bonjour")
	if pt.polyglot.Get("fr", "en") != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt.polyglot.Get("fr", "en"))
	}
	if pt.polyglot.Get("de", "en") != "Hello" {
		t.Errorf("expected fallback 'Hello', got %s", pt.polyglot.Get("de", "en"))
	}
}

func TestPolyglotText_Get_MissingFallback(t *testing.T) {
	pt := NewPolyglotText("en", "Hello")
	if pt.polyglot.Get("de", "fr") != "" {
		t.Errorf("expected empty string for missing fallback, got %s", pt.polyglot.Get("de", "fr"))
	}
}

func TestPolyglotText_Build(t *testing.T) {
	polyglot, err := NewPolyglotText("en", "Hello").Build()
	if err != nil {
		t.Fatalf("expected NewPolyglotText to build without error, got %v", err)
	}
	if polyglot["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", polyglot["en"])
	}
}

func TestPolyglotText_BuildSurfacesErrors(t *testing.T) {
	polyglot, err := NewPolyglotText("en").With("fr", "Bonjour").Build()
	if err == nil {
		t.Fatalf("expected NewPolyglotText to return an error, got nil")
	}
	if len(polyglot) != 0 {
		t.Errorf("expected empty PolyglotText, got %v", polyglot)
	}
}

func TestPolyglotTextBuilder_WithShortCircuitErrors(t *testing.T) {
	pb := NewPolyglotText("en", "Hello")
	pb.err = errors.New("test error")

	text, err := pb.With("fr", "Bonjour").Build()
	if err == nil || err.Error() != "test error" {
		t.Fatalf("expected 'test error', got %v", err)
	}
	if len(text) != 0 {
		t.Fatal("expected empty polyglot text for builder with error")
	}
	if _, ok := pb.polyglot["fr"]; ok {
		t.Fatal("expected With to leave the polyglot text unchanged after an error")
	}
}

func TestPolyglotTextBuilder_Err(t *testing.T) {
	pb := NewPolyglotText("en", "Hello")
	pb.err = errors.New("test error")
	if pb.Err() == nil {
		t.Fatal("expected error for builder with error")
	}
	if pb.Err().Error() != "test error" {
		t.Fatal("expected error message 'test error', got", pb.Err())
	}
}
