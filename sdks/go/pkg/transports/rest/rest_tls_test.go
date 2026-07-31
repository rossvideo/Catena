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
 * @brief Tests for TLS on the REST transport.
 * @file rest_tls_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @date 2026-07-30
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package rest

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/internal/testcerts"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/internal/transporttest"
)

// reserveTestPort finds a free TCP port for a transport to listen on.
func reserveTestPort(t *testing.T) int {
	t.Helper()
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("net.Listen: %v", err)
	}
	port := listener.Addr().(*net.TCPAddr).Port
	listener.Close()
	return port
}

// startTLSTransport starts a REST transport with the given TLS options on a
// free port and registers shutdown cleanup. Returns the port it listens on.
func startTLSTransport(t *testing.T, tlsOpts config.TLSOptions) int {
	t.Helper()
	port := reserveTestPort(t)
	transport := NewTransport(config.RestOptions{Port: port, TLS: tlsOpts})
	runtime := transporttest.MakeStubServerRuntime(t)
	runtime.ShutdownTransportConnsFn = func(ctx context.Context, gotTransport catena.Transport) {}

	if err := transport.Start(context.Background(), runtime); err != nil {
		t.Fatalf("Start: %v", err)
	}
	t.Cleanup(func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		transport.Shutdown(ctx)
	})
	// give the serve goroutine a moment to begin accepting
	time.Sleep(100 * time.Millisecond)
	return port
}

// caPool loads the test CA into a cert pool for client-side verification.
func caPool(t *testing.T, caFile string) *x509.CertPool {
	t.Helper()
	caPEM, err := os.ReadFile(caFile)
	if err != nil {
		t.Fatalf("read CA file: %v", err)
	}
	pool := x509.NewCertPool()
	if !pool.AppendCertsFromPEM(caPEM) {
		t.Fatal("failed to parse test CA")
	}
	return pool
}

func TestTransport_Start_TLS(t *testing.T) {
	certs := testcerts.Generate(t)
	port := startTLSTransport(t, config.TLSOptions{
		Enabled:  true,
		CertFile: certs.ServerCertFile,
		KeyFile:  certs.ServerKeyFile,
	})

	client := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{RootCAs: caPool(t, certs.CACertFile)},
		},
		Timeout: 2 * time.Second,
	}

	// any unknown path is fine; a well-formed HTTP response proves the TLS
	// handshake and HTTP serving both work
	resp, err := client.Get(fmt.Sprintf("https://127.0.0.1:%d/st2138-api/v1", port))
	if err != nil {
		t.Fatalf("HTTPS GET: %v", err)
	}
	resp.Body.Close()
	if resp.StatusCode != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, resp.StatusCode)
	}
	if resp.TLS == nil {
		t.Error("expected response to have TLS connection state")
	}

	// a plain HTTP request against the TLS listener must be rejected. Go's
	// http.Server answers such requests with a plaintext "400 Bad Request"
	// ("client sent an HTTP request to an HTTPS server") rather than
	// dropping the connection, so accept either an error or that 400.
	plainClient := &http.Client{Timeout: 2 * time.Second}
	plainResp, err := plainClient.Get(fmt.Sprintf("http://127.0.0.1:%d/st2138-api/v1", port))
	if err == nil {
		defer plainResp.Body.Close()
		if plainResp.StatusCode != http.StatusBadRequest {
			t.Errorf("expected plain HTTP request to be rejected with 400, got status %d", plainResp.StatusCode)
		}
	}
}

func TestTransport_Start_TLS_MutualAuth(t *testing.T) {
	certs := testcerts.Generate(t)
	port := startTLSTransport(t, config.TLSOptions{
		Enabled:    true,
		CertFile:   certs.ServerCertFile,
		KeyFile:    certs.ServerKeyFile,
		CAFile:     certs.CACertFile,
		MutualAuth: true,
	})
	url := fmt.Sprintf("https://127.0.0.1:%d/st2138-api/v1", port)

	t.Run("client with certificate succeeds", func(t *testing.T) {
		clientCert, err := tls.LoadX509KeyPair(certs.ClientCertFile, certs.ClientKeyFile)
		if err != nil {
			t.Fatalf("load client cert: %v", err)
		}
		client := &http.Client{
			Transport: &http.Transport{
				TLSClientConfig: &tls.Config{
					RootCAs:      caPool(t, certs.CACertFile),
					Certificates: []tls.Certificate{clientCert},
				},
			},
			Timeout: 2 * time.Second,
		}
		resp, err := client.Get(url)
		if err != nil {
			t.Fatalf("HTTPS GET with client cert: %v", err)
		}
		resp.Body.Close()
		if resp.StatusCode != http.StatusNotFound {
			t.Errorf("expected status %d, got %d", http.StatusNotFound, resp.StatusCode)
		}
	})

	t.Run("client without certificate is rejected", func(t *testing.T) {
		client := &http.Client{
			Transport: &http.Transport{
				TLSClientConfig: &tls.Config{RootCAs: caPool(t, certs.CACertFile)},
			},
			Timeout: 2 * time.Second,
		}
		resp, err := client.Get(url)
		if err == nil {
			resp.Body.Close()
			t.Errorf("expected request without client cert to fail, got status %d", resp.StatusCode)
		}
	})
}

func TestTransport_Start_TLS_BadConfig(t *testing.T) {
	tests := []struct {
		name string
		tls  config.TLSOptions
	}{
		{
			name: "missing cert and key paths",
			tls:  config.TLSOptions{Enabled: true},
		},
		{
			name: "nonexistent cert file",
			tls: config.TLSOptions{
				Enabled:  true,
				CertFile: filepath.Join(t.TempDir(), "missing.crt"),
				KeyFile:  filepath.Join(t.TempDir(), "missing.key"),
			},
		},
		{
			name: "mutual auth without CA",
			tls: func() config.TLSOptions {
				certs := testcerts.Generate(t)
				return config.TLSOptions{
					Enabled:    true,
					CertFile:   certs.ServerCertFile,
					KeyFile:    certs.ServerKeyFile,
					MutualAuth: true,
				}
			}(),
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			transport := NewTransport(config.RestOptions{Port: reserveTestPort(t), TLS: tt.tls})
			runtime := transporttest.MakeStubServerRuntime(t)
			err := transport.Start(context.Background(), runtime)
			if err == nil {
				ctx, cancel := context.WithTimeout(context.Background(), time.Second)
				defer cancel()
				transport.Shutdown(ctx)
				t.Fatal("expected Start to return a TLS configuration error, got nil")
			}
		})
	}
}
