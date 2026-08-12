# Catena Onboarding Guide (Co-op to Co-op)

> Written by an outgoing co-op student for the next one. This isn't an official Ross Video document — it's the explanation I wish someone had given me on day one. Where the codebase disagrees with existing docs, or where I'm inferring instead of being sure, I've said so explicitly.

**Last verified:** 2026-08-12
**Verified against branch/commit:** develop / `e373c12`
**Maintainer:** ...

## Contents

1. [What Catena is](#1-what-catena-is)
2. [Why Catena exists](#2-why-catena-exists)
3. [The big-picture architecture](#3-the-big-picture-architecture)
4. [Important repository areas](#4-important-repository-areas)
5. [Core concepts](#5-core-concepts)
6. [A simple end-to-end example](#6-a-simple-end-to-end-example)
7. [C++ and Go SDK overview](#7-c-and-go-sdk-overview)
8. [Key technologies and tools](#8-key-technologies-and-tools)
9. [How to build, run, and test](#9-how-to-build-run-and-test)
10. [Common sources of confusion](#10-common-sources-of-confusion)

---

## 1. What Catena is

**Catena is a rulebook ("protocol") that lets hardware or software describe itself to a controller app, so the controller doesn't need to know anything about that device ahead of time.**

Picture a TV studio full of boxes and services from different vendors: audio mixers, video switchers, graphics engines, encoders. Normally, each vendor invents its own way to control its own box. If you want one dashboard that operates all of them, you'd have to write custom code for every vendor.

Catena fixes this by giving every device one standard way to say: "Here are all my settings and buttons, here's each one's type, here's what values are allowed, and here's how to change them." A generic client can read that description and build a working control panel automatically — no device-specific code required.

**Analogy:** plugging in a USB mouse doesn't require installing a mouse-specific driver. The mouse tells the OS "I'm a HID device, here are my buttons," and the OS handles the rest. Catena does the same thing for media/broadcast devices — instead of USB, it's a **device model** (see [§5](#5-core-concepts)) sent over gRPC or REST.

---

## 2. Why Catena exists

Catena grew out of Ross Video's **openGear** platform (in production since 2006, used by 100+ partner companies) — see the root [`README.md`](../README.md). It's a redesign of that idea as an open, vendor-neutral standard rather than a Ross-only protocol.

The specification is referred to in this repo as both **ST 2138** and **ST 2138-a** (the latter is also the name of the [`smpte/`](../smpte) submodule's upstream repo). According to `README.md`, it's *"currently in discussion with [OSA] with the goal of SMPTE standardization"* — so treat it as a draft/emerging standard, not a finalized one.

Catena is a **control-plane** protocol only — it's for configuration, status, and commands, not for moving actual video/audio signal data. In a real deployment it runs alongside the media transport (SDI, NDI, ST 2110, etc.), not instead of it.

This repository contains three things:
1. The **specification** (protobuf messages + a JSON/YAML schema) — the rulebook.
2. Two **reference SDKs** that implement it: **C++** (mature, full-featured) and **Go** (newer).
3. Supporting **tooling**: a code generator, validators, and example device models.

---

## 3. The big-picture architecture

A Catena deployment has two roles:

- **A "device"** — could be real hardware or just a microservice in a container. It hosts a **device model**: a tree of parameters, commands, and metadata describing itself.
- **A client** (often called a "dashboard") — connects to a device, asks it to describe itself, then reads/writes values and issues commands.

They talk over one of two transports:

- **gRPC** — a binary RPC protocol over HTTP/2. Efficient, supports proper streaming. This is the primary transport used by most examples.
- **REST** — JSON over plain HTTP. Slower, but easy to test with curl, Postman, or a browser.

Both transports cover the same core operations (get/set a value, run a command, fetch the device description, subscribe to live updates), and the protocol itself doesn't mandate either one — `docs/Introduction.md` says "connection management is outside of Catena's scope." REST currently doesn't implement a couple of the less common RPCs (e.g. token refresh/revocation) that gRPC does — check the `.proto` file before assuming your own code is wrong if an operation seems missing.

```
 Client (dashboard, script, curl...)
              │
              │ 1. "Describe yourself"  (GetDevice / DeviceRequest)
              ▼
 ┌───────────────────────────┐
 │          Device           │
 │ (a program using an SDK)  │
 │  - device model           │
 │  - your business logic    │
 └───────────────────────────┘
              │
              │ 2. GetValue / SetValue / ExecuteCommand
              │ 3. Connect (long-lived stream of live updates)
              ▼
      Client's UI stays in sync
```

Two SDKs implement this protocol, and they are architecturally different even though they speak the same wire protocol: **C++** loads a device model from a file **at build time** via code generation, while **Go** builds the device model directly in code with no code-generation step. Don't assume knowledge transfers directly between them — see [§7](#7-c-and-go-sdk-overview) for the full comparison.

---

## 4. Important repository areas

```
Catena/
├── smpte/                    ← the protocol SPEC (git submodule, from SMPTE)
│   └── interface/proto/      ← the .proto files — source of truth for the wire protocol
│   └── interface/schemata/   ← JSON Schema for hand-authored device model files
├── sdks/
│   ├── cpp/
│   │   ├── common/           ← shared device-model runtime (Device, Param, Value, Constraint, ...)
│   │   ├── connections/      ← gRPC and REST transport implementations, built on top of common/
│   │   └── docs/             ← C++-specific build/toolchain docs
│   └── go/
│       ├── pkg/catena/       ← the Server (business-logic/connection-manager layer)
│       ├── pkg/st2138/       ← Go wrapper types around the generated protobuf messages
│       ├── pkg/transports/   ← grpc/ and rest/ wire-protocol implementations
│       ├── pkg/config/       ← environment-variable-driven configuration
│       └── examples/         ← hello_world and oneOfEverything reference programs
├── tools/codegen/            ← Node.js tool: device model (JSON/YAML) → generated C++ source
├── unittests/cpp/            ← C++ unit tests (separate top-level folder, not nested in sdks/cpp/)
├── docs/                     ← project documentation (this file lives here too)
├── scripts/                  ← dev scripts: make_cert.sh, run_coverage.sh, scan_security.sh
├── .devcontainer/            ← Docker-based dev environments for cpp/ and go/
└── .github/workflows/        ← CI pipelines
```

A few things worth knowing:

- **`smpte/` is a git submodule**, pulled from `https://github.com/SMPTE/st2138-a.git`. If you clone fresh, run `git submodule update --init --recursive` or it'll be empty. It's the most authoritative source for "what does the protocol actually say" — more reliable than some of the prose in `docs/` (see [§10](#10-common-sources-of-confusion)).
- **`tools/codegen/`** is the bridge between hand-written device models and the C++ SDK. The Go SDK doesn't use it.
- **`docs/`** is a mix of accurate, useful pages (`Validation.md`, `DevProcess.md`, `CONTRIBUTING.md`) and a couple of literal stubs (`PolyglotText.md`, `AlarmTable.md` just say "to do"). Don't assume every page is current.
- **`build/`, `coverage/`, `logs/`** are generated output, not source — safe to ignore or delete.

---

## 5. Core concepts

A handful of ideas show up constantly across both SDKs and the spec. Learn these first — you'll run into them immediately.

**OID / fqoid** — every parameter, command, and menu has a path-like identifier, e.g. `/location/latitude` or `/inputs/2/gain`. Think of it like a file path — you can navigate into nested structs and array elements the same way you'd navigate into subfolders.

**Slot** — one Catena connection can host multiple devices, each identified by an integer `slot`. Think of it like plugging several cards into one rack frame: the connection is to the frame, the `slot` picks which card. This is why almost every RPC payload includes a `slot` field even for the simplest single-device setup.

**Value (the `oneof`)** — a parameter's value is a protobuf `oneof`, meaning one `Value` message holds exactly one of: an int, float, string, struct, array, or "variant." This is why raw Catena JSON looks like `{"int32_value": 32}` instead of just `32` — the wrapper name carries the type. It trips up almost everyone the first time they look at raw Catena JSON; see `docs/Value.md` for a worked example with a nested struct.

**Constraints** — optional rules on a parameter's legal values: a numeric range (with an optional step size), a pick-list of choices, or an "alarm table" (a bitfield of named alarm conditions with severities). Constraints can be defined inline on one parameter, or as a shared constraint referenced by OID from several parameters at once, so they stay in sync.

**Commands** — structurally the same message as a parameter, but they live in a separate `commands` map and are meant to be *executed* (e.g. "reboot") rather than just read or written.

**Detail levels & subscriptions** — a client can ask for `FULL`, `MINIMAL`, `SUBSCRIPTIONS`, `COMMANDS`, or `NONE` detail when fetching the device model or connecting for live updates. This exists so a device with thousands of parameters (e.g. per-channel audio meters) doesn't have to send all of them to every client all the time.
> ⚠️ The *numeric* values for `DetailLevel` differ between the `.proto` file and the `.yaml` schema (the string names always agree) — always write `"FULL"` in device models, never a bare number. See [§10](#10-common-sources-of-confusion).

**Access scopes and authorization** — every device defines access scopes (by convention `st2138:mon` / `op` / `cfg` / `adm` — monitor/operate/configure/admin). Each parameter declares (or inherits from its parent) a scope. Clients authenticate with a JWT bearer token; the token's `scope` claim is decoded, and both SDKs check it against a parameter's required scope before allowing a read or write.

A few related concepts—variant arrays, templates, language packs, the `product` parameter, and the connection-manager/business-logic split—also appear in the codebase. Most new co-ops can learn them later when their assigned work requires them.

---

## 6. A simple end-to-end example

The clearest complete example in the repo is the Go **`hello_world`** program (`sdks/go/examples/hello_world/main.go`). Here's the flow, start to finish:

1. **Build a device model in memory.** `buildDevice()` creates an `st2138.Device` with one string parameter, `hello_world`.
2. **Create the server.** `catena.NewServer(...)` gives you a `Server` — at this point it just holds empty handler maps; it doesn't know what a device is yet.
3. **Register handlers.** `srv.RegisterGetValueHandler(...)`, `RegisterSetValueHandler(...)`, etc. — plain functions that read/write your own in-memory state.
4. **Register transports.** `srv.RegisterTransport(rest.NewTransport(...))` and `srv.RegisterTransport(grpc.NewTransport(...))` start both a REST server and a gRPC server (this example's default ports are 9080 and 6254 respectively), both driven by the *same* handlers.
5. **A client asks for a value**, e.g. `GET /st2138-api/v1/0/value/hello_world`. The REST transport parses the URL (slot `0`, oid `hello_world`), checks the caller's access scope, looks up the registered handler for that slot/oid, and calls it.
6. **The response flows back out**, serialized to JSON (REST) or binary protobuf (gRPC) — same handler, same data, different wire format. A REST response for a string parameter looks like `{"string_value": "Hello, World!"}` — recall from [§5](#5-core-concepts) that the wrapper name (`string_value`) is how the client knows the value's type.
7. **On a `SetValue`**, the handler updates its state and calls `srv.BroadcastUpdate(...)`, which pushes the new value to every client currently listening on the `Connect` stream — this is how a UI stays "live" without polling.

The C++ SDK performs the same overall flow through its generated device model. See [§7](#7-c-and-go-sdk-overview) for the architectural differences.

---

## 7. C++ and Go SDK overview

### C++ (`sdks/cpp/`)

- `common/` is an **in-memory, type-safe device-model runtime**: a `Device` object owns all of a device's parameters, commands, constraints, menus, and language packs, and provides thread-safe get/set plus protobuf serialization.
- Your device model (JSON/YAML) is **not** parsed at runtime. `tools/codegen/` reads it at build time and generates C++ source containing static objects that register themselves into a global `Device` when the program starts — wired into the build through `generate_catena_device(...)` in `CMakeLists.txt`.
- `connections/gRPC/` and `connections/REST/` are separate libraries on top of `common/`, each with one small "controller" per RPC. Both end up calling the same `Device` APIs — only the wire handling differs.
- Example programs live under `common/examples/` and `connections/*/examples/` — `hello_world` is the simplest; `use_structs`, `use_constraints`, and `audiodeck` show progressively more realistic device models.

### Go (`sdks/go/`)

- There's no code generation. `pkg/st2138` provides fluent builder types (`st2138.Device`, `st2138.Param`, ...) that wrap the generated protobuf messages directly — you build your device model in Go code.
- `pkg/catena` provides the `Server`: you register a handler function per operation, per device slot, and it doesn't care which transport is driving it.
- `pkg/transports/grpc` and `pkg/transports/rest` are separate packages that plug into the `Server` without depending on each other — a REST-only build never pulls in gRPC, and vice versa.
- `examples/hello_world` is the minimal starting point; `examples/oneOfEverything` is a larger reference model showing most parameter types and constraints in one place.

**Bottom line:** C++ is compile-time and code-generated; Go is runtime and hand-built. Pick whichever SDK matches the language you're working in and learn that one deeply before comparing — don't try to map concepts 1:1 between them.

---

## 8. Key technologies and tools

| Technology | Why it matters here |
|---|---|
| **Protocol Buffers / gRPC** | The wire format and interface-definition language for the whole spec — this is what's in `smpte/interface/proto/`. |
| **Boost** — C++ only | Powers the C++ SDK's REST server, CLI parsing, and config handling. |
| **jwt-cpp** — C++ only | Decodes JWT bearer tokens for authorization checks. |
| **AJV (JSON Schema validator)** | Validates device model JSON/YAML files against the schema before code generation (`tools/codegen`). |
| **google.golang.org/grpc, google.golang.org/protobuf** — Go only | Generated Go bindings and the gRPC runtime used by `pkg/protos` and `pkg/transports/grpc`. |
| **Docker / VS Code Dev Containers** | The supported way to get a consistent build toolchain without installing everything locally — see `.devcontainer/cpp/` and `.devcontainer/go/`. |
| **GitHub Actions** | Runs the build/test pipelines, plus security scanning (Trivy) and static analysis (CodeQL) on every PR. |

---

## 9. How to build, run, and test

Run this after a fresh checkout, or whenever generated schema files are missing or outdated:
```bash
cd ~/Catena/smpte && ./build-openapi.sh
```
C++ builds run this automatically (it's a CMake dependency). The Go `Makefile` doesn't require it, but CI runs it before Go tests too, so it's safest to run once regardless of SDK.

Both SDKs are configured through environment variables and CLI flags — neither uses application config files. C++ uses `CATENA_*` vars plus CMake flags like `-DCONNECTIONS="gRPC;REST"`. Go uses a configurable prefix (default `CATENA_`), e.g. `CATENA_PORT`, `CATENA_LOG_LEVEL`. Both write timestamped log files under a `logs/` folder — check there first when something misbehaves.

### C++
Recommended: open the repo in VS Code and use **Dev Containers: Rebuild and Reopen in Container** with the C++ container.
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCONNECTIONS="gRPC;REST" \
  -B ~/Catena/build/cpp -S ~/Catena/sdks/cpp
cd ~/Catena/build/cpp && ninja
ctest --output-on-failure          # run all tests
./common/examples/hello_world/hello_world
```
Coverage: `./scripts/run_coverage.sh`. Format before pushing: `npm run clang-format`.

### Go
Recommended: open in the Go dev container.
```bash
cd ~/Catena/sdks/go
make            # build
make test       # run unit tests
go run ./examples/hello_world
```
Once it's running, try it (both commands are printed by the example on startup):
```bash
curl http://localhost:9080/st2138-api/v1/0/value/hello_world
grpcurl -plaintext -import-path ~/Catena/smpte/interface/proto -proto service.proto \
  -d '{"slot":0,"oid":"hello_world"}' localhost:6254 st2138.CatenaService/GetValue
```

### What a successful run looks like
- The Go `hello_world` example logs that it's starting REST and gRPC listeners (ports 9080 and 6254 by default for this example) and prints the exact `curl`/`grpcurl` commands shown above.
- The `curl` command should return a small JSON body containing the parameter's value, wrapped by type — e.g. `{"string_value": "Hello, World!"}` (see [§5](#5-core-concepts) for why it's wrapped like this).
- The `grpcurl` command should return the equivalent value as JSON-formatted protobuf output.
- If a command hangs or refuses to connect, check that nothing else is already using that port, and check the relevant `logs/` folder for errors before assuming the SDK itself is broken.

### Validating a device model without building anything
```bash
node ~/Catena/smpte/tools/validate.js ./path/to/device.MyDevice.json
```

### Debugging
`.vscode/launch.json` has ready-made gdb/lldb configs for many example binaries and unit tests, plus a Node debugger config for the codegen tool itself ("Debug Codegen"). REST is much easier to poke at manually than gRPC — use curl, Postman, or a browser (`docs/TestingWithPostman.md` has a worked example, and REST URLs follow the pattern `/st2138-api/v1/<slot>/<endpoint>/<oid>`); for gRPC use `grpcurl` (as above) or `grpc_cli`. Note that several `launch.json` configs point at example binaries that no longer exist in this checkout — see [§10](#10-common-sources-of-confusion) before assuming your setup is broken.

### Where to start

A practical first task, not a full curriculum:

1. **Choose the SDK** that matches the work you've been assigned.
2. **Build and run that SDK's `hello_world` example** (see above).
3. **Make one `GetValue` request and one `SetValue` request** against it with curl or `grpcurl`.
4. **Trace one request from the example code** into the relevant SDK/transport code — for Go, from `main.go` into `pkg/catena/server.go` and the transport package you used; for C++, from the example's `.cpp` file into `IDevice.h`/`IParam.h`.
5. **Read the relevant `.proto` files** in `smpte/interface/proto/` once you've seen the protocol working — they'll mean much more now. Explore deeper topics only when your assigned work requires them.

---

## 10. Common sources of confusion

- **Not all prose docs in `docs/` are current.** Some are excellent (`Validation.md`, `CONTRIBUTING.md`, `DevProcess.md`). Some are stubs (`PolyglotText.md`, `AlarmTable.md`). `Constraints.md` marks alarm-table constraints as "not implemented yet" — but that page describes float-range constraints as supported, and both are in fact defined in `constraint.proto`. **When in doubt, trust the `.proto`/`.yaml` files over the prose.**
- **Enum numbers disagree between the `.proto` and the `.yaml` schema** for both `DetailLevel` and `ConstraintType` — the string names always agree. Always write the string (`"FULL"`, `"INT_RANGE"`, etc.), never a bare number, in a device model.
- **`.vscode/launch.json` references example programs that no longer exist** (`full_service`, `start_here`, `basic_param_access`, `api_only`). If a debug config can't find its binary, that's why — pick a config for an example that still exists (`hello_world`, `use_structs`, `audiodeck`, ...).
- **The Go SDK's own top-level `README.md` describes a design that isn't fully what's implemented.** It describes a shared, lockable device model co-owned by business logic and the connection manager, similar to C++. What's actually there is closer to plain handler registration — you own your own state. Trust `pkg/config/README.md`, `pkg/transports/README.md`, and the example code over the top-level README.
