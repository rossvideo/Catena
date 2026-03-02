# Catena gRPC Server

A complete gRPC server implementation for Catena that implements all endpoints defined in `BaseServer`.

## Features

The gRPC server implements the full ST2138 Catena service specification:

### Implemented Endpoints

✅ **Core Functionality:**
- `GetPopulatedSlots` - Returns list of available device slots
- `DeviceRequest` - Streams device information
- `GetValue` - Retrieves parameter values
- `SetValue` - Sets a single parameter value
- `MultiSetValue` - Sets multiple parameter values
- `ExternalObjectRequest` - Retrieves external objects (assets)
- `ExecuteCommand` - Executes commands on devices
- `Connect` - Establishes streaming connection for push updates

⚠️ **Not Yet Implemented:**
- `GetParam` - Get parameter metadata
- `ParamInfoRequest` - Stream parameter information
- `UpdateSubscriptions` - Handle parameter subscriptions
- `AddLanguage` - Add language packs
- `LanguagePackRequest` - Get language packs
- `ListLanguages` - List available languages
- `RefreshToken` - Refresh authentication tokens
- `RevokeAccess` - Revoke access tokens

## Usage

### Basic Server Setup

```go
package main

import (
    "log"
    
    "github.com/rossvideo/catena/sdks/go/pkg/catena"
    grpcServer "github.com/rossvideo/catena/sdks/go/pkg/catena/grpc"
    "github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func main() {
    // Initialize logger
    logger.Init(logger.DebugSettings())
    
    // Create server with slots
    server := grpcServer.NewServer([]int{0, 1, 2})
    
    // Register handlers (see below)
    
    // Start server on port 50051
    if err := server.Start(50051); err != nil {
        log.Fatalf("Server failed: %v", err)
    }
}
```

### Register Handlers

All handler registrations from `BaseServer` are available:

```go
// Get Device
server.RegisterGetDeviceHandler(0, func() (catena.CatenaDevice, catena.StatusResult) {
    deviceMap := map[string]any{
        "slot":         uint32(0),
        "detail_level": catena.DetailLevelFull,
        "display_name": "My Device",
    }
    
    device, _ := catena.ToCatenaDevice(deviceMap)
    return device, catena.StatusWithCode(catena.OK, "")
})

// Get Value
server.RegisterGetValueHandler(0, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
    value := int32(42)
    protoValue, _ := catena.ToProto(value)
    
    return catena.CatenaValue{Value: protoValue}, catena.StatusWithCode(catena.OK, "")
})

// Set Value
server.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
    // Handle the set operation
    log.Printf("Set %s = %v", fqoid, value)
    return catena.StatusWithCode(catena.OK, "")
})

// Execute Command
server.RegisterExecuteCommandHandler(0, func(slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
    result, _ := catena.ToProto("Command executed")
    return catena.CatenaValue{Value: result}, catena.StatusWithCode(catena.OK, "")
})

// Get Asset
server.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
    // Return asset data
    assetMap := map[string]any{
        "oid": fqoid,
        "data": []byte("asset content"),
    }
    
    asset, _ := catena.ToCatenaAsset(assetMap)
    return asset, catena.StatusWithCode(catena.OK, "")
})
```

### Broadcasting Updates to Connected Clients

The gRPC server supports broadcasting push updates to all clients connected via the `Connect` streaming endpoint:

```go
// Register SetValue handler that broadcasts changes
server.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
    // Update your state
    updateParameter(fqoid, value)
    
    // Broadcast to all streaming clients
    server.BroadcastUpdate(slot, fqoid, value)
    
    return catena.StatusWithCode(catena.OK, "")
})
```

**Client-side streaming example:**

```go
// Connect to server
stream, err := client.Connect(context.Background(), &protos.ConnectPayload{})
if err != nil {
    log.Fatal(err)
}

// Receive updates in a loop
for {
    update, err := stream.Recv()
    if err != nil {
        log.Printf("Stream ended: %v", err)
        break
    }
    
    switch kind := update.Kind.(type) {
    case *protos.PushUpdates_SlotsAdded:
        log.Printf("Slots added: %v", kind.SlotsAdded.Slots)
    case *protos.PushUpdates_Value:
        log.Printf("Parameter changed: %s = %v", 
            kind.Value.Oid, kind.Value.Value)
    }
}
```

### Graceful Shutdown

```go
import (
    "os"
    "os/signal"
    "syscall"
)

// Handle graceful shutdown
sigChan := make(chan os.Signal, 1)
signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

go func() {
    <-sigChan
    logger.Info("Shutting down...")
    server.Shutdown()  // Closes all connections and stops gRPC server
    os.Exit(0)
}()

server.Start(50051)
```

## Error Handling

The server automatically converts Catena status codes to gRPC status codes:

| Catena Code | gRPC Code |
|------------|-----------|
| `OK` | `OK` |
| `NOT_FOUND` | `NotFound` |
| `INVALID_ARGUMENT` | `InvalidArgument` |
| `UNIMPLEMENTED` | `Unimplemented` |
| `INTERNAL` | `Internal` |
| `PERMISSION_DENIED` | `PermissionDenied` |
| etc. | ... |

## Examples

See `~/Catena/sdks/go/examples` for complete working examples.

## Testing

Run the test suite:

```bash
cd ~/Catena/sdks/go
go test ./pkg/catena/grpc/... -v
```

## Architecture

The gRPC server:
1. Embeds `BaseServer` from `internal` package
2. Implements the generated `protos.CatenaServiceServer` interface
3. Delegates to registered handlers via `BaseServer`
4. Converts between Catena and gRPC types/errors

This architecture ensures consistency between REST and gRPC implementations while keeping protocol-specific logic separate.

## Generating gRPC Code

To regenerate the gRPC service code:

```bash
cd ~/Catena/sdks/go
make protos-grpc
```

This requires:
- `protoc` (Protocol Buffer compiler)
- `protoc-gen-go-grpc` plugin

## License

Copyright © 2026 Ross Video Ltd
