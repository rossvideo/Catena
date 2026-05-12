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
 * @brief Status code handling for the Catena SDK.
 * @file status_code.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package catena

// StatusCode is a transport-neutral outcome for handlers. Values 0–16 match
// gRPC's canonical code space and map 1:1 in the gRPC transport. Values 200+
// are legacy HTTP-shaped codes; prefer 0–16 in new handler code—see the
// package doc (Phase B) for which constants to use and how REST chooses 200 vs
// 204 vs 201.
//
// Based on Google's gRPC project (Apache 2.0 license):
// https://github.com/grpc/grpc
// http://www.apache.org/licenses/LICENSE-2.0
type StatusCode int

type StatusResult struct {
	Code  StatusCode
	Error string
}

const (
	// gRPC-compatible status codes (0-16)

	// OK indicates the operation completed successfully.
	OK StatusCode = 0

	// CANCELLED indicates the operation was cancelled (typically by the caller).
	CANCELLED StatusCode = 1

	// UNKNOWN indicates an unknown error occurred.
	UNKNOWN StatusCode = 2

	// INVALID_ARGUMENT indicates the client specified an invalid argument.
	INVALID_ARGUMENT StatusCode = 3

	// DEADLINE_EXCEEDED indicates the deadline expired before operation could complete.
	DEADLINE_EXCEEDED StatusCode = 4

	// NOT_FOUND indicates some requested entity was not found.
	NOT_FOUND StatusCode = 5

	// ALREADY_EXISTS indicates the entity that we attempted to create already exists.
	ALREADY_EXISTS StatusCode = 6

	// PERMISSION_DENIED indicates the caller does not have permission to execute the operation.
	PERMISSION_DENIED StatusCode = 7

	// RESOURCE_EXHAUSTED indicates some resource has been exhausted.
	RESOURCE_EXHAUSTED StatusCode = 8

	// FAILED_PRECONDITION indicates the operation was rejected because the system is not
	// in a state required for the operation's execution.
	FAILED_PRECONDITION StatusCode = 9

	// ABORTED indicates the operation was aborted, typically due to a concurrency issue.
	ABORTED StatusCode = 10

	// OUT_OF_RANGE indicates the operation was attempted past the valid range.
	OUT_OF_RANGE StatusCode = 11

	// UNIMPLEMENTED indicates the operation is not implemented or not supported.
	UNIMPLEMENTED StatusCode = 12

	// INTERNAL indicates internal errors. Something is very broken.
	INTERNAL StatusCode = 13

	// UNAVAILABLE indicates the service is currently unavailable.
	UNAVAILABLE StatusCode = 14

	// DATA_LOSS indicates unrecoverable data loss or corruption.
	DATA_LOSS StatusCode = 15

	// UNAUTHENTICATED indicates the request does not have valid authentication credentials.
	UNAUTHENTICATED StatusCode = 16

	// REST-specific status codes (legacy; see package doc Phase B)

	// CREATED indicates the request has succeeded and a new resource has been created.
	// Prefer OK in handlers; let the REST transport emit 201 when appropriate.
	CREATED StatusCode = 201

	// ACCEPTED indicates the request has been accepted for processing.
	// Prefer OK in handlers; let the REST transport emit 202 when appropriate.
	ACCEPTED StatusCode = 202

	// NO_CONTENT indicates success with no response body.
	// Prefer OK in handlers; let the REST transport emit 204 when appropriate.
	NO_CONTENT StatusCode = 204

	// BAD_REQUEST indicates the request was malformed.
	// Prefer INVALID_ARGUMENT for new code so gRPC and REST map consistently.
	BAD_REQUEST StatusCode = 400

	// METHOD_NOT_ALLOWED indicates the method is not allowed for the requested resource.
	METHOD_NOT_ALLOWED StatusCode = 405

	// CONFLICT indicates the request could not be completed due to a conflict.
	CONFLICT StatusCode = 409

	// UNPROCESSABLE_ENTITY indicates the request was well-formed but semantically erroneous.
	UNPROCESSABLE_ENTITY StatusCode = 422

	// TOO_MANY_REQUESTS indicates the user has sent too many requests.
	TOO_MANY_REQUESTS StatusCode = 429

	// BAD_GATEWAY indicates the server received an invalid response from upstream.
	BAD_GATEWAY StatusCode = 502

	// SERVICE_UNAVAILABLE indicates the server is not ready to handle the request.
	SERVICE_UNAVAILABLE StatusCode = 503

	// GATEWAY_TIMEOUT indicates the server did not receive a timely response from upstream.
	GATEWAY_TIMEOUT StatusCode = 504
)

// ResponseType is a constraint for types that can be returned from handlers
type ResponseType interface {
	CatenaValue | CatenaAsset | CatenaDevice
}

// Reply returns a successful response (OK) with the given value.
// Usage: catena.Reply(value), catena.Reply(asset), catena.Reply(device)
func Reply[T ResponseType](value T) (T, StatusResult) {
	return value, StatusResult{Code: OK}
}

// ReplyWithCode returns a response with the given value and status code.
// Usage: catena.ReplyWithCode(value, catena.CREATED)
func ReplyWithCode[T ResponseType](value T, code StatusCode) (T, StatusResult) {
	return value, StatusResult{Code: code}
}

// ReplyError returns an error response with the given status code and message.
// The value returned is the zero value of T.
// Usage: catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "not found")
func ReplyError[T ResponseType](code StatusCode, msg string) (T, StatusResult) {
	var zero T
	return zero, StatusResult{Code: code, Error: msg}
}

// StatusWithCode returns a StatusResult with the given StatusCode and optional message.

func StatusWithCode(code StatusCode, msg string) StatusResult {
	return StatusResult{Code: code, Error: msg}
}
