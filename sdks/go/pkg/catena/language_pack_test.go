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
 * @brief Unit tests for language pack handling in the Catena SDK.
 * @file language_pack_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import "testing"

func TestLanguagePack(t *testing.T) {
	t.Run("NewInitializesProto", func(t *testing.T) {
		lp := NewLanguagePack()
		if lp.Proto == nil {
			t.Fatal("expected non-nil Proto")
		}
	})

	t.Run("BuilderMergesWords", func(t *testing.T) {
		lp := NewLanguagePack().
			WithName("Global Spanish").
			WithWord("greeting", "Hola").
			WithWords(map[string]string{"parting": "Adiós", "greeting": "Buenos días"})

		if lp.GetName() != "Global Spanish" {
			t.Errorf("expected name 'Global Spanish', got %q", lp.GetName())
		}
		// WithWords should merge and overwrite the earlier WithWord entry.
		if lp.GetWords()["greeting"] != "Buenos días" {
			t.Errorf("expected greeting 'Buenos días', got %q", lp.GetWords()["greeting"])
		}
		if lp.GetWords()["parting"] != "Adiós" {
			t.Errorf("expected parting 'Adiós', got %q", lp.GetWords()["parting"])
		}
	})

	t.Run("WithWordsInitializesNilMap", func(t *testing.T) {
		// WithWords on a pack whose Words map is still nil must initialize it.
		lp := NewLanguagePack().WithWords(map[string]string{"greeting": "Hallo"})

		if lp.GetWords()["greeting"] != "Hallo" {
			t.Errorf("expected greeting 'Hallo', got %q", lp.GetWords()["greeting"])
		}
	})

	t.Run("BuilderZeroValueLazyInit", func(t *testing.T) {
		// Builder methods must be safe on a zero-value LanguagePack (nil proto).
		lp := LanguagePack{}.WithName("French").WithWord("greeting", "Bonjour")

		if lp.Proto == nil {
			t.Fatal("expected builder to lazily initialize the proto")
		}
		if lp.GetName() != "French" {
			t.Errorf("expected name 'French', got %q", lp.GetName())
		}
		if lp.GetWords()["greeting"] != "Bonjour" {
			t.Errorf("expected greeting 'Bonjour', got %q", lp.GetWords()["greeting"])
		}
	})
}
