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
 * @brief Tests for the catena-healthcheck utility.
 * @file main_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-10-26
 */

package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/pem"
	"net"
	"net/http"
	"net/http/httptest"
	"net/url"
	"os"
	"path/filepath"
	"reflect"
	"strconv"
	"sync"
	"testing"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/health"
	healthpb "google.golang.org/grpc/health/grpc_health_v1"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/transports"
)

const (
	testPrefix = "HCTEST"
	localhost  = "127.0.0.1"
)

// probe runs catena-healthcheck against localhost with an isolated env prefix so
// the developer's CATENA_* / HEALTHCHECK_* environment can't leak into tests.
func probe(t *testing.T, args ...string) int {
	t.Helper()
	for _, name := range []string{
		"HEALTHCHECK_PREFIX", "HEALTHCHECK_HOST", "HEALTHCHECK_TIMEOUT",
		"HEALTHCHECK_VERIFY_TLS", "HEALTHCHECK_TLS_CA_FILE",
	} {
		t.Setenv(name, "")
	}
	base := []string{"--prefix", testPrefix, "--host", localhost, "--timeout", "2s"}
	return run(append(base, args...))
}

func portOf(t *testing.T, rawURL string) string {
	t.Helper()
	u, err := url.Parse(rawURL)
	if err != nil {
		t.Fatalf("parse %q: %v", rawURL, err)
	}
	return u.Port()
}

// refusedPort is a port nothing can be listening on: connecting to port 0
// fails immediately, so "nothing listening" cases need no free-port lookup
// (and so have no window for another process to grab the port).
const refusedPort = "0"

// startRealServer runs a real catena server with the SDK's REST and gRPC
// transports on free ports and returns the ports plus a stop func.
//
// The transports bind their own listeners and don't expose the bound address,
// so ports can't be left to the OS (port 0) here. Instead each port is picked
// and then bound by the transport; if another process wins that race the bind
// fails synchronously in RegisterFrom and the whole attempt is retried.
func startRealServer(t *testing.T) (restPort, grpcPort string, stop func()) {
	t.Helper()
	pick := func() int {
		l, err := net.Listen("tcp", localhost+":0")
		if err != nil {
			t.Fatalf("listen: %v", err)
		}
		defer l.Close()
		return l.Addr().(*net.TCPAddr).Port
	}
	shutdown := func(srv catena.Server) {
		ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
		defer cancel()
		srv.Shutdown(ctx)
	}

	var lastErr error
	for range 5 {
		opts := config.DefaultRuntimeOptions()
		opts.UseRest, opts.UseGrpc = true, true
		opts.Rest.Port, opts.Grpc.Port = pick(), pick()
		opts.Server.AuthzEnabled = false

		srv, err := catena.NewServer(opts.Server)
		if err != nil {
			t.Fatalf("NewServer: %v", err)
		}
		if err := transports.RegisterFrom(srv, opts); err != nil {
			lastErr = err
			shutdown(srv)
			continue
		}

		var once sync.Once
		stop = func() { once.Do(func() { shutdown(srv) }) }
		t.Cleanup(stop)
		return strconv.Itoa(opts.Rest.Port), strconv.Itoa(opts.Grpc.Port), stop
	}
	t.Fatalf("could not bind transports after retries: %v", lastErr)
	return
}

// restServer starts a server that answers the REST health path with the given code.
func restServer(t *testing.T, code int) *httptest.Server {
	t.Helper()
	mux := http.NewServeMux()
	mux.HandleFunc(restHealthPath, func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(code)
	})
	srv := httptest.NewServer(mux)
	t.Cleanup(srv.Close)
	return srv
}

// grpcServer starts a gRPC server and returns its port. A nil healthSrv leaves
// the health service unregistered, like a transport with health disabled.
func grpcServer(t *testing.T, healthSrv *health.Server, creds credentials.TransportCredentials) string {
	t.Helper()
	l, err := net.Listen("tcp", localhost+":0")
	if err != nil {
		t.Fatalf("listen: %v", err)
	}
	var opts []grpc.ServerOption
	if creds != nil {
		opts = append(opts, grpc.Creds(creds))
	}
	srv := grpc.NewServer(opts...)
	if healthSrv != nil {
		healthpb.RegisterHealthServer(srv, healthSrv)
	}
	go srv.Serve(l)
	t.Cleanup(srv.Stop)
	return strconv.Itoa(l.Addr().(*net.TCPAddr).Port)
}

// writeCAFile writes the test server's self-signed certificate as a PEM CA bundle.
func writeCAFile(t *testing.T, srv *httptest.Server) string {
	t.Helper()
	path := filepath.Join(t.TempDir(), "ca.pem")
	pemBytes := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: srv.Certificate().Raw})
	if err := os.WriteFile(path, pemBytes, 0o600); err != nil {
		t.Fatalf("write CA: %v", err)
	}
	return path
}

func TestSplitArgs(t *testing.T) {
	tests := []struct {
		name    string
		args    []string
		wantOwn []string
		wantSDK []string
	}{
		{"empty", nil, nil, nil},
		{"sdk only", []string{"--use-rest", "--rest-port", "9080"}, nil, []string{"--use-rest", "--rest-port", "9080"}},
		{"own valued separate", []string{"--host", "h", "--use-grpc"}, []string{"--host", "h"}, []string{"--use-grpc"}},
		{"own valued equals", []string{"--timeout=5s"}, []string{"--timeout=5s"}, nil},
		{"own bool", []string{"--verify-tls", "--use-rest"}, []string{"--verify-tls"}, []string{"--use-rest"}},
		{"single dash", []string{"-prefix", "X", "-use-rest"}, []string{"-prefix", "X"}, []string{"-use-rest"}},
		{"trailing valued flag without value goes to sdk", []string{"--host"}, nil, []string{"--host"}},
		{"mixed", []string{"--rest-port=1", "--tls-ca-file", "ca.pem", "--grpc-port", "2"}, []string{"--tls-ca-file", "ca.pem"}, []string{"--rest-port=1", "--grpc-port", "2"}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			own, sdk := splitArgs(tt.args)
			if !reflect.DeepEqual(own, tt.wantOwn) {
				t.Errorf("own = %v, want %v", own, tt.wantOwn)
			}
			if !reflect.DeepEqual(sdk, tt.wantSDK) {
				t.Errorf("sdk = %v, want %v", sdk, tt.wantSDK)
			}
		})
	}
}

func TestRun_Usage(t *testing.T) {
	t.Run("no transport enabled", func(t *testing.T) {
		if got := probe(t); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("unknown flag", func(t *testing.T) {
		if got := probe(t, "--no-such-flag"); got != 2 {
			t.Errorf("exit = %d, want 2", got)
		}
	})
	t.Run("sdk flag outside the allowlist", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--dev"); got != 2 {
			t.Errorf("exit = %d, want 2", got)
		}
	})
	t.Run("invalid own flag value", func(t *testing.T) {
		if got := run([]string{"--timeout=notaduration"}); got != 2 {
			t.Errorf("exit = %d, want 2", got)
		}
	})
	t.Run("help", func(t *testing.T) {
		if got := probe(t, "--help"); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
}

func TestRun_REST(t *testing.T) {
	t.Run("healthy", func(t *testing.T) {
		srv := restServer(t, http.StatusOK)
		if got := probe(t, "--use-rest", "--rest-port", portOf(t, srv.URL)); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("any 2xx is healthy", func(t *testing.T) {
		srv := restServer(t, http.StatusNoContent)
		if got := probe(t, "--use-rest", "--rest-port", portOf(t, srv.URL)); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("non-2xx is unhealthy", func(t *testing.T) {
		srv := restServer(t, http.StatusServiceUnavailable)
		if got := probe(t, "--use-rest", "--rest-port", portOf(t, srv.URL)); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("nothing listening", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--rest-port", refusedPort); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("times out on a hung server", func(t *testing.T) {
		release := make(chan struct{})
		srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			select {
			case <-release:
			case <-r.Context().Done():
			}
		}))
		t.Cleanup(srv.Close)
		t.Cleanup(func() { close(release) })

		start := time.Now()
		got := probe(t, "--use-rest", "--rest-port", portOf(t, srv.URL), "--timeout", "200ms")
		if got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
		if elapsed := time.Since(start); elapsed > 2*time.Second {
			t.Errorf("probe took %v, expected it to honor the timeout", elapsed)
		}
	})
}

func TestRun_GRPC(t *testing.T) {
	t.Run("serving", func(t *testing.T) {
		port := grpcServer(t, health.NewServer(), nil)
		if got := probe(t, "--use-grpc", "--grpc-port", port); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("not serving", func(t *testing.T) {
		hs := health.NewServer()
		hs.Shutdown()
		port := grpcServer(t, hs, nil)
		if got := probe(t, "--use-grpc", "--grpc-port", port); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("health service disabled counts as up", func(t *testing.T) {
		port := grpcServer(t, nil, nil)
		if got := probe(t, "--use-grpc", "--grpc-port", port); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("nothing listening", func(t *testing.T) {
		if got := probe(t, "--use-grpc", "--grpc-port", refusedPort); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("plaintext probe against a TLS server", func(t *testing.T) {
		ts := httptest.NewTLSServer(http.NotFoundHandler())
		t.Cleanup(ts.Close)
		port := grpcServer(t, health.NewServer(), credentials.NewTLS(&tls.Config{Certificates: ts.TLS.Certificates}))
		if got := probe(t, "--use-grpc", "--grpc-port", port); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
}

func TestRun_AllTransportsMustBeHealthy(t *testing.T) {
	restOK := restServer(t, http.StatusOK)
	restBad := restServer(t, http.StatusInternalServerError)
	grpcOK := grpcServer(t, health.NewServer(), nil)
	badHealth := health.NewServer()
	badHealth.Shutdown()
	grpcBad := grpcServer(t, badHealth, nil)

	tests := []struct {
		name     string
		restPort string
		grpcPort string
		want     int
	}{
		{"both healthy", portOf(t, restOK.URL), grpcOK, 0},
		{"rest unhealthy", portOf(t, restBad.URL), grpcOK, 1},
		{"grpc unhealthy", portOf(t, restOK.URL), grpcBad, 1},
		{"both unhealthy", portOf(t, restBad.URL), grpcBad, 1},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := probe(t, "--use-rest", "--use-grpc", "--rest-port", tt.restPort, "--grpc-port", tt.grpcPort)
			if got != tt.want {
				t.Errorf("exit = %d, want %d", got, tt.want)
			}
		})
	}
}

func TestRun_EnvConfig(t *testing.T) {
	srv := restServer(t, http.StatusOK)
	t.Setenv(testPrefix+"_USE_REST", "true")
	t.Setenv(testPrefix+"_REST_PORT", portOf(t, srv.URL))
	if got := probe(t); got != 0 {
		t.Errorf("exit = %d, want 0", got)
	}

	t.Run("flag overrides env", func(t *testing.T) {
		if got := probe(t, "--rest-port", refusedPort); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("env prefix is respected", func(t *testing.T) {
		t.Setenv("OTHER_USE_REST", "true")
		t.Setenv("OTHER_REST_PORT", portOf(t, srv.URL))
		if got := run([]string{"--prefix", "OTHER", "--host", localhost}); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
}

func TestRun_HostAndTimeoutFromEnv(t *testing.T) {
	srv := restServer(t, http.StatusOK)
	t.Setenv("HEALTHCHECK_PREFIX", testPrefix)
	t.Setenv("HEALTHCHECK_HOST", localhost)
	t.Setenv("HEALTHCHECK_TIMEOUT", "2s")
	t.Setenv(testPrefix+"_USE_REST", "true")
	t.Setenv(testPrefix+"_REST_PORT", portOf(t, srv.URL))
	if got := run(nil); got != 0 {
		t.Errorf("exit = %d, want 0", got)
	}
}

// TestRun_RealServer probes a real catena server with the SDK's real REST and
// gRPC transports (built the way the examples build theirs), so the probe is
// checked against what the SDK actually serves rather than only against fakes.
func TestRun_RealServer(t *testing.T) {
	restPort, grpcPort, stop := startRealServer(t)

	args := []string{"--use-rest", "--use-grpc", "--rest-port", restPort, "--grpc-port", grpcPort}
	// Start has already bound the listeners; this just tolerates a slow Serve goroutine.
	deadline := time.Now().Add(5 * time.Second)
	for probe(t, args...) != 0 {
		if time.Now().After(deadline) {
			t.Fatal("real server never became healthy")
		}
		time.Sleep(50 * time.Millisecond)
	}

	t.Run("each transport on its own", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--rest-port", restPort); got != 0 {
			t.Errorf("rest exit = %d, want 0", got)
		}
		if got := probe(t, "--use-grpc", "--grpc-port", grpcPort); got != 0 {
			t.Errorf("grpc exit = %d, want 0", got)
		}
	})

	stop()
	t.Run("unhealthy after shutdown", func(t *testing.T) {
		if got := probe(t, args...); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
}

func TestRun_TLS(t *testing.T) {
	ts := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))
	t.Cleanup(ts.Close)
	caFile := writeCAFile(t, ts)
	restPort := portOf(t, ts.URL)
	grpcPort := grpcServer(t, health.NewServer(), credentials.NewTLS(&tls.Config{Certificates: ts.TLS.Certificates}))

	t.Run("rest skips verification by default", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", restPort); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("rest verifies against the given CA", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", restPort, "--tls-ca-file", caFile); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("rest verification fails for an untrusted cert", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", restPort, "--verify-tls"); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("rest TLS probe against plaintext server", func(t *testing.T) {
		plain := restServer(t, http.StatusOK)
		if got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", portOf(t, plain.URL)); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("grpc skips verification by default", func(t *testing.T) {
		if got := probe(t, "--use-grpc", "--grpc-tls-enabled", "--grpc-port", grpcPort); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("grpc verifies against the given CA", func(t *testing.T) {
		if got := probe(t, "--use-grpc", "--grpc-tls-enabled", "--grpc-port", grpcPort, "--tls-ca-file", caFile); got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("grpc verification fails for an untrusted cert", func(t *testing.T) {
		if got := probe(t, "--use-grpc", "--grpc-tls-enabled", "--grpc-port", grpcPort, "--verify-tls"); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("missing CA file", func(t *testing.T) {
		if got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", restPort, "--tls-ca-file", filepath.Join(t.TempDir(), "nope.pem")); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("CA file without certificates", func(t *testing.T) {
		bad := filepath.Join(t.TempDir(), "bad.pem")
		if err := os.WriteFile(bad, []byte("not a cert"), 0o600); err != nil {
			t.Fatal(err)
		}
		if got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", restPort, "--tls-ca-file", bad); got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
}

func TestRun_MutualTLS(t *testing.T) {
	// The probe presents the transport's own cert/key as its client
	// certificate, so a server requiring client certs accepts it.
	ts := httptest.NewUnstartedServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))
	ts.TLS = &tls.Config{ClientAuth: tls.RequireAnyClientCert}
	ts.StartTLS()
	t.Cleanup(ts.Close)

	dir := t.TempDir()
	certFile := filepath.Join(dir, "cert.pem")
	keyFile := filepath.Join(dir, "key.pem")
	if err := os.WriteFile(certFile, pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: ts.Certificate().Raw}), 0o600); err != nil {
		t.Fatal(err)
	}
	keyDER, err := x509.MarshalPKCS8PrivateKey(ts.TLS.Certificates[0].PrivateKey)
	if err != nil {
		t.Fatalf("marshal key: %v", err)
	}
	if err := os.WriteFile(keyFile, pem.EncodeToMemory(&pem.Block{Type: "PRIVATE KEY", Bytes: keyDER}), 0o600); err != nil {
		t.Fatal(err)
	}
	port := portOf(t, ts.URL)

	t.Run("client certificate is presented", func(t *testing.T) {
		got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-tls-mutual-auth",
			"--rest-tls-cert-file", certFile, "--rest-tls-key-file", keyFile, "--rest-port", port)
		if got != 0 {
			t.Errorf("exit = %d, want 0", got)
		}
	})
	t.Run("no client certificate is rejected", func(t *testing.T) {
		got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-port", port)
		if got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
	t.Run("unreadable client certificate", func(t *testing.T) {
		got := probe(t, "--use-rest", "--rest-tls-enabled", "--rest-tls-mutual-auth",
			"--rest-tls-cert-file", filepath.Join(dir, "nope.pem"), "--rest-tls-key-file", keyFile, "--rest-port", port)
		if got != 1 {
			t.Errorf("exit = %d, want 1", got)
		}
	})
}
