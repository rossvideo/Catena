/*
 * Copyright 2026 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief load configuration for the Catena SDK.
 * @file load.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-29
 */

package config

import (
	"flag"
	"fmt"
	"log/slog"
	"os"
	"slices"
	"strconv"
	"strings"
)

var ErrHelp = flag.ErrHelp

type InitOption func(opts *initOptions)

type initOptions struct {
	prefix           string
	defaults         RuntimeOptions
	suppressedInputs []string
}

func WithPrefix(prefix string) InitOption {
	return func(opts *initOptions) {
		opts.prefix = prefix
	}
}

func WithDefaults(defaults RuntimeOptions) InitOption {
	return func(opts *initOptions) {
		opts.defaults = defaults
	}
}

func WithSuppressedInputs(inputs ...string) InitOption {
	return func(opts *initOptions) {
		opts.suppressedInputs = inputs
	}
}

// InitOptions builds RuntimeOptions from defaults, environment variables,
// and command-line flags.
//
// Value precedence is:
// 1. defaults (DefaultRuntimeOptions, or WithDefaults if provided)
// 2. environment variables
// 3. command-line flags
//
// By default, environment variable names use the CATENA prefix, such as
// CATENA_LOG_LEVEL. WithPrefix changes this prefix.
//
// appName is used as the flag set name, appears in help output, is assigned to
// Logger.AppName, and may be used for log file naming. args should contain the
// command-line arguments to parse, excluding argv[0].
//
// Optional InitOption values customize loading behavior:
//   - WithPrefix("MYAPP") uses MYAPP_* environment variable names.
//   - WithDefaults(opts) replaces the default base RuntimeOptions before env/flag overrides.
//   - WithSuppressedInputs(names...) suppresses both env and CLI loading for the
//     provided option names (matched by CLI flag name, e.g. "dashboard-node-id").
//     Suppressed options are not registered in flag help output and their env vars
//     are ignored.
//
// If the help flag is provided, InitOptions prints usage and returns ErrHelp.
// If an environment variable or command-line flag cannot be parsed, it prints
// the error, prints usage, and returns the parse error. Callers should generally
// handle any non-nil error by choosing an exit code rather than printing the
// error again.
func InitOptions(appName string, args []string, initOpts ...InitOption) (RuntimeOptions, error) {
	// default init options before applying any custom options
	resolvedInitOpts := &initOptions{
		// default prefix is CATENA but can be overridden by init options
		prefix: "CATENA",
		// start off with the default runtime options, users may inject custom defaults via init options which will override these
		defaults: DefaultRuntimeOptions(),
	}

	// apply any custom init options to override defaults like the env var prefix or default option values
	for _, opt := range initOpts {
		opt(resolvedInitOpts)
	}

	// start with defaults and then override them with env vars and cli flags
	// cli flags take precedence over env vars
	opts := resolvedInitOpts.defaults
	// manually inject the app name into the Logger options
	opts.Logger.AppName = appName

	// Continue on err so we can send the err back to caller and letting them
	// handle exiting if they want to
	flags := flag.NewFlagSet(appName, flag.ContinueOnError)
	loader := &configLoader{
		flags:            flags,
		envPrefix:        resolvedInitOpts.prefix + "_",
		suppressedInputs: resolvedInitOpts.suppressedInputs,
	}
	// chain building all the options up
	loader.
		// runtime options
		extractBool("USE_GRPC", "use-grpc", "Enable gRPC transport", &opts.UseGrpc).
		extractBool("USE_REST", "use-rest", "Enable REST transport", &opts.UseRest).
		// transport options
		extractInt("REST_PORT", "rest-port", "Port the REST transport listens on", &opts.Rest.Port).
		extractInt("GRPC_PORT", "grpc-port", "Port the gRPC transport listens on", &opts.Grpc.Port).
		extractBool("GRPC_REFLECTION", "grpc-reflection", "Enable gRPC server reflection", &opts.Grpc.Reflection).
		//server options
		extractInt("MAX_CONNECTIONS", "max-connections", "Maximum number of concurrent connections", &opts.Server.MaxConnections).
		extractBool("DEV_MODE", "dev", "Enable development mode", &opts.Server.IsDev).
		extractBool("AUTHZ", "authz", "Enable authorization checks (requires auth). Use --authz=false to disable", &opts.Server.AuthzEnabled).
		// JWT validation options
		extractString("JWT_ISSUER", "jwt-issuer", "Expected JWT issuer for validating incoming requests", &opts.Server.JwtOptions.Issuer).
		extractString("JWT_AUDIENCE", "jwt-audience", "Expected JWT audience for validating incoming requests", &opts.Server.JwtOptions.Audience).
		extractBool("JWT_VALIDATE_SIGNATURE", "jwt-validate-signature", "Whether to validate the JWT signature or just the claims", &opts.Server.JwtOptions.ValidateSignature).
		// DashBoard connection-props options
		extractString("DASHBOARD_SERVICE_HOSTNAME", "dashboard-service-hostname", "Advertised hostname/address for DashBoard connection props", &opts.Dashboard.ServiceHostname).
		extractInt("DASHBOARD_PORT", "dashboard-port", "Port for the DashBoard connection-props HTTP server", &opts.Dashboard.Port).
		extractInt("DASHBOARD_SERVICE_PORT", "dashboard-service-port", "Catena service port advertised to DashBoard", &opts.Dashboard.ServicePort).
		extractBool("DASHBOARD_SERVICE_TLS_ENABLED", "dashboard-service-tls-enabled", "Whether the advertised DashBoard connection uses TLS", &opts.Dashboard.ServiceTLS).
		extractConnectionProtocol("DASHBOARD_PROTOCOL", "dashboard-protocol", "Transport advertised to DashBoard (st2138-rest, st2138-grpc, catena)", &opts.Dashboard.Protocol).
		extractString("DASHBOARD_SERVICE_NAME", "dashboard-service-name", "Advertised service URL for DashBoard connection props", &opts.Dashboard.ServiceName).
		extractString("DASHBOARD_NODE_NAME", "dashboard-node-name", "Human-readable node name advertised to DashBoard", &opts.Dashboard.NodeName).
		extractString("DASHBOARD_NODE_ID", "dashboard-node-id", "Unique node identifier advertised to DashBoard", &opts.Dashboard.NodeID).
		extractString("DASHBOARD_ENDPOINT", "dashboard-endpoint", "Path served by the DashBoard connection-props HTTP server", &opts.Dashboard.Endpoint).
		// logging options
		extractBool("SILENT", "silent", "Suppress all log output", &opts.Logger.Silent).
		extractString("LOG_DIR", "log-dir", "Directory for log files", &opts.Logger.LogDir).
		extractLogLevel("LOG_LEVEL", "log-level", "Minimum log level to output", &opts.Logger.Level).
		extractBool("LOG_TO_FILE", "log-to-file", "Enable file logging", &opts.Logger.WriteToFile).
		extractBool("LOG_TO_CONSOLE", "log-to-console", "Enable console logging", &opts.Logger.WriteToConsole).
		extractBool("LOG_USE_JSON", "log-use-json", "Output logs in JSON format", &opts.Logger.UseJSON)

	// basic flag lib doesn't support repeating flags, so we can just manually do the 3
	// levels of verbosity as a special case that sets the log level
	flagWarn := flags.Bool("v", false, "Warning logging level (equivalent to log level WARN)")
	flagInfo := flags.Bool("vv", false, "Info logging level (equivalent to log level INFO)")
	flagDebug := flags.Bool("vvv", false, "Debug logging level (equivalent to log level DEBUG)")

	// if there was an error setting up the flags (e.g. invalid env var) return that
	if loader.err != nil {
		fmt.Fprintf(flags.Output(), "Error parsing configuration: %v\n\n", loader.err)
		flags.Usage()
		return RuntimeOptions{}, loader.err
	}

	// do the cli parse now so it overrides any env vars
	err := flags.Parse(args)
	if err != nil {
		return RuntimeOptions{}, err
	}

	// check if log level was set via the log-level flag
	setLogLevel := false
	flags.Visit(func(f *flag.Flag) {
		if f.Name == "log-level" {
			setLogLevel = true
		}
	})
	// if log level flag wasn't set, check the shorthand verbosity flags
	if !setLogLevel {
		if *flagWarn {
			opts.Logger.Level = slog.LevelWarn
		}
		if *flagInfo {
			opts.Logger.Level = slog.LevelInfo
		}
		if *flagDebug {
			opts.Logger.Level = slog.LevelDebug
		}
	}

	// done, return the filled options
	return opts, nil
}

type configLoader struct {
	err              error
	flags            *flag.FlagSet
	envPrefix        string
	suppressedInputs []string
}

func (l *configLoader) prefixEnv(name string) string {
	return l.envPrefix + name
}

func (l *configLoader) shouldSuppressInput(cliName string) bool {
	return slices.Contains(l.suppressedInputs, cliName)
}

func (l *configLoader) extractBool(envName, cliName, usage string, val *bool) *configLoader {
	// skip if suppressed, this allows user code to customize which options are exposed via cli and env.
	if l.shouldSuppressInput(cliName) {
		return l
	}
	envName = l.prefixEnv(envName)
	// we have to do custom parsing for bools since strconv.ParseBool is very strict and
	// doesn't handle common env var values like "1" or "yes" also the flag package's
	// BoolVar doesn't support custom parsing, so we have to do it manually here instead
	// of using loadParser
	l.flags.BoolVar(val, cliName, *val, usage+" (env: "+envName+")")
	if l.err != nil {
		return l
	}
	if v := os.Getenv(envName); v != "" {
		switch strings.ToLower(v) {
		case "true", "1", "yes", "on":
			*val = true
		case "false", "0", "no", "off":
			*val = false
		default:
			l.err = fmt.Errorf("%s is not a valid bool for %s", v, envName)
		}
	}
	return l
}

func (l *configLoader) extractInt(envName, cliName, usage string, val *int) *configLoader {
	// just wrap strconv.Atoi, works cause the signature is the same
	loadParser(l, envName, cliName, usage, val, strconv.Atoi)
	return l
}

func (l *configLoader) extractString(envName, cliName, usage string, val *string) *configLoader {
	// dummy passthrough parser func
	loadParser(l, envName, cliName, usage, val, func(s string) (string, error) {
		return s, nil
	})
	return l
}

func (l *configLoader) extractConnectionProtocol(envName, cliName, usage string, val *ConnectionProtocol) *configLoader {
	// custom parser that validates the protocol against the known values
	loadParser(l, envName, cliName, usage, val, func(s string) (ConnectionProtocol, error) {
		switch ConnectionProtocol(s) {
		case ProtocolST2138Rest, ProtocolST2138Grpc, ProtocolST2138Catena:
			return ConnectionProtocol(s), nil
		default:
			return "", fmt.Errorf("%q is not a valid protocol (valid: %s, %s, %s)", s,
				ProtocolST2138Rest, ProtocolST2138Grpc, ProtocolST2138Catena)
		}
	})
	return l
}

func (l *configLoader) extractLogLevel(envName, cliName, usage string, val *slog.Level) *configLoader {
	// custom parser to convert from string to slog.Level
	loadParser(l, envName, cliName, usage, val, func(s string) (slog.Level, error) {
		var level slog.Level
		err := level.UnmarshalText([]byte(s))
		return level, err
	})
	return l
}

// loadParser is a helper that abstracts the common pattern of loading a config value from
// env and cli with error handling. It takes configLoader as a parameter instead of
// being a method because generic methods on non-generic types are not supported in Go.
func loadParser[T any](l *configLoader, envName, cliName, usage string, val *T, parser func(string) (T, error)) {
	// skip if suppressed, this allows user code to customize which options are exposed via cli and env.
	if l.shouldSuppressInput(cliName) {
		return
	}

	envName = l.prefixEnv(envName)

	// always register the flag so it appears in help
	l.flags.Var(newParserValue(val, parser), cliName, usage+" (env: "+envName+")")
	// if there's an error skip env parsing. just pass over this so it
	// cascades down to the end of the options parsing and the usage will show everything
	if l.err != nil {
		return
	}
	// parse the env var now, cli will overwrite during flags.Parse if set
	if v := os.Getenv(envName); v != "" {
		parsed, err := parser(v)
		if err != nil {
			l.err = err
			return
		}
		*val = parsed
	}
}

// compile-time check that parserValue implements flag.Value
var _ flag.Value = (*parserValue[any])(nil)

type parserValue[T any] struct {
	val    *T
	parser func(string) (T, error)
}

func newParserValue[T any](val *T, parser func(string) (T, error)) *parserValue[T] {
	return &parserValue[T]{val: val, parser: parser}
}

func (p *parserValue[T]) String() string {
	if p.val == nil {
		return ""
	}
	return fmt.Sprintf("%v", *p.val)
}

func (p *parserValue[T]) Set(s string) error {
	parsed, err := p.parser(s)
	if err != nil {
		return err
	}
	*p.val = parsed
	return nil
}
