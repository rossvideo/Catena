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
	opts := DefaultRuntimeOptions()

	if !reflect.DeepEqual(opts, RuntimeOptions{
		Rest: RestOptions{Port: 9080},
		Grpc: GrpcOptions{Port: 6254, Reflection: false},
		Server: ServerOptions{
			IsDev:          false,
			MaxConnections: 100,
			AuthzEnabled:   true,
			JwtOptions: JwtValidationOptions{
				AllowedAlgs:       nil,
				Audience:          "",
				Issuer:            "",
				ValidateSignature: false,
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
		Dashboard: DashboardOptions{
			ServiceHostname: "localhost",
			Port:            8080,
			ServicePort:     6254,
			ServiceTLS:      false,
			Protocol:        ProtocolST2138Grpc,
			RefreshInterval: 30000,
			ServiceName:     "service:catena-device",
			Endpoint:        "/connect/connection-props.xml",
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
			AuthzEnabled:   true,
			JwtOptions: JwtValidationOptions{
				AllowedAlgs:       []string{"TESTALG"},
				Audience:          "test-audience",
				Issuer:            "test-issuer",
				Leeway:            12,
				ValidateSignature: true,
			},
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
		Dashboard: DashboardOptions{
			ServiceHostname: "test-host",
			Port:            8080,
			ServicePort:     6254,
			ServiceTLS:      true,
			Protocol:        ProtocolST2138Grpc,
			RefreshInterval: 30000,
			NodeName:        "test-node",
			NodeID:          "test-id",
			ServiceName:     "service:test",
			Endpoint:        "/connect/connection-props.xml",
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
		"use_grpc": false,
		"use_rest": false,
		"rest": map[string]any{
			"port": json.Number("0"),
		},
		"grpc": map[string]any{
			"port":       json.Number("0"),
			"reflection": false,
		},
		"server": map[string]any{
			"dev":             true,
			"max_connections": json.Number("123"),
			"authz_enabled":   true,
			"jwt_validation": map[string]any{
				"allowed_algs":       "TESTALG",
				"audience":           "test-audience",
				"issuer":             "test-issuer",
				"leeway":             json.Number("12"),
				"validate_signature": true,
			},
		},
		"logger": map[string]any{
			"app_name":         "test-app",
			"silent":           false,
			"log_dir":          "/tmp/test-logs",
			"level":            "DEBUG",
			"write_to_file":    true,
			"write_to_console": true,
			"use_json":         true,
		},
		"dashboard": map[string]any{
			"service_hostname": "test-host",
			"port":             json.Number("8080"),
			"service_port":     json.Number("6254"),
			"service_tls":      true,
			"protocol":         "st2138-grpc",
			"refresh_interval": json.Number("30000"),
			"node_name":        "test-node",
			"node_id":          "test-id",
			"service_name":     "service:test",
			"endpoint":         "/connect/connection-props.xml",
		},
	}

	if !reflect.DeepEqual(got, want) {
		t.Fatalf("logged options mismatch\n got: %#v\nwant: %#v", got, want)
	}
}

func TestJwtValidationOptions_ResolvedAllowedAlgs(t *testing.T) {
	t.Run("defaults to ES256 when unset", func(t *testing.T) {
		o := JwtValidationOptions{}

		got := o.ResolvedAllowedAlgs()
		want := []string{"ES256"}
		if !reflect.DeepEqual(got, want) {
			t.Fatalf("ResolvedAllowedAlgs() mismatch\n got: %#v\nwant: %#v", got, want)
		}
	})

	t.Run("returns copy of configured list", func(t *testing.T) {
		o := JwtValidationOptions{AllowedAlgs: []string{"HS256", "RS256"}}

		got := o.ResolvedAllowedAlgs()
		got[0] = "MUTATED"

		if o.AllowedAlgs[0] != "HS256" {
			t.Fatalf("ResolvedAllowedAlgs() should return a copy, got original mutated: %#v", o.AllowedAlgs)
		}
	})
}

func TestJwtValidationOptions_LogValue_DefaultAllowedAlgs(t *testing.T) {
	var buf bytes.Buffer

	log := slog.New(slog.NewJSONHandler(&buf, &slog.HandlerOptions{
		Level: slog.LevelDebug,
	}))

	log.Info("jwt options", "options", JwtValidationOptions{})

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

	if got["allowed_algs"] != "ES256" {
		t.Fatalf("allowed_algs mismatch: got %v, want ES256", got["allowed_algs"])
	}
}

func TestDefaultOptions_TransportFlags(t *testing.T) {
	opts := DefaultRuntimeOptions()
	if opts.Grpc.Reflection != false {
		t.Errorf("expected default Grpc.Reflection=false, got %v", opts.Grpc.Reflection)
	}
	if opts.UseGrpc != false {
		t.Errorf("expected default UseGrpc=false, got %v", opts.UseGrpc)
	}
	if opts.UseRest != false {
		t.Errorf("expected default UseRest=false, got %v", opts.UseRest)
	}
}
