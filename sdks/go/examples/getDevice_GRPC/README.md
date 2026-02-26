# GetDevice gRPC Example

Demonstrates how to implement device information retrieval using the Catena gRPC server.

## Features

This example showcases:
- Multiple device slots (0: Video Processor, 1: Audio Mixer)
- Complete device models with parameters, constraints, menus, and commands
- Multi-language support (English and French)
- Handler registration for GetDevice endpoint
- Graceful shutdown handling

## Building

```bash
cd /home/ctwarog/Catena/sdks/go
make getDevice_GRPC
```

Binary will be created at: `~/Catena/build/go/getDevice_GRPC/getDevice_GRPC`

## Running

```bash
# From build directory
~/Catena/build/go/getDevice_GRPC/getDevice_GRPC

# Or directly with go run
cd /home/ctwarog/Catena/sdks/go/examples/getDevice_GRPC
go run .
```

## Configuration

Set the port via environment variable:
```bash
CATENA_PORT=50051 ~/Catena/build/go/getDevice_GRPC/getDevice_GRPC
```

Default port: Parsed from CATENA_PORT or config defaults

## Testing with grpcurl

### List services
```bash
grpcurl -plaintext localhost:6254 list
```

### Get populated slots
```bash
grpcurl -plaintext localhost:6254 st2138.CatenaService/GetPopulatedSlots
```

### Get device for slot 0 (Video Processor)
```bash
grpcurl -plaintext -d '{"slot": 0}' localhost:6254 st2138.CatenaService/DeviceRequest
```

### Get device for slot 1 (Audio Mixer)
```bash
grpcurl -plaintext -d '{"slot": 1}' localhost:6254 st2138.CatenaService/DeviceRequest
```

## Device Structure

### Slot 0: Primary Video Processor
- **Parameters:**
  - `brightness` (int32, 0-100)
  - `contrast` (int32, 0-100)
  - `input_source` (string choice: SDI, HDMI, IP)
- **Menu Groups:** Video (Basic, Input Configuration), System (Device Info)
- **Commands:** reboot, reset
- **Languages:** English, French

### Slot 1: Audio Mixer
- **Parameters:**
  - `master_gain` (float32, -60.0 to 12.0 dB)
  - `input_1_gain` (float32, -60.0 to 12.0 dB)
  - `mute` (int32 boolean)
- **Menu Groups:** Audio (Gain Control)
- **Commands:** mute_all
- **Languages:** English

## Code Structure

```go
// Create server
server := grpcServer.NewServer([]int{0, 1})

// Register GetDevice handler
server.RegisterGetDeviceHandler(slot, func() (catena.CatenaDevice, catena.StatusResult) {
    device, err := catena.ToCatenaDevice(deviceMap)
    if err != nil {
        return catena.ReplyError[catena.CatenaDevice](catena.INTERNAL, "failed")
    }
    return catena.Reply(device)
})

// Start server
server.Start(port)
```

## Comparison with REST

| Feature | REST Example | gRPC Example |
|---------|-------------|--------------|
| Protocol | HTTP/1.1 | HTTP/2 |
| Endpoint | `GET /st2138-api/v1/{slot}` | `DeviceRequest` RPC |
| Response | JSON | Protobuf (streamed) |
| Browser | ✅ Direct | ❌ Requires proxy |
| Testing | curl | grpcurl |

## Related Examples

- REST version: `/home/ctwarog/Catena/sdks/go/examples/getDevice_REST/`
- Get/Set Value (gRPC): `/home/ctwarog/Catena/sdks/go/examples/getSetValue_GRPC/`
- Get/Set Value (REST): `/home/ctwarog/Catena/sdks/go/examples/getSetValue_REST/`

## License

Copyright © 2026 Ross Video Ltd
