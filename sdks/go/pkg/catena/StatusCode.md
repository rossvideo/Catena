# Catena `StatusCode`

`StatusCode` is the transport-neutral outcome type used by Catena device
handlers. The REST and gRPC transports map each `StatusCode` to the
appropriate wire shape (HTTP status line, gRPC `codes.Code`). Handlers should
think in `StatusCode`, not in HTTP numbers or gRPC codes directly.

## What this package gives you

- `StatusCode` — see `status_code.go`.
- `StatusResult` — a `StatusCode` plus an optional error message.
- `Reply` / `ReplyError` / `ReplyWithCode` / `StatusWithCode` helpers.

## Normative references

- **SMPTE PCD ST 2138-11** (Catena — gRPC Connection Type), §6.2 Table 2 —
  "RPC Status Codes". Defines the wire vocabulary for the gRPC transport:
  codes 0–16 are all required. *"RPCs shall either terminate with a success
  code, or one of the failure codes as defined in Table 2."*
- **SMPTE PCD ST 2138-12** (Catena — REST Connection Type), §7.3 Table 1 —
  "Frequently Used Failure Response Codes". The default failure vocabulary
  for REST is `{401, 403, 404, 500, 503}`. Per-endpoint tables in §7.4–§7.11
  use `{200, 204}` as the only successful responses.

The OpenAPI in `smpte/interface/openapi/paths/*.yaml` adds `400` on most
write/query endpoints. §7.3 permits this via *"Unless superseded in the path
definition"*, so route-level `400` is consistent with the spec. A proposal
to add `400` to §7.3 Table 1 is tracked in
[SMPTE/st2138-12#5](https://github.com/SMPTE/st2138-12/issues/5).

## Quick rules for handler authors

- Return `StatusCodeOk` for any success. Do not pick 200 vs 204 — the REST
  layer chooses the wire status from the operation.
- Use `StatusCodeInvalidArgument` for bad client input (bad JSON, wrong
  type, malformed `fqoid`, missing required fields).
- Use `StatusCodeUnavailable` for "not ready, try again."
- Use `StatusCodeResourceExhausted` for rate / quota limits.
- HTTP-shaped concepts that are not in this vocabulary
  (`MethodNotAllowed`, `Conflict`, `BadGateway`, `GatewayTimeout`,
  `Created`, `Accepted`) are not part of `StatusCode`. Method enforcement
  and proxy/gateway conditions are transport concerns and are handled by
  the REST router directly, bypassing `StatusCode`.

## Cheat sheet

| `StatusCode`                   | Meaning (handler view)            | Typical wire result                          |
|--------------------------------|-----------------------------------|----------------------------------------------|
| `StatusCodeOk`                 | success                           | HTTP 200 / 204; gRPC `OK`                    |
| `StatusCodeCanceled`           | caller stopped the work           | HTTP 499; gRPC `Canceled`                    |
| `StatusCodeUnknown`            | unclassified failure              | HTTP 500; gRPC `Unknown`                     |
| `StatusCodeInvalidArgument`    | bad client input                  | HTTP 400 (per-route); gRPC `InvalidArgument` |
| `StatusCodeDeadlineExceeded`   | time ran out                      | HTTP 504; gRPC `DeadlineExceeded`            |
| `StatusCodeNotFound`           | resource missing                  | HTTP 404; gRPC `NotFound`                    |
| `StatusCodeAlreadyExists`      | duplicate create                  | HTTP 409; gRPC `AlreadyExists`               |
| `StatusCodePermissionDenied`   | authenticated but not allowed     | HTTP 403; gRPC `PermissionDenied`            |
| `StatusCodeResourceExhausted`  | quota / rate limit                | HTTP 429; gRPC `ResourceExhausted`           |
| `StatusCodeFailedPrecondition` | wrong system state for the call   | HTTP 400; gRPC `FailedPrecondition`          |
| `StatusCodeAborted`            | concurrency / txn conflict        | HTTP 409; gRPC `Aborted`                     |
| `StatusCodeOutOfRange`         | value outside valid range         | HTTP 400; gRPC `OutOfRange`                  |
| `StatusCodeUnimplemented`      | not implemented / not enabled     | HTTP 501; gRPC `Unimplemented`               |
| `StatusCodeInternal`           | server bug / invariant broken     | HTTP 500; gRPC `Internal`                    |
| `StatusCodeUnavailable`        | transient outage, retry later     | HTTP 503; gRPC `Unavailable`                 |
| `StatusCodeDataLoss`           | unrecoverable corruption          | HTTP 500; gRPC `DataLoss`                    |
| `StatusCodeUnauthenticated`    | missing / bad credentials         | HTTP 401; gRPC `Unauthenticated`             |

HTTP codes outside the ST 2138-12 §7.3 default set (everything except 401,
403, 404, 500, 503 plus successes 200 and 204) are only valid where a
specific endpoint table opts into them.

> **HTTP 412 Precondition Failed is unrelated to `StatusCodeFailedPrecondition`.**
> HTTP 412 shares a name with gRPC's `FailedPrecondition` but means
> something different: HTTP 412 is the response when a request's conditional
> headers (e.g. `If-Match`, `If-Unmodified-Since`) do not match.
> Catena does not use HTTP 412, and `StatusCodeFailedPrecondition` does not
> map to it.

## Code-by-code notes

Examples in parentheses are taken from ST 2138-11 §6.2 Table 2 where given.

### `StatusCodeOk`

- **Cause:** the operation succeeded.
- **When to return:** any successful path. Do not branch on "did I write a
  body?".
- **What clients infer:** success; the body (if any) is the result.

### `StatusCodeCanceled`

- **Cause:** the operation was canceled by either the client or the device.
- **When to return:** you detected explicit cancellation. Not for plain
  timeouts.
- **What clients infer:** the work was abandoned, no result.

### `StatusCodeUnknown`

- **Cause:** there is not sufficient information to use any other status code.
- **When to return:** forwarding an error you do not own. Prefer a more
  specific code when you can.
- **What clients infer:** treat as a non-actionable failure.

### `StatusCodeInvalidArgument`

- **Cause:** something was wrong with the request itself (bad JSON, wrong
  type, malformed `fqoid`, missing required fields). Per the spec example:
  *"calling the GetValue RPC with an oid that does not map to a parameter
  would cause this response."*
- **When to return:** any "this request can never succeed as-is" situation.
- **What clients infer:** fix the request before retrying.

### `StatusCodeDeadlineExceeded`

- **Cause:** a time-out, set by the client, expired before the work
  finished.
- **When to return:** timeouts.
- **What clients infer:** retry only if the operation is safe to repeat.

### `StatusCodeNotFound`

- **Cause:** some requested entity (such as an external object) was not
  found, probably because the wrong identifier was supplied.
- **When to return:** the lookup itself fails.
- **What clients infer:** do not assume permission was the problem.

### `StatusCodeAlreadyExists`

- **Cause:** the service was asked to, or attempted to, create a file,
  directory, or other item that already exists.
- **When to return:** creation conflicts with an existing entity.
- **What clients infer:** distinct from `StatusCodeAborted` (concurrency)
  and `StatusCodeFailedPrecondition` (wrong state).

### `StatusCodePermissionDenied`

- **Cause:** the access token's scopes did not include those of the command
  or parameter accessed.
- **When to return:** scope / role / policy refuses the action.
- **What clients infer:** not a credentials problem (see
  `StatusCodeUnauthenticated`).

### `StatusCodeResourceExhausted`

- **Cause:** some resource has been exhausted (e.g. out of disk space,
  quota hit, rate limit).
- **When to return:** you are throttling or backed up.
- **What clients infer:** back off and retry.

### `StatusCodeFailedPrecondition`

- **Cause:** the operation was rejected because the service was not in a
  state necessary for the requested operation to complete.
- **When to return:** the input is fine, but the state is not. "Bad state"
  vs "bad input" (which is `StatusCodeInvalidArgument`).
- **What clients infer:** fix the state, then retry.
- **See also:** the HTTP 412 note above the code-by-code section.

### `StatusCodeAborted`

- **Cause:** the service needed to make transactional calls on other
  resources and a transaction failed.
- **When to return:** another change won; retrying may succeed.
- **What clients infer:** retry at a higher level (refresh, re-read, then
  retry).

### `StatusCodeOutOfRange`

- **Cause:** value past the end of a valid range. Per the spec example,
  *"attempting to access band 5 of a 4-band eq would create this status
  code."*
- **When to return:** choose this over `StatusCodeFailedPrecondition` when
  the rule is "out of range" rather than "wrong state".
- **What clients infer:** the request is structurally fine, but the value
  is not allowed.

### `StatusCodeUnimplemented`

- **Cause:** the RPC / endpoint is not implemented or supported.
- **When to return:** stub paths, disabled features.
- **What clients infer:** do not retry; the API does not offer this.

### `StatusCodeInternal`

- **Cause:** unexpected failures by the service; an invariant is broken.
- **When to return:** you know the failure is server-side.
- **What clients infer:** not a client-side fix.

### `StatusCodeUnavailable`

- **Cause:** the service is currently unavailable.
- **When to return:** overload, shutdown in progress, dependency offline.
- **What clients infer:** retry with backoff.

### `StatusCodeDataLoss`

- **Cause:** unrecoverable data loss or corruption.
- **When to return:** you must report an irrecoverable data problem.
- **What clients infer:** data is gone or unusable; surface clearly.

### `StatusCodeUnauthenticated`

- **Cause:** the authentication credentials were invalid (e.g. the access
  token was not issued by the registered authorization server).
- **When to return:** no token, expired token, signature failed.
- **What clients infer:** re-authenticate.

## gRPC policy

- The wire vocabulary on gRPC is fixed by ST 2138-11 §6.2 Table 2: codes
  0–16 (`OK` through `Unauthenticated`). The transport accepts and may emit
  any of them.
- The 17 `StatusCode` constants in this package match
  `google.golang.org/grpc/codes` 1:1 and are mapped directly by the gRPC
  transport.
- The preferred handler set above is SDK ergonomics — guidance for authors
  of business-logic handlers. Infrastructure code, proxies, and third-party
  libraries can still emit any standard gRPC code.

## REST policy

- The default failure vocabulary is the spec's §7.3 set:
  `{401, 403, 404, 500, 503}`.
- The success vocabulary across §7.4–§7.11 is `{200, 204}` only.
  `201, 202, 405, 409, 422, 429, 502, 504` do not appear in ST 2138-12. The
  REST mapper does not emit them by default; emit them only from a per-route
  mapping that a specific endpoint opts into.
- `400` appears in several routes in our OpenAPI YAML; ST 2138-12 permits
  this via *"Unless superseded in the path definition"*. Adding `400` to
  §7.3 Table 1 is proposed in
  [SMPTE/st2138-12#5](https://github.com/SMPTE/st2138-12/issues/5).
- `SetValue` / `SetValues` / language-pack mutations return 204 (no body)
  per §7.7.2, §7.8.4–5, §7.11.3–5; the REST layer chooses 204 vs 200 based
  on the route, not the handler's `StatusCode`.
- HTTP method enforcement (e.g. *"only GET allowed"*) is handled directly
  by the router via `writeHTTPMethodNotAllowed`, which writes HTTP 405 and
  bypasses `StatusCode` entirely. Method routing is a transport concern,
  not a handler outcome.
