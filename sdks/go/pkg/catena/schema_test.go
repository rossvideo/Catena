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
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"testing"
)

func TestValidateFqoid_Valid(t *testing.T) {
	longIndex := "8" + strings.Repeat("0", 15)
	maxSegments := "root"
	for i := 0; i < 30; i++ {
		maxSegments += "/child"
	}

	tests := []string{
		"brightness",
		"product/name",
		"inputs/0/gain",
		"items/-",
		"streaming",
		"Stream",
		"a" + strings.Repeat("b", 30),
		"items/" + longIndex,
		"nines/" + strings.Repeat("9", 15),
		maxSegments,
	}

	for _, fqoid := range tests {
		t.Run(fqoid, func(t *testing.T) {
			result := ValidateFqoid(fqoid)
			if result.Code != StatusCodeOk {
				t.Errorf("ValidateFqoid(%q) = %v, %s", fqoid, result.Code, result.Error)
			}
		})
	}
}

func TestValidateFqoid_Invalid(t *testing.T) {

	t.Run("empty", func(t *testing.T) {
		expectInvalidFqoid(t, "")
	})
	t.Run("leading slash", func(t *testing.T) {
		expectInvalidFqoid(t, "/brightness")
	})
	t.Run("dot", func(t *testing.T) {
		expectInvalidFqoid(t, "foo.bar")
	})
	t.Run("reserved stream", func(t *testing.T) {
		expectInvalidFqoid(t, "stream")
	})
	t.Run("stream segment", func(t *testing.T) {
		expectInvalidFqoid(t, "foo/stream")
	})
	t.Run("stream in the middle", func(t *testing.T) {
		expectInvalidFqoid(t, "foo/stream/bar")
	})
	t.Run("dash not last", func(t *testing.T) {
		expectInvalidFqoid(t, "items/-/name")
	})
	t.Run("dash only", func(t *testing.T) {
		expectInvalidFqoid(t, "-")
	})
	t.Run("index first", func(t *testing.T) {
		expectInvalidFqoid(t, "0")
	})
	t.Run("leading zero index", func(t *testing.T) {
		expectInvalidFqoid(t, "items/01")
	})
	t.Run("double zero index", func(t *testing.T) {
		expectInvalidFqoid(t, "items/00")
	})
	t.Run("index too wide", func(t *testing.T) {
		expectInvalidFqoid(t, "items/9"+strings.Repeat("0", 15))
	})
	t.Run("segment too long", func(t *testing.T) {
		expectInvalidFqoid(t, "a"+strings.Repeat("b", 31))
	})
	t.Run("empty segment", func(t *testing.T) {
		expectInvalidFqoid(t, "foo//bar")
	})
	t.Run("trailing slash", func(t *testing.T) {
		expectInvalidFqoid(t, "foo/")
	})
	t.Run("too many segments", func(t *testing.T) {
		tooMany := "root"
		for i := 0; i < 31; i++ {
			tooMany += "/child"
		}

		expectInvalidFqoid(t, tooMany)
	})
}

func expectInvalidFqoid(t *testing.T, fqoid string) {
	t.Helper()
	result := ValidateFqoid(fqoid)
	if result.Code != StatusCodeInvalidArgument {
		t.Errorf("ValidateFqoid(%q) = %v, want InvalidArgument", fqoid, result.Code)
	}
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

func TestValidateLanguageCode_Invalid(t *testing.T) {
	tests := []struct {
		name string
		code string
	}{
		{name: "empty", code: ""},
		{name: "one letter", code: "e"},
		{name: "three letters", code: "eng"},
		{name: "uppercase language", code: "EN"},
		{name: "lowercase region", code: "en-us"},
		{name: "three letter region", code: "en-USA"},
		{name: "underscore", code: "en_US"},
		{name: "word", code: "english"},
		{name: "extra segment", code: "en-US-x"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result := ValidateLanguageCode(tt.code)
			if result.Code != StatusCodeInvalidArgument {
				t.Errorf("ValidateLanguageCode(%q) = %v, want InvalidArgument", tt.code, result.Code)
			}
		})
	}
}

// TestSchemaPatternsMatchDeviceYAML keeps the copies in schema.go equal to
// smpte/interface/schemata/device.yaml. The server cannot load that file at
// runtime because it is outside the Go module.
func TestSchemaPatternsMatchDeviceYAML(t *testing.T) {
	_, thisFile, _, ok := runtime.Caller(0)
	if !ok {
		t.Fatal("runtime.Caller failed")
	}
	schemaPath := filepath.Join(filepath.Dir(thisFile), "..", "..", "..", "..", "smpte", "interface", "schemata", "device.yaml")
	data, err := os.ReadFile(schemaPath)
	if err != nil {
		t.Fatalf("read schema: %v", err)
	}
	schema := string(data)

	tests := []struct {
		def  string
		want string
	}{
		{def: "fqoid", want: fqoidPattern},
		{def: "language_code", want: languageCodePattern},
	}
	for _, tt := range tests {
		t.Run(tt.def, func(t *testing.T) {
			got := schemaDefPattern(schema, tt.def)
			if got == "" {
				t.Fatalf("%s pattern not found in device.yaml", tt.def)
			}
			if got != tt.want {
				t.Errorf("%s pattern differs from device.yaml\n schema: %s\n    go: %s", tt.def, got, tt.want)
			}
		})
	}
}

// schemaDefPattern returns the pattern line under a two-space $defs key.
// The definition is "  <name>:"; its pattern is the following four-space "pattern:" line.
func schemaDefPattern(schema, name string) string {
	inDef := false
	header := "  " + name + ":"
	for _, line := range strings.Split(schema, "\n") {
		if line == header {
			inDef = true
			continue
		}
		if !inDef {
			continue
		}
		if strings.HasPrefix(line, "  ") && !strings.HasPrefix(line, "   ") {
			return ""
		}
		const prefix = "    pattern:"
		if strings.HasPrefix(line, prefix) {
			return strings.TrimSpace(line[len(prefix):])
		}
	}
	return ""
}
