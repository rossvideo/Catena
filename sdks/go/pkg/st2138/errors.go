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
 * @brief Sentinel errors for the st2138 model/conversion helpers.
 * @file errors.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package st2138

import "errors"

// Sentinel errors returned (wrapped) by the st2138 conversion and model
// helpers, named after the stdlib io/fs idiom. The st2138 package is transport-
// and SDK-neutral, so it does not know about the SDK's StatusResult/StatusCode
// contract, and it deliberately does not mirror those code names. Callers match
// these sentinels with errors.Is and pick a StatusCode themselves based on
// context: the same ErrInvalid means StatusCodeInvalidArgument when the bad
// value came from the client, but StatusCodeInternal when the server itself
// constructed the value it passed in.
//
// Helpers wrap a sentinel with context using fmt.Errorf("...: %w", ErrX) so the
// returned error carries a descriptive message while remaining matchable.
var (
	// ErrInvalid indicates an input that cannot be converted or is otherwise
	// invalid (nil value, unsupported type, malformed payload).
	ErrInvalid = errors.New("invalid argument")

	// ErrNotExist indicates a referenced entity does not exist (e.g. an unknown
	// param OID while walking a device subtree).
	ErrNotExist = errors.New("does not exist")
)
