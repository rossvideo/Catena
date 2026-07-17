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
 * @brief Language pack handling for the Catena SDK.
 * @file language_pack.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import "github.com/rossvideo/catena/sdks/go/pkg/protos"

// LanguagePack wraps a protos.LanguagePack and exposes a fluent builder API so
// SDK users don't have to touch protos directly. A language pack is a display
// name plus a dictionary of string identifiers (e.g. "greeting") to translated
// strings, all in the same language. The language tag itself (e.g. "es") is
// carried separately by the handlers, not by the pack.
type LanguagePack struct {
	Proto *protos.LanguagePack
}

// NewLanguagePack creates an empty LanguagePack ready to be populated via the
// WithName/WithWord/WithWords builder methods.
func NewLanguagePack() LanguagePack {
	return LanguagePack{Proto: &protos.LanguagePack{}}
}

// WithName sets the human-readable name of the language pack (e.g. "Global Spanish").
func (lp LanguagePack) WithName(name string) LanguagePack {
	lp.ensurePack()
	lp.Proto.Name = name
	return lp
}

// WithWord adds or overwrites a single translation entry (e.g. "greeting" -> "Hola").
func (lp LanguagePack) WithWord(key, value string) LanguagePack {
	lp.ensurePack()
	if lp.Proto.Words == nil {
		lp.Proto.Words = map[string]string{}
	}
	lp.Proto.Words[key] = value
	return lp
}

// WithWords merges the given translation entries into the language pack.
func (lp LanguagePack) WithWords(words map[string]string) LanguagePack {
	lp.ensurePack()
	if lp.Proto.Words == nil {
		lp.Proto.Words = map[string]string{}
	}
	for key, value := range words {
		lp.Proto.Words[key] = value
	}
	return lp
}

// GetName returns the language pack's display name, or "" if unset.
func (lp LanguagePack) GetName() string {
	return lp.Proto.GetName()
}

// GetWords returns the language pack's word dictionary, or nil if unset.
func (lp LanguagePack) GetWords() map[string]string {
	return lp.Proto.GetWords()
}

// ensurePack lazily initializes the wrapped proto so the builder methods are
// safe to call even on a zero-value LanguagePack.
func (lp *LanguagePack) ensurePack() {
	if lp.Proto == nil {
		lp.Proto = &protos.LanguagePack{}
	}
}
