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
 * @brief Response contract for the Catena SDK.
 * @file status_code.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// StatusCode is a transport-neutral outcome for handlers. It is defined in the
// st2138 package (see st2138/StatusCode.md for per-code semantics); this alias
// keeps the catena.StatusCode spelling for the server and response contract.
type StatusCode = st2138.StatusCode

// StatusResult pairs a StatusCode with an optional error message. It is defined
// in the st2138 package; this alias keeps the catena.StatusResult spelling.
type StatusResult = st2138.StatusResult

const (
	StatusCodeOk                 = st2138.StatusCodeOk
	StatusCodeCancelled          = st2138.StatusCodeCancelled
	StatusCodeUnknown            = st2138.StatusCodeUnknown
	StatusCodeInvalidArgument    = st2138.StatusCodeInvalidArgument
	StatusCodeDeadlineExceeded   = st2138.StatusCodeDeadlineExceeded
	StatusCodeNotFound           = st2138.StatusCodeNotFound
	StatusCodeAlreadyExists      = st2138.StatusCodeAlreadyExists
	StatusCodePermissionDenied   = st2138.StatusCodePermissionDenied
	StatusCodeResourceExhausted  = st2138.StatusCodeResourceExhausted
	StatusCodeFailedPrecondition = st2138.StatusCodeFailedPrecondition
	StatusCodeAborted            = st2138.StatusCodeAborted
	StatusCodeOutOfRange         = st2138.StatusCodeOutOfRange
	StatusCodeUnimplemented      = st2138.StatusCodeUnimplemented
	StatusCodeInternal           = st2138.StatusCodeInternal
	StatusCodeUnavailable        = st2138.StatusCodeUnavailable
	StatusCodeDataLoss           = st2138.StatusCodeDataLoss
	StatusCodeUnauthenticated    = st2138.StatusCodeUnauthenticated
)

// ResponseType is a constraint for types that can be returned from handlers
type ResponseType interface {
	st2138.Value | st2138.Asset | st2138.Device | st2138.Param | LanguagePack
}

// Reply returns a successful response (StatusCodeOk) with the given value.
// Usage: catena.Reply(value), catena.Reply(asset), catena.Reply(device)
func Reply[T ResponseType](value T) (T, StatusResult) {
	return value, StatusResult{Code: StatusCodeOk}
}

// ReplyWithCode returns a response with the given value and status code.
// Usage: catena.ReplyWithCode(value, catena.StatusCodeOk)
func ReplyWithCode[T ResponseType](value T, code StatusCode) (T, StatusResult) {
	return value, StatusResult{Code: code}
}

// ReplyError returns an error response with the given status code and message.
// The value returned is the zero value of T.
// Usage: catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "not found")
func ReplyError[T ResponseType](code StatusCode, msg string) (T, StatusResult) {
	var zero T
	return zero, StatusResult{Code: code, Error: msg}
}

// StatusWithCode returns a StatusResult with the given StatusCode and optional message.
func StatusWithCode(code StatusCode, msg string) StatusResult {
	return st2138.StatusWithCode(code, msg)
}
