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
 * @brief Tests for the TLS configuration builder.
 * @file tls_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @date 2026-07-30
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package config

import (
	"crypto/tls"
	"os"
	"path/filepath"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/internal/testcerts"
)

func TestTLSOptions_ServerTLSConfig(t *testing.T) {
	certs := testcerts.Generate(t)

	t.Run("disabled returns nil config and nil error", func(t *testing.T) {
		// even with files set, disabled means no TLS config
		cfg, err := TLSOptions{
			CertFile: certs.ServerCertFile,
			KeyFile:  certs.ServerKeyFile,
		}.ServerTLSConfig()
		if err != nil {
			t.Fatalf("expected no error, got: %v", err)
		}
		if cfg != nil {
			t.Fatalf("expected nil config when disabled, got: %+v", cfg)
		}
	})

	t.Run("valid cert and key", func(t *testing.T) {
		cfg, err := TLSOptions{
			Enabled:  true,
			CertFile: certs.ServerCertFile,
			KeyFile:  certs.ServerKeyFile,
		}.ServerTLSConfig()
		if err != nil {
			t.Fatalf("expected no error, got: %v", err)
		}
		if cfg == nil {
			t.Fatal("expected non-nil config")
		}
		if len(cfg.Certificates) != 1 {
			t.Errorf("expected 1 certificate, got %d", len(cfg.Certificates))
		}
		if cfg.MinVersion != tls.VersionTLS12 {
			t.Errorf("expected MinVersion TLS 1.2, got %x", cfg.MinVersion)
		}
		if cfg.ClientAuth != tls.NoClientCert {
			t.Errorf("expected no client cert requirement, got %v", cfg.ClientAuth)
		}
		if cfg.ClientCAs != nil {
			t.Error("expected nil ClientCAs without mutual auth")
		}
	})

	t.Run("missing cert file path errors", func(t *testing.T) {
		_, err := TLSOptions{
			Enabled: true,
			KeyFile: certs.ServerKeyFile,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for missing cert file, got nil")
		}
	})

	t.Run("missing key file path errors", func(t *testing.T) {
		_, err := TLSOptions{
			Enabled:  true,
			CertFile: certs.ServerCertFile,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for missing key file, got nil")
		}
	})

	t.Run("nonexistent cert file errors", func(t *testing.T) {
		_, err := TLSOptions{
			Enabled:  true,
			CertFile: filepath.Join(t.TempDir(), "does-not-exist.crt"),
			KeyFile:  certs.ServerKeyFile,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for nonexistent cert file, got nil")
		}
	})

	t.Run("invalid PEM cert errors", func(t *testing.T) {
		badCert := filepath.Join(t.TempDir(), "bad.crt")
		if err := os.WriteFile(badCert, []byte("not a certificate"), 0o600); err != nil {
			t.Fatalf("failed to write bad cert: %v", err)
		}
		_, err := TLSOptions{
			Enabled:  true,
			CertFile: badCert,
			KeyFile:  certs.ServerKeyFile,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for invalid PEM cert, got nil")
		}
	})

	t.Run("mutual auth without CA file errors", func(t *testing.T) {
		_, err := TLSOptions{
			Enabled:    true,
			CertFile:   certs.ServerCertFile,
			KeyFile:    certs.ServerKeyFile,
			MutualAuth: true,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for mutual auth without CA file, got nil")
		}
	})

	t.Run("mutual auth with CA file", func(t *testing.T) {
		cfg, err := TLSOptions{
			Enabled:      true,
			CertFile:     certs.ServerCertFile,
			KeyFile:      certs.ServerKeyFile,
			ClientCAFile: certs.CACertFile,
			MutualAuth:   true,
		}.ServerTLSConfig()
		if err != nil {
			t.Fatalf("expected no error, got: %v", err)
		}
		if cfg.ClientAuth != tls.RequireAndVerifyClientCert {
			t.Errorf("expected RequireAndVerifyClientCert, got %v", cfg.ClientAuth)
		}
		if cfg.ClientCAs == nil {
			t.Error("expected ClientCAs to be set for mutual auth")
		}
	})

	t.Run("mutual auth with nonexistent CA file errors", func(t *testing.T) {
		_, err := TLSOptions{
			Enabled:      true,
			CertFile:     certs.ServerCertFile,
			KeyFile:      certs.ServerKeyFile,
			ClientCAFile: filepath.Join(t.TempDir(), "does-not-exist-ca.crt"),
			MutualAuth:   true,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for nonexistent CA file, got nil")
		}
	})

	t.Run("mutual auth with invalid CA PEM errors", func(t *testing.T) {
		badCA := filepath.Join(t.TempDir(), "bad-ca.crt")
		if err := os.WriteFile(badCA, []byte("not a certificate"), 0o600); err != nil {
			t.Fatalf("failed to write bad CA: %v", err)
		}
		_, err := TLSOptions{
			Enabled:      true,
			CertFile:     certs.ServerCertFile,
			KeyFile:      certs.ServerKeyFile,
			ClientCAFile: badCA,
			MutualAuth:   true,
		}.ServerTLSConfig()
		if err == nil {
			t.Fatal("expected error for invalid CA PEM, got nil")
		}
	})

	t.Run("CA file without mutual auth is ignored", func(t *testing.T) {
		cfg, err := TLSOptions{
			Enabled:      true,
			CertFile:     certs.ServerCertFile,
			KeyFile:      certs.ServerKeyFile,
			ClientCAFile: certs.CACertFile,
		}.ServerTLSConfig()
		if err != nil {
			t.Fatalf("expected no error, got: %v", err)
		}
		if cfg.ClientCAs != nil {
			t.Error("expected ClientCAs to be nil when mutual auth is disabled")
		}
		if cfg.ClientAuth != tls.NoClientCert {
			t.Errorf("expected no client cert requirement, got %v", cfg.ClientAuth)
		}
	})
}
