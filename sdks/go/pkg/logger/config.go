/*
 * Copyright 2025 Ross Video Ltd
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
 * @brief Config struct for logger.
 * @file config.go
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-12-09
 */

package logger

import (
	"log/slog"
	"os"
	"strings"
)

// Config holds logger configuration options
type Config struct {
	// AppName is used in log file naming
	AppName string

	// LogDir is the directory for log files
	LogDir string

	// Silent suppresses all log output
	Silent bool

	// Level is the minimum log level to output
	Level slog.Level

	// WriteToFile enables file logging
	WriteToFile bool

	// WriteToConsole enables console (stderr) logging
	WriteToConsole bool

	// UseJSON outputs logs in JSON format (useful for structured logging)
	UseJSON bool
}

// DefaultConfig returns sensible defaults for release mode
func DefaultConfig() Config {
	return Config{
		AppName:        "catena",
		LogDir:         "./logs",
		Silent:         false,
		Level:          slog.LevelError, // ERROR only by default
		WriteToFile:    true,
		WriteToConsole: true,
		UseJSON:        false,
	}
}

// DebugConfig returns configuration for debug/development mode
func DebugConfig() Config {
	cfg := DefaultConfig()
	cfg.Level = slog.LevelDebug
	return cfg
}

// ParseFromEnv creates a Config from environment variables.
// The prefix parameter specifies the environment variable prefix (e.g., "CATENA").
// Environment variables checked:
//   - {PREFIX}_LOG_DIR: directory for log files (default: "./logs")
//   - {PREFIX}_SILENT: "true" to suppress all output
//   - {PREFIX}_LOG_LEVEL: "debug", "info", "warn", "error" (default: "error")
//   - {PREFIX}_LOG_FILE: "true"/"false" to enable file logging (default: "true")
//   - {PREFIX}_LOG_CONSOLE: "true"/"false" to enable console logging (default: "true")
//   - {PREFIX}_LOG_JSON: "true" to output JSON format
func ParseFromEnv(prefix string) Config {
	if prefix == "" {
		prefix = "CATENA"
	}

	cfg := DefaultConfig()

	if v := os.Getenv(prefix + "_LOG_DIR"); v != "" {
		cfg.LogDir = v
	}

	if os.Getenv(prefix+"_SILENT") == "true" {
		cfg.Silent = true
	}

	if v := os.Getenv(prefix + "_LOG_LEVEL"); v != "" {
		cfg.Level = ParseLevel(v)
	}

	if v := os.Getenv(prefix + "_LOG_FILE"); v != "" {
		cfg.WriteToFile = v == "true"
	}

	if v := os.Getenv(prefix + "_LOG_CONSOLE"); v != "" {
		cfg.WriteToConsole = v == "true"
	}

	if os.Getenv(prefix+"_LOG_JSON") == "true" {
		cfg.UseJSON = true
	}

	return cfg
}

// ParseLevel parses a string level name to slog.Level
// Accepts: "debug", "info", "warn", "warning", "error" (case-insensitive)
// Returns slog.LevelError for unrecognized values
func ParseLevel(s string) slog.Level {
	switch strings.ToLower(strings.TrimSpace(s)) {
	case "debug":
		return slog.LevelDebug
	case "info":
		return slog.LevelInfo
	case "warn", "warning":
		return slog.LevelWarn
	case "error":
		return slog.LevelError
	default:
		return slog.LevelError
	}
}

// ParseVerbosity parses command-line arguments for -v and -vv flags
// Returns the corresponding slog.Level:
//   - 0 (no -v): ERROR only
//   - 1 (-v): WARNING and ERROR
//   - 2+ (-vv): INFO, WARNING, and ERROR
func ParseVerbosity(args []string) slog.Level {
	verbosity := 0
	for _, arg := range args {
		switch arg {
		case "-vv":
			return slog.LevelInfo // -vv is max verbosity for release
		case "-v":
			verbosity++
		default:
			// Check for combined flags like -vvv (only consecutive v's)
			if strings.HasPrefix(arg, "-") && !strings.HasPrefix(arg, "--") {
				flagContent := arg[1:]
				// Only count if the flag consists entirely of 'v' characters
				if len(flagContent) > 0 && strings.Trim(flagContent, "v") == "" {
					verbosity += len(flagContent)
				}
			}
		}
	}

	switch verbosity {
	case 0:
		return slog.LevelError
	case 1:
		return slog.LevelWarn
	default:
		return slog.LevelInfo
	}
}

// ParseVerbosityFromOS parses verbosity from os.Args and returns slog.Level
func ParseVerbosityFromOS() slog.Level {
	return ParseVerbosity(os.Args[1:])
}

// ParseConfigWithVerbosity parses config from environment variables and applies
// CLI verbosity flags (-v, -vv). CLI flags override env level if more verbose.
func ParseConfigWithVerbosity(prefix string) Config {
	cfg := ParseFromEnv(prefix)
	cliLevel := ParseVerbosityFromOS()
	if cliLevel < cfg.Level {
		cfg.Level = cliLevel
	}
	return cfg
}
