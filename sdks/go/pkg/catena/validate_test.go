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
 * @brief Tests that validate.go matches the ST 2138 device schema.
 * @file validate_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Omar Shah (omar.shah@rossvideo.com)
 * @date 2026-10-09
 */

package catena

import (
	"testing"

	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestValidateRequestStart(t *testing.T) {
	tests := []struct {
		name     string
		value    string
		wantCode StatusCode
		want     time.Time
	}{
		{name: "valid", value: "1700000000123", wantCode: StatusCodeOk, want: time.UnixMilli(1700000000123)},
		{name: "zero", value: "0", wantCode: StatusCodeOk, want: time.UnixMilli(0)},
		{name: "max int64", value: "9223372036854775807", wantCode: StatusCodeOk, want: time.UnixMilli(9223372036854775807)},
		{name: "empty", value: "", wantCode: StatusCodeInvalidArgument},
		{name: "non-numeric", value: "abc", wantCode: StatusCodeInvalidArgument},
		{name: "fractional", value: "1.5", wantCode: StatusCodeInvalidArgument},
		{name: "negative", value: "-1", wantCode: StatusCodeInvalidArgument},
		{name: "overflow", value: "9223372036854775808", wantCode: StatusCodeInvalidArgument},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, status := ValidateRequestStart(tt.value)
			if status.Code != tt.wantCode {
				t.Fatalf("expected code %v, got %v", tt.wantCode, status)
			}
			if tt.wantCode == StatusCodeOk {
				if !got.Equal(tt.want) {
					t.Errorf("expected %v, got %v", tt.want, got)
				}
				return
			}
			if !got.IsZero() {
				t.Errorf("expected zero time on error, got %v", got)
			}
		})
	}
}

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

func TestValidateDetailLevel(t *testing.T) {
	t.Run("valid", func(t *testing.T) {
		levels := []st2138.DetailLevel{
			st2138.DetailLevelFull,
			st2138.DetailLevelSubscriptions,
			st2138.DetailLevelMinimal,
			st2138.DetailLevelCommands,
			st2138.DetailLevelNone,
			st2138.DetailLevelUnset,
		}
		for _, level := range levels {
			result := ValidateDetailLevel(level)
			if result.Code != StatusCodeOk {
				t.Errorf("ValidateDetailLevel(%d) = %v, %s", level, result.Code, result.Error)
			}
		}
	})

	t.Run("invalid", func(t *testing.T) {
		levels := []st2138.DetailLevel{-1, 6, 99}
		for _, level := range levels {
			result := ValidateDetailLevel(level)
			if result.Code != StatusCodeInvalidArgument {
				t.Errorf("ValidateDetailLevel(%d) = %v, want InvalidArgument", level, result.Code)
			}
			if result.Error == "" {
				t.Errorf("ValidateDetailLevel(%d) returned empty error message", level)
			}
		}
	})
}

func TestValidateDetailLevelString(t *testing.T) {
	t.Run("valid names", func(t *testing.T) {
		tests := []struct {
			input string
			want  st2138.DetailLevel
		}{
			{"FULL", st2138.DetailLevelFull},
			{"SUBSCRIPTIONS", st2138.DetailLevelSubscriptions},
			{"MINIMAL", st2138.DetailLevelMinimal},
			{"COMMANDS", st2138.DetailLevelCommands},
			{"NONE", st2138.DetailLevelNone},
			{"UNSET", st2138.DetailLevelUnset},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				result, res := ValidateDetailLevelString(tt.input)
				if res.Code != StatusCodeOk {
					t.Errorf("ValidateDetailLevelString(%q) = %v, %s", tt.input, res.Code, res.Error)
				}
				if result != tt.want {
					t.Errorf("ValidateDetailLevelString(%q) = %d, want %d", tt.input, result, tt.want)
				}
			})
		}
	})

	t.Run("valid int string", func(t *testing.T) {
		tests := []struct {
			input string
			want  st2138.DetailLevel
		}{
			{"0", st2138.DetailLevelFull},
			{"1", st2138.DetailLevelSubscriptions},
			{"2", st2138.DetailLevelMinimal},
			{"3", st2138.DetailLevelCommands},
			{"4", st2138.DetailLevelNone},
			{"5", st2138.DetailLevelUnset},
		}
		for _, tt := range tests {
			t.Run(tt.input, func(t *testing.T) {
				result, res := ValidateDetailLevelString(tt.input)
				if res.Code != StatusCodeOk {
					t.Errorf("ValidateDetailLevelString(%q) = %v, %s", tt.input, res.Code, res.Error)
				}
				if result != tt.want {
					t.Errorf("ValidateDetailLevelString(%q) = %d, want %d", tt.input, result, tt.want)
				}
			})
		}
	})

	t.Run("invalid", func(t *testing.T) {
		tests := []struct {
			name  string
			input string
		}{
			{name: "empty", input: ""},
			{name: "lowercase name", input: "minimal"},
			{name: "unknown name", input: "BOGUS"},
			{name: "out-of-range int", input: "6"},
			{name: "negative int", input: "-1"},
			{name: "int overflows int32", input: "4294967298"},
			{name: "non-integer number", input: "1.5"},
		}
		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				result, res := ValidateDetailLevelString(tt.input)
				if res.Code != StatusCodeInvalidArgument {
					t.Errorf("ValidateDetailLevelString(%q) = %v, want InvalidArgument", tt.input, res.Code)
				}
				if result != 0 {
					t.Errorf("expected result 0 on error, got %v", result)
				}
			})
		}
	})
}
