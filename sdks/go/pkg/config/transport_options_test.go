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
 * @brief tests for transport (port/reflection) configuration options.
 * @file transport_options_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-26
 */

package config

import "testing"

func TestDefaultOptions_TransportPorts(t *testing.T) {
	opts := defaultRuntimeOptions()
	if opts.RestPort != 8080 {
		t.Errorf("expected default RestPort 8080, got %d", opts.RestPort)
	}
	if opts.GrpcPort != 6254 {
		t.Errorf("expected default GrpcPort 6254, got %d", opts.GrpcPort)
	}
}

func TestDefaultOptions_TransportFlags(t *testing.T) {
	opts := defaultRuntimeOptions()
	if opts.GrpcReflection != false {
		t.Errorf("expected default GrpcReflection=false, got %v", opts.GrpcReflection)
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
	if opts.RestPort != 9090 {
		t.Errorf("expected RestPort 9090 from env var, got %d", opts.RestPort)
	}
}

func TestInitOptions_GrpcPortVar(t *testing.T) {
	t.Setenv("TESTCFG_GRPC_PORT", "7000")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.GrpcPort != 7000 {
		t.Errorf("expected GrpcPort 7000 from env var, got %d", opts.GrpcPort)
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
			if opts.GrpcReflection != tt.expected {
				t.Errorf("expected GrpcReflection=%v for %q, got %v", tt.expected, tt.value, opts.GrpcReflection)
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
	if opts.RestPort != 8080 {
		t.Errorf("expected default RestPort 8080 when env var unset, got %d", opts.RestPort)
	}
}

func TestInitOptions_GrpcPortDefaultsWhenUnset(t *testing.T) {
	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.GrpcPort != 6254 {
		t.Errorf("expected default GrpcPort 6254 when env var unset, got %d", opts.GrpcPort)
	}
}

func TestInitOptions_TransportPortsIndependent(t *testing.T) {
	t.Setenv("TESTCFG_REST_PORT", "5001")
	t.Setenv("TESTCFG_GRPC_PORT", "5002")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.RestPort != 5001 {
		t.Errorf("expected RestPort=5001, got %d", opts.RestPort)
	}
	if opts.GrpcPort != 5002 {
		t.Errorf("expected GrpcPort=5002, got %d", opts.GrpcPort)
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
	if !opts.GrpcReflection {
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
	if opts.RestPort != 1 {
		t.Errorf("expected RestPort=1 (min valid), got %d", opts.RestPort)
	}
	if opts.GrpcPort != 65535 {
		t.Errorf("expected GrpcPort=65535 (max valid), got %d", opts.GrpcPort)
	}
}

func TestInitOptions_CliOverridesTransportPortEnv(t *testing.T) {
	t.Setenv("TESTCFG_REST_PORT", "9090")

	opts, err := InitOptionsPrefix("test_app", "TESTCFG", []string{"--rest-port=4444"})
	if err != nil {
		t.Fatalf("InitOptionsPrefix() failed: %v", err)
	}
	if opts.RestPort != 4444 {
		t.Errorf("expected CLI flag to override env (RestPort=4444), got %d", opts.RestPort)
	}
}
