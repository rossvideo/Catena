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
 * @brief Configuration structures
 * @file config.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-29
 */

package config

import (
	"log/slog"
	"net/http"
	"time"
)

type RuntimeOptions struct {
	UseGrpc bool
	UseRest bool
	Server  ServerOptions
	Logger  LoggerOptions
}

type ServerOptions struct {
	// true for development mode, false for production mode
	IsDev bool

	// maximum number of concurrent connections (default: 100)
	MaxConnections int
	// AuthzEnabled enables authorization checks for all endpoints. If false, all requests are allowed.
	AuthzEnabled bool
	// JwtOptions configures JWT validation for incoming requests when AuthzEnabled is true
	JwtOptions JwtValidationOptions
}

// JwtValidationOptions controls optional claim validation and HTTP behavior.
type JwtValidationOptions struct {
	// AllowedAlgs specifies which signing algorithms are allowed. If empty, defaults to ES256.
	AllowedAlgs []string
	// Audience used to validate the "aud" claim. Optional.
	Audience string
	// Issuer used to validate the "iss" claim and discover the JWKS endpoint if ValidateSignature is true. Optional.
	Issuer string
	// Leeway allows some clock skew when validating "exp", "nbf", and "iat" claims. Optional.
	Leeway time.Duration
	// ValidateSignature controls whether the JWT signature should be validated against the JWKS. If false, only claims are validated.
	ValidateSignature bool

	// Http allows users to provide a custom HTTP client for discovering the JWKS. Optional.
	Http *http.Client
}

type LoggerOptions struct {
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

func defaultRuntimeOptions() RuntimeOptions {
	return RuntimeOptions{
		UseGrpc: false,
		UseRest: false,
		Server:  DefaultServerOptions(),
		Logger:  DefaultLoggerOptions(),
	}
}

func DefaultServerOptions() ServerOptions {
	return ServerOptions{
		IsDev:          false,
		MaxConnections: 100,
		AuthzEnabled:   true,
		JwtOptions:     DefaultJwtValidationOptions(),
	}
}

func DefaultJwtValidationOptions() JwtValidationOptions {
	return JwtValidationOptions{
		AllowedAlgs:       nil, // will default to ES256 in the code if empty
		Audience:          "",
		Issuer:            "",
		Leeway:            0,
		ValidateSignature: true,
		Http:              nil, // will default to http.DefaultClient in the code if nil
	}
}

// DefaultLoggerOptions returns sensible defaults for release mode
func DefaultLoggerOptions() LoggerOptions {
	return LoggerOptions{
		AppName:        "catena",
		LogDir:         "./logs",
		Silent:         false,
		Level:          slog.LevelInfo, // INFO default in prod
		WriteToFile:    true,
		WriteToConsole: true,
		UseJSON:        false,
	}
}

// make sure RuntimeOptions implements slog.LogValuer for structured logging of the config
var _ slog.LogValuer = RuntimeOptions{}

func (o RuntimeOptions) LogValue() slog.Value {
	return slog.GroupValue(
		slog.Bool("dev", o.Server.IsDev),
		slog.Int("max_connections", o.Server.MaxConnections),
		slog.Group("logger",
			slog.String("app_name", o.Logger.AppName),
			slog.Bool("silent", o.Logger.Silent),
			slog.String("log_dir", o.Logger.LogDir),
			slog.String("level", o.Logger.Level.String()),
			slog.Bool("write_to_file", o.Logger.WriteToFile),
			slog.Bool("write_to_console", o.Logger.WriteToConsole),
			slog.Bool("use_json", o.Logger.UseJSON),
		),
	)
}
