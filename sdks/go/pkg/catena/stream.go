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
 * @brief Streaming primitives for the Catena SDK.
 * @file stream.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"context"

	"google.golang.org/protobuf/proto"
)

// Message is implemented by the wrapper types that can be sent as a single
// chunk of a streamed response. Wire returns the fully-formed protobuf message
// for that chunk - the transport-neutral wire representation.
//
// A Message is self-contained: it carries everything the transport needs to
// emit one chunk. Transports never reach past Wire to inspect wrapper internals
// - the gRPC path sends the returned proto directly, and the REST path applies
// its own JSON marshalling rules to it.
type Message interface {
	Wire() proto.Message
}

// Stream is the handler-facing sink for a streamed response. Each endpoint's
// stream is parameterized by its own chunk type, so a handler can only send the
// chunk type that endpoint expects. Send delivers one chunk to the client and
// returns an error if the chunk could not be sent.
type Stream[T Message] interface {
	Send(chunk T) error
}

// shutdownStream wraps a handler-facing Stream so Send fails once the server's
// shutdown context is done, without the handler having to check for
// cancellation itself. The wrapped inner stream already enforces client
// disconnect at Send time (it carries the request/stream context); this adds
// the server-shutdown half, which the transport's stream does not observe
// during a graceful drain (in-flight sends keep succeeding). The combined
// effect: Send stops on either a client disconnect or a server shutdown.
//
// The server installs this around every streaming handler's transport stream,
// so shutdown cancellation is transparent to the produce-and-send path. It does
// NOT replace HandlerContext.Context(): a handler that computes without sending
// (or a unary handler with no stream at all) still needs that context to
// observe cancellation cooperatively.
type shutdownStream[T Message] struct {
	inner    Stream[T]
	shutdown context.Context
}

// compile-time interface check
var _ Stream[Message] = (*shutdownStream[Message])(nil)

// Send delivers chunk to the inner stream unless the shutdown context is
// already done, in which case it returns that context's error and sends
// nothing.
func (s shutdownStream[T]) Send(chunk T) error {
	if err := s.shutdown.Err(); err != nil {
		return err
	}
	return s.inner.Send(chunk)
}

// nullStream is a Stream that discards every chunk. The server substitutes it
// for the transport's stream when a caller opts out of responses (respond=false)
// so the gobbling is enforced in one place for all transports: the handler still
// runs to completion, but nothing it sends reaches the client. Send always
// returns nil so a handler that ignores respond behaves identically to one that
// honors it.
type nullStream[T Message] struct{}

var _ Stream[Message] = (*nullStream[Message])(nil)

// Send discards the chunk and always returns nil.
func (nullStream[T]) Send(chunk T) error { return nil }
