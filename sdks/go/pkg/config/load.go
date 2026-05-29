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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
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
	"strconv"
	"strings"
)

var ErrHelp = flag.ErrHelp

// InitOptions uses "CATENA" as the default prefix for environment variables.
// See InitOptionsPrefix for more details.
func InitOptions(appName string, args []string) (RuntimeOptions, error) {
	return InitOptionsPrefix(appName, "CATENA", args)
}

// InitOptionsPrefix builds RuntimeOptions from defaults, environment variables,
// and command-line flags.
//
// Environment variables are read using prefix to form option names, such as
// prefix+"_LOG_LEVEL". Command-line flags override environment values, and
// environment values override defaults.
//
// appName is used as the flag set name, appears in help output, is assigned to
// Logger.AppName, and may be used for log file naming. args should contain the
// command-line arguments to parse, excluding argv[0].
//
// If the help flag is provided, InitOptionsPrefix prints usage and returns
// ErrHelp. If an environment variable or command-line flag cannot be parsed,
// it prints the error, prints usage, and returns the parse error. Callers
// should generally handle any non-nil error by choosing an exit code rather
// than printing the error again.
func InitOptionsPrefix(appName, prefix string, args []string) (RuntimeOptions, error) {
	// start with defaults and then override them with env vars and cli flags
	// cli flags take precedence over env vars
	opts := defaultRuntimeOptions()
	// manually inject the app name into the Logger options
	opts.Logger.AppName = appName

	// Continue on err so we can send the err back to caller and letting them
	// handle exiting if they want to
	flags := flag.NewFlagSet(appName, flag.ContinueOnError)
	loader := &loader{
		flags: flags,
	}
	// chain building all the options up
	loader.
		Bool(prefix+"_USE_GRPC", "use-grpc", "Enable gRPC transport", &opts.UseGrpc).
		Bool(prefix+"_USE_REST", "use-rest", "Enable REST transport", &opts.UseRest).
		Int(prefix+"_MAX_CONNECTIONS", "max-connections", "Maximum number of concurrent connections", &opts.Server.MaxConnections).
		Bool(prefix+"_DEV_MODE", "dev", "Enable development mode", &opts.Server.IsDev).
		Bool(prefix+"_SILENT", "silent", "Suppress all log output", &opts.Logger.Silent).
		String(prefix+"_LOG_DIR", "log-dir", "Directory for log files", &opts.Logger.LogDir).
		LogLevel(prefix+"_LOG_LEVEL", "log-level", "Minimum log level to output", &opts.Logger.Level).
		Bool(prefix+"_LOG_TO_FILE", "log-to-file", "Enable file logging", &opts.Logger.WriteToFile).
		Bool(prefix+"_LOG_TO_CONSOLE", "log-to-console", "Enable console logging", &opts.Logger.WriteToConsole).
		Bool(prefix+"_LOG_USE_JSON", "log-use-json", "Output logs in JSON format", &opts.Logger.UseJSON)

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

type loader struct {
	err       error
	flags     *flag.FlagSet
	envPrefix string
	cliPrefix string
}

func (l *loader) WithEnvPrefix(prefix string) *loader {
	l.envPrefix = prefix
	return l
}

func (l *loader) WithCliPrefix(prefix string) *loader {
	l.cliPrefix = prefix
	return l
}

func (l *loader) Bool(envName, cliName, usage string, val *bool) *loader {
	// custom bool parsing to handle cli flags specially
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

func (l *loader) Int(envName, cliName, usage string, val *int) *loader {
	// just wrap strconv.Atoi, works cause the signature is the same
	l.err = loadParser(l.err, l.flags, envName, cliName, usage, val, strconv.Atoi)
	return l
}

func (l *loader) String(envName, cliName, usage string, val *string) *loader {
	// dummy passthrough parser func
	l.err = loadParser(l.err, l.flags, envName, cliName, usage, val, func(s string) (string, error) {
		return s, nil
	})
	return l
}

func (l *loader) LogLevel(envName, cliName, usage string, val *slog.Level) *loader {
	// custom parser to convert from string to slog.Level
	l.err = loadParser(l.err, l.flags, envName, cliName, usage, val, func(s string) (slog.Level, error) {
		var level slog.Level
		err := level.UnmarshalText([]byte(s))
		return level, err
	})
	return l
}

// loadParser is a helper that abstracts the common pattern of loading a config value from
// env and cli with error handling. Is not a method on loader since in go, methods can't be
// generic but we want to use it for different types.
func loadParser[T any](err error, flags *flag.FlagSet, envName, cliName, usage string, val *T, parser func(string) (T, error)) error {
	// always register the flag so it appears in help
	flags.Var(newParserValue(val, parser), cliName, usage+" (env: "+envName+")")
	// if there's an error skip parsing just pass over this so it
	// cascades down to the end of the options parsing
	if err != nil {
		return err
	}
	// parse the env var now, cli will overwrite during flags.Parse if set
	if v := os.Getenv(envName); v != "" {
		parsed, err := parser(v)
		if err != nil {
			return err
		}
		*val = parsed
	}
	return nil
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
