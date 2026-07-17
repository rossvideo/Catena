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
 * @brief Test helpers for the st2138 streaming primitives.
 * @file stream_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package st2138

var _ Stream[ParamInfo] = &sliceStream[ParamInfo]{}

// sliceStream is an in-memory Stream that collects every chunk it receives.
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
