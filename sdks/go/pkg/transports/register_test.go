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

	// "strings"
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
	fail       bool
	failError  error
}

func (s *recordingServer) RegisterTransport(t catena.Transport) error {
	if s.fail {
		return s.failError
	}
	s.registered = append(s.registered, t)
	return nil
}

func TestRegisterFrom_GRPCOnly(t *testing.T) {
	srv := &recordingServer{}
	opts := config.RuntimeOptions{
		UseGrpc: true,
		Grpc:    config.GrpcOptions{Port: 0},
	}

	if err := transports.RegisterFrom(srv, opts); err != nil {
		t.Fatal(err)
	}
	if len(srv.registered) != 1 {
		t.Fatalf("registered %d transports, want 1", len(srv.registered))
	}
	assertTransport(t, srv.registered[0], (*grpc.Transport)(nil), 0)
}

func TestRegisterFrom_RESTOnly(t *testing.T) {
	srv := &recordingServer{}
	opts := config.RuntimeOptions{
		UseRest: true,
		Rest:    config.RestOptions{Port: 0},
	}

	if err := transports.RegisterFrom(srv, opts); err != nil {
		t.Fatal(err)
	}
	if len(srv.registered) != 1 {
		t.Fatalf("registered %d transports, want 1", len(srv.registered))
	}
	assertTransport(t, srv.registered[0], (*rest.Transport)(nil), 0)
}

func TestRegisterFrom_Both(t *testing.T) {
	srv := &recordingServer{}
	opts := config.RuntimeOptions{
		UseGrpc: true,
		UseRest: true,
		Grpc:    config.GrpcOptions{Port: 0},
		Rest:    config.RestOptions{Port: 0},
	}

	if err := transports.RegisterFrom(srv, opts); err != nil {
		t.Fatal(err)
	}
	if len(srv.registered) != 2 {
		t.Fatalf("registered %d transports, want 2", len(srv.registered))
	}
	assertTransport(t, srv.registered[0], (*grpc.Transport)(nil), 0)
	assertTransport(t, srv.registered[1], (*rest.Transport)(nil), 0)
}

func TestRegisterFrom_Neither(t *testing.T) {
	srv := &recordingServer{}

	err := transports.RegisterFrom(srv, config.RuntimeOptions{})
	if !errors.Is(err, transports.ErrNoTransportEnabled) {
		t.Fatalf("RegisterFrom() error = %v, want %v", err, transports.ErrNoTransportEnabled)
	}
	if len(srv.registered) != 0 {
		t.Fatalf("registered %d transports, want 0", len(srv.registered))
	}
}

func TestRegisterFrom_GRPCError(t *testing.T) {
	srv := &recordingServer{}
	opts := config.RuntimeOptions{
		UseGrpc: true,
		Grpc:    config.GrpcOptions{Port: 0},
	}

	srv.fail = true
	srv.failError = errors.New("Test error")
	err := transports.RegisterFrom(srv, opts)
	if err == nil {
		t.Fatal("RegisterFrom() error = nil, want registration failure")
	}
	if err.Error() != "register gRPC transport: Test error" || !errors.Is(err, srv.failError) {
		t.Fatalf("RegisterFrom() error = %v, want %v", err, "register gRPC transport: Test error")
	}
	if len(srv.registered) != 0 {
		t.Fatalf("registered %d transports, want 0", len(srv.registered))
	}
}

func TestRegisterFrom_RESTError(t *testing.T) {
	srv := &recordingServer{}
	opts := config.RuntimeOptions{
		UseRest: true,
		Rest:    config.RestOptions{Port: 0},
	}

	srv.fail = true
	srv.failError = errors.New("Test error")
	err := transports.RegisterFrom(srv, opts)
	if err == nil {
		t.Fatal("RegisterFrom() error = nil, want registration failure")
	}
	if err.Error() != "register REST transport: Test error" || !errors.Is(err, srv.failError) {
		t.Fatalf("RegisterFrom() error = %v, want %v", err, "register REST transport: Test error")
	}
	if len(srv.registered) != 0 {
		t.Fatalf("registered %d transports, want 0", len(srv.registered))
	}
}

func assertTransport(t *testing.T, got catena.Transport, want any, port int) {
	t.Helper()
	if reflect.TypeOf(got) != reflect.TypeOf(want) {
		t.Fatalf("type = %T, want %T", got, want)
	}
	gotPort := int(reflect.ValueOf(got).Elem().FieldByName("port").Int())
	if gotPort != port {
		t.Fatalf("port = %d, want %d", gotPort, port)
	}
}
