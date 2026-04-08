# Internal Package Architecture

The `internal` package provides shared infrastructure for both REST and gRPC server implementations.

## Shared Components

### BaseServer

Provides core handler management and connection management for all server types.

**Responsibilities:**
- Handler registration (GetDevice, GetValue, SetValue, GetAsset, ExecuteCommand)
- Handler lookup with defaults
- Slot management
- Connection queue management
- Broadcast updates to connected clients

**Usage:**
```go
type Server struct {
    *internal.BaseServer  // Embedded in both REST and gRPC servers
    // ... protocol-specific fields (mux, grpcServer, etc.)
}
```

### ConnectionQueue

Unified connection queue for managing streaming connections in both REST (SSE) and gRPC servers.

**Features:**
- Thread-safe connection registration/deregistration
- Broadcast updates to all connected clients
- Configurable connection limits
- Graceful shutdown with wait groups
- Protocol-specific logging (SSE vs gRPC)

**Design:**
```go
type Connection struct {
    ID      int
    Updates chan *protos.PushUpdates
    Done    chan struct{}
}

type ConnectionQueue struct {
    // Thread-safe map of connections
    connections    map[int]*Connection
    maxConnections int
}
```

## Why internal Package?

The `internal` package is special in Go:
- ✅ Can be imported by code in `sdks/go/pkg/` and subdirectories
- ❌ **Cannot** be imported by code outside `sdks/go/pkg/`

This provides:
1. **Code reuse** - REST and gRPC share common logic
2. **Encapsulation** - Implementation details hidden from external users
3. **API cleanliness** - Only public server interfaces exposed

## Package Structure

```
sdks/go/pkg/
├── catena/          # Public API (types, status codes, conversions)
├── rest/            # REST server implementation
│   └── server.go    # HTTP routing + BaseServer
├── grpc/            # gRPC server implementation
│   └── server.go    # gRPC service + BaseServer
├── internal/        # Shared private infrastructure
│   ├── base_server.go         # Handler + connection management
│   └── connection_queue.go    # Connection implementation
└── logger/          # Shared utilities
```

## Design Principles

### 1. Single Responsibility
- `BaseServer` - Handler + connection management (shared infrastructure)
- `ConnectionQueue` - Low-level connection implementation
- `Server` (REST/gRPC) - Protocol-specific routing only

### 2. DRY (Don't Repeat Yourself)
**Before:** Duplicate connection queue code in REST and gRPC  
**After:** Single `ConnectionQueue` implementation shared by both

### 3. Protocol Agnostic
`ConnectionQueue` works for both:
- REST: Server-Sent Events (SSE) over HTTP
- gRPC: Native streaming over HTTP/2

The only difference is the protocol name in logs.

## Usage in Servers

### REST Server
```go
func NewServer(slots []int, maxConnections int) *Server {
    return &Server{
        BaseServer: internal.NewBaseServer(slots, maxConnections),
        mux:        http.NewServeMux(),
        // ConnectionQueue is now in BaseServer
    }
}
```

### gRPC Server
```go
func NewServer(slots []int) *Server {
    return &Server{
        BaseServer: internal.NewBaseServer(slots, 0),
        grpcServer: grpc.NewServer(),
        // ConnectionQueue is now in BaseServer
    }
}
```

## Benefits of Consolidation

| Aspect | Before | After |
|--------|--------|-------|
| **Code duplication** | ~150 lines × 2 = 300 lines | 150 lines (shared) |
| **Maintenance** | Fix bugs twice | Fix once |
| **Testing** | Test twice | Test once + integration |
| **Consistency** | Can diverge | Always in sync |
| **Connection management** | Separate logic | Unified |

## Testing Strategy

1. **Unit Tests** - Test `StreamQueue` directly (`stream_queue_test.go`)
2. **Integration Tests** - Test within REST/gRPC servers
3. **Protocol-Specific** - Test SSE vs gRPC streaming separately

## Future Enhancements

The shared `ConnectionQueue` makes it easy to add features to both servers:

- Connection limits per slot
- Connection authentication/authorization
- Metrics and monitoring
- Rate limiting per connection
- Connection health checks

Just add once in `ConnectionQueue`, both servers benefit!

## License

Copyright © 2026 Ross Video Ltd
