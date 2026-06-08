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
 * @file config_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-29
 */

package config

import (
	"bytes"
	"encoding/json"
	"log/slog"
	"reflect"
	"testing"
)

func TestDefaultOptions(t *testing.T) {
	opts := defaultRuntimeOptions()

	if !reflect.DeepEqual(opts, RuntimeOptions{
		Server: ServerOptions{
			IsDev:          false,
			MaxConnections: 100,
			AuthzEnabled:   true,
			JwtOptions: JwtValidationOptions{
				AllowedAlgs:       nil,
				Audience:          "",
				Issuer:            "",
				ValidateSignature: true,
				Http:              nil,
			},
		},
		Logger: LoggerOptions{
			AppName:        "catena",
			LogDir:         "./logs",
			Silent:         false,
			Level:          slog.LevelInfo,
			WriteToFile:    true,
			WriteToConsole: true,
			UseJSON:        false,
		},
	}) {
		t.Errorf("defaultRuntimeOptions() returned unexpected value: %+v", opts)
	}
}

func TestRuntimeOptions_LogValuer(t *testing.T) {
	opts := RuntimeOptions{
		Server: ServerOptions{
			IsDev:          true,
			MaxConnections: 123,
		},
		Logger: LoggerOptions{
			AppName:        "test-app",
			Silent:         false,
			LogDir:         "/tmp/test-logs",
			Level:          slog.LevelDebug,
			WriteToFile:    true,
			WriteToConsole: true,
			UseJSON:        true,
		},
	}

	var buf bytes.Buffer

	log := slog.New(slog.NewJSONHandler(&buf, &slog.HandlerOptions{
		Level: slog.LevelDebug,
	}))

	log.Info("runtime options", "options", opts)

	var entry map[string]any
	dec := json.NewDecoder(bytes.NewReader(buf.Bytes()))
	dec.UseNumber()

	if err := dec.Decode(&entry); err != nil {
		t.Fatalf("decode slog output: %v\noutput: %s", err, buf.String())
	}

	got, ok := entry["options"].(map[string]any)
	if !ok {
		t.Fatalf("options field missing or wrong type: %#v", entry["options"])
	}

	want := map[string]any{
		"dev":             true,
		"max_connections": json.Number("123"),
		"logger": map[string]any{
			"app_name":         "test-app",
			"silent":           false,
			"log_dir":          "/tmp/test-logs",
			"level":            "DEBUG",
			"write_to_file":    true,
			"write_to_console": true,
			"use_json":         true,
		},
	}

	if !reflect.DeepEqual(got, want) {
		t.Fatalf("logged options mismatch\n got: %#v\nwant: %#v", got, want)
	}
}
