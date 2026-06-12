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
 * @brief Tests for deriving transport configs from config.RuntimeOptions.
 * @file config_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-06-12
 */

package transports

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

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
			cfg, err := GrpcConfigFromOptions(config.RuntimeOptions{GrpcPort: tt.port})
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
			cfg, err := RestConfigFromOptions(config.RuntimeOptions{RestPort: tt.port})
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
