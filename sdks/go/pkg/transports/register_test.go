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

package transports_test

import (
	"errors"
	"reflect"
	"strings"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/transports"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/grpc"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/rest"
)

// recordingServer records RegisterTransport calls without starting listeners.
type recordingServer struct {
	catena.Server
	registered []catena.Transport
	fail       error
	failAfter  int
}

func (s *recordingServer) RegisterTransport(t catena.Transport) error {
	if s.fail != nil && len(s.registered) == s.failAfter {
		return s.fail
	}
	s.registered = append(s.registered, t)
	return nil
}

func TestRegisterFrom(t *testing.T) {
	tests := []struct {
		name      string
		useGrpc   bool
		useRest   bool
		wantTypes []any
		wantErr   error
	}{
		{
			name:      "grpc only",
			useGrpc:   true,
			wantTypes: []any{(*grpc.Transport)(nil)},
		},
		{
			name:      "rest only",
			useRest:   true,
			wantTypes: []any{(*rest.Transport)(nil)},
		},
		{
			name:      "both",
			useGrpc:   true,
			useRest:   true,
			wantTypes: []any{(*grpc.Transport)(nil), (*rest.Transport)(nil)},
		},
		{
			name:    "neither",
			wantErr: transports.ErrNoTransportEnabled,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := &recordingServer{}
			opts := config.RuntimeOptions{
				UseGrpc: tt.useGrpc,
				UseRest: tt.useRest,
				Grpc: config.GrpcOptions{
					Port: 70000,
				},
				Rest: config.RestOptions{
					Port: 70001,
				},
			}

			// Test server registration
			err := transports.RegisterFrom(srv, opts)
			if !errors.Is(err, tt.wantErr) {
				t.Fatalf("RegisterFrom() error = %v, want %v", err, tt.wantErr)
			}
			if len(srv.registered) != len(tt.wantTypes) {
				t.Fatalf("registered %d transports, want %d", len(srv.registered), len(tt.wantTypes))
			}
			for i, want := range tt.wantTypes {
				if got := srv.registered[i]; !sameTransportType(got, want) {
					t.Fatalf("transport %d type = %T, want %T", i, got, want)
				}
			}

			// Test transport configuration
			if tt.name == "grpc only" {
				grpcCfg := reflect.ValueOf(srv.registered[0]).Elem().FieldByName("port").Int()
				if grpcCfg != int64(opts.Grpc.Port) {
					t.Fatalf("grpc transport port = %d, want %d", grpcCfg, opts.Grpc.Port)
				}
			}
			if tt.name == "rest only" {
				restCfg := reflect.ValueOf(srv.registered[0]).Elem().FieldByName("port").Int()
				if restCfg != int64(opts.Rest.Port) {
					t.Fatalf("rest transport port = %d, want %d", restCfg, opts.Rest.Port)
				}
			}
			if tt.name == "both" {
				grpcCfg := reflect.ValueOf(srv.registered[0]).Elem().FieldByName("port").Int()
				if grpcCfg != int64(opts.Grpc.Port) {
					t.Fatalf("grpc transport port = %d, want %d", grpcCfg, opts.Grpc.Port)
				}
				restCfg := reflect.ValueOf(srv.registered[1]).Elem().FieldByName("port").Int()
				if restCfg != int64(opts.Rest.Port) {
					t.Fatalf("rest transport port = %d, want %d", restCfg, opts.Rest.Port)
				}
			}
		})
	}
}

func TestRegisterFrom_RegistrationError(t *testing.T) {
	stubErr := errors.New("listen failed")

	tests := []struct {
		name      string
		useGrpc   bool
		useRest   bool
		failAfter int
		wantMsg   string
		wantCount int
	}{
		{
			name:    "grpc",
			useGrpc: true,
			wantMsg: "register gRPC transport",
		},
		{
			name:    "rest",
			useRest: true,
			wantMsg: "register REST transport",
		},
		{
			name:      "rest after grpc success",
			useGrpc:   true,
			useRest:   true,
			failAfter: 1,
			wantMsg:   "register REST transport",
			wantCount: 1,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := &recordingServer{fail: stubErr, failAfter: tt.failAfter}
			opts := config.RuntimeOptions{UseGrpc: tt.useGrpc, UseRest: tt.useRest}

			err := transports.RegisterFrom(srv, opts)
			if err == nil {
				t.Fatal("RegisterFrom() error = nil, want registration failure")
			}
			if !errors.Is(err, stubErr) {
				t.Fatalf("RegisterFrom() error = %v, want wrap of %v", err, stubErr)
			}
			if got := err.Error(); !strings.Contains(got, tt.wantMsg) {
				t.Fatalf("RegisterFrom() error = %q, want it to contain %q", got, tt.wantMsg)
			}
			if len(srv.registered) != tt.wantCount {
				t.Fatalf("registered %d transports, want %d", len(srv.registered), tt.wantCount)
			}
		})
	}
}

func sameTransportType(got, want any) bool {
	switch want.(type) {
	case *grpc.Transport:
		_, ok := got.(*grpc.Transport)
		return ok
	case *rest.Transport:
		_, ok := got.(*rest.Transport)
		return ok
	default:
		return false
	}
}
