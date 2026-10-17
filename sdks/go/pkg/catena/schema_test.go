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
 * @brief Tests that schema.go matches the ST 2138 device schema.
 * @file schema_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Omar Shah (@omar.shah@rossvideo.com)
 * @date 2026-10-09
 */

package catena

import (
	"fmt"
	"testing"
)

func TestValidateSlot_Valid(t *testing.T) {
	tests := []struct {
		name string
		in   uint32
	}{
		{name: "zero", in: 0},
		{name: "one", in: 1},
		{name: "typical", in: 42},
		{name: "max uint16", in: 65535},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlot(tt.in)
			if err.Code != StatusCodeOk {
				t.Errorf("expected no error, got %v", err)
			}
			if result != uint16(tt.in) {
				t.Errorf("expected result %v, got %v", tt.in, result)
			}
		})
	}
}

func TestValidateSlot_Invalid(t *testing.T) {
	tests := []struct {
		name string
		in   uint32
	}{
		{name: "negative (uint32 underflow)", in: ^uint32(0)}, // 0xFFFFFFFF = 4294967295, which is -1 if interpreted as signed
		{name: "too large", in: 65536},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlot(tt.in)
			if err.Code != StatusCodeInvalidArgument {
				t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
			}
			if result != 0 {
				t.Errorf("expected result 0 on error, got %v", result)
			}
		})
	}
}

func TestValidateSlotString_Valid(t *testing.T) {
	tests := []struct {
		name string
		in   string
	}{
		{name: "zero", in: "0"},
		{name: "one", in: "1"},
		{name: "typical", in: "42"},
		{name: "max uint16", in: "65535"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlotString(tt.in)
			if err.Code != StatusCodeOk {
				t.Errorf("expected no error, got %v", err)
			}
			expected := uint16(0)
			fmt.Sscanf(tt.in, "%d", &expected)
			if result != expected {
				t.Errorf("expected result %v, got %v", expected, result)
			}
		})
	}
}

func TestValidateSlotString_Invalid(t *testing.T) {
	tests := []struct {
		name string
		in   string
	}{
		{name: "negative", in: "-1"},
		{name: "too large", in: "65536"},
		{name: "non-numeric", in: "abc"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result, err := ValidateSlotString(tt.in)
			if err.Code != StatusCodeInvalidArgument {
				t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
			}
			if result != 0 {
				t.Errorf("expected result 0 on error, got %v", result)
			}
		})
	}
}

func expectInvalidFqoid(t *testing.T, fqoid string) {
	t.Helper()
	result := ValidateFqoid(fqoid)
	if result.Code != StatusCodeInvalidArgument {
		t.Errorf("ValidateFqoid(%q) = %v, want InvalidArgument", fqoid, result.Code)
	}
}

func TestValidateFqoid(t *testing.T) {

	t.Run("valid", func(t *testing.T) {
		result := ValidateFqoid("inputs/0/gain")
		if result.Code != StatusCodeOk {
			t.Errorf("ValidateFqoid(%q) = %v, %s", "inputs/0/gain", result.Code, result.Error)
		}
	})

	t.Run("empty", func(t *testing.T) {
		expectInvalidFqoid(t, "")
	})
	t.Run("leading slash", func(t *testing.T) {
		expectInvalidFqoid(t, "/brightness")
	})
	t.Run("trailing slash", func(t *testing.T) {
		expectInvalidFqoid(t, "foo/")
	})
	t.Run("dot", func(t *testing.T) {
		expectInvalidFqoid(t, "foo.bar")
	})
	t.Run("reserved stream", func(t *testing.T) {
		expectInvalidFqoid(t, "stream")
	})
	t.Run("empty segment", func(t *testing.T) {
		expectInvalidFqoid(t, "foo//bar")
	})
}

func TestValidateLanguageCode_Valid(t *testing.T) {
	tests := []string{
		"en",
		"fr",
		"en-US",
		"zz-ZZ",
	}

	for _, code := range tests {
		t.Run(code, func(t *testing.T) {
			result := ValidateLanguageCode(code)
			if result.Code != StatusCodeOk {
				t.Errorf("ValidateLanguageCode(%q) = %v, %s", code, result.Code, result.Error)
			}
		})
	}
}

func expectInvalidLanguageCode(t *testing.T, code string) {
	t.Helper()
	result := ValidateLanguageCode(code)
	if result.Code != StatusCodeInvalidArgument {
		t.Errorf("ValidateLanguageCode(%q) = %v, want InvalidArgument", code, result.Code)
	}
}

func TestValidateLanguageCode(t *testing.T) {

	t.Run("valid", func(t *testing.T) {
		result := ValidateLanguageCode("en")
		if result.Code != StatusCodeOk {
			t.Errorf("ValidateLanguageCode(%q) = %v, %s", "en", result.Code, result.Error)
		}
	})

	t.Run("empty", func(t *testing.T) {
		expectInvalidLanguageCode(t, "")
	})
	t.Run("one letter", func(t *testing.T) {
		expectInvalidLanguageCode(t, "e")
	})
	t.Run("three letters", func(t *testing.T) {
		expectInvalidLanguageCode(t, "eng")
	})
	t.Run("uppercase language", func(t *testing.T) {
		expectInvalidLanguageCode(t, "EN")
	})
	t.Run("region code", func(t *testing.T) {
		expectInvalidLanguageCode(t, "en-us")
	})
	t.Run("word", func(t *testing.T) {
		expectInvalidLanguageCode(t, "english")
	})
}
