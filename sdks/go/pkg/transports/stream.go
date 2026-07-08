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
 * @brief Transport-side adapters for catena.Stream.
 * @file stream.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package transports

import (
	"context"
	"fmt"
	"net/http"

	"google.golang.org/grpc"
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// grpcStream adapts a gRPC server stream to catena.Stream. Because
// grpc.ServerStream.SendMsg accepts any proto message, one generic adapter
// serves every streaming endpoint - each chunk is sent as its wire proto.
type grpcStream[T catena.Message] struct {
	ss grpc.ServerStream
}

// Send transmits the chunk's wire proto over the gRPC stream.
func (s *grpcStream[T]) Send(chunk T) error {
	return s.ss.SendMsg(chunk.Wire())
}

// restStream adapts an HTTP response to catena.Stream, emitting each chunk as a
// Server-Sent Events frame. The SSE headers are written lazily on the first
// successful Send so that, if the handler produces nothing, the transport can
// still choose a different HTTP status. marshal is supplied per endpoint so the
// REST transport keeps ownership of its JSON rules.
type restStream[T catena.Message] struct {
	w              http.ResponseWriter
	flusher        http.Flusher
	marshal        func(proto.Message) ([]byte, error)
	ctx            context.Context
	headersWritten bool
	sent           int
}

// Send marshals the chunk and writes it as one SSE frame, flushing so the
// client receives it immediately. A non-nil error means the chunk was not
// delivered and the caller should stop streaming; it can come from the request
// context being cancelled (client disconnected), the marshal step failing, or
// the underlying write failing. The cause is not distinguished because every
// case is terminal for this stream.
func (s *restStream[T]) Send(chunk T) error {
	select {
	case <-s.ctx.Done():
		return s.ctx.Err()
	default:
	}

	data, err := s.marshal(chunk.Wire())
	if err != nil {
		return err
	}

	if !s.headersWritten {
		s.writeHeaders()
	}

	if _, err := fmt.Fprintf(s.w, "data: %s\n\n", data); err != nil {
		return err
	}
	s.flusher.Flush()
	s.sent++
	return nil
}

// writeHeaders commits the SSE response headers and a 200 status. It runs once,
// on the first chunk, so an error before any chunk can still set its own status.
func (s *restStream[T]) writeHeaders() {
	s.w.Header().Set("Content-Type", "text/event-stream")
	s.w.Header().Set("Cache-Control", "no-cache")
	s.w.Header().Set("Connection", "keep-alive")
	s.w.WriteHeader(http.StatusOK)
	s.flusher.Flush()
	s.headersWritten = true
}

// collectStream is a catena.Stream that accumulates up to max chunks in memory,
// silently discarding any beyond that. The REST transport uses it for unary
// requests, where the streaming handler still emits chunks but only the first is
// written back as a single JSON response, so retaining the rest would just waste
// memory. Discarding is not an error: Send always returns nil so the handler
// runs to completion and can still report StatusCodeOk.
type collectStream[T catena.Message] struct {
	items []T
	max   int
}

// Send retains the chunk while fewer than max chunks have been collected, and
// otherwise discards it. It always returns nil.
func (s *collectStream[T]) Send(chunk T) error {
	if len(s.items) < s.max {
		s.items = append(s.items, chunk)
	}
	return nil
}
