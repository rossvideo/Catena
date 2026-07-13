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

// LanguagePack wraps a protos.DeviceComponent_ComponentLanguagePack and exposes
// a fluent builder API so SDK users don't have to touch protos directly.
type LanguagePack struct {
	Proto *protos.DeviceComponent_ComponentLanguagePack
}

// NewLanguagePack creates a LanguagePack for the given language code (e.g. "es").
func NewLanguagePack(language string) LanguagePack {
	return LanguagePack{
		Proto: &protos.DeviceComponent_ComponentLanguagePack{
			Language:     language,
			LanguagePack: &protos.LanguagePack{},
		},
	}
}

// WithName sets the human-readable name of the language pack (e.g. "Global Spanish").
func (lp LanguagePack) WithName(name string) LanguagePack {
	lp.ensurePack()
	lp.Proto.LanguagePack.Name = name
	return lp
}

// WithWord adds or overwrites a single translation entry (e.g. "greeting" -> "Hola").
func (lp LanguagePack) WithWord(key, value string) LanguagePack {
	lp.ensurePack()
	if lp.Proto.LanguagePack.Words == nil {
		lp.Proto.LanguagePack.Words = map[string]string{}
	}
	lp.Proto.LanguagePack.Words[key] = value
	return lp
}

// WithWords merges the given translation entries into the language pack.
func (lp LanguagePack) WithWords(words map[string]string) LanguagePack {
	lp.ensurePack()
	if lp.Proto.LanguagePack.Words == nil {
		lp.Proto.LanguagePack.Words = map[string]string{}
	}
	for key, value := range words {
		lp.Proto.LanguagePack.Words[key] = value
	}
	return lp
}

// ensurePack lazily initializes the wrapped proto so the builder methods are
// safe to call even on a zero-value LanguagePack.
func (lp *LanguagePack) ensurePack() {
	if lp.Proto == nil {
		lp.Proto = &protos.DeviceComponent_ComponentLanguagePack{}
	}
	if lp.Proto.LanguagePack == nil {
		lp.Proto.LanguagePack = &protos.LanguagePack{}
	}
}
