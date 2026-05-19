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
// # Normative references
//
//   - SMPTE PCD ST 2138-11 (Catena — gRPC Connection Type), §6.2 Table 2 —
//     "RPC Status Codes". Defines the wire vocabulary for the gRPC transport.
//     "RPCs shall either terminate with a success code, or one of the failure
//     codes as defined in Table 2." (codes 0–16, all required.)
//   - SMPTE PCD ST 2138-12 (Catena — REST Connection Type), §7.3 Table 1 —
//     "Frequently Used Failure Response Codes". Defines the default failure
//     vocabulary for REST as {401, 403, 404, 500, 503}. Per-endpoint tables in
//     §7.4–§7.11 use {200, 204} as the only successful responses.
//
// Phase A inventory: the spec's REST default failure set (above) is narrower
// than the current OpenAPI in smpte/interface/openapi/paths/*.yaml, which adds
// 400 on most write/query endpoints. That divergence is a spec-vs-YAML issue
// to flag in standards review, not an SDK bug.
//
// # Quick rules for handler authors
//
//   - Return OK for any success — do NOT pick 200 vs 204; the REST layer
//     decides based on the operation and whether a body was written.
//   - Use INVALID_ARGUMENT for bad client input — do NOT use BAD_REQUEST.
//   - Use UNAVAILABLE for "not ready, try again" — not SERVICE_UNAVAILABLE.
//   - Use RESOURCE_EXHAUSTED for rate / quota limits — not TOO_MANY_REQUESTS.
//   - METHOD_NOT_ALLOWED, CONFLICT, UNPROCESSABLE_ENTITY, BAD_GATEWAY,
//     GATEWAY_TIMEOUT, CREATED, ACCEPTED — not in ST 2138-12; do not use.
//
// # Cheat sheet
//
//	Code                  Meaning (handler view)            Typical wire result
//	--------------------- --------------------------------  --------------------
//	OK                    success                            HTTP 200 / 204; gRPC OK
//	CANCELLED             caller stopped the work            HTTP 499; gRPC Canceled
//	UNKNOWN               unclassified failure               HTTP 500; gRPC Unknown
//	INVALID_ARGUMENT      bad client input                   HTTP 400 (per-route); gRPC InvalidArgument
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
// HTTP codes outside the ST 2138-12 §7.3 default set (everything except 401,
// 403, 404, 500, 503 plus successes 200, 204) are only valid where a specific
// endpoint table opts in to them.
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
// Examples in parentheses are taken from ST 2138-11 §6.2 Table 2 where given.
//
//     OK
//     Cause:  the operation succeeded.
//     When:   any successful path. Do not branch on "did I write a body?".
//     Client: success; the body (if any) is the result.
//
//     CANCELLED
//     Cause:  the operation was canceled by either the client or the device.
//     When:   you detected explicit cancellation. NOT for plain timeouts.
//     Client: the work was abandoned, no result.
//     Spec note: ST 2138-11 spells this "CANCELED" (one L). The Catena Go
//     constant is CANCELLED for backward compatibility; rename in Phase D.
//
//     UNKNOWN
//     Cause:  there is not sufficient information to use any other status code.
//     When:   forwarding an error you do not own. Prefer a more specific code
//     when you can.
//     Client: treat as a non-actionable failure.
//
//     INVALID_ARGUMENT
//     Cause:  something was wrong with the request itself (bad JSON, wrong
//     type, malformed fqoid, missing required fields). Per the spec example,
//     "calling the GetValue RPC with an oid that does not map to a parameter
//     would cause this response."
//     When:   any "this request can never succeed as-is" situation. Use this
//     instead of the legacy BAD_REQUEST.
//     Client: fix the request before retrying.
//
//     DEADLINE_EXCEEDED
//     Cause:  a time-out, set by the client, expired before the work finished.
//     When:   timeouts.
//     Client: retry only if the operation is safe to repeat.
//     Spec note: ST 2138-11 names this "TIMEOUT EXCEEDED" (value 4); standard
//     gRPC and the Catena Go constant use DEADLINE_EXCEEDED. To reconcile in
//     Phase D — recommend keeping the gRPC standard name.
//
//     NOT_FOUND
//     Cause:  some requested entity (such as an external object) was not
//     found, probably because the wrong identifier was supplied.
//     When:   the lookup itself fails.
//     Client: do not assume permission was the problem.
//
//     ALREADY_EXISTS
//     Cause:  the service was asked to, or attempted to, create a file,
//     directory, or other item that already exists.
//     When:   creation conflicts with an existing entity.
//     Client: this is distinct from ABORTED (concurrency) and
//     FAILED_PRECONDITION (wrong state).
//
//     PERMISSION_DENIED
//     Cause:  the access token's scopes did not include those of the command
//     or parameter accessed.
//     When:   scope / role / policy refuses the action.
//     Client: not a credentials problem (see UNAUTHENTICATED).
//
//     RESOURCE_EXHAUSTED
//     Cause:  some resource has been exhausted (e.g. out of disk space, quota
//     hit, rate limit).
//     When:   you are throttling or backed up.
//     Client: back off and retry.
//
//     FAILED_PRECONDITION
//     Cause:  the operation was rejected because the service was not in a
//     state necessary for the requested operation to complete.
//     When:   the input is fine, but the state is not. "Bad state" vs
//     "bad input" (which is INVALID_ARGUMENT).
//     Client: fix the state, then retry.
//
//     ABORTED
//     Cause:  the service needed to make transactional calls on other
//     resources and a transaction failed.
//     When:   another change won; retrying may succeed.
//     Client: retry at a higher level (refresh, re-read, then retry).
//
//     OUT_OF_RANGE
//     Cause:  value past the end of a valid range. Per the spec example,
//     "attempting to access band 5 of a 4-band eq would create this status
//     code."
//     When:   choose this over FAILED_PRECONDITION when the rule is "out of
//     range" rather than "wrong state".
//     Client: the request is structurally fine, but the value is not allowed.
//
//     UNIMPLEMENTED
//     Cause:  the RPC / endpoint is not implemented or supported.
//     When:   stub paths, disabled features.
//     Client: do not retry; the API does not offer this.
//
//     INTERNAL
//     Cause:  unexpected failures by the service; an invariant is broken.
//     When:   you know the failure is server-side.
//     Client: not a client-side fix.
//
//     UNAVAILABLE
//     Cause:  the service is currently unavailable.
//     When:   overload, shutdown in progress, dependency offline.
//     Client: retry with backoff.
//
//     DATA_LOSS
//     Cause:  unrecoverable data loss or corruption.
//     When:   you must report an irrecoverable data problem.
//     Client: data is gone or unusable; surface clearly.
//
//     UNAUTHENTICATED
//     Cause:  the authentication credentials were invalid (e.g. the access
//     token was not issued by the registered authorization server).
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
// Constants marked "remove in Phase D" have no basis in ST 2138-12 — the spec
// neither lists them in §7.3 nor uses them in any §7.4–§7.11 endpoint table.
//
//	Legacy constant         What to use instead                        ST 2138-12?
//	----------------------  -----------------------------------------  ------------------
//	CREATED                 OK                                         no — remove in Phase D
//	ACCEPTED                OK                                         no — remove in Phase D
//	NO_CONTENT              OK   (REST layer chooses 204)              204 is a spec success
//	BAD_REQUEST             INVALID_ARGUMENT                           400 not in §7.3 default
//	SERVICE_UNAVAILABLE     UNAVAILABLE                                503 in §7.3 default
//	TOO_MANY_REQUESTS       RESOURCE_EXHAUSTED                         no — remove in Phase D
//	METHOD_NOT_ALLOWED      (transport only; do not use in handlers)   no — remove in Phase D
//	CONFLICT                ALREADY_EXISTS or ABORTED                  no — remove in Phase D
//	UNPROCESSABLE_ENTITY    INVALID_ARGUMENT (or FAILED_PRECONDITION)  no — remove in Phase D
//	BAD_GATEWAY             (transport only; do not use in handlers)   no — remove in Phase D
//	GATEWAY_TIMEOUT         DEADLINE_EXCEEDED                          no — remove in Phase D
//
// # gRPC policy
//
//   - The wire vocabulary on gRPC is fixed by ST 2138-11 §6.2 Table 2: codes
//     0–16 (OK through UNAUTHENTICATED). The transport must accept and may
//     emit any of them; the SDK cannot trim that vocabulary.
//   - Values 0–16 in this package match google.golang.org/grpc/codes 1:1 and
//     are mapped directly by the gRPC transport.
//   - The "preferred handler set" above is SDK ergonomics only — guidance for
//     authors of business-logic handlers. Infrastructure code, proxies, and
//     third-party libraries can still emit any standard gRPC code.
//   - Legacy HTTP-shaped constants (CREATED, ACCEPTED, NO_CONTENT) are folded
//     to codes.OK on gRPC. A gRPC client cannot tell HTTP 201 / 202 / 204
//     apart from status alone. If that distinction matters, send it in the
//     message body.
//
// # REST policy
//
//   - The default failure vocabulary is the spec's §7.3 set:
//     {401, 403, 404, 500, 503}.
//   - The success vocabulary observed across §7.4–§7.11 is {200, 204} only.
//     201, 202, 405, 409, 422, 429, 502, 504 do not appear anywhere in
//     ST 2138-12. The REST mapper should not emit them by default; emit them
//     only from a per-route mapping that a specific endpoint opts into.
//   - 400 is used by several routes in our OpenAPI YAML even though it is not
//     in the §7.3 default set; that is allowed by the spec's "Unless
//     superseded in the path definition" wording, but worth raising with the
//     working group.
//   - SetValue / SetValues / language-pack mutations return 204 (no body) per
//     §7.7.2, §7.8.4–5, §7.11.3–5; the REST layer chooses 204 vs 200 based on
//     the route, not the handler's StatusCode.
//
// # Naming divergence vs ST 2138-11 (resolve in Phase D)
//
//	Spec name (ST 2138-11)     Catena Go constant     Standard gRPC name
//	-------------------------  ---------------------  ---------------------
//	CANCELED (one L)           CANCELLED              Canceled
//	TIMEOUT EXCEEDED           DEADLINE_EXCEEDED      DeadlineExceeded
//
// Recommendation: align Catena with the standard gRPC names (Canceled,
// DeadlineExceeded), since ST 2138-11 cites the gRPC project and these two
// labels appear to be drift in the spec text. Confirm with the WG.
//
// # Naming (future)
//
// A later phase (Phase D) will rename exported identifiers to Go style:
// StatusCodeOk, StatusCodeInvalidArgument, etc. The current names (OK,
// INVALID_ARGUMENT, …) stay until that coordinated migration.
package catena
