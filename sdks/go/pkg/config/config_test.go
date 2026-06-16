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
		Rest: RestOptions{Port: 8080},
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

func TestDefaultRestConfig(t *testing.T) {
	cfg := DefaultRestConfig()
	if cfg.Port != 8080 {
		t.Fatalf("DefaultRestConfig() port = %d, want 8080", cfg.Port)
	}
}

func TestDefaultGrpcConfig(t *testing.T) {
	cfg := DefaultGrpcConfig()
	if cfg.Port != 6254 {
		t.Fatalf("DefaultGrpcConfig() port = %d, want 6254", cfg.Port)
	}
	if cfg.Reflection {
		t.Fatalf("DefaultGrpcConfig() reflection = true, want false")
	}
}

func TestGrpcConfigFromOptions(t *testing.T) {
	tests := []struct {
		name     string
		port     int
		wantErr  bool
		wantPort uint16
	}{
		{name: "valid port", port: 6254, wantPort: 6254},
		{name: "minimum port", port: 0, wantPort: 0},
		{name: "maximum port", port: 65535, wantPort: 65535},
		{name: "negative port", port: -1, wantErr: true},
		{name: "above maximum wraps to valid-looking port", port: 70000, wantErr: true},
		{name: "65536 would wrap to 0", port: 65536, wantErr: true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cfg, err := GrpcConfigFromOptions(RuntimeOptions{Grpc: GrpcOptions{Port: tt.port}})
			if tt.wantErr {
				if err == nil {
					t.Fatalf("GrpcConfigFromOptions(%d) expected error, got nil (port=%d)", tt.port, cfg.Port)
				}
				return
			}
			if err != nil {
				t.Fatalf("GrpcConfigFromOptions(%d) unexpected error: %v", tt.port, err)
			}
			if cfg.Port != tt.wantPort {
				t.Fatalf("GrpcConfigFromOptions(%d) port = %d, want %d", tt.port, cfg.Port, tt.wantPort)
			}
		})
	}
}

func TestRestConfigFromOptions(t *testing.T) {
	tests := []struct {
		name    string
		port    int
		wantErr bool
	}{
		{name: "valid port", port: 8080},
		{name: "minimum port", port: 0},
		{name: "maximum port", port: 65535},
		{name: "negative port", port: -1, wantErr: true},
		{name: "above maximum", port: 70000, wantErr: true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			cfg, err := RestConfigFromOptions(RuntimeOptions{Rest: RestOptions{Port: tt.port}})
			if tt.wantErr {
				if err == nil {
					t.Fatalf("RestConfigFromOptions(%d) expected error, got nil", tt.port)
				}
				return
			}
			if err != nil {
				t.Fatalf("RestConfigFromOptions(%d) unexpected error: %v", tt.port, err)
			}
			if cfg.Port != tt.port {
				t.Fatalf("RestConfigFromOptions(%d) port = %d, want %d", tt.port, cfg.Port, tt.port)
			}
		})
	}
}

func TestDefaultOptions_TransportPorts(t *testing.T) {
	opts := defaultRuntimeOptions()
	if opts.Rest.Port != 8080 {
		t.Errorf("expected default Rest.Port 8080, got %d", opts.Rest.Port)
	}
	if opts.Grpc.Port != 6254 {
		t.Errorf("expected default Grpc.Port 6254, got %d", opts.Grpc.Port)
	}
}

func TestDefaultOptions_TransportFlags(t *testing.T) {
	opts := defaultRuntimeOptions()
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

func TestInitOptions_RestPortVar(t *testing.T) {
	t.Setenv("TESTCFG_REST_PORT", "9090")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Rest.Port != 9090 {
		t.Errorf("expected Rest.Port 9090 from env var, got %d", opts.Rest.Port)
	}
}

func TestInitOptions_GrpcPortVar(t *testing.T) {
	t.Setenv("TESTCFG_GRPC_PORT", "7000")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Grpc.Port != 7000 {
		t.Errorf("expected Grpc.Port 7000 from env var, got %d", opts.Grpc.Port)
	}
}

// GrpcReflection uses the package's lenient bool parsing, so "1" and "yes"
// resolve to true (unlike the legacy strict v == "true" behaviour).
func TestInitOptions_GrpcReflectionVar(t *testing.T) {
	tests := []struct {
		value    string
		expected bool
	}{
		{"true", true},
		{"false", false},
		{"1", true},
		{"yes", true},
	}

	for _, tt := range tests {
		t.Run(tt.value, func(t *testing.T) {
			t.Setenv("TESTCFG_GRPC_REFLECTION", tt.value)

			opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
			if err != nil {
				t.Fatalf("InitOptionsPrefix() failed: %v", err)
			}
			if opts.Grpc.Reflection != tt.expected {
				t.Errorf("expected Grpc.Reflection=%v for %q, got %v", tt.expected, tt.value, opts.Grpc.Reflection)
			}
		})
	}
}

func TestInitOptions_UseGrpcVar(t *testing.T) {
	tests := []struct {
		value    string
		expected bool
	}{
		{"true", true},
		{"false", false},
		{"1", true},
	}

	for _, tt := range tests {
		t.Run(tt.value, func(t *testing.T) {
			t.Setenv("TESTCFG_USE_GRPC", tt.value)

			opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
			if err != nil {
				t.Fatalf("InitOptionsPrefix() failed: %v", err)
			}
			if opts.UseGrpc != tt.expected {
				t.Errorf("expected UseGrpc=%v for %q, got %v", tt.expected, tt.value, opts.UseGrpc)
			}
		})
	}
}

func TestInitOptions_UseRestVar(t *testing.T) {
	tests := []struct {
		value    string
		expected bool
	}{
		{"true", true},
		{"false", false},
	}

	for _, tt := range tests {
		t.Run(tt.value, func(t *testing.T) {
			t.Setenv("TESTCFG_USE_REST", tt.value)

			opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
			if err != nil {
				t.Fatalf("InitOptionsPrefix() failed: %v", err)
			}
			if opts.UseRest != tt.expected {
				t.Errorf("expected UseRest=%v for %q, got %v", tt.expected, tt.value, opts.UseRest)
			}
		})
	}
}

func TestInitOptions_RestPortDefaultsWhenUnset(t *testing.T) {
	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Rest.Port != 8080 {
		t.Errorf("expected default Rest.Port 8080 when env var unset, got %d", opts.Rest.Port)
	}
}

func TestInitOptions_GrpcPortDefaultsWhenUnset(t *testing.T) {
	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Grpc.Port != 6254 {
		t.Errorf("expected default Grpc.Port 6254 when env var unset, got %d", opts.Grpc.Port)
	}
}

func TestInitOptions_TransportPortsIndependent(t *testing.T) {
	t.Setenv("TESTCFG_REST_PORT", "5001")
	t.Setenv("TESTCFG_GRPC_PORT", "5002")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Rest.Port != 5001 {
		t.Errorf("expected Rest.Port=5001, got %d", opts.Rest.Port)
	}
	if opts.Grpc.Port != 5002 {
		t.Errorf("expected Grpc.Port=5002, got %d", opts.Grpc.Port)
	}
}

func TestInitOptions_BooleanFieldsCombined(t *testing.T) {
	t.Setenv("TESTCFG_USE_GRPC", "true")
	t.Setenv("TESTCFG_USE_REST", "true")
	t.Setenv("TESTCFG_GRPC_REFLECTION", "true")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if !opts.UseGrpc {
		t.Error("expected UseGrpc=true")
	}
	if !opts.UseRest {
		t.Error("expected UseRest=true")
	}
	if !opts.Grpc.Reflection {
		t.Error("expected GrpcReflection=true")
	}
}

func TestInitOptions_PortBoundaryValues(t *testing.T) {
	t.Setenv("TESTCFG_REST_PORT", "1")
	t.Setenv("TESTCFG_GRPC_PORT", "65535")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Rest.Port != 1 {
		t.Errorf("expected Rest.Port=1 (min valid), got %d", opts.Rest.Port)
	}
	if opts.Grpc.Port != 65535 {
		t.Errorf("expected Grpc.Port=65535 (max valid), got %d", opts.Grpc.Port)
	}
}

func TestInitOptions_CliOverridesTransportPortEnv(t *testing.T) {
	t.Setenv("TESTCFG_REST_PORT", "9090")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{"--rest-port=4444"})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.Rest.Port != 4444 {
		t.Errorf("expected CLI flag to override env (Rest.Port=4444), got %d", opts.Rest.Port)
	}
}
