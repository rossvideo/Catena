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
 * @brief Unit tests for the transport-side catena.Stream adapters.
 * @file stream_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package transports

import (
	"context"
	"errors"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"google.golang.org/grpc/metadata"
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// fakeServerStream is a minimal grpc.ServerStream that records the messages
// passed to SendMsg, or fails every SendMsg when sendErr is set.
type fakeServerStream struct {
	msgs    []any
	sendErr error
}

func (f *fakeServerStream) SetHeader(metadata.MD) error  { return nil }
func (f *fakeServerStream) SendHeader(metadata.MD) error { return nil }
func (f *fakeServerStream) SetTrailer(metadata.MD)       {}
func (f *fakeServerStream) Context() context.Context     { return context.Background() }
func (f *fakeServerStream) RecvMsg(any) error            { return nil }

func (f *fakeServerStream) SendMsg(m any) error {
	if f.sendErr != nil {
		return f.sendErr
	}
	f.msgs = append(f.msgs, m)
	return nil
}

// failingResponseWriter is an http.ResponseWriter+http.Flusher whose Write
// always fails, used to exercise the SSE write-error branch.
type failingResponseWriter struct {
	header http.Header
	status int
}

func (w *failingResponseWriter) Header() http.Header {
	if w.header == nil {
		w.header = http.Header{}
	}
	return w.header
}
func (w *failingResponseWriter) Write([]byte) (int, error) { return 0, errors.New("write failed") }
func (w *failingResponseWriter) WriteHeader(status int)    { w.status = status }
func (w *failingResponseWriter) Flush()                    {}

func testParamInfo(oid string) catena.ParamInfo {
	return catena.NewParamInfo(oid, nil, catena.ParamTypeInt32, "", 0)
}

func TestCollectStream(t *testing.T) {
	t.Run("discards chunks beyond max", func(t *testing.T) {
		stream := &collectStream[catena.ParamInfo]{max: 1}

		if err := stream.Send(testParamInfo("first")); err != nil {
			t.Fatalf("Send(first) returned error: %v", err)
		}
		if err := stream.Send(testParamInfo("second")); err != nil {
			t.Fatalf("Send(second) returned error: %v", err)
		}

		if len(stream.items) != 1 {
			t.Fatalf("retained %d chunks, want 1", len(stream.items))
		}
		if got := stream.items[0].GetProtoInfo().GetOid(); got != "first" {
			t.Errorf("items[0] oid = %q, want %q", got, "first")
		}
	})
}

func TestGrpcStream(t *testing.T) {
	t.Run("sends chunk wire proto", func(t *testing.T) {
		fake := &fakeServerStream{}
		stream := &grpcStream[catena.ParamInfo]{ss: fake}

		if err := stream.Send(testParamInfo("a")); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}

		if len(fake.msgs) != 1 {
			t.Fatalf("stream received %d messages, want 1", len(fake.msgs))
		}
		if _, ok := fake.msgs[0].(*protos.ParamInfoResponse); !ok {
			t.Errorf("sent message type = %T, want *protos.ParamInfoResponse", fake.msgs[0])
		}
	})

	t.Run("propagates SendMsg error", func(t *testing.T) {
		wantErr := errors.New("send failed")
		fake := &fakeServerStream{sendErr: wantErr}
		stream := &grpcStream[catena.ParamInfo]{ss: fake}

		if err := stream.Send(testParamInfo("a")); !errors.Is(err, wantErr) {
			t.Fatalf("Send error = %v, want %v", err, wantErr)
		}
	})
}

func TestRestStream(t *testing.T) {
	t.Run("writes SSE frame and lazy headers", func(t *testing.T) {
		rec := httptest.NewRecorder()
		stream := &restStream[catena.ParamInfo]{
			w:       rec,
			flusher: rec,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
		}

		if err := stream.Send(testParamInfo("a")); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}
		if err := stream.Send(testParamInfo("b")); err != nil {
			t.Fatalf("second Send returned error: %v", err)
		}

		if stream.sent != 2 {
			t.Errorf("sent = %d, want 2", stream.sent)
		}
		if rec.Code != http.StatusOK {
			t.Errorf("status = %d, want %d", rec.Code, http.StatusOK)
		}
		if ct := rec.Header().Get("Content-Type"); ct != "text/event-stream" {
			t.Errorf("Content-Type = %q, want text/event-stream", ct)
		}
		if got := strings.Count(rec.Body.String(), "data:"); got != 2 {
			t.Errorf("SSE data frames = %d, want 2\nbody:\n%s", got, rec.Body.String())
		}
	})

	t.Run("returns context error and writes nothing when cancelled", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		cancel()

		rec := httptest.NewRecorder()
		stream := &restStream[catena.ParamInfo]{
			w:       rec,
			flusher: rec,
			marshal: MarshalProtoJSON,
			ctx:     ctx,
		}

		if err := stream.Send(testParamInfo("a")); !errors.Is(err, context.Canceled) {
			t.Fatalf("Send error = %v, want context.Canceled", err)
		}
		if stream.sent != 0 {
			t.Errorf("sent = %d, want 0", stream.sent)
		}
		if stream.headersWritten {
			t.Error("headers should not be written when the context is already cancelled")
		}
		if rec.Body.Len() != 0 {
			t.Errorf("body = %q, want empty", rec.Body.String())
		}
	})

	t.Run("propagates marshal error before writing headers", func(t *testing.T) {
		wantErr := errors.New("marshal failed")
		rec := httptest.NewRecorder()
		stream := &restStream[catena.ParamInfo]{
			w:       rec,
			flusher: rec,
			marshal: func(proto.Message) ([]byte, error) { return nil, wantErr },
			ctx:     context.Background(),
		}

		if err := stream.Send(testParamInfo("a")); !errors.Is(err, wantErr) {
			t.Fatalf("Send error = %v, want %v", err, wantErr)
		}
		if stream.sent != 0 {
			t.Errorf("sent = %d, want 0", stream.sent)
		}
		if stream.headersWritten {
			t.Error("headers should not be written when marshalling fails")
		}
	})

	t.Run("propagates write error", func(t *testing.T) {
		w := &failingResponseWriter{}
		stream := &restStream[catena.ParamInfo]{
			w:       w,
			flusher: w,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
		}

		if err := stream.Send(testParamInfo("a")); err == nil {
			t.Fatal("Send returned nil, want a write error")
		}
		if stream.sent != 0 {
			t.Errorf("sent = %d, want 0 after a failed write", stream.sent)
		}
	})
}
