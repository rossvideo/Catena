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
 * @brief PolyglotText type for multilingual display strings.
 * @file polyglot_text.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-10
 */

package catena

import (
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// PolyglotText maps BCP-47 language codes to display strings.
type PolyglotText map[string]string

// NewPolyglotText creates a PolyglotText from zero or more (lang, text) pairs.
// Calling it with no arguments yields an empty PolyglotText, which is useful
// for building entries up in a loop before passing the result to a builder:
//
//	name := NewPolyglotText()
//	for lang := range langs {
//		name.With(lang, translate(lang, "Hello"))
//	}
func NewPolyglotText(pairs ...string) PolyglotText {
	p := PolyglotText{}
	for i := 0; i+1 < len(pairs); i += 2 {
		p[pairs[i]] = pairs[i+1]
	}
	if len(pairs)%2 != 0 {
		logger.Warning("NewPolyglotText: odd number of arguments; ignoring trailing lang with no text",
			"lang", pairs[len(pairs)-1])
	}
	return p
}

// With returns the PolyglotText with an additional language entry added.
func (p PolyglotText) With(lang, text string) PolyglotText {
	p[lang] = text
	return p
}

// Get returns the display string for lang, falling back to fallback if not present.
func (p PolyglotText) Get(lang, fallback string) string {
	if s, ok := p[lang]; ok {
		return s
	}
	return p[fallback]
}
