# Catena Go SDK Config System

## Introduction

The Catena SDK is built around typed options structs that are passed into SDK constructors and setup functions.

This config package provides a convenient way to populate those options from environment variables and command-line flags.

Using this package is optional:

- Use it when you want a standard env/CLI loading path with defaults and precedence already handled.
- Skip it when you already have your own configuration system; in that case, populate the same options structs yourself and pass them directly to SDK APIs.

This package builds runtime configuration from:

- code defaults
- environment variables
- command-line flags

Precedence is:

1. command-line flags
2. environment variables
3. defaults

## Entry Points

- `InitOptions(appName, args)`
- `InitOptionsPrefix(appName, prefix, args)`

Use `InitOptions` for standard `CATENA_*` variables. Use `InitOptionsPrefix` when you need a custom env prefix.

## Basic Usage

```go
package main

import (
	"errors"
	"os"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

func main() {
	opts, err := config.InitOptions("my-app", os.Args[1:])
	if err != nil {
		if errors.Is(err, config.ErrHelp) {
			os.Exit(0)
		}
		// InitOptions already prints help/errors consistently.
		os.Exit(1)
	}

	_ = opts
}
```

## Example Scenarios

### Cloud / env-first

```bash
export CATENA_USE_GRPC=true
export CATENA_USE_REST=false
export CATENA_MAX_CONNECTIONS=500
export CATENA_AUTHZ=true
export CATENA_JWT_ISSUER=https://issuer.example.com
export CATENA_JWT_AUDIENCE=catena-api
export CATENA_LOG_LEVEL=info
```

Then run your binary normally. No CLI flags are required.

### Local testing / CLI overrides

```bash
CATENA_MAX_CONNECTIONS=100 ./my-app \
  --max-connections=20 \
  --dev \
  --use-grpc \
  --log-level=debug
```

In this case, `--max-connections=20` overrides `CATENA_MAX_CONNECTIONS=100`.

## RuntimeOptions Shape

`RuntimeOptions` includes:

- transport toggles (`UseGrpc`, `UseRest`)
- `ServerOptions`
- `LoggerOptions`

Defaults are defined in:

- `defaultRuntimeOptions`
- `DefaultServerOptions`
- `DefaultJwtValidationOptions`
- `DefaultLoggerOptions`

## Structured Logging

`RuntimeOptions` implements `slog.LogValuer` via `LogValue()`.

That means you can safely log config in structured form:

```go
logger.Info("startup config", "options", opts)
```

The logged payload is an explicit allow-list of fields from `RuntimeOptions`.
This gives a leak-resistant pattern: only fields included in `LogValue()` are emitted.
There are currently no secret fields in options, but this model protects future growth by default.

## Supported Environment Variables and Flags

All options can be configured via env and CLI.

### Runtime

- `PREFIX_USE_GRPC` <-> `--use-grpc`
- `PREFIX_USE_REST` <-> `--use-rest`

### Server

- `PREFIX_MAX_CONNECTIONS` <-> `--max-connections`
- `PREFIX_DEV_MODE` <-> `--dev`
- `PREFIX_AUTHZ` <-> `--authz`

### JWT

- `PREFIX_JWT_ISSUER` <-> `--jwt-issuer`
- `PREFIX_JWT_AUDIENCE` <-> `--jwt-audience`
- `PREFIX_JWT_VALIDATE_SIGNATURE` <-> `--jwt-validate-signature`

### Logger

- `PREFIX_SILENT` <-> `--silent`
- `PREFIX_LOG_DIR` <-> `--log-dir`
- `PREFIX_LOG_LEVEL` <-> `--log-level`
- `PREFIX_LOG_TO_FILE` <-> `--log-to-file`
- `PREFIX_LOG_TO_CONSOLE` <-> `--log-to-console`
- `PREFIX_LOG_USE_JSON` <-> `--log-use-json`

### Verbosity Shortcuts

- `-v` sets warn
- `-vv` sets info
- `-vvv` sets debug

If `--log-level` is explicitly set, it takes priority over these shortcuts.

## Parsing Notes

- bool CLI flags use Go's standard bool parser.
- for any bool option that defaults to true, disable it with `--flag=false` (for example, `--authz=false`).
- bool CLI values accepted: `true`, `false`, `1`, `0`, `t`, `f` (case-insensitive)
- bool env values accepted: `true`, `1`, `yes`, `on`, `false`, `0`, `no`, `off`
- the loader always registers flags first so help output is complete
- environment parse failures are surfaced as errors
- help requests return `ErrHelp`

## Testing

Use `t.Setenv` to set temporary environment values in tests. `t.Setenv` restores previous values automatically when the test ends.
