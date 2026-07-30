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
 * @brief TLS configuration builder for transport listeners.
 * @file tls.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @date 2026-07-30
 */

package config

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"log/slog"
	"os"
)

// ServerTLSConfig builds a *tls.Config for a server listener from the options.
//
// It returns (nil, nil) when TLS is not enabled. When enabled, CertFile and
// KeyFile are required, and CAFile is required when MutualAuth is true. Any
// missing or unparseable file results in an error so callers can fail startup
// instead of silently serving plaintext.
func (o TLSOptions) ServerTLSConfig() (*tls.Config, error) {
	if !o.Enabled {
		return nil, nil
	}

	if o.CertFile == "" {
		return nil, fmt.Errorf("TLS is enabled but no certificate file was provided")
	}
	if o.KeyFile == "" {
		return nil, fmt.Errorf("TLS is enabled but no private key file was provided")
	}

	cert, err := tls.LoadX509KeyPair(o.CertFile, o.KeyFile)
	if err != nil {
		return nil, fmt.Errorf("failed to load TLS key pair (cert %q, key %q): %w", o.CertFile, o.KeyFile, err)
	}

	tlsConfig := &tls.Config{
		Certificates: []tls.Certificate{cert},
		MinVersion:   tls.VersionTLS12,
	}

	if o.MutualAuth {
		if o.CAFile == "" {
			return nil, fmt.Errorf("TLS mutual auth is enabled but no client CA file was provided")
		}
		caPEM, err := os.ReadFile(o.CAFile)
		if err != nil {
			return nil, fmt.Errorf("failed to read client CA file %q: %w", o.CAFile, err)
		}
		caPool := x509.NewCertPool()
		if !caPool.AppendCertsFromPEM(caPEM) {
			return nil, fmt.Errorf("client CA file %q contains no valid PEM certificates", o.CAFile)
		}
		tlsConfig.ClientCAs = caPool
		tlsConfig.ClientAuth = tls.RequireAndVerifyClientCert
	} else if o.CAFile != "" {
		slog.Warn("TLS client CA file is set but mutual auth is disabled; the CA file will be ignored",
			"ca_file", o.CAFile)
	}

	return tlsConfig, nil
}
