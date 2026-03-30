# Catena REST Server

A complete REST HTTP server implementation for Catena that implements all endpoints defined in `BaseServer` following the ST2138 specification.

## Features

The REST server implements the full ST2138 Catena HTTP API specification:

### Implemented Endpoints

✅ **Core Functionality:**
- `GET /st2138-api/v1/{slot}` - Get device information
- `GET /st2138-api/v1/{slot}/value/{oid}` - Get parameter value
- `PUT /st2138-api/v1/{slot}/value/{oid}` - Set parameter value
- `GET /st2138-api/v1/{slot}/asset/{oid}` - Get external object (asset)
- `POST /st2138-api/v1/{slot}/command/{oid}` - Execute command
- `GET /st2138-api/v1/connect` - SSE streaming for push updates

✅ **Additional Features:**
- Server-Sent Events (SSE) for real-time parameter updates
- Connection queue management with configurable limits
- CORS support for cross-origin requests
- Fallback handler for custom routing
- Graceful shutdown support

## Usage

### Basic Server Setup

```go
package main

import (
    "log"
    
    "github.com/rossvideo/catena/sdks/go/pkg/catena"
    "github.com/rossvideo/catena/sdks/go/pkg/logger"
    "github.com/rossvideo/catena/sdks/go/pkg/rest"
)

func main() {
    // Initialize logger
    logger.Init(logger.DebugSettings())
    
    // Create server with slots and max connections
    server := rest.NewServer([]int{0, 1, 2}, 100)
    
    // Register handlers (see below)
    
    // Start server on port 8080
    if err := server.Start(8080); err != nil {
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
    
    // Broadcast update to all SSE clients
    server.BroadcastUpdate(slot, fqoid, value)
    
    return catena.StatusWithCode(catena.NO_CONTENT, "")
})

// Execute Command (REST-specific signature with http.ResponseWriter)
server.RegisterExecuteCommandHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
    result, _ := catena.ToProto("Command executed")
    return catena.CatenaValue{Value: result}, catena.StatusWithCode(catena.OK, "")
})

// Get Asset
server.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
    assetMap := map[string]any{
        "oid": fqoid,
        "data": []byte("asset content"),
    }
    
    asset, _ := catena.ToCatenaAsset(assetMap)
    return asset, catena.StatusWithCode(catena.OK, "")
})
```

### Server-Sent Events (SSE)

The REST server includes built-in SSE support for real-time updates:

```go
// Set maximum concurrent SSE connections
server.SetMaxConnections(100)

// Broadcast updates to all connected clients
server.RegisterSetValueHandler(0, func(value any, slot int, fqoid string) catena.StatusResult {
    // Update your state
    updateParameter(fqoid, value)
    
    // Notify all SSE clients about the change
    server.BroadcastUpdate(slot, fqoid, value)
    
    return catena.StatusWithCode(catena.NO_CONTENT, "")
})
```

**Client-side SSE connection:**
```javascript
const eventSource = new EventSource('http://localhost:8080/st2138-api/v1/connect');

eventSource.onmessage = (event) => {
    const update = JSON.parse(event.data);
    console.log('Parameter updated:', update);
};
```

### Custom Fallback Handler

Add custom routing for endpoints not in the ST2138 spec:

```go
server.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
    // Handle custom endpoints
    if r.URL.Path == "/health" {
        response, _ := catena.ToProto("healthy")
        return catena.CatenaValue{Value: response}, catena.StatusWithCode(catena.OK, "")
    }
    
    return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found")
})
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
    server.Shutdown()  // Closes all SSE connections gracefully
    os.Exit(0)
}()

server.Start(8080)
```

## REST API Endpoints

### Device Information
```
GET /st2138-api/v1/{slot}
```
Returns the device model for the specified slot.

**Example:**
```bash
curl http://localhost:8080/st2138-api/v1/0
```

### Parameter Operations

**Get Value:**
```
GET /st2138-api/v1/{slot}/value/{oid}
```

**Example:**
```bash
curl http://localhost:8080/st2138-api/v1/0/value/brightness
```

**Set Value:**
```
PUT /st2138-api/v1/{slot}/value/{oid}
Content-Type: application/json

{"int32_value": 75}
```

**Example:**
```bash
curl -X PUT http://localhost:8080/st2138-api/v1/0/value/brightness \
  -H "Content-Type: application/json" \
  -d '{"int32_value": 75}'
```

### Asset (External Object)
```
GET /st2138-api/v1/{slot}/asset/{oid}
```

**Example:**
```bash
curl http://localhost:8080/st2138-api/v1/0/asset/logo
```

### Command Execution
```
POST /st2138-api/v1/{slot}/command/{oid}
Content-Type: application/json

{"string_value": "reset"}
```

**Example:**
```bash
curl -X POST http://localhost:8080/st2138-api/v1/0/command/reset \
  -H "Content-Type: application/json" \
  -d '{"string_value": "factory"}'
```

### Server-Sent Events
```
GET /st2138-api/v1/connect
```

Establishes an SSE connection for real-time parameter updates.

**Example:**
```bash
curl -N http://localhost:8080/st2138-api/v1/connect
```

## Error Handling

The server automatically converts Catena status codes to HTTP status codes:

| Catena Code | HTTP Status |
|------------|-------------|
| `OK` | 200 OK |
| `CREATED` | 201 Created |
| `NO_CONTENT` | 204 No Content |
| `INVALID_ARGUMENT` | 400 Bad Request |
| `UNAUTHENTICATED` | 401 Unauthorized |
| `PERMISSION_DENIED` | 403 Forbidden |
| `NOT_FOUND` | 404 Not Found |
| `METHOD_NOT_ALLOWED` | 405 Method Not Allowed |
| `CONFLICT` | 409 Conflict |
| `UNIMPLEMENTED` | 501 Not Implemented |
| `UNAVAILABLE` | 503 Service Unavailable |
| etc. | ... |

**Error Response Format:**
```json
{
  "error": "parameter not found"
}
```

In production mode (non-dev), detailed error messages are replaced with standard HTTP status text.

## Examples

Complete working examples:
- `~/Catena/sdks/go/examples/getSetValue_REST/` - Parameter get/set
- `~/Catena/sdks/go/examples/getDevice_REST/` - Device information
- `~/Catena/sdks/go/examples/getAsset_REST/` - Asset retrieval
- `~/Catena/sdks/go/examples/executeCommand_REST/` - Command execution

## Testing

Run the test suite:

```bash
cd ~/Catena/sdks/go
go test ./pkg/rest/... -v
```

## Connection Management

### SSE Connection Queue

The REST server includes a connection queue to manage SSE clients:

```go
// Set maximum concurrent connections (0 = unlimited)
server.SetMaxConnections(100)

// Broadcast updates to all connected clients
server.BroadcastUpdate(slot, "brightness", int32(80))
```

When the limit is reached, new connections receive a `503 Service Unavailable` error.

### Graceful Shutdown

```go
server.Shutdown()  // Closes all SSE connections and waits for goroutines
```

This:
1. Signals all SSE connections to close
2. Waits for all connection goroutines to finish
3. Ensures clean shutdown without data loss

## Architecture

The REST server:
1. Embeds `BaseServer` from `internal` package
2. Uses `http.ServeMux` for HTTP routing
3. Delegates to registered handlers via `BaseServer`
4. Converts between HTTP requests/responses and Catena types
5. Manages SSE connections via `connectionQueue`

```
BaseServer (internal)
    ↑
    |
REST Server (pkg/rest)
    ├── ServeMux (HTTP routing)
    ├── ConnectionQueue (SSE management)
    └── Handler methods (HTTP-specific)
```

This architecture ensures consistency between REST and gRPC implementations while keeping protocol-specific logic separate.

## Configuration

### Environment Variables

The server respects standard Catena environment variables:

- `CATENA_PORT` - Server port (default varies by example)
- `CATENA_LOG_LEVEL` - Logging level: debug, info, warn, error
- `CATENA_LOG_FILE` - Enable file logging: true/false
- `CATENA_LOG_CONSOLE` - Enable console logging: true/false

See `logger.ParseFromEnv()` for complete list.

## CORS Support

CORS headers are automatically added to SSE connections and can be extended for all endpoints if needed. Current implementation includes:

- `Access-Control-Allow-Origin`
- `Access-Control-Allow-Methods`
- `Access-Control-Allow-Headers`
- `Access-Control-Allow-Credentials`

## Performance Considerations

### Connection Limits

Set appropriate limits based on your use case:

```go
// High-traffic production
server.SetMaxConnections(1000)

// Development/testing
server.SetMaxConnections(10)

// Unlimited (use with caution)
server.SetMaxConnections(0)
```

### SSE Channel Buffers

Each SSE connection has a buffered channel (100 messages). If a client can't keep up, updates are dropped with a warning log.

## Comparison: REST vs gRPC

| Feature | REST Server | gRPC Server |
|---------|-------------|-------------|
| Protocol | HTTP/1.1 | HTTP/2 |
| Serialization | JSON | Protobuf |
| Streaming | SSE (Server-Sent Events) | Native bidirectional |
| Browser Support | ✅ Direct | ❌ Requires proxy |
| Connection Management | Manual (connectionQueue) | Built-in |
| Reflection/Discovery | Manual | Built-in |
| Performance | Good | Excellent |
| Debugging | curl, browser | grpcurl, grpc_cli |

**Use REST when:**
- Browser clients need direct access
- JSON is preferred over protobuf
- HTTP/1.1 infrastructure required

**Use gRPC when:**
- Backend-to-backend communication
- Performance is critical
- Bidirectional streaming needed

## License

Copyright © 2026 Ross Video Ltd
