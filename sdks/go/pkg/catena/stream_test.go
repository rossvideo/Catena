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
 * @brief Tests for the Catena streaming primitives.
 * @file stream_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"context"
	"errors"
	"testing"

	"google.golang.org/protobuf/proto"
)

type stubMessage struct {
	id int
}

var _ Message = stubMessage{}

func (stubMessage) Wire() proto.Message {
	return nil
}

func TestShutdownStream(t *testing.T) {
	t.Run("delegates to inner while shutdown context is live", func(t *testing.T) {
		inner := &sliceStream[stubMessage]{}
		stream := shutdownStream[stubMessage]{inner: inner, shutdown: context.Background()}

		if err := stream.Send(stubMessage{id: 1}); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}
		if len(inner.Items) != 1 || inner.Items[0].id != 1 {
			t.Fatalf("inner did not receive the chunk: %+v", inner.Items)
		}
	})

	t.Run("fails and sends nothing once shutdown context is done", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		cancel()
		inner := &sliceStream[stubMessage]{}
		stream := shutdownStream[stubMessage]{inner: inner, shutdown: ctx}

		if err := stream.Send(stubMessage{id: 1}); !errors.Is(err, context.Canceled) {
			t.Fatalf("Send error = %v, want context.Canceled", err)
		}
		if len(inner.Items) != 0 {
			t.Fatalf("inner received %d chunks after shutdown, want 0", len(inner.Items))
		}
	})

	t.Run("propagates inner Send error while shutdown context is live", func(t *testing.T) {
		wantErr := errors.New("inner failed")
		inner := &sliceStream[stubMessage]{Err: wantErr, FailAfter: 0}
		stream := shutdownStream[stubMessage]{inner: inner, shutdown: context.Background()}

		if err := stream.Send(stubMessage{id: 1}); !errors.Is(err, wantErr) {
			t.Fatalf("Send error = %v, want %v", err, wantErr)
		}
	})
}

func TestNullStream(t *testing.T) {
	t.Run("Nothing", func(t *testing.T) {
		stream := &nullStream[stubMessage]{}

		if err := stream.Send(stubMessage{id: 1}); err != nil {
			t.Fatalf("Send returned error: %v", err)
		}
	})
}

// This is just testing the test infrastructure
func TestSliceStream(t *testing.T) {
	t.Run("collects chunks in order", func(t *testing.T) {
		stream := &sliceStream[stubMessage]{}

		first := stubMessage{id: 1}
		second := stubMessage{id: 2}
		third := stubMessage{id: 3}

		if err := stream.Send(first); err != nil {
			t.Fatalf("Send(first) returned error: %v", err)
		}
		if err := stream.Send(second); err != nil {
			t.Fatalf("Send(second) returned error: %v", err)
		}
		if err := stream.Send(third); err != nil {
			t.Fatalf("Send(third) returned error: %v", err)
		}

		collected := stream.Items
		if len(collected) != 3 {
			t.Fatalf("collected %d chunks, want 3", len(collected))
		}
		if got := collected[0].id; got != 1 {
			t.Errorf("collected[0] id = %d, want %d", got, 1)
		}
		if got := collected[1].id; got != 2 {
			t.Errorf("collected[1] id = %d, want %d", got, 2)
		}
		if got := collected[2].id; got != 3 {
			t.Errorf("collected[2] id = %d, want %d", got, 3)
		}
	})

	t.Run("returns Err after FailAfter chunks", func(t *testing.T) {
		wantErr := errors.New("send failed")
		stream := &sliceStream[stubMessage]{Err: wantErr, FailAfter: 1}

		if err := stream.Send(stubMessage{id: 1}); err != nil {
			t.Fatalf("Send(first) returned error before FailAfter: %v", err)
		}
		if err := stream.Send(stubMessage{id: 2}); !errors.Is(err, wantErr) {
			t.Fatalf("Send(second) error = %v, want %v", err, wantErr)
		}

		// Only the chunks accepted before the failure should be recorded.
		if len(stream.Items) != 1 {
			t.Fatalf("recorded %d chunks, want 1", len(stream.Items))
		}
		if got := stream.Items[0].id; got != 1 {
			t.Errorf("Items[0] id = %d, want %d", got, 1)
		}
	})
}

var _ Stream[stubMessage] = &sliceStream[stubMessage]{}

// SliceStream is an in-memory Stream that collects every chunk it receives.
// It is intended for tests, where a handler can be driven against a real Stream
// and the accumulated chunks inspected afterwards.
type sliceStream[T Message] struct {
	Items []T
	// Err, when non-nil, is returned by Send once FailAfter chunks have been
	// accepted, letting tests exercise a handler's error path mid-stream.
	Err error
	// FailAfter is the number of chunks Send accepts before returning Err.
	// It has no effect while Err is nil. Zero fails on the first Send.
	FailAfter int
}

// Send appends chunk to Items and returns nil, unless Err is set and FailAfter
// chunks have already been accepted, in which case it returns Err without
// recording the chunk.
func (s *sliceStream[T]) Send(chunk T) error {
	if s.Err != nil && len(s.Items) >= s.FailAfter {
		return s.Err
	}
	s.Items = append(s.Items, chunk)
	return nil
}
