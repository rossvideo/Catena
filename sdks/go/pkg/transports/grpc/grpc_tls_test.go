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
 * @brief Tests for TLS on the gRPC transport.
 * @file grpc_tls_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @date 2026-07-30
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package grpc

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"net"
	"os"
	"path/filepath"
	"testing"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/credentials/insecure"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/internal/testcerts"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/protos/rpc"
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

// startTLSTransport starts a gRPC transport with the given TLS options on a
// free TCP port and registers shutdown cleanup. Returns the server address.
func startTLSTransport(t *testing.T, tlsOpts config.TLSOptions) string {
	t.Helper()
	port := reserveTestPort(t)
	transport := NewTransport(config.GrpcOptions{Port: port, TLS: tlsOpts})
	runtime := transporttest.MakeStubServerRuntime(t)
	runtime.Slots = []uint16{0}
	runtime.ShutdownTransportConnsFn = func(ctx context.Context, gotTransport catena.Transport) {}

	if err := transport.Start(context.Background(), runtime); err != nil {
		t.Fatalf("Start: %v", err)
	}
	t.Cleanup(func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		transport.Shutdown(ctx)
	})
	return fmt.Sprintf("127.0.0.1:%d", port)
}

// clientTLSConfig builds client TLS credentials trusting the test CA, with an
// optional client certificate for mTLS.
func clientTLSConfig(t *testing.T, certs testcerts.Bundle, withClientCert bool) *tls.Config {
	t.Helper()
	caPEM, err := os.ReadFile(certs.CACertFile)
	if err != nil {
		t.Fatalf("read CA file: %v", err)
	}
	pool := x509.NewCertPool()
	if !pool.AppendCertsFromPEM(caPEM) {
		t.Fatal("failed to parse test CA")
	}
	cfg := &tls.Config{RootCAs: pool}
	if withClientCert {
		clientCert, err := tls.LoadX509KeyPair(certs.ClientCertFile, certs.ClientKeyFile)
		if err != nil {
			t.Fatalf("load client cert: %v", err)
		}
		cfg.Certificates = []tls.Certificate{clientCert}
	}
	return cfg
}

// callGetPopulatedSlots dials the address with the given credentials and
// performs a unary RPC, returning the RPC error (nil on success).
func callGetPopulatedSlots(t *testing.T, addr string, creds credentials.TransportCredentials) error {
	t.Helper()
	conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(creds))
	if err != nil {
		t.Fatalf("grpc.NewClient: %v", err)
	}
	defer conn.Close()

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	client := rpc.NewCatenaServiceClient(conn)
	_, err = client.GetPopulatedSlots(ctx, &protos.Empty{})
	return err
}

func TestTransport_Start_TLS(t *testing.T) {
	certs := testcerts.Generate(t)
	addr := startTLSTransport(t, config.TLSOptions{
		Enabled:  true,
		CertFile: certs.ServerCertFile,
		KeyFile:  certs.ServerKeyFile,
	})

	t.Run("tls client succeeds", func(t *testing.T) {
		creds := credentials.NewTLS(clientTLSConfig(t, certs, false))
		if err := callGetPopulatedSlots(t, addr, creds); err != nil {
			t.Fatalf("expected TLS RPC to succeed, got: %v", err)
		}
	})

	t.Run("insecure client fails", func(t *testing.T) {
		if err := callGetPopulatedSlots(t, addr, insecure.NewCredentials()); err == nil {
			t.Fatal("expected plaintext RPC against TLS server to fail, got nil")
		}
	})
}

func TestTransport_Start_TLS_MutualAuth(t *testing.T) {
	certs := testcerts.Generate(t)
	addr := startTLSTransport(t, config.TLSOptions{
		Enabled:    true,
		CertFile:   certs.ServerCertFile,
		KeyFile:    certs.ServerKeyFile,
		CAFile:     certs.CACertFile,
		MutualAuth: true,
	})

	t.Run("client with certificate succeeds", func(t *testing.T) {
		creds := credentials.NewTLS(clientTLSConfig(t, certs, true))
		if err := callGetPopulatedSlots(t, addr, creds); err != nil {
			t.Fatalf("expected mTLS RPC to succeed, got: %v", err)
		}
	})

	t.Run("client without certificate is rejected", func(t *testing.T) {
		creds := credentials.NewTLS(clientTLSConfig(t, certs, false))
		if err := callGetPopulatedSlots(t, addr, creds); err == nil {
			t.Fatal("expected RPC without client cert to fail, got nil")
		}
	})
}

func TestTransport_New_TLS_BadConfig(t *testing.T) {
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
			transport := NewTransport(config.GrpcOptions{Port: reserveTestPort(t), TLS: tt.tls})
			if transport.initErr == nil {
				t.Error("expected initErr to be set for bad TLS config")
			}

			runtime := transporttest.MakeStubServerRuntime(t)
			err := transport.Start(context.Background(), runtime)
			if err == nil {
				t.Fatal("expected Start to return the TLS configuration error, got nil")
			}

			// Shutdown on a transport that failed construction must be a no-op
			ctx, cancel := context.WithTimeout(context.Background(), time.Second)
			defer cancel()
			if err := transport.Shutdown(ctx); err != nil {
				t.Errorf("expected Shutdown to be a no-op, got: %v", err)
			}
		})
	}
}
