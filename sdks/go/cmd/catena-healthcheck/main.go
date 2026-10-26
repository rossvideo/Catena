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
 * @brief Container healthcheck for Catena Go SDK devices.
 * @file main.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-10-26
 */

package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"flag"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/credentials/insecure"
	healthpb "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/status"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

const restHealthPath = "/st2138-api/v1/health"

type probeOptions struct {
	prefix    string
	host      string
	timeout   time.Duration
	verifyTLS bool
	caFile    string
}

func main() {
	os.Exit(run(os.Args[1:]))
}

func run(args []string) int {
	ownArgs, sdkArgs := splitArgs(args)

	po := probeOptions{}
	fs := flag.NewFlagSet("catena-healthcheck", flag.ContinueOnError)
	fs.StringVar(&po.prefix, "prefix", envOr("HEALTHCHECK_PREFIX", "CATENA"), "Env var prefix of the SDK config to read (env: HEALTHCHECK_PREFIX)")
	fs.StringVar(&po.host, "host", envOr("HEALTHCHECK_HOST", "localhost"), "Host to probe (env: HEALTHCHECK_HOST)")
	fs.DurationVar(&po.timeout, "timeout", envDuration("HEALTHCHECK_TIMEOUT", 3*time.Second), "Per-transport probe timeout (env: HEALTHCHECK_TIMEOUT)")
	fs.BoolVar(&po.verifyTLS, "verify-tls", envOr("HEALTHCHECK_VERIFY_TLS", "") == "true", "Verify the server certificate when TLS is enabled (env: HEALTHCHECK_VERIFY_TLS)")
	fs.StringVar(&po.caFile, "tls-ca-file", envOr("HEALTHCHECK_TLS_CA_FILE", ""), "CA bundle for server verification; implies --verify-tls (env: HEALTHCHECK_TLS_CA_FILE)")
	if err := fs.Parse(ownArgs); err != nil {
		return 2
	}
	if po.caFile != "" {
		po.verifyTLS = true
	}

	// Same env var names and flags as the server for the options the probe uses,
	// so the same env works for both. Everything else is allowlisted out, which
	// keeps --help accurate and needs no upkeep as the SDK config grows.
	opts, err := config.InitOptions("catena-healthcheck", sdkArgs,
		config.WithPrefix(po.prefix),
		config.WithOnlyInputs(
			"use-grpc", "use-rest",
			"grpc-port", "rest-port",
			"grpc-tls-*", "rest-tls-*",
		),
	)
	if err != nil {
		if err == config.ErrHelp {
			fmt.Fprintln(os.Stderr, "\nHealthcheck options:")
			fs.SetOutput(os.Stderr)
			fs.PrintDefaults()
			return 0
		}
		return 2
	}

	if !opts.UseGrpc && !opts.UseRest {
		fmt.Fprintf(os.Stderr, "catena-healthcheck: no transport enabled (set %s_USE_GRPC and/or %s_USE_REST)\n", po.prefix, po.prefix)
		return 1
	}

	type check struct {
		name string
		fn   func(context.Context) error
	}
	var checks []check
	if opts.UseGrpc {
		addr := net.JoinHostPort(po.host, strconv.Itoa(opts.Grpc.Port))
		checks = append(checks, check{"grpc " + addr, func(ctx context.Context) error {
			return checkGRPC(ctx, addr, opts.Grpc.TLS, po)
		}})
	}
	if opts.UseRest {
		addr := net.JoinHostPort(po.host, strconv.Itoa(opts.Rest.Port))
		checks = append(checks, check{"rest " + addr, func(ctx context.Context) error {
			return checkREST(ctx, addr, opts.Rest.TLS, po)
		}})
	}

	ctx, cancel := context.WithTimeout(context.Background(), po.timeout)
	defer cancel()

	var (
		wg     sync.WaitGroup
		mu     sync.Mutex
		failed bool
	)
	for _, c := range checks {
		wg.Add(1)
		go func() {
			defer wg.Done()
			if err := c.fn(ctx); err != nil {
				mu.Lock()
				failed = true
				fmt.Fprintf(os.Stderr, "catena-healthcheck: %s: %v\n", c.name, err)
				mu.Unlock()
			}
		}()
	}
	wg.Wait()

	if failed {
		return 1
	}
	return 0
}

func checkGRPC(ctx context.Context, addr string, tlsOpts config.TLSOptions, po probeOptions) error {
	creds := insecure.NewCredentials()
	tlsCfg, err := clientTLS(tlsOpts, po)
	if err != nil {
		return err
	}
	if tlsCfg != nil {
		creds = credentials.NewTLS(tlsCfg)
	}

	conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(creds))
	if err != nil {
		return err
	}
	defer conn.Close()

	resp, err := healthpb.NewHealthClient(conn).Check(ctx, &healthpb.HealthCheckRequest{})
	switch status.Code(err) {
	case codes.OK:
		if resp.GetStatus() != healthpb.HealthCheckResponse_SERVING {
			return fmt.Errorf("health status is %s", resp.GetStatus())
		}
		return nil
	case codes.Unimplemented:
		// The health service is disabled on the server (--grpc-disable-health).
		// Getting a gRPC status back still proves the server is up.
		return nil
	default:
		return err
	}
}

func checkREST(ctx context.Context, addr string, tlsOpts config.TLSOptions, po probeOptions) error {
	scheme := "http"
	transport := &http.Transport{}
	tlsCfg, err := clientTLS(tlsOpts, po)
	if err != nil {
		return err
	}
	if tlsCfg != nil {
		scheme = "https"
		transport.TLSClientConfig = tlsCfg
	}
	client := &http.Client{Transport: transport}
	defer transport.CloseIdleConnections()

	req, err := http.NewRequestWithContext(ctx, http.MethodGet, scheme+"://"+addr+restHealthPath, nil)
	if err != nil {
		return err
	}
	resp, err := client.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	_, _ = io.Copy(io.Discard, resp.Body)

	if resp.StatusCode < 200 || resp.StatusCode > 299 {
		return fmt.Errorf("unexpected status %s", resp.Status)
	}
	return nil
}

// clientTLS returns nil when TLS is not enabled for the transport.
func clientTLS(t config.TLSOptions, po probeOptions) (*tls.Config, error) {
	if !t.Enabled {
		return nil, nil
	}
	cfg := &tls.Config{MinVersion: tls.VersionTLS12}
	if !po.verifyTLS {
		// Local liveness probe: the cert is usually issued for the public name, not localhost.
		cfg.InsecureSkipVerify = true
	} else if po.caFile != "" {
		pem, err := os.ReadFile(po.caFile)
		if err != nil {
			return nil, fmt.Errorf("read CA file: %w", err)
		}
		pool := x509.NewCertPool()
		if !pool.AppendCertsFromPEM(pem) {
			return nil, fmt.Errorf("CA file %q has no valid certificates", po.caFile)
		}
		cfg.RootCAs = pool
	}
	if t.MutualAuth {
		cert, err := tls.LoadX509KeyPair(t.CertFile, t.KeyFile)
		if err != nil {
			return nil, fmt.Errorf("load client certificate: %w", err)
		}
		cfg.Certificates = []tls.Certificate{cert}
	}
	return cfg, nil
}

// splitArgs separates the probe's own flags from SDK config flags, because
// config.InitOptions fails on flags it doesn't know.
func splitArgs(args []string) (own, sdk []string) {
	valued := map[string]bool{"prefix": true, "host": true, "timeout": true, "tls-ca-file": true}
	boolean := map[string]bool{"verify-tls": true}
	for i := 0; i < len(args); i++ {
		a := args[i]
		name := strings.TrimLeft(a, "-")
		if name == a { // value of an SDK flag, or positional
			sdk = append(sdk, a)
			continue
		}
		base, _, hasVal := strings.Cut(name, "=")
		switch {
		case boolean[base], valued[base] && hasVal:
			own = append(own, a)
		case valued[base] && i+1 < len(args):
			own = append(own, a, args[i+1])
			i++
		default:
			sdk = append(sdk, a)
		}
	}
	return own, sdk
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

func envDuration(key string, def time.Duration) time.Duration {
	if d, err := time.ParseDuration(os.Getenv(key)); err == nil {
		return d
	}
	return def
}
