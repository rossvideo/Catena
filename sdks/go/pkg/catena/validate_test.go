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
 * @author Omar Shah (omar.shah@rossvideo.com)
 * @date 2026-10-09
 */

package catena

import (
	"testing"
)

func TestValidateSlot(t *testing.T) {

	t.Run("valid slot, zero value", func(t *testing.T) {
		result, err := ValidateSlot(0)
		if err.Code != StatusCodeOk {
			t.Errorf("expected no error, got %v", err)
		}
		if result != uint16(0) {
			t.Errorf("expected result %v, got %v", 0, result)
		}
	})

	t.Run("valid slot, typical value", func(t *testing.T) {
		result, err := ValidateSlot(42)
		if err.Code != StatusCodeOk {
			t.Errorf("expected no error, got %v", err)
		}
		if result != uint16(42) {
			t.Errorf("expected result %v, got %v", 42, result)
		}
	})

	t.Run("valid slot max uint16", func(t *testing.T) {
		result, err := ValidateSlot(65535)
		if err.Code != StatusCodeOk {
			t.Errorf("expected no error, got %v", err)
		}
		if result != uint16(65535) {
			t.Errorf("expected result %v, got %v", 65535, result)
		}
	})

	t.Run("invalid slot negative (uint32 underflow)", func(t *testing.T) {
		result, err := ValidateSlot(^uint32(0))
		if err.Code != StatusCodeInvalidArgument {
			t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0 on error, got %v", result)
		}
	})

	t.Run("invalid slot too large", func(t *testing.T) {
		result, err := ValidateSlot(65536)
		if err.Code != StatusCodeInvalidArgument {
			t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0 on error, got %v", result)
		}
	})
}

func TestValidateSlotString(t *testing.T) {
	t.Run("zero", func(t *testing.T) {
		result, err := ValidateSlotString("0")
		if err.Code != StatusCodeOk {
			t.Errorf("expected no error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0, got %v", result)
		}
	})

	t.Run("typical", func(t *testing.T) {
		result, err := ValidateSlotString("42")
		if err.Code != StatusCodeOk {
			t.Errorf("expected no error, got %v", err)
		}
		if result != 42 {
			t.Errorf("expected result 42, got %v", result)
		}
	})

	t.Run("max uint16", func(t *testing.T) {
		result, err := ValidateSlotString("65535")
		if err.Code != StatusCodeOk {
			t.Errorf("expected no error, got %v", err)
		}
		if result != uint16(65535) {
			t.Errorf("expected result 65535, got %v", result)
		}
	})

	t.Run("empty", func(t *testing.T) {
		result, err := ValidateSlotString("")
		if err.Code != StatusCodeInvalidArgument {
			t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0 on error, got %v", result)
		}
	})

	t.Run("negative", func(t *testing.T) {
		result, err := ValidateSlotString("-1")
		if err.Code != StatusCodeInvalidArgument {
			t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0 on error, got %v", result)
		}
	})

	t.Run("too large", func(t *testing.T) {
		result, err := ValidateSlotString("65536")
		if err.Code != StatusCodeInvalidArgument {
			t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0 on error, got %v", result)
		}
	})

	t.Run("non-numeric", func(t *testing.T) {
		result, err := ValidateSlotString("abc")
		if err.Code != StatusCodeInvalidArgument {
			t.Errorf("expected StatusCodeInvalidArgument error, got %v", err)
		}
		if result != 0 {
			t.Errorf("expected result 0 on error, got %v", result)
		}
	})
}

func TestValidateFqoid(t *testing.T) {
	t.Run("valid", func(t *testing.T) {
		tests := []struct {
			name  string
			fqoid string
		}{
			{name: "single segment", fqoid: "gain"},
			{name: "indexed path", fqoid: "inputs/0/gain"},
		}
		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				result := ValidateFqoid(tt.fqoid)
				if result.Code != StatusCodeOk {
					t.Errorf("ValidateFqoid(%q) = %v, %s", tt.fqoid, result.Code, result.Error)
				}
			})
		}
	})

	t.Run("invalid", func(t *testing.T) {
		tests := []struct {
			name  string
			fqoid string
		}{
			{name: "empty", fqoid: ""},
			{name: "leading slash", fqoid: "/brightness"},
			{name: "reserved stream", fqoid: "stream"},
		}

		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				result := ValidateFqoid(tt.fqoid)
				if result.Code != StatusCodeInvalidArgument {
					t.Errorf("ValidateFqoid(%q) = %v, want InvalidArgument", tt.fqoid, result.Code)
				}
			})
		}
	})
}

func TestValidateLanguageCode(t *testing.T) {
	t.Run("valid", func(t *testing.T) {
		tests := []struct {
			name string
			code string
		}{
			{name: "language", code: "en"},
			{name: "language and region", code: "en-US"},
		}

		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				result := ValidateLanguageCode(tt.code)
				if result.Code != StatusCodeOk {
					t.Errorf("ValidateLanguageCode(%q) = %v, %s", tt.code, result.Code, result.Error)
				}
			})
		}
	})

	t.Run("invalid", func(t *testing.T) {
		tests := []struct {
			name string
			code string
		}{
			{name: "empty", code: ""},
			{name: "uppercase language", code: "EN"},
			{name: "word", code: "english"},
		}

		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				result := ValidateLanguageCode(tt.code)
				if result.Code != StatusCodeInvalidArgument {
					t.Errorf("ValidateLanguageCode(%q) = %v, want InvalidArgument", tt.code, result.Code)
				}
			})
		}
	})
}
