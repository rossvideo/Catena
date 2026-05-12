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

// Package catena holds the shared types and helpers used by Catena device
// handlers and by the REST and gRPC transports.
//
// # What this package gives you
//
//   - StatusCode: a transport-neutral way to describe the outcome of a handler
//     call (success, bad input, not found, …). See status_code.go.
//   - StatusResult: a StatusCode plus an optional error message.
//   - Reply / ReplyError / StatusWithCode helpers for building results.
//
// Handlers should think in StatusCode. The REST and gRPC layers translate
// StatusCode into the right wire shape (HTTP status line, gRPC codes.Code).
//
// # Quick rules for handler authors
//
//   - Return OK for any success — do NOT pick 200 vs 201 vs 204; the REST
//     layer decides based on the operation and whether a body was written.
//   - Use INVALID_ARGUMENT for bad client input — do NOT use BAD_REQUEST.
//   - Use UNAVAILABLE for "not ready, try again" — not SERVICE_UNAVAILABLE.
//   - Use RESOURCE_EXHAUSTED for rate / quota limits — not TOO_MANY_REQUESTS.
//   - METHOD_NOT_ALLOWED, CONFLICT, UNPROCESSABLE_ENTITY, BAD_GATEWAY,
//     GATEWAY_TIMEOUT belong to the transport layer; avoid them in handlers.
//
// # Cheat sheet
//
//	Code                  Meaning (handler view)            Typical wire result
//	--------------------- --------------------------------  --------------------
//	OK                    success                            HTTP 200 / 204 / 201; gRPC OK
//	CANCELLED             caller stopped the work            HTTP 499; gRPC Canceled
//	UNKNOWN               unclassified failure               HTTP 500; gRPC Unknown
//	INVALID_ARGUMENT      bad client input                   HTTP 400; gRPC InvalidArgument
//	DEADLINE_EXCEEDED     time ran out                       HTTP 504; gRPC DeadlineExceeded
//	NOT_FOUND             resource missing                   HTTP 404; gRPC NotFound
//	ALREADY_EXISTS        duplicate create                   HTTP 409; gRPC AlreadyExists
//	PERMISSION_DENIED     authenticated but not allowed      HTTP 403; gRPC PermissionDenied
//	RESOURCE_EXHAUSTED    quota / rate limit                 HTTP 429; gRPC ResourceExhausted
//	FAILED_PRECONDITION   wrong system state for the call    HTTP 400; gRPC FailedPrecondition
//	ABORTED               concurrency / txn conflict         HTTP 409; gRPC Aborted
//	OUT_OF_RANGE          value outside valid range          HTTP 400; gRPC OutOfRange
//	UNIMPLEMENTED         not implemented / not enabled      HTTP 501; gRPC Unimplemented
//	INTERNAL              server bug / invariant broken      HTTP 500; gRPC Internal
//	UNAVAILABLE           transient outage, retry later      HTTP 503; gRPC Unavailable
//	DATA_LOSS             unrecoverable corruption           HTTP 500; gRPC DataLoss
//	UNAUTHENTICATED       missing / bad credentials          HTTP 401; gRPC Unauthenticated
//
// # Code-by-code notes
//
// Each entry below answers three questions:
//
//   - Cause: what went wrong (or right).
//
//   - When to return: what handler situations fit.
//
//   - What clients should infer: how callers should react.
//
//     OK
//     Cause:  the operation succeeded.
//     When:   any successful path. Do not branch on "did I write a body?".
//     Client: success; the body (if any) is the result.
//
//     CANCELLED
//     Cause:  the caller stopped the operation (closed a stream, cancelled
//     its context).
//     When:   you detected explicit cancellation. NOT for plain timeouts.
//     Client: the work was abandoned, no result.
//
//     UNKNOWN
//     Cause:  something failed and you cannot classify it safely.
//     When:   forwarding an error you do not own. Prefer a more specific code
//     when you can.
//     Client: treat as a non-actionable failure.
//
//     INVALID_ARGUMENT
//     Cause:  the request itself is wrong (bad JSON, wrong type, malformed
//     fqoid, missing required fields).
//     When:   any "this request can never succeed as-is" situation. Use this
//     instead of the legacy BAD_REQUEST.
//     Client: fix the request before retrying.
//
//     DEADLINE_EXCEEDED
//     Cause:  the deadline expired before the work finished.
//     When:   timeouts.
//     Client: retry only if the operation is safe to repeat.
//
//     NOT_FOUND
//     Cause:  a referenced resource does not exist (unknown slot, missing
//     param, unknown route at the application layer).
//     When:   the lookup itself fails.
//     Client: do not assume permission was the problem.
//
//     ALREADY_EXISTS
//     Cause:  a create would duplicate a unique resource.
//     When:   creation conflicts with an existing entity.
//     Client: this is distinct from ABORTED (concurrency) and
//     FAILED_PRECONDITION (wrong state).
//
//     PERMISSION_DENIED
//     Cause:  the caller is authenticated but not authorized.
//     When:   scope / role / policy refuses the action.
//     Client: not a credentials problem (see UNAUTHENTICATED).
//
//     RESOURCE_EXHAUSTED
//     Cause:  quota or rate limit hit.
//     When:   you are throttling.
//     Client: back off and retry.
//
//     FAILED_PRECONDITION
//     Cause:  the system is in the wrong state for this call (e.g. device not
//     ready).
//     When:   the input is fine, but the state is not. "Bad state" vs
//     "bad input" (which is INVALID_ARGUMENT).
//     Client: fix the state, then retry.
//
//     ABORTED
//     Cause:  concurrency conflict / aborted transaction.
//     When:   another change won; retrying may succeed.
//     Client: retry at a higher level (refresh, re-read, then retry).
//
//     OUT_OF_RANGE
//     Cause:  value past the end of a valid range (index, numeric domain).
//     When:   choose this over FAILED_PRECONDITION when the rule is "out of
//     range" rather than "wrong state".
//     Client: the request is structurally fine, but the value is not allowed.
//
//     UNIMPLEMENTED
//     Cause:  the operation or capability is not available here.
//     When:   stub paths, disabled features.
//     Client: do not retry; the API does not offer this.
//
//     INTERNAL
//     Cause:  invariant broken or unexpected server failure.
//     When:   you know the failure is server-side.
//     Client: not a client-side fix.
//
//     UNAVAILABLE
//     Cause:  transient service / dependency outage.
//     When:   overload, shutdown in progress, dependency offline.
//     Client: retry with backoff.
//
//     DATA_LOSS
//     Cause:  unrecoverable data corruption / loss.
//     When:   you must report an irrecoverable data problem.
//     Client: data is gone or unusable; surface clearly.
//
//     UNAUTHENTICATED
//     Cause:  missing or invalid credentials.
//     When:   no token, expired token, signature failed.
//     Client: re-authenticate.
//
// # Legacy / transport-shaped constants
//
// Some constants in status_code.go use HTTP numbers (e.g. CREATED = 201,
// BAD_REQUEST = 400, SERVICE_UNAVAILABLE = 503). They exist because earlier
// REST code returned those status lines directly. They remain for backward
// compatibility, but new handler code should follow the table below.
//
//	Legacy constant         New code handlers should use
//	----------------------  -----------------------------------------------
//	CREATED                 OK   (REST layer chooses 201)
//	ACCEPTED                OK   (REST layer chooses 202)
//	NO_CONTENT              OK   (REST layer chooses 204)
//	BAD_REQUEST             INVALID_ARGUMENT
//	SERVICE_UNAVAILABLE     UNAVAILABLE
//	TOO_MANY_REQUESTS       RESOURCE_EXHAUSTED
//	METHOD_NOT_ALLOWED      (transport layer; do not use in handlers)
//	CONFLICT                ALREADY_EXISTS or ABORTED
//	UNPROCESSABLE_ENTITY    INVALID_ARGUMENT (or FAILED_PRECONDITION)
//	BAD_GATEWAY             (transport layer; do not use in handlers)
//	GATEWAY_TIMEOUT         DEADLINE_EXCEEDED
//
// # gRPC policy
//
//   - Values 0–16 (OK through UNAUTHENTICATED) match google.golang.org/grpc/codes
//     1:1 and are mapped directly by the gRPC transport.
//   - Legacy success codes (CREATED, ACCEPTED, NO_CONTENT) are folded to
//     codes.OK on gRPC, so a gRPC client cannot tell HTTP 201 / 202 / 204 apart
//     from status alone. If the distinction matters, send it in the message.
//   - The full gRPC enum remains valid on the wire (proxies, third-party libs
//     may emit any code). The handler contract is the minimal set above; codes
//     outside that set should only come from infrastructure, not from core
//     device handlers, unless explicitly documented for that operation.
//
// # Naming (future)
//
// A later phase (Phase D) will rename exported identifiers to Go style:
// StatusCodeOk, StatusCodeInvalidArgument, etc. The current names (OK,
// INVALID_ARGUMENT, …) stay until that coordinated migration.
package catena
