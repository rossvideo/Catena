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
 * @brief tests for loading configuration for the Catena SDK.
 * @file load_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-29
 */

package config

import (
	"errors"
	"flag"
	"log/slog"
	"reflect"
	"strconv"
	"testing"
	"time"
)

func TestInitOptions(t *testing.T) {
	// just test that it doesn't error and sets the options, the actual parsing is tested in loadParser
	t.Run("success", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{})

		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		// should return default options with the app name set
		defaultOpts := DefaultRuntimeOptions()
		defaultOpts.Logger.AppName = "test_app"
		if !reflect.DeepEqual(opts, defaultOpts) {
			t.Errorf("Expected options to be %v got: %v", defaultOpts, opts)
		}
	})

	// check if help flag causes ErrHelp to be returned
	t.Run("help", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"-h"})

		if err != ErrHelp {
			t.Errorf("Expected help error got: %v", err)
		}
		if !reflect.DeepEqual(opts, RuntimeOptions{}) {
			t.Errorf("Expected options to be empty got: %v", opts)
		}
	})

	// check if invalid flag causes an error and doesn't set options
	t.Run("invalid flag", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"-invalid"})

		if err == nil {
			t.Errorf("Expected error got nil")
		}
		if !reflect.DeepEqual(opts, RuntimeOptions{}) {
			t.Errorf("Expected options to be empty got: %v", opts)
		}
	})

	// check if invalid env var causes an error and doesn't set options
	t.Run("invalid env var", func(t *testing.T) {
		t.Setenv("CATENA_DEV_MODE", "notabool")
		opts, err := InitOptions("test_app", []string{})

		if err == nil {
			t.Errorf("Expected error got nil")
		}
		if !reflect.DeepEqual(opts, RuntimeOptions{}) {
			t.Errorf("Expected options to be empty got: %v", opts)
		}
	})

	// check if CLI overrides env var
	t.Run("cli overrides env var", func(t *testing.T) {
		t.Setenv("CATENA_MAX_CONNECTIONS", "10")
		opts, err := InitOptions("test_app", []string{"--max-connections=20"})

		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 20 {
			t.Errorf("Expected MaxConnections to be 20 got: %v", opts.Server.MaxConnections)
		}
	})

	// check the -v -vv -vvv flags set the log level
	t.Run("verbosity flags set log level", func(t *testing.T) {
		tests := []struct {
			name     string
			args     []string
			expected slog.Level
		}{
			{"default", []string{}, slog.LevelInfo}, // default log level is INFO
			{"warn", []string{"-v"}, slog.LevelWarn},
			{"info", []string{"-vv"}, slog.LevelInfo},
			{"debug", []string{"-vvv"}, slog.LevelDebug},
		}
		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				opts, err := InitOptions("test_app", tt.args)
				if err != nil {
					t.Errorf("Expected no error got: %v", err)
				}
				if opts.Logger.Level != tt.expected {
					t.Errorf("Expected log level to be %v got: %v", tt.expected, opts.Logger.Level)
				}
			})
		}
	})

	// make sure --log-level overrides -v flags
	t.Run("log level priority over verbosity flags", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"-vvv", "--log-level=error"})
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Logger.Level != slog.LevelError {
			t.Errorf("Expected log level to be %v got: %v", slog.LevelError, opts.Logger.Level)
		}
	})

	// check that default true flags can still be set to false with --flag=false
	t.Run("authz can be disabled with explicit false", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"--authz=false"})
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.AuthzEnabled {
			t.Errorf("Expected AuthzEnabled to be false")
		}
	})

	// give the env var a different prefix
	t.Run("WithPrefix", func(t *testing.T) {
		t.Setenv("MYAPP_MAX_CONNECTIONS", "321")
		opts, err := InitOptions("test_app", []string{}, WithPrefix("MYAPP"))
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 321 {
			t.Errorf("Expected MaxConnections to be 321 got: %v", opts.Server.MaxConnections)
		}
	})

	// object passed to WithDefaults is used as the options base
	t.Run("WithDefaults", func(t *testing.T) {
		customDefaults := DefaultRuntimeOptions()
		customDefaults.Server.MaxConnections = 222
		customDefaults.Logger.Level = slog.LevelDebug

		opts, err := InitOptions("test_app", []string{}, WithDefaults(customDefaults))
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 222 {
			t.Errorf("Expected MaxConnections to be 222 got: %v", opts.Server.MaxConnections)
		}
		if opts.Logger.Level != slog.LevelDebug {
			t.Errorf("Expected Logger.Level to be debug got: %v", opts.Logger.Level)
		}
	})

	// custom defaults can be overridden by env and CLI
	t.Run("WithDefaults still respects env and cli precedence", func(t *testing.T) {
		customDefaults := DefaultRuntimeOptions()
		customDefaults.Server.MaxConnections = 111

		t.Setenv("CATENA_MAX_CONNECTIONS", "222")
		opts, err := InitOptions("test_app", []string{"--max-connections=333"}, WithDefaults(customDefaults))
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 333 {
			t.Errorf("Expected MaxConnections to be 333 got: %v", opts.Server.MaxConnections)
		}
	})

	// check if suppressed inputs ignore CLI for suppressed field
	t.Run("WithSuppressedInputs ignores CLI for suppressed field", func(t *testing.T) {
		_, err := InitOptions("test_app", []string{"--max-connections=333"}, WithSuppressedInputs("max-connections"))
		// if the flag wasn't regsistered due to suppression, flags will err with invalid flag
		if err == nil {
			t.Errorf("Expected error for the invalid flag got nil")
		}
	})

	// check if suppressed inputs ignore env for suppressed field
	t.Run("WithSuppressedInputs ignores env for suppressed field", func(t *testing.T) {
		customDefaults := DefaultRuntimeOptions()
		customDefaults.Server.MaxConnections = 111

		t.Setenv("CATENA_MAX_CONNECTIONS", "222")
		opts, err := InitOptions(
			"test_app",
			[]string{},
			WithDefaults(customDefaults),
			WithSuppressedInputs("max-connections"),
		)
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 111 {
			t.Errorf("Expected MaxConnections to stay at default 111 got: %v", opts.Server.MaxConnections)
		}
	})

	// suppressing an unknown input does nothing
	t.Run("WithSuppressedInputs unknown name is no-op", func(t *testing.T) {
		t.Setenv("CATENA_MAX_CONNECTIONS", "222")
		opts, err := InitOptions("test_app", []string{}, WithSuppressedInputs("does-not-exist"))
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 222 {
			t.Errorf("Expected MaxConnections to be 222 got: %v", opts.Server.MaxConnections)
		}
	})
}

// TestOptionMappings locks the env-var/flag -> options-field contract. The
// generic loader tests already prove a loader deposits a parsed value into the
// pointer it was given; this table instead pins WHICH pointer each input is
// wired to. Adding a config field means adding one line here.
func TestOptionMappings(t *testing.T) {
	// mapping pins one env/flag -> field relationship. set is a NON-DEFAULT
	// value encoded as a string: if a loader is wired to the wrong field (or
	// not at all) got(opts) won't match want. The non-default requirement is
	// load-bearing: a bool that defaults true must be set "false" here, or an
	// unwired field would still pass.
	type mapping struct {
		env  string                   // env suffix (the loader adds the prefix)
		flag string                   // cli flag name
		set  string                   // string encoding of a non-default value
		want any                      // expected resolved value
		got  func(RuntimeOptions) any // reads the target field
	}

	mappings := []mapping{
		// runtime
		{"USE_GRPC", "use-grpc", "true", true, func(o RuntimeOptions) any { return o.UseGrpc }},
		{"USE_REST", "use-rest", "true", true, func(o RuntimeOptions) any { return o.UseRest }},
		// REST transport
		{"REST_PORT", "rest-port", "9090", 9090, func(o RuntimeOptions) any { return o.Rest.Port }},
		{"REST_TLS_ENABLED", "rest-tls-enabled", "true", true, func(o RuntimeOptions) any { return o.Rest.TLS.Enabled }},
		{"REST_TLS_CERT_FILE", "rest-tls-cert-file", "/certs/rest.crt", "/certs/rest.crt", func(o RuntimeOptions) any { return o.Rest.TLS.CertFile }},
		{"REST_TLS_KEY_FILE", "rest-tls-key-file", "/certs/rest.key", "/certs/rest.key", func(o RuntimeOptions) any { return o.Rest.TLS.KeyFile }},
		{"REST_TLS_CLIENT_CA_FILE", "rest-tls-client-ca-file", "/certs/rest-ca.crt", "/certs/rest-ca.crt", func(o RuntimeOptions) any { return o.Rest.TLS.ClientCAFile }},
		{"REST_TLS_MUTUAL_AUTH", "rest-tls-mutual-auth", "true", true, func(o RuntimeOptions) any { return o.Rest.TLS.MutualAuth }},
		{"REST_ALLOWED_ORIGINS", "rest-allowed-origins", "https://a.example,https://b.example", []string{"https://a.example", "https://b.example"}, func(o RuntimeOptions) any { return o.Rest.AllowedOrigins }},
		{"REST_EXTRA_ALLOWED_HEADERS", "rest-extra-allowed-headers", "X-A,X-B", []string{"X-A", "X-B"}, func(o RuntimeOptions) any { return o.Rest.ExtraAllowedHeaders }},
		{"REST_EXTRA_ALLOWED_METHODS", "rest-extra-allowed-methods", "PATCH", []string{"PATCH"}, func(o RuntimeOptions) any { return o.Rest.ExtraAllowedMethods }},
		{"REST_CORS_MAX_AGE", "rest-cors-max-age", "5m", 5 * time.Minute, func(o RuntimeOptions) any { return o.Rest.CorsMaxAge }},
		// gRPC transport
		{"GRPC_PORT", "grpc-port", "7000", 7000, func(o RuntimeOptions) any { return o.Grpc.Port }},
		{"GRPC_REFLECTION", "grpc-reflection", "true", true, func(o RuntimeOptions) any { return o.Grpc.Reflection }},
		{"GRPC_TLS_ENABLED", "grpc-tls-enabled", "true", true, func(o RuntimeOptions) any { return o.Grpc.TLS.Enabled }},
		{"GRPC_TLS_CERT_FILE", "grpc-tls-cert-file", "/certs/grpc.crt", "/certs/grpc.crt", func(o RuntimeOptions) any { return o.Grpc.TLS.CertFile }},
		{"GRPC_TLS_KEY_FILE", "grpc-tls-key-file", "/certs/grpc.key", "/certs/grpc.key", func(o RuntimeOptions) any { return o.Grpc.TLS.KeyFile }},
		{"GRPC_TLS_CLIENT_CA_FILE", "grpc-tls-client-ca-file", "/certs/grpc-ca.crt", "/certs/grpc-ca.crt", func(o RuntimeOptions) any { return o.Grpc.TLS.ClientCAFile }},
		{"GRPC_TLS_MUTUAL_AUTH", "grpc-tls-mutual-auth", "true", true, func(o RuntimeOptions) any { return o.Grpc.TLS.MutualAuth }},
		// server
		{"MAX_CONNECTIONS", "max-connections", "250", 250, func(o RuntimeOptions) any { return o.Server.MaxConnections }},
		{"DEV_MODE", "dev", "true", true, func(o RuntimeOptions) any { return o.Server.IsDev }},
		{"AUTHZ", "authz", "false", false, func(o RuntimeOptions) any { return o.Server.AuthzEnabled }}, // defaults true
		// JWT validation
		{"JWT_ISSUER", "jwt-issuer", "https://issuer.example", "https://issuer.example", func(o RuntimeOptions) any { return o.Server.JwtOptions.Issuer }},
		{"JWT_AUDIENCE", "jwt-audience", "catena-aud", "catena-aud", func(o RuntimeOptions) any { return o.Server.JwtOptions.Audience }},
		{"JWT_INSECURE_SKIP_SIGNATURE_VALIDATION", "jwt-insecure-skip-signature-validation", "true", true, func(o RuntimeOptions) any { return o.Server.JwtOptions.InsecureSkipSignatureValidation }},
		{"JWT_STARTUP_RETRY_MAX_ELAPSED", "jwt-startup-retry-max-elapsed", "90s", 90 * time.Second, func(o RuntimeOptions) any { return o.Server.JwtOptions.StartupRetryMaxElapsedTime }},
		// DashBoard connection props
		{"DASHBOARD_SERVICE_HOSTNAME", "dashboard-service-hostname", "example.host", "example.host", func(o RuntimeOptions) any { return o.Dashboard.ServiceHostname }},
		{"DASHBOARD_PORT", "dashboard-port", "8090", 8090, func(o RuntimeOptions) any { return o.Dashboard.Port }},
		{"DASHBOARD_SERVICE_PORT", "dashboard-service-port", "7001", 7001, func(o RuntimeOptions) any { return o.Dashboard.ServicePort }},
		{"DASHBOARD_SERVICE_TLS_ENABLED", "dashboard-service-tls-enabled", "true", true, func(o RuntimeOptions) any { return o.Dashboard.ServiceTLS }},
		{"DASHBOARD_PROTOCOL", "dashboard-protocol", "st2138-rest", ProtocolST2138Rest, func(o RuntimeOptions) any { return o.Dashboard.Protocol }}, // defaults grpc
		{"DASHBOARD_SERVICE_NAME", "dashboard-service-name", "service:other", "service:other", func(o RuntimeOptions) any { return o.Dashboard.ServiceName }},
		{"DASHBOARD_NODE_NAME", "dashboard-node-name", "node-name", "node-name", func(o RuntimeOptions) any { return o.Dashboard.NodeName }},
		{"DASHBOARD_NODE_ID", "dashboard-node-id", "node-id", "node-id", func(o RuntimeOptions) any { return o.Dashboard.NodeID }},
		{"DASHBOARD_ENDPOINT", "dashboard-endpoint", "/other/endpoint", "/other/endpoint", func(o RuntimeOptions) any { return o.Dashboard.Endpoint }},
		// logging
		{"SILENT", "silent", "true", true, func(o RuntimeOptions) any { return o.Logger.Silent }},
		{"LOG_DIR", "log-dir", "/var/log/catena", "/var/log/catena", func(o RuntimeOptions) any { return o.Logger.LogDir }},
		{"LOG_LEVEL", "log-level", "error", slog.LevelError, func(o RuntimeOptions) any { return o.Logger.Level }},
		{"LOG_TO_FILE", "log-to-file", "false", false, func(o RuntimeOptions) any { return o.Logger.WriteToFile }},          // defaults true
		{"LOG_TO_CONSOLE", "log-to-console", "false", false, func(o RuntimeOptions) any { return o.Logger.WriteToConsole }}, // defaults true
		{"LOG_USE_JSON", "log-use-json", "true", true, func(o RuntimeOptions) any { return o.Logger.UseJSON }},
	}

	// env in isolation: each env suffix must land in its own field.
	t.Run("env isolation", func(t *testing.T) {
		for _, m := range mappings {
			t.Run(m.env, func(t *testing.T) {
				t.Setenv("TESTCFG_"+m.env, m.set)
				opts, err := InitOptions("test_app", nil, WithPrefix("TESTCFG"))
				if err != nil {
					t.Fatalf("InitOptions: %v", err)
				}
				if got := m.got(opts); !reflect.DeepEqual(got, m.want) {
					t.Errorf("env TESTCFG_%s: got %v (%T), want %v (%T)", m.env, got, got, m.want, m.want)
				}
			})
		}
	})

	// flag in isolation: each cli flag must land in its own field.
	t.Run("flag isolation", func(t *testing.T) {
		for _, m := range mappings {
			t.Run(m.flag, func(t *testing.T) {
				opts, err := InitOptions("test_app", []string{"--" + m.flag + "=" + m.set}, WithPrefix("TESTCFG"))
				if err != nil {
					t.Fatalf("InitOptions: %v", err)
				}
				if got := m.got(opts); !reflect.DeepEqual(got, m.want) {
					t.Errorf("flag --%s: got %v (%T), want %v (%T)", m.flag, got, got, m.want, m.want)
				}
			})
		}
	})

	// every env set at once: guards against cross-wired or duplicated field
	// pointers that isolation can miss (one loader overwriting another's field).
	t.Run("all together", func(t *testing.T) {
		for _, m := range mappings {
			t.Setenv("TESTCFG_"+m.env, m.set)
		}
		opts, err := InitOptions("test_app", nil, WithPrefix("TESTCFG"))
		if err != nil {
			t.Fatalf("InitOptions: %v", err)
		}
		for _, m := range mappings {
			if got := m.got(opts); !reflect.DeepEqual(got, m.want) {
				t.Errorf("combined TESTCFG_%s: got %v (%T), want %v (%T)", m.env, got, got, m.want, m.want)
			}
		}
	})
}

func makeTestLoader(t *testing.T) *configLoader {
	t.Helper()
	return &configLoader{
		flags: flag.NewFlagSet("TEST", flag.ContinueOnError),
	}
}

func TestLoader_Bool(t *testing.T) {
	// test basic bool load
	t.Run("success", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := true
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if loader.flags.Lookup("test-bool") == nil {
			t.Errorf("Expected flag 'test-bool' to be registered")
		}
		if val != true {
			t.Errorf("Expected val to be true got: %v", val)
		}
	})

	// all the different valid env vars
	t.Run("env var parsing", func(t *testing.T) {
		tests := []struct {
			input    string
			expected bool
		}{
			{"true", true},
			{"TrUe", true},
			{"1", true},
			{"yes", true},
			{"YES", true},
			{"on", true},
			{"false", false},
			{"False", false},
			{"0", false},
			{"no", false},
			{"off", false},
			{"oFF", false},
		}
		for _, tt := range tests {
			loader := makeTestLoader(t)
			t.Run(tt.input, func(t *testing.T) {
				val := false
				t.Setenv("TEST_BOOL", tt.input)
				loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
				if loader.err != nil {
					t.Errorf("Expected no error got: %v", loader.err)
				}
				if val != tt.expected {
					t.Errorf("Expected val to be %v got: %v", tt.expected, val)
				}
			})
		}
	})

	// invalid env var should return an error
	t.Run("env var parsing error", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := true
		t.Setenv("TEST_BOOL", "notabool")
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if !val {
			t.Errorf("Expected val to be unchanged at true got: %v", val)
		}
	})

	// make sure the cascade works, if there's already an error, should only register the flag
	t.Run("skips parsing on existing error", func(t *testing.T) {
		loader := makeTestLoader(t)
		loader.err = strconv.ErrSyntax
		val := true
		t.Setenv("TEST_BOOL", "false")
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if !val {
			t.Errorf("Expected val to be unchanged at true got: %v", val)
		}
	})

	// check if suppressed inputs ignore env for suppressed field
	t.Run("WithSuppressedInputs ignores env for suppressed field", func(t *testing.T) {
		loader := &configLoader{
			flags:            flag.NewFlagSet("TEST", flag.ContinueOnError),
			envPrefix:        "",
			suppressedInputs: []string{"test-bool"},
		}
		val := true
		t.Setenv("TEST_BOOL", "false")
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if loader.flags.Lookup("test-bool") != nil {
			t.Errorf("Expected suppressed flag 'test-bool' to not be registered")
		}
		if !val {
			t.Errorf("Expected val to be unchanged at true for suppressed input got: %v", val)
		}
	})
}

// load an int, error cases are covered in the loadParser tests
func TestLoader_Int(t *testing.T) {
	loader := makeTestLoader(t)
	val := 12
	t.Setenv("TEST_INT", "34")
	loader.extractInt("TEST_INT", "test-int", "Test int flag", &val)
	if loader.err != nil {
		t.Errorf("Expected no error got: %v", loader.err)
	}
	if loader.flags.Lookup("test-int") == nil {
		t.Errorf("Expected flag 'test-int' to be registered")
	}
	if val != 34 {
		t.Errorf("Expected val to be 34 got: %d", val)
	}
}

// load a string, error cases are covered in the loadParser tests
func TestLoader_String(t *testing.T) {
	loader := makeTestLoader(t)
	val := "default"
	t.Setenv("TEST_STRING", "hello")
	loader.extractString("TEST_STRING", "test-string", "Test string flag", &val)
	if loader.err != nil {
		t.Errorf("Expected no error got: %v", loader.err)
	}
	if loader.flags.Lookup("test-string") == nil {
		t.Errorf("Expected flag 'test-string' to be registered")
	}
	if val != "hello" {
		t.Errorf("Expected val to be 'hello' got: %s", val)
	}
}

func TestParseStringSlice(t *testing.T) {
	tests := []struct {
		name string
		in   string
		want []string
	}{
		{"empty", "", nil},
		{"whitespace", "  , , ", nil},
		{"trim and drop empties", "a, b, ,c", []string{"a", "b", "c"}},
		{"star", "*", []string{"*"}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := parseStringSlice(tt.in)
			if err != nil {
				t.Fatalf("parseStringSlice(%q) error: %v", tt.in, err)
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("parseStringSlice(%q)=%v, want %v", tt.in, got, tt.want)
			}
		})
	}
}

func TestLoader_StringSlice(t *testing.T) {
	t.Run("env var parsing", func(t *testing.T) {
		loader := makeTestLoader(t)
		var val []string
		t.Setenv("TEST_SLICE", "a, b, ,c")
		loader.extractStringSlice("TEST_SLICE", "test-slice", "Test slice flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		want := []string{"a", "b", "c"}
		if !reflect.DeepEqual(val, want) {
			t.Errorf("Expected val %v got: %v", want, val)
		}
	})

	t.Run("empty flag is nil", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := []string{"keep"}
		loader.extractStringSlice("TEST_SLICE", "test-slice", "Test slice flag", &val)
		if err := loader.flags.Parse([]string{"--test-slice="}); err != nil {
			t.Fatalf("Parse: %v", err)
		}
		if val != nil {
			t.Errorf("expected empty flag to yield nil, got %v", val)
		}
	})
}

func TestLoader_Duration(t *testing.T) {
	t.Run("env var parsing", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_DURATION", "10m")
		loader.extractDuration("TEST_DURATION", "test-duration", "Test duration flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if val != 10*time.Minute {
			t.Errorf("Expected val 10m got: %v", val)
		}
	})

	t.Run("unitless duration from env", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_DURATION", "50")
		loader.extractDuration("TEST_DURATION", "test-duration", "Test duration flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if val != 50*time.Second {
			t.Errorf("Expected val 50s got: %v", val)
		}
	})

	t.Run("invalid duration", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_DURATION", "notaduration")
		loader.extractDuration("TEST_DURATION", "test-duration", "Test duration flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
	})
}

func TestLoader_PositiveDuration(t *testing.T) {
	t.Run("env var parsing", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_POSITIVE_DURATION", "10m")
		loader.extractPositiveDuration("TEST_POSITIVE_DURATION", "test-positive-duration", "Test positive duration flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if val != 10*time.Minute {
			t.Errorf("Expected val 10m got: %v", val)
		}
	})

	t.Run("unitless duration from env", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_POSITIVE_DURATION", "50")
		loader.extractPositiveDuration("TEST_POSITIVE_DURATION", "test-positive-duration", "Test positive duration flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if val != 50*time.Second {
			t.Errorf("Expected val 50s got: %v", val)
		}
	})

	t.Run("invalid duration", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_POSITIVE_DURATION", "notaduration")
		loader.extractPositiveDuration("TEST_POSITIVE_DURATION", "test-positive-duration", "Test positive duration flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
	})

	t.Run("negative duration", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_POSITIVE_DURATION", "-10m")
		loader.extractPositiveDuration("TEST_POSITIVE_DURATION", "test-positive-duration", "Test positive duration flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
	})

	t.Run("zero duration", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := time.Second
		t.Setenv("TEST_POSITIVE_DURATION", "0")
		loader.extractPositiveDuration("TEST_POSITIVE_DURATION", "test-positive-duration", "Test positive duration flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if val != 0 {
			t.Errorf("Expected val to be 0 got: %v", val)
		}
	})
}

func TestLoader_LogLevel(t *testing.T) {
	// valid log levels should be parsed correctly
	tests := []struct {
		input    string
		expected slog.Level
	}{
		{"debug", slog.LevelDebug},
		{"INFO", slog.LevelInfo},
		{"Warn", slog.LevelWarn},
		{"ERROR", slog.LevelError},
	}
	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			loader := makeTestLoader(t)
			val := slog.LevelInfo
			t.Setenv("TEST_LOG_LEVEL", tt.input)
			loader.extractLogLevel("TEST_LOG_LEVEL", "test-log-level", "Test log level flag", &val)
			if loader.err != nil {
				t.Errorf("Expected no error got: %v", loader.err)
			}
			if val != tt.expected {
				t.Errorf("Expected val to be %v got: %v", tt.expected, val)
			}
		})
	}

	// test an invalid log level
	t.Run("invalid log level", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := slog.LevelInfo
		t.Setenv("TEST_LOG_LEVEL", "notalevel")
		loader.extractLogLevel("TEST_LOG_LEVEL", "test-log-level", "Test log level flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if val != slog.LevelInfo {
			t.Errorf("Expected val to be unchanged at LevelInfo got: %v", val)
		}
	})
}

func TestLoader_ConnectionProtocol(t *testing.T) {
	// valid protocols should be parsed correctly
	tests := []struct {
		input    string
		expected ConnectionProtocol
	}{
		{"st2138-rest", ProtocolST2138Rest},
		{"st2138-grpc", ProtocolST2138Grpc},
		{"catena", ProtocolST2138Catena},
	}
	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			loader := makeTestLoader(t)
			val := ProtocolST2138Rest
			t.Setenv("TEST_PROTOCOL", tt.input)
			loader.extractConnectionProtocol("TEST_PROTOCOL", "test-protocol", "Test protocol flag", &val)
			if loader.err != nil {
				t.Errorf("Expected no error got: %v", loader.err)
			}
			if val != tt.expected {
				t.Errorf("Expected val to be %v got: %v", tt.expected, val)
			}
		})
	}

	// test an invalid protocol
	t.Run("invalid protocol", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := ProtocolST2138Rest
		t.Setenv("TEST_PROTOCOL", "notaprotocol")
		loader.extractConnectionProtocol("TEST_PROTOCOL", "test-protocol", "Test protocol flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if val != ProtocolST2138Rest {
			t.Errorf("Expected val to be unchanged at ProtocolST2138Rest got: %v", val)
		}
	})
}

func TestLoadParser(t *testing.T) {
	// basic success case, should register the flag and set the value to the default
	t.Run("success", func(t *testing.T) {
		loader := &configLoader{
			flags:     flag.NewFlagSet("TEST", flag.ContinueOnError),
			envPrefix: "",
		}
		val := 12
		loadParser(loader, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if loader.err != nil {
			t.Errorf("Expected no err got: %v", loader.err)
		}
		if val != 12 {
			t.Errorf("Expected val to be 12 got: %d", val)
		}
		if loader.flags.Lookup("test") == nil {
			t.Errorf("Expected flag 'test' to be registered")
		}
	})

	// loads the env var
	t.Run("env var parsing", func(t *testing.T) {
		loader := &configLoader{
			flags:     flag.NewFlagSet("TEST", flag.ContinueOnError),
			envPrefix: "",
		}
		val := 12
		t.Setenv("TEST_ENV", "34")
		loadParser(loader, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if loader.err != nil {
			t.Errorf("Expected no err got: %v", loader.err)
		}
		if val != 34 {
			t.Errorf("Expected val to be 34 got: %d", val)
		}
	})

	// there was an error parsing the env var
	// should set loader.err and not change the value
	t.Run("env var parsing error", func(t *testing.T) {
		loader := &configLoader{
			flags:     flag.NewFlagSet("TEST", flag.ContinueOnError),
			envPrefix: "",
		}
		val := 12
		t.Setenv("TEST_ENV", "notanint")
		loadParser(loader, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if val != 12 {
			t.Errorf("Expected val to be unchanged at 12 got: %d", val)
		}
	})

	// skips over env parsing if there's already an error
	t.Run("skips parsing on existing error", func(t *testing.T) {
		loader := &configLoader{
			flags:     flag.NewFlagSet("TEST", flag.ContinueOnError),
			envPrefix: "",
		}
		val := 12
		testErr := errors.New("test error")
		loader.err = testErr
		loadParser(loader, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if loader.err != testErr {
			t.Errorf("Expected error to be %v got: %v", testErr, loader.err)
		}
		if val != 12 {
			t.Errorf("Expected val to be unchanged at 12 got: %d", val)
		}
	})

	// check that suppressed inputs ignore env for suppressed field
	t.Run("suppressed input", func(t *testing.T) {
		loader := &configLoader{
			flags:            flag.NewFlagSet("TEST", flag.ContinueOnError),
			envPrefix:        "",
			suppressedInputs: []string{"test"},
		}
		val := 12
		t.Setenv("TEST_ENV", "34")
		loadParser(loader, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if loader.err != nil {
			t.Errorf("Expected no err got: %v", loader.err)
		}
		if val != 12 {
			t.Errorf("Expected val to stay 12 for suppressed input got: %d", val)
		}
		if loader.flags.Lookup("test") != nil {
			t.Errorf("Expected suppressed flag 'test' to not be registered")
		}
	})
}

// quick coverage tests for the flag.Value interface implementation for parserValue
func TestParserValue(t *testing.T) {
	t.Run("new", func(t *testing.T) {
		val := 12
		parser := newParserValue(&val, strconv.Atoi)
		if parser.val != &val {
			t.Errorf("Expected val pointer to be %v got: %v", &val, parser.val)
		}
		if parser.parser == nil {
			t.Errorf("Expected parser to be set got nil")
		}
	})

	t.Run("String", func(t *testing.T) {
		val := 12
		parser := newParserValue(&val, strconv.Atoi)
		str := parser.String()
		if str != "12" {
			t.Errorf("Expected String() to return '12' got: %s", str)
		}
	})

	t.Run("String nil", func(t *testing.T) {
		parser := newParserValue[any](nil, nil)
		str := parser.String()
		if str != "" {
			t.Errorf("Expected String() to return '' got: %s", str)
		}
	})

	t.Run("Set", func(t *testing.T) {
		val := 12
		called := false
		parser := newParserValue(&val, func(s string) (int, error) {
			called = true
			if s != "input" {
				t.Errorf("Expected parser to be called with 'input' got: %s", s)
			}
			return 34, nil
		})
		err := parser.Set("input")
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if !called {
			t.Errorf("Expected parser to be called")
		}
		if val != 34 {
			t.Errorf("Expected val to be 34 got: %d", val)
		}
	})

	t.Run("Set parser error", func(t *testing.T) {
		val := 12
		parser := newParserValue(&val, func(s string) (int, error) {
			return 0, errors.New("parse error")
		})
		err := parser.Set("34")
		if err == nil {
			t.Errorf("Expected error got nil")
		}
		if val != 12 {
			t.Errorf("Expected val to be unchanged at 12 got: %d", val)
		}
	})
}
