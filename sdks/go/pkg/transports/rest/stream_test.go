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

package rest

import (
	"context"
	"errors"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/wrapperspb"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// failingResponseWriter is an http.ResponseWriter+http.Flusher that serves the
// first failAfter writes and fails every one after that, used to exercise the
// SSE write-error branches. The zero value fails the very first write.
type failingResponseWriter struct {
	header    http.Header
	status    int
	writes    int
	failAfter int
}

func (w *failingResponseWriter) Header() http.Header {
	if w.header == nil {
		w.header = http.Header{}
	}
	return w.header
}
func (w *failingResponseWriter) Write(b []byte) (int, error) {
	w.writes++
	if w.writes > w.failAfter {
		return 0, errors.New("write failed")
	}
	return len(b), nil
}
func (w *failingResponseWriter) WriteHeader(status int) { w.status = status }
func (w *failingResponseWriter) Flush()                 {}

// unflushableResponseWriter is an http.ResponseWriter that deliberately does not
// implement http.Flusher, which SSE routes require.
type unflushableResponseWriter struct {
	header http.Header
	status int
}

func (w *unflushableResponseWriter) Header() http.Header {
	if w.header == nil {
		w.header = http.Header{}
	}
	return w.header
}
func (w *unflushableResponseWriter) Write(b []byte) (int, error) { return len(b), nil }
func (w *unflushableResponseWriter) WriteHeader(status int)      { w.status = status }

// stubMessage is a minimal catena.Message for exercising the generic stream
// adapters without depending on any domain chunk type. Wire returns a real
// proto (a StringValue carrying value) so the gRPC and REST adapters have a
// concrete message to send and marshal.
type stubMessage struct {
	value string
}

var _ catena.Message = stubMessage{}

func (m stubMessage) Wire() proto.Message {
	return wrapperspb.String(m.value)
}

// Each SSE route's own tests cover streamSSE's normal paths through the mux;
// these two cases are easier to reach by driving it directly.
func TestStreamSSE(t *testing.T) {
	t.Run("rejects a writer that cannot flush", func(t *testing.T) {
		// SSE has to push each frame as it is produced, so a writer that cannot
		// flush is rejected before the handler runs.
		w := &unflushableResponseWriter{}
		invoked := false

		streamSSE(&Transport{}, w, httptest.NewRequest(http.MethodGet, "/", nil), MarshalProtoJSON,
			func(catena.Stream[stubMessage]) catena.StatusResult {
				invoked = true
				return catena.StatusWithCode(catena.StatusCodeOk, "")
			})

		if invoked {
			t.Error("handler should not run without a flushable writer")
		}
		if w.status != http.StatusInternalServerError {
			t.Errorf("status = %d, want %d", w.status, http.StatusInternalServerError)
		}
	})

	t.Run("tolerates a failed error event after a chunk was sent", func(t *testing.T) {
		// The chunk commits a 200, so the handler's late error can only go out as
		// an SSE error event; when that write fails too there is nothing left to
		// do but log it.
		w := &failingResponseWriter{failAfter: 1}

		streamSSE(&Transport{}, w, httptest.NewRequest(http.MethodGet, "/", nil), MarshalProtoJSON,
			func(stream catena.Stream[stubMessage]) catena.StatusResult {
				if err := stream.Send(stubMessage{value: "a"}); err != nil {
					t.Fatalf("first Send returned error: %v", err)
				}
				return catena.StatusWithCode(catena.StatusCodeInternal, "boom")
			})

		if w.writes < 2 {
			t.Errorf("writes = %d, want the chunk plus an attempted error event", w.writes)
		}
		if w.status != http.StatusOK {
			t.Errorf("status = %d, want the already-committed %d", w.status, http.StatusOK)
		}
	})
}

func TestDeviceAggregateStream_NestedParamOrderIndependent(t *testing.T) {
	// Component chunk order must not matter: a parent arriving after a nested
	// child must keep children already hung under a synthesized intermediate.
	sendOrders := []struct {
		name   string
		chunks func() []st2138.DeviceComponent
	}{
		{
			name: "childBeforeParent",
			chunks: func() []st2138.DeviceComponent {
				parent := st2138.NewParamEmpty()
				parent.Proto.Type = protos.ParamType_STRUCT
				return []st2138.DeviceComponent{
					st2138.ComponentParam("monitor/eq", st2138.NewParamInt32(42)),
					st2138.ComponentParam("monitor", parent),
				}
			},
		},
		{
			name: "parentBeforeChild",
			chunks: func() []st2138.DeviceComponent {
				parent := st2138.NewParamEmpty()
				parent.Proto.Type = protos.ParamType_STRUCT
				return []st2138.DeviceComponent{
					st2138.ComponentParam("monitor", parent),
					st2138.ComponentParam("monitor/eq", st2138.NewParamInt32(42)),
				}
			},
		},
	}

	for _, tc := range sendOrders {
		t.Run(tc.name, func(t *testing.T) {
			stream := &deviceAggregateStream{}
			for _, chunk := range tc.chunks() {
				if err := stream.Send(chunk); err != nil {
					t.Fatalf("Send returned error: %v", err)
				}
			}

			device, ok := stream.result()
			if !ok {
				t.Fatal("expected an assembled device")
			}
			monitor := device.Proto.GetParams()["monitor"]
			if monitor == nil {
				t.Fatal("expected params.monitor")
			}
			if monitor.GetType() != protos.ParamType_STRUCT {
				t.Errorf("monitor type = %v, want STRUCT", monitor.GetType())
			}
			eq := monitor.GetParams()["eq"]
			if eq == nil {
				t.Fatal("expected params.monitor.params.eq to survive aggregation")
			}
			if got := eq.GetValue().GetInt32Value(); got != 42 {
				t.Errorf("eq value = %d, want 42", got)
			}
		})
	}
}

func TestDeviceAggregateStream_NestedCommandOrderIndependent(t *testing.T) {
	// Commands nest the same way as params; parent-after-child must not wipe
	// a nested command hung under a synthesized intermediate.
	parent := st2138.NewParamEmpty()
	parent.Proto.Type = protos.ParamType_STRUCT
	child := st2138.NewParamEmpty()
	child.Proto.Type = protos.ParamType_INT32

	stream := &deviceAggregateStream{}
	for _, chunk := range []st2138.DeviceComponent{
		st2138.ComponentCommand("group/action", child),
		st2138.ComponentCommand("group", parent),
	} {
		if err := stream.Send(chunk); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}
	}

	device, ok := stream.result()
	if !ok {
		t.Fatal("expected an assembled device")
	}
	group := device.Proto.GetCommands()["group"]
	if group == nil {
		t.Fatal("expected commands.group")
	}
	if group.GetType() != protos.ParamType_STRUCT {
		t.Errorf("group type = %v, want STRUCT", group.GetType())
	}
	action := group.GetParams()["action"]
	if action == nil {
		t.Fatal("expected commands.group.params.action to survive aggregation")
	}
	if action.GetType() != protos.ParamType_INT32 {
		t.Errorf("action type = %v, want INT32", action.GetType())
	}
}

func TestDeviceAggregateStream_IgnoresChunksWithNothingToMerge(t *testing.T) {
	// Every component kind guards against a missing body, and a menu oid that
	// does not name its group cannot be placed. None of these chunks contribute
	// anything, so no device is assembled at all.
	chunks := []st2138.DeviceComponent{
		{}, // no proto at all
		st2138.ComponentDevice(nil),
		st2138.ComponentParam("brightness", nil),
		st2138.ComponentConstraint("range", nil),
		st2138.ComponentMenu("status/main", nil),
		// A menu oid must be "menu-group-name/menu-name".
		st2138.ComponentMenu("groupless", st2138.NewMenu()),
		st2138.ComponentCommand("reset", nil),
		// ComponentLanguagePack always builds a pack, so the empty-pack chunk is
		// assembled by hand.
		{Proto: &protos.DeviceComponent{
			Kind: &protos.DeviceComponent_LanguagePack{
				LanguagePack: &protos.DeviceComponent_ComponentLanguagePack{Language: "en"},
			},
		}},
	}

	stream := &deviceAggregateStream{}
	for i, chunk := range chunks {
		if err := stream.Send(chunk); err != nil {
			t.Fatalf("Send(chunks[%d]) returned error: %v", i, err)
		}
	}

	if device, ok := stream.result(); ok {
		t.Errorf("expected no assembled device, got %v", device.Proto)
	}
}

func TestDeviceAggregateStream_MergesSecondDeviceChunk(t *testing.T) {
	// A handler may split the device itself across two Device chunks; the second
	// is merged into the first rather than replacing it.
	stream := &deviceAggregateStream{}
	for _, chunk := range []st2138.DeviceComponent{
		st2138.ComponentDevice(st2138.NewDevice(1).WithDetailLevel(st2138.DetailLevelMinimal)),
		st2138.ComponentDevice(st2138.NewDevice(1).
			WithMultiSetEnabled(true).
			WithParam("brightness", st2138.NewParamInt32(50))),
	} {
		if err := stream.Send(chunk); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}
	}

	device, ok := stream.result()
	if !ok {
		t.Fatal("expected an assembled device")
	}
	if got := device.Proto.GetDetailLevel(); got != st2138.DetailLevelMinimal {
		t.Errorf("detail level = %v, want MINIMAL to survive the merge", got)
	}
	if !device.Proto.GetMultiSetEnabled() {
		t.Error("expected multi_set_enabled from the second chunk")
	}
	if device.Proto.GetParams()["brightness"] == nil {
		t.Error("expected params.brightness from the second chunk")
	}
}

func TestFirstStream(t *testing.T) {
	// firstStream retains only the first chunk sent, discarding any subsequent chunks.
	t.Run("KeepsFirst", func(t *testing.T) {
		stream := &firstStream[stubMessage]{}

		if err := stream.Send(stubMessage{value: "first"}); err != nil {
			t.Fatalf("Send(first) returned error: %v", err)
		}
		if err := stream.Send(stubMessage{value: "second"}); err != nil {
			t.Fatalf("Send(second) returned error: %v", err)
		}

		if !stream.has {
			t.Fatal("expected stream to have retained a chunk")
		}
		if got := stream.item.value; got != "first" {
			t.Errorf("item value = %q, want %q", got, "first")
		}
	})
}

func TestLastStream(t *testing.T) {
	// lastStream retains only the last sent chunk, discarding any previous ones.
	t.Run("KeepsLast", func(t *testing.T) {
		stream := &lastStream[stubMessage]{}

		if err := stream.Send(stubMessage{value: "first"}); err != nil {
			t.Fatalf("Send(first) returned error: %v", err)
		}
		if err := stream.Send(stubMessage{value: "second"}); err != nil {
			t.Fatalf("Send(second) returned error: %v", err)
		}

		if !stream.has {
			t.Fatal("expected stream to have retained a chunk")
		}
		if got := stream.item.value; got != "second" {
			t.Errorf("item value = %q, want %q", got, "second")
		}
	})
}

func TestStream(t *testing.T) {
	t.Run("writes SSE frame and lazy headers", func(t *testing.T) {
		rec := httptest.NewRecorder()
		stream := &restStream[stubMessage]{
			w:       rec,
			flusher: rec,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
		}

		if err := stream.Send(stubMessage{value: "a"}); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}
		if err := stream.Send(stubMessage{value: "b"}); err != nil {
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
		stream := &restStream[stubMessage]{
			w:       rec,
			flusher: rec,
			marshal: MarshalProtoJSON,
			ctx:     ctx,
		}

		if err := stream.Send(stubMessage{value: "a"}); !errors.Is(err, context.Canceled) {
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
		stream := &restStream[stubMessage]{
			w:       rec,
			flusher: rec,
			marshal: func(proto.Message) ([]byte, error) { return nil, wantErr },
			ctx:     context.Background(),
		}

		if err := stream.Send(stubMessage{value: "a"}); !errors.Is(err, wantErr) {
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
		stream := &restStream[stubMessage]{
			w:       w,
			flusher: w,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
		}

		if err := stream.Send(stubMessage{value: "a"}); err == nil {
			t.Fatal("Send returned nil, want a write error")
		}
		if stream.sent != 0 {
			t.Errorf("sent = %d, want 0 after a failed write", stream.sent)
		}
	})

	t.Run("sendError exposes the detailed message in dev mode", func(t *testing.T) {
		rec := httptest.NewRecorder()
		stream := &restStream[stubMessage]{
			w:       rec,
			flusher: rec,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
			devMode: true,
		}

		if err := stream.sendError(catena.StatusWithCode(catena.StatusCodeInternal, "boom")); err != nil {
			t.Fatalf("sendError returned error: %v", err)
		}

		body := rec.Body.String()
		if !strings.Contains(body, "event: error\n") {
			t.Errorf("body missing error event line:\n%s", body)
		}
		if !strings.Contains(body, `"code":500`) {
			t.Errorf("body missing converted status code:\n%s", body)
		}
		if !strings.Contains(body, `"message":"boom"`) {
			t.Errorf("dev mode should expose the detailed message:\n%s", body)
		}
	})

	t.Run("sendError generalizes the message outside dev mode", func(t *testing.T) {
		rec := httptest.NewRecorder()
		stream := &restStream[stubMessage]{
			w:       rec,
			flusher: rec,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
			devMode: false,
		}

		if err := stream.sendError(catena.StatusWithCode(catena.StatusCodeNotFound, "param /foo not found")); err != nil {
			t.Fatalf("sendError returned error: %v", err)
		}

		body := rec.Body.String()
		if !strings.Contains(body, `"code":404`) {
			t.Errorf("body missing converted status code:\n%s", body)
		}
		if !strings.Contains(body, `"message":"Not Found"`) {
			t.Errorf("non-dev mode should generalize to the status text:\n%s", body)
		}
		if strings.Contains(body, "param /foo not found") {
			t.Errorf("non-dev mode must not leak the detailed message:\n%s", body)
		}
	})

	t.Run("sendError propagates a write error", func(t *testing.T) {
		w := &failingResponseWriter{}
		stream := &restStream[stubMessage]{
			w:       w,
			flusher: w,
			marshal: MarshalProtoJSON,
			ctx:     context.Background(),
		}

		if err := stream.sendError(catena.StatusWithCode(catena.StatusCodeInternal, "boom")); err == nil {
			t.Fatal("sendError returned nil, want a write error")
		}
	})
}
