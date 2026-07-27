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

package grpc

import (
	"context"
	"errors"
	"testing"

	"google.golang.org/grpc/metadata"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/wrapperspb"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
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

func TestStream(t *testing.T) {
	t.Run("sends chunk wire proto", func(t *testing.T) {
		fake := &fakeServerStream{}
		stream := &grpcStream[stubMessage]{ss: fake}

		if err := stream.Send(stubMessage{value: "a"}); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}

		if len(fake.msgs) != 1 {
			t.Fatalf("stream received %d messages, want 1", len(fake.msgs))
		}
		if _, ok := fake.msgs[0].(*wrapperspb.StringValue); !ok {
			t.Errorf("sent message type = %T, want *wrapperspb.StringValue", fake.msgs[0])
		}
	})

	t.Run("propagates SendMsg error", func(t *testing.T) {
		wantErr := errors.New("send failed")
		fake := &fakeServerStream{sendErr: wantErr}
		stream := &grpcStream[stubMessage]{ss: fake}

		if err := stream.Send(stubMessage{value: "a"}); !errors.Is(err, wantErr) {
			t.Fatalf("Send error = %v, want %v", err, wantErr)
		}
	})
}
