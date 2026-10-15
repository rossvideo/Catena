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
 * @brief Copies of regular expressions from the ST 2138 device schema.
 * @file schema.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Omar Shah (@omar.shah@rossvideo.com)
 * @date 2026-10-09
 */

package catena

import (
	"fmt"
	"math"
	"strconv"

	"github.com/dlclark/regexp2"
)

// Schema patterns copied from smpte/interface/schemata/device.yaml.
// The schema file lives outside this Go module, so the running server cannot
// read it. TestSchemaPatternsMatchDeviceYAML fails if a copy diverges.
// JSON Schema regular expressions are ECMAScript. fqoid uses lookaheads, which
// Go's regexp package cannot compile, so these are compiled with regexp2.
const (
	// fqoidPattern is $defs/fqoid.pattern.
	fqoidPattern = `^(?!.*\.)(?!stream(?:$|/))[A-Za-z][A-Za-z0-9_]{0,30}(?:/(?:(?!stream(?:$|/))[A-Za-z][A-Za-z0-9_]{0,30}|(?:0|[1-9]\d{0,14}|[1-8]\d{15}))){0,29}(?:/(?:(?!stream$)[A-Za-z][A-Za-z0-9_]{0,30}|(?:0|[1-9]\d{0,14}|[1-8]\d{15})|-))?$`

	// languageCodePattern is $defs/language_code.pattern.
	languageCodePattern = `^[a-z]{2}(-[A-Z]{2})?$`
)

var (
	fqoidRegexp        = regexp2.MustCompile(fqoidPattern, regexp2.ECMAScript)
	languageCodeRegexp = regexp2.MustCompile(languageCodePattern, regexp2.ECMAScript)
)

func ValidateSlot(slot uint32) (uint16, StatusResult) {
	if slot > uint32(math.MaxUint16) {
		return 0, StatusWithCode(StatusCodeInvalidArgument, fmt.Sprintf("invalid slot number: %d", slot))

	}
	return uint16(slot), StatusWithCode(StatusCodeOk, "")
}

func ValidateSlotString(slot string) (uint16, StatusResult) {

	if slot == "" {
		return 0, StatusWithCode(StatusCodeInvalidArgument, "slot is required")
	}

	slotInt, err := strconv.ParseUint(slot, 10, 32)
	if err != nil {
		return 0, StatusWithCode(StatusCodeInvalidArgument, fmt.Sprintf("invalid slot string: %s", slot))
	}
	return ValidateSlot(uint32(slotInt))
}

// ValidateFqoid reports whether fqoid matches fqoidPattern. An empty string is
// reported separately: proto3 delivers an omitted oid as "", and the spec
// requires the field.
func ValidateFqoid(fqoid string) StatusResult {
	if fqoid == "" {
		return StatusWithCode(StatusCodeInvalidArgument, "fqoid is required")
	}
	matched, err := fqoidRegexp.MatchString(fqoid)
	if err != nil || !matched {
		return StatusWithCode(StatusCodeInvalidArgument, "invalid fqoid: "+fqoid)
	}
	return StatusWithCode(StatusCodeOk, "")
}

// ValidateLanguageCode reports whether languageCode matches languageCodePattern.
// An empty string is reported separately: proto3 delivers an omitted language
// code as "", and the spec requires the field.
func ValidateLanguageCode(languageCode string) StatusResult {
	if languageCode == "" {
		return StatusWithCode(StatusCodeInvalidArgument, "language code is required")
	}
	matched, err := languageCodeRegexp.MatchString(languageCode)
	if err != nil || !matched {
		return StatusWithCode(StatusCodeInvalidArgument, "invalid language code: "+languageCode)
	}
	return StatusWithCode(StatusCodeOk, "")
}
