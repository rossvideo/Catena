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

package rest

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

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
	devMode        bool
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

// sendError reports a StatusResult as an SSE "error" event (per the SseError
// schema). It is used when an error occurs after the response headers have
// already been committed - once a 200 is on the wire the HTTP status can no
// longer change, so the failure is reported in-band as an error event instead.
// The StatusCode is converted to an HTTP status, and the message follows the
// same policy as unary errors: the detailed message is only exposed in dev mode,
// otherwise it is generalized to the standard HTTP status text (a blank message
// is omitted from the payload). It returns any write/flush error.
func (s *restStream[T]) sendError(result catena.StatusResult) error {
	httpStatus := ToHTTPStatus(result.Code)
	message := http.StatusText(httpStatus)
	if s.devMode {
		message = result.Error
	}

	payload := struct {
		Code    int    `json:"code"`
		Message string `json:"message,omitempty"`
	}{Code: httpStatus, Message: message}

	data, err := json.Marshal(payload)
	if err != nil {
		return err
	}

	if _, err := fmt.Fprintf(s.w, "event: error\ndata: %s\n\n", data); err != nil {
		return err
	}
	s.flusher.Flush()
	return nil
}

// firstStream retains only the first sent chunk, silently discarding the rest.
// The REST transport uses it for unary param-info requests, where the streaming
// handler still emits chunks but only the first is written back as a single JSON
// response, so retaining the rest would just waste memory. Discarding is not an
// error: Send always returns nil so the handler runs to completion and can still
// report StatusCodeOk.
type firstStream[T catena.Message] struct {
	item T
	has  bool
}

// Send retains the first chunk and discards any that follow. It always returns nil.
func (s *firstStream[T]) Send(chunk T) error {
	if !s.has {
		s.item = chunk
		s.has = true
	}
	return nil
}

// lastStream retains only the most recently sent chunk. The REST ExecuteCommand
// endpoint is unary: the handler may stream several CommandResponses, but the HTTP
// reply carries a single response, so only the final Send is kept (earlier ones
// are overwritten). Send always returns nil so the handler runs to completion.
type lastStream[T catena.Message] struct {
	item T
	has  bool
}

// Send overwrites the retained chunk with the latest one. It always returns nil.
func (s *lastStream[T]) Send(chunk T) error {
	s.item = chunk
	s.has = true
	return nil
}
