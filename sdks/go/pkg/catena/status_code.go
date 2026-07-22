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
	"errors"

	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// StatusCode is a transport-neutral outcome for handlers. Values 0-16 match
// gRPC's canonical code space (ST 2138-11 §6.2 Table 2) and map 1:1 in the
// gRPC transport. See StatusCode.md for per-code semantics and how the REST
// transport chooses 200 vs 204 per ST 2138-12.
//
// Based on Google's gRPC project (Apache 2.0 license):
// https://github.com/grpc/grpc
// http://www.apache.org/licenses/LICENSE-2.0
type StatusCode int

// StatusResult pairs a StatusCode with an optional error message. It is the
// SDK-level response contract handlers return; the lower-level st2138 package
// reports failures with idiomatic errors (see st2138's sentinel errors), which
// the server maps into a StatusResult via statusFromError.
type StatusResult struct {
	Code  StatusCode
	Error string
}

func (s StatusResult) IsOk() bool {
	return s.Code == StatusCodeOk
}

func (s StatusResult) IsError() bool {
	return s.Code != StatusCodeOk
}

const (
	// StatusCodeOk indicates the operation completed successfully.
	StatusCodeOk StatusCode = 0

	// StatusCodeCancelled indicates the operation was cancelled, typically by
	// the caller (closed stream, cancelled context).
	StatusCodeCancelled StatusCode = 1

	// StatusCodeUnknown indicates a failure that cannot be classified more
	// specifically.
	StatusCodeUnknown StatusCode = 2

	// StatusCodeInvalidArgument indicates the client supplied a request that
	// cannot succeed as-is (bad JSON, wrong type, malformed fqoid, missing
	// required field).
	StatusCodeInvalidArgument StatusCode = 3

	// StatusCodeDeadlineExceeded indicates the deadline expired before the
	// operation could complete.
	StatusCodeDeadlineExceeded StatusCode = 4

	// StatusCodeNotFound indicates a referenced entity does not exist
	// (unknown slot, missing param, unknown route).
	StatusCodeNotFound StatusCode = 5

	// StatusCodeAlreadyExists indicates a create would duplicate a unique
	// resource.
	StatusCodeAlreadyExists StatusCode = 6

	// StatusCodePermissionDenied indicates the caller is authenticated but
	// not authorized (token scope insufficient for the command/param).
	StatusCodePermissionDenied StatusCode = 7

	// StatusCodeResourceExhausted indicates a quota or rate limit was hit,
	// or another resource has been exhausted.
	StatusCodeResourceExhausted StatusCode = 8

	// StatusCodeFailedPrecondition indicates the system is in the wrong
	// state for the requested operation. (Distinct from HTTP 412 Precondition
	// Failed; see StatusCode.md.)
	StatusCodeFailedPrecondition StatusCode = 9

	// StatusCodeAborted indicates a concurrency or transaction conflict;
	// the client may retry at a higher level.
	StatusCodeAborted StatusCode = 10

	// StatusCodeOutOfRange indicates a value is past the end of a valid
	// range (e.g. accessing band 5 of a 4-band eq).
	StatusCodeOutOfRange StatusCode = 11

	// StatusCodeUnimplemented indicates the operation or capability is not
	// implemented or not enabled.
	StatusCodeUnimplemented StatusCode = 12

	// StatusCodeInternal indicates an unexpected server failure or broken
	// invariant.
	StatusCodeInternal StatusCode = 13

	// StatusCodeUnavailable indicates a transient outage; the client may
	// retry with backoff.
	StatusCodeUnavailable StatusCode = 14

	// StatusCodeDataLoss indicates unrecoverable data loss or corruption.
	StatusCodeDataLoss StatusCode = 15

	// StatusCodeUnauthenticated indicates missing or invalid credentials.
	StatusCodeUnauthenticated StatusCode = 16
)

// StatusFromError maps an error returned by the st2138 package into a
// StatusResult. It recognizes st2138's exported sentinel errors and translates
// each to the matching StatusCode; a nil error is Ok and any unrecognized error
// falls back to StatusCodeUnknown. Business logic that calls st2138 conversion
// helpers (e.g. st2138.ToValue, st2138.FromProto) directly can use this to
// convert their error return into the StatusResult a handler must return.
func StatusFromError(err error) StatusResult {
	switch {
	case err == nil:
		return StatusResult{Code: StatusCodeOk}
	case errors.Is(err, st2138.ErrNotFound):
		return StatusResult{Code: StatusCodeNotFound, Error: err.Error()}
	case errors.Is(err, st2138.ErrInternal):
		return StatusResult{Code: StatusCodeInternal, Error: err.Error()}
	case errors.Is(err, st2138.ErrInvalidArgument):
		return StatusResult{Code: StatusCodeInvalidArgument, Error: err.Error()}
	default:
		return StatusResult{Code: StatusCodeUnknown, Error: err.Error()}
	}
}

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
	return StatusResult{Code: code, Error: msg}
}
