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
 * @brief Language pack wrapper type for the Catena SDK.
 * @file language.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package catena

import (
	"maps"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// LanguagePack wraps a protos.LanguagePack. A language pack is a display name
// plus a dictionary of string identifiers (e.g. "greeting") to translated
// strings, all in the same language.
type LanguagePack struct {
	pack *protos.LanguagePack
}

// NewLanguagePack creates a LanguagePack with the given display name and words.
// The words map is copied so later mutations by the caller do not affect the
// pack.
func NewLanguagePack(name string, words map[string]string) LanguagePack {
	return LanguagePack{
		pack: &protos.LanguagePack{
			Name:  name,
			Words: maps.Clone(words),
		},
	}
}

// GetProtoLanguagePack returns the underlying protos.LanguagePack.
func (lp LanguagePack) GetProtoLanguagePack() *protos.LanguagePack {
	return lp.pack
}

// GetName returns the language pack's display name, or "" if unset.
func (lp LanguagePack) GetName() string {
	return lp.pack.GetName()
}

// GetWords returns the language pack's word dictionary, or nil if unset.
func (lp LanguagePack) GetWords() map[string]string {
	return lp.pack.GetWords()
}
