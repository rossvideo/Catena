# Catena SDK Configuration Guide

Catena is an embeddable SDK that enables communication over the Catena protocol. It is designed to be **simple for third‑party applications to integrate**, with flexible configuration via:

1.  **Command‑line flags**
2.  **Environment variables** (with a configurable prefix)
3.  **Direct configuration via globals** (opt‑out path)

***

## Configuration Resolution Order

When using Catena’s configuration system, values are resolved in the following order (highest priority first):

1.  **Command‑line flags**
2.  **Prefixed environment variables**
3.  **Runtime defaults**

If you choose not to use the configuration system, you may configure Catena directly via global variables.

***

## Enabling the Catena Configuration System

Consumers of the Catena SDK enable configuration by calling `initConfigVariables(...)` during startup.

### Minimal Boilerplate

```cpp
#include <Config.h>

int main(int argc, char* argv[])
{
    const auto [exit, code] =
        catena::common::config::initConfigVariables(argc, argv);

    if (exit) {
        // --help requested, or a fatal configuration error occurred
        return code;
    }

    // Application logic continues here
}
```

This single call enables:

*   Command‑line flag parsing
*   Environment variable parsing
*   Runtime default initialization
*   Validation and early exit behavior

***

## Environment Variable Configuration

Every configuration option may also be set via environment variables using a **prefix**.

### Default Prefix

    CATENA_

### Example

```bash
export CATENA_PORT=6254
export CATENA_SECURE_COMMS=tls
export CATENA_LOG_LEVEL=info
```

***

### Name Mapping Rules

Environment variable names are derived from flag names as follows:

*   Remove leading `--`
*   Convert to uppercase

| Command‑Line Flag   | Environment Variable     |
| ------------------- | ------------------------ |
| `--secure_comms`    | `CATENA_SECURE_COMMS`    |
| `--log_dir`         | `CATENA_LOG_DIR`         |
| `--max_connections` | `CATENA_MAX_CONNECTIONS` |

***

## Custom Environment Variable Prefix

The environment variable prefix is configurable via the **third, optional parameter** to `initConfigVariables`.

```cpp
catena::common::config::initConfigVariables(
    argc,
    argv,
    "MYAPP_CATENA_"
);
```

This allows Catena to coexist cleanly inside larger systems with shared environments.

If omitted, the default prefix `CATENA_` is used.

***

## Boolean Option Semantics

Catena’s configuration system follows common C++ command‑line conventions for boolean options, with behavior designed to be intuitive across **flags**, **environment variables**, and **defaults**.

### Boolean Flags (Command Line)

Boolean options may be expressed in several equivalent ways:

```bash
--silent
--silent=1
--silent=true
--silent=on
```

All of the above enable the option.

Likewise, the following disable the option:

```bash
--silent=0
--silent=false
--silent=off
```

For boolean options that declare an optional value, supplying the flag **without an explicit value** enables it.

Example:

```bash
--log_console
```

is equivalent to:

```bash
--log_console=1
```

***

### Boolean Values (Environment Variables)

When configured via environment variables, boolean options accept the same set of values.

All of the following are treated as **true**:

```text
1
true
on
yes
```

All of the following are treated as **false**:

```text
0
false
off
no
```

Examples:

```bash
export CATENA_SILENT=1
export CATENA_MUTUAL_AUTHC=true
export CATENA_LOG_CONSOLE=off
```

> Value matching is **case‑insensitive**.

***

### Defaults and Explicit Overrides

If a boolean option has a default value (e.g. `0`), it will remain unchanged unless explicitly overridden by:

1.  A command‑line flag
2.  A corresponding environment variable

Example:

```text
--mutual_authc (default: 0)
```

*   No flag or env var → **disabled**
*   `--mutual_authc` → **enabled**
*   `CATENA_MUTUAL_AUTHC=0` → **explicitly disabled**

This allows clear, deterministic behavior even in complex deployment environments.

***

### Recommended Usage Patterns

✅ **Preferred (clear and explicit):**

```bash
--authz=on
CATENA_LOG_FILE=false
```

⚠️ **Avoid ambiguous values:**

```bash
CATENA_LOG_FILE=enabled   # not recognized
CATENA_SILENT=yesplease # invalid
```

Use canonical values (`0/1`, `true/false`, `on/off`) to ensure portability and clarity.

***

## Default Paths and Runtime Resolution

Several default paths are **resolved at runtime** using the user’s home directory as defined by the environment (e.g. `$HOME` on Linux/macOS).

> **Important:** Defaults are *not hard‑coded*. They are constructed dynamically by reading the environment during initialization.

Examples:

*   `certs`:
    ```text
    ${HOME}/test_certs
    ```

*   `log_dir`:
    ```text
    ${HOME}/Catena/logs
    ```

This ensures portability across systems and users.

***

## Configuration Options

### General

| Option   | Default | Description         |
| -------- | ------- | ------------------- |
| `--help` | —       | Print help and exit |

***

### Network & Service

| Option              | Default   | Description                        |
| ------------------- | --------- | ---------------------------------- |
| `--hostname`        | `0.0.0.0` | Publicly accessible hostname or IP |
| `--port`            | `6254`    | Catena service port                |
| `--max_connections` | `16`      | Max concurrent connections         |

***

### Dashboard / Connection Properties

| Option                    | Default | Description                        |
| ------------------------- | ------- | ---------------------------------- |
| `--dashboard_port`        | `8080`  | Port serving `connectionprops.xml` |
| `--dashboard_tls_enabled` | `0`     | Indicates TLS usage to dashboard   |

***

### Secure Communications (TLS)

| Option           | Default              | Description                     |
| ---------------- | -------------------- | ------------------------------- |
| `--secure_comms` | `off`                | `off` or `tls`                  |
| `--certs`        | `${HOME}/test_certs` | Directory containing cert files |
| `--cert_file`    | `server.crt`         | TLS certificate file            |
| `--key_file`     | `server.key`         | TLS private key                 |
| `--ca_file`      | `ca.crt`             | CA certificate                  |
| `--private_ca`   | `0`                  | Use a private CA                |
| `--mutual_authc` | `0`                  | Require client certificate auth |

***

### Authorization

| Option    | Default | Description                      |
| --------- | ------- | -------------------------------- |
| `--authz` | `0`     | Enable OAuth token authorization |

***

### Static Content

| Option          | Default   | Description                         |
| --------------- | --------- | ----------------------------------- |
| `--static_root` | `${HOME}` | Root directory for external objects |

***

### Parameter Limits

| Option                       | Default | Description                      |
| ---------------------------- | ------- | -------------------------------- |
| `--default_max_array_size`   | `1024`  | Max length per array or string   |
| `--default_total_array_size` | `1024`  | Max cumulative string array size |

***

### Logging

| Option                 | Default               | Description                               |
| ---------------------- | --------------------- | ----------------------------------------- |
| `--log_dir`            | `${HOME}/Catena/logs` | Log output directory                      |
| `--silent`             | `0`                   | Disable all logging                       |
| `--log_level`          | `trace`               | trace, debug, info, warning, error, fatal |
| `--log_console`        | `1`                   | Enable console logging                    |
| `--log_file`           | `1`                   | Enable file logging                       |  
| `--log_append`         | `1`                   | Append to existing log file               |
| `--log_size`           | `10`                  | Max MB before rotation                    |
| `--log_count`          | `5`                   | Number of retained log files              |
| `--log_max_size`       | `50`                  | Total log size budget (MB)                |
| `--log_final_rotation` | `0`                   | Archive active log at shutdown            |

***

## Opting Out: Direct Configuration (No Config System)

Use this approach if:

*   Your application already manages configuration
*   You do not want CLI or environment parsing
*   You embed Catena as a pure library

In this case, **do not call** `initConfigVariables(...)`.

### Example

```cpp
#include <Config.h>

using namespace catena::common;

int main()
{
    config::secure_comms = "tls";
    config::port = 6254;
    config::log_level = "info";
    config::max_connections = 32;

    // Initialize Catena-enabled components directly
}
```

> ⚠️ When bypassing the config system, it is the application’s responsibility
> to ensure values are set **before** Catena APIs are used.
