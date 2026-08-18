# REST Transport

`rest.Transport` exposes Catena handlers over HTTP and Server-Sent Events (SSE).

## Import
```go
import "github.com/rossvideo/catena/sdks/go/pkg/transports/rest"
```

## Constructor

```go
restTransport := rest.NewTransport(rest.DefaultOptions())
// or start from the defaults and customize
opts := rest.DefaultOptions()
opts.Port = 9080
restTransport := rest.NewTransport(opts)
```

Register it on the shared server:

```go
srv := catena.NewServer(100)
if err := srv.RegisterTransport(restTransport); err != nil {
    panic(err)
}
```

## TLS

The listener serves HTTPS when `Options.TLS.Enabled` is true:

```go
opts := rest.DefaultOptions()
opts.TLS.Enabled = true
opts.TLS.CertFile = "/etc/catena/certs/server.crt" // PEM server certificate
opts.TLS.KeyFile = "/etc/catena/certs/server.key"  // PEM private key
// Optional mutual TLS (client certificate verification):
// opts.TLS.ClientCAFile = "/etc/catena/certs/clients-ca.crt"
// opts.TLS.MutualAuth = true
restTransport := rest.NewTransport(opts)
```

Behavior:

- `CertFile` and `KeyFile` are required when TLS is enabled; `Start` returns an error if either is missing, unreadable, or invalid PEM.
- With `MutualAuth`, clients must present a certificate signed by the CA bundle in `ClientCAFile` or the TLS handshake is rejected.
- Minimum accepted protocol version is TLS 1.2.
- Env/CLI configuration: `{PREFIX}_REST_TLS_ENABLED`, `{PREFIX}_REST_TLS_CERT_FILE`, `{PREFIX}_REST_TLS_KEY_FILE`, `{PREFIX}_REST_TLS_CLIENT_CA_FILE`, `{PREFIX}_REST_TLS_MUTUAL_AUTH` (flags `--rest-tls-*`).

## Implemented Endpoints

Core routes:

- `GET /st2138-api/v1/{slot}` - DeviceRequest equivalent (unary; aggregates the component stream into one device)
- `GET /st2138-api/v1/{slot}/stream` - SSE DeviceRequest component stream
- `GET /st2138-api/v1/{slot}/value/{oid}` - GetValue
- `PUT /st2138-api/v1/{slot}/value/{oid}` - SetValue
- `PUT /st2138-api/v1/{slot}/values` - SetValues (MultiSetValue)
- `GET /st2138-api/v1/{slot}/param/{oid...}` - GetParam (full param: metadata + value)
- `GET /st2138-api/v1/{slot}/asset/{oid}` - ExternalObjectRequest equivalent (ReadAsset; unary, aggregates the chunk stream into one asset)
- `GET /st2138-api/v1/{slot}/asset/{oid}/stream` - SSE ReadAsset chunk stream
- `POST /st2138-api/v1/{slot}/asset/{oid}` - CreateAsset (store a new asset, returns 204)
- `PUT /st2138-api/v1/{slot}/asset/{oid}` - UpdateAsset (replace an asset, returns 204)
- `DELETE /st2138-api/v1/{slot}/asset/{oid}` - DeleteAsset (remove an asset, returns 204)
- `POST /st2138-api/v1/{slot}/command/{oid}` - ExecuteCommand (unary; add `?respond=true` to receive a response)
- `POST /st2138-api/v1/{slot}/command/{oid}/stream` - SSE ExecuteCommand response stream
- `GET /st2138-api/v1/{slot}/param-info/{oid...}` - unary param info
- `GET /st2138-api/v1/{slot}/param-info/{oid...}/stream` - SSE param info stream (add `?recursive` to walk the subtree)
- `GET /st2138-api/v1/connect` - SSE push updates
- `GET /st2138-api/v1/devices` - populated slots
- `GET /st2138-api/v1/health` - health check

Fallback and root behavior:

- `RegisterFallbackHandler(...)` supports custom routes outside ST2138 routes
- Unknown routes return `NOT_FOUND` when no fallback is registered

## Handler Mapping

REST routes invoke handlers registered on `catena.Server`:

- Device routes -> `RegisterGetDeviceHandler` (both the unary and `/stream` routes invoke the same streaming handler)
- Value routes -> `RegisterGetValueHandler`, `RegisterSetValueHandler` (the set route delivers a one-element `[]SetValueEntry`)
- Param route -> `RegisterGetParamHandler`
- Values route -> `RegisterSetValueHandler` (delivers the full `[]SetValueEntry` for atomic application)
- Asset routes -> `RegisterReadAssetHandler` (GET, both unary and `/stream`), `RegisterCreateAssetHandler` (POST), `RegisterUpdateAssetHandler` (PUT), `RegisterDeleteAssetHandler` (DELETE)
- Command routes -> `RegisterExecuteCommandHandler` (both the unary and `/stream` routes invoke the same streaming handler)
- Param-info routes -> `RegisterParamInfoHandler` (both the unary and `/stream` routes invoke the same streaming handler)

## Streaming Responses (SSE)

Device, asset read, param-info, and command each expose a unary route and a
`/stream` route. The transport adapts the underlying response stream to the
requested shape: the unary route collapses the stream to a single JSON body,
while the `/stream` route emits Server-Sent Events.

SSE framing:

- Each chunk is written as one `data: <json>\n\n` frame and flushed immediately.
- Response headers (`Content-Type: text/event-stream`) and the `200` status are
  written lazily on the first chunk, so an error raised before the first chunk
  can still return a normal HTTP error status.
- A stream that produces no chunks yields a well-formed empty SSE `200` response
  (no `data:` frames).

Mid-stream errors (`SseError`):

- Once a chunk has been sent the `200` is committed and the HTTP status can no
  longer change, so a later error is reported in-band as an SSE `event: error`
  frame whose data is `{"code": <http-status>, "message": <text>}`.
- The detailed message is only exposed in dev mode; otherwise it is generalized
  to the standard HTTP status text (and omitted when blank).

Device routes:

- The unary route aggregates the handler's `DeviceComponent` chunks back into
  one complete device JSON body: a `device` chunk is the base, and later
  `param` / `shared_constraint` / `menu` / `command` / `language_pack` chunks
  are merged into the corresponding device maps. A success with no chunks is
  reported as an internal error.
- The `/stream` route forwards each `DeviceComponent` as one SSE frame.

Asset routes:

- The unary GET aggregates the handler's chunk stream into one
  `external_object_payload` JSON body: metadata, digest, cachable, and the
  payload encoding come from the first chunk, and the embedded payload bytes of
  every chunk are concatenated in send order. A success with no chunks is
  reported as an internal error.
- The `/stream` route forwards each `ExternalObjectPayload` chunk as one SSE
  frame; the client concatenates the embedded payload bytes. `?compression=` is
  not supported on the streaming route (transcoding needs the whole payload).

Param-info routes:

- The unary route requires an fqoid, is never recursive, and returns a single
  JSON param-info object.
- The `/stream` route accepts `?recursive` to walk the subtree.

Command routes:

- The request body is the command payload (a JSON value). An empty body is
  valid and is treated as "no value" (a nil payload), bypassing body
  validation; a non-empty body is fully validated and unmarshalled. Emptiness
  is determined by the body itself, not the `Content-Length` header.
- `respond` defaults to `false`; only `?respond=true` opts into receiving
  responses, and `respond` is passed through to the handler.
- The unary route replies with the final `CommandResponse` from the stream, or an
  explicit `no_response` when `respond=false` or nothing was produced.
- The `/stream` route forwards every `CommandResponse`.

## SSE Push Updates

`GET /st2138-api/v1/connect` receives updates from server broadcasts.

Connection lifecycle details:

- On connect, the transport registers with the runtime via `RegisterTransportConnection(owner)`.
- It then forwards each message from `connection.Updates` to the SSE stream.
- It listens on `connection.Done` for server-driven shutdown/cleanup and exits the stream loop.
- On disconnect/exit, it deregisters via `DeregisterConnection(connID)`.

Why owner is required for registration:

- The runtime tags each connection with the registering transport instance (`owner`).
- That allows `ShutdownTransportConnections(owner)` to clean up only REST-owned streams when REST is deregistered or shut down.
- gRPC and REST can run side-by-side without cross-transport stream interruption.

Server side:

```go
srv.BroadcastUpdate(slot, "product/version", "1.2.3")
```

Client side:

```javascript
const source = new EventSource("http://localhost:9080/st2138-api/v1/connect");
source.onmessage = (event) => {
  const update = JSON.parse(event.data);
  console.log(update);
};
```

## Error Mapping

Catena status codes are mapped to HTTP status codes using `ToHTTPStatus`.

Examples:

- `OK` -> `200`
- `NO_CONTENT` -> `204`
- `INVALID_ARGUMENT` -> `400`
- `NOT_FOUND` -> `404`
- `METHOD_NOT_ALLOWED` -> `405`
- `UNIMPLEMENTED` -> `501`

In production mode, error payload messages are sanitized to HTTP status text.

## Asset Compression

Unary asset reads support optional payload transcoding via query parameter:

- `GET /st2138-api/v1/{slot}/asset/{oid}?compression=<encoding>`

Transcoding is applied to the fully assembled payload after chunk aggregation.
If the requested encoding is invalid or transcoding fails, the transport
returns a Catena error mapped to HTTP. The `/stream` route does not support
`?compression=`.

## Graceful Shutdown

The parent `catena.Server` handles transport shutdown and active connection cleanup:

```go
srv.Shutdown(ctx)
srv.Wait()
```

You can deregister REST independently:

```go
_ = srv.DeregisterTransport(ctx, rest)
```

REST shutdown behavior:

- REST starts HTTP server shutdown and also requests `ShutdownTransportConnections(ctx, owner)` to signal REST-owned SSE connections to exit.
- Transports that do not use runtime-managed streams can skip this step.
- Server-level shutdown still acts as a final force-drain safety net.

Context contract:

- `Start(ctx, runtime)` includes `ctx` for future-proof, context-aware transport lifetime management.
- `Shutdown(ctx)` must treat `ctx` as a hard deadline/cancel signal and MUST return by that deadline.

Caller guidance:

- Prefer `context.WithTimeout` for shutdown contexts so teardown remains bounded and predictable.
