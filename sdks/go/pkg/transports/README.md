# Catena Go Transports

This package contains transport implementations for the Catena Go SDK.

The server architecture is now transport-agnostic:

- Create one `catena.Server`
- Register your slot handlers once
- Attach one or more transports from this package (REST, gRPC)

## Package Overview

- `rest.Transport`: HTTP/REST transport (`st2138-api` routes, SSE streaming)
- `grpc.Transport`: gRPC transport (`CatenaService`, server-streaming endpoints)
- Shared status and JSON helpers used by the REST layer

## Quick Start

```go
package main

import (
    "context"
    "net/http"
    "os"
    "os/signal"
    "syscall"

    "github.com/rossvideo/catena/sdks/go/pkg/catena"
    "github.com/rossvideo/catena/sdks/go/pkg/st2138"
    "github.com/rossvideo/catena/sdks/go/pkg/transports/grpc"
    "github.com/rossvideo/catena/sdks/go/pkg/transports/rest"
)

func main() {
    srv := catena.NewServer(100) // max concurrent push connections

    // The SDK manages the mandatory product struct; register it once per slot.
    srv.RegisterProductStruct(0, catena.ProductStruct{
        Name:         "My Device",
        Vendor:       "Ross Video",
        Version:      "1.0.0",
        SerialNumber: "SN-0001",
    })

    // Register handlers once. All registered transports share this runtime.
    srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx catena.HandlerContext) (st2138.Device, catena.StatusResult) {
        device := st2138.NewDevice(slot)
        return catena.Reply(*device)
    })

    if err := srv.RegisterTransport(grpc.NewTransport(grpc.DefaultOptions())); err != nil {
        panic(err)
    }

    restTransport := rest.NewTransport(rest.DefaultOptions())
    if err := srv.RegisterTransport(restTransport); err != nil {
        panic(err)
    }

    // Optional custom fallback endpoints on REST.
    restTransport.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (st2138.Value, catena.StatusResult) {
        return catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "endpoint not found")
    })

    ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
    defer stop()

    <-ctx.Done()
    srv.Shutdown(context.Background())
    srv.Wait()
}
```

## Transport Lifecycle

`catena.Server` owns transport lifecycle:

- `RegisterTransport(transport)` starts the transport
- `DeregisterTransport(ctx, transport)` removes and shuts down one transport
- `Shutdown(ctx)` shuts down all registered transports and active push connections
- `Wait()` blocks until server shutdown completes

## Shared Handler Surface

Both transports invoke the same registered handlers from `catena.ServerRuntime`:

- `RegisterGetDeviceHandler`
- `RegisterGetValueHandler`
- `RegisterSetValueHandler` (handles both single and multi set requests; single endpoints deliver a one-element `[]SetValueEntry`, multi endpoints deliver the full slice)
- `RegisterReadAssetHandler` (REST GET, gRPC ExternalObjectRequest)
- `RegisterCreateAssetHandler` (REST POST / CreateAsset)
- `RegisterUpdateAssetHandler` (REST PUT / UpdateAsset)
- `RegisterDeleteAssetHandler` (REST DELETE / DeleteAsset)
- `RegisterExecuteCommandHandler`
- `RegisterParamInfoHandler`
- `RegisterHeartbeatHandler`

The mandatory product struct is managed by the SDK, not by a handler: call `RegisterProductStruct(slot, catena.ProductStruct{...})` and the SDK will inject it into GetDevice and serve product/* on GetValue and ParamInfo (rejecting writes by default). This means common product handling doesn't need any separate handler fallback. If desired, business logic can still implement its own handler for custom or fallback behavior, but the SDK manages all standard product struct handling and fallback messaging automatically. The product param carries the `st2138:mon` access scope: when authorization is enabled, callers without the monitor read scope do not receive the product param on GetDevice and are denied product/* requests on GetValue, GetParam, and ParamInfo.

Use `BroadcastUpdate(slot, fqoid, value)` to fan out push updates to all active REST and gRPC stream clients.

## Connect Connection Model

Streaming endpoints (`GET /st2138-api/v1/connect` and gRPC `Connect`) use shared runtime-managed connections.

Transport behavior:

- The transport calls `ServerRuntime.RegisterTransportConnection(owner)` when a client subscribes.
- The runtime returns `(connID, connection)`.
- If the connection is not accepted (for example, too many active connections), `connID` is negative and the transport should reject the subscription.
- The transport reads `connection.Updates` and propagates each update over its protocol stream (SSE or gRPC).
- The transport monitors `connection.Done` to terminate streams when the server is shutting down or cleaning up that transport.
- The transport calls `ServerRuntime.DeregisterConnection(connID)` when the stream ends.

Why `owner` is required:

- Each connection is associated with the transport instance that registered it.
- During deregistration/shutdown of a single transport, the transport can optionally request runtime cleanup via `ShutdownTransportConnections(ctx, owner)` to close only that transport's active connections.
- This prevents one transport from accidentally disrupting another transport's client streams and ensures targeted cleanup.

Shutdown model across transports:

- Transports handle protocol-specific graceful drain in their own `Shutdown(ctx)` implementation.
- Requesting owner-scoped cleanup through `ShutdownTransportConnections(ctx, owner)` is optional in the API contract, but is often important in practice for transports with long-lived streams.
- gRPC and REST both use this owner-scoped runtime drain to help active stream handlers exit promptly during graceful shutdown.
- A transport that truly has no runtime-managed streams may skip the request.
- The parent server still performs final shutdown cleanup as a safety net if a transport does not finish cleanup by itself.

## Transport Context Contract

The `catena.Transport` interface requires contexts for both lifecycle functions:

- `Start(ctx, runtime)`:
    - `ctx` is included for forward compatibility and graceful lifetime coordination.
    - Even if a transport does not fully consume the context today, the API shape avoids future breaking refactors when a transport needs context-aware startup/cleanup behavior.
    - `Start` should return quickly after launching background serving work.
- `Shutdown(ctx)`:
    - `ctx` is the shutdown deadline/cancellation boundary.
    - Transport implementations MUST respect `ctx` and return by the context deadline/cancellation.
    - Graceful drain is preferred, but implementations must not block indefinitely.
    - Implementations may optionally request runtime-owned stream signaling; for transports with long-lived runtime-managed streams this is typically part of graceful completion.

Caller guidance:

- Callers are strongly encouraged to pass a timeout-bounded context (for example, `context.WithTimeout`) to `Shutdown`.

## Choosing Transports

- Use `rest.Transport` for browser-first and JSON/HTTP workflows
- Use `grpc.Transport` for service-to-service and protobuf-native workflows
- Register both when you want one server process to expose both APIs

## Defaults

- REST default port: `9080`
- gRPC default port: `6254`
- gRPC reflection default: disabled
- TLS default: disabled on both transports

## TLS

Both transports support optional server-side TLS, configured independently via the `TLS` field on their options structs. Example:

```go
grpcOpts := grpc.DefaultOptions()
grpcOpts.TLS.Enabled = true
grpcOpts.TLS.CertFile = "/etc/catena/certs/server.crt"
grpcOpts.TLS.KeyFile = "/etc/catena/certs/server.key"
grpcTransport := grpc.NewTransport(grpcOpts)

restOpts := rest.DefaultOptions()
restOpts.TLS.Enabled = true
restOpts.TLS.CertFile = "/etc/catena/certs/server.crt"
restOpts.TLS.KeyFile = "/etc/catena/certs/server.key"
restOpts.TLS.ClientCAFile = "/etc/catena/certs/clients-ca.crt"
restOpts.TLS.MutualAuth = true // require client certificates (mTLS)
restTransport := rest.NewTransport(restOpts)
```

- `CertFile` and `KeyFile` are required when `Enabled` is true; a missing or unreadable file causes transport startup to fail rather than silently serving plaintext.
- `MutualAuth` requires clients to present a certificate signed by the CA bundle in `ClientCAFile`.
- The minimum accepted protocol version is TLS 1.2.
- When loading options from env/CLI via `config.InitOptions`, the corresponding inputs are `{PREFIX}_REST_TLS_*` / `--rest-tls-*` and `{PREFIX}_GRPC_TLS_*` / `--grpc-tls-*` (see `pkg/config/README.md`).

## Related Docs

- `REST.md` for REST transport details and endpoints
- `GRPC.md` for gRPC transport details and RPC coverage
