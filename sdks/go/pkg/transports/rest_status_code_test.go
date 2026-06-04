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

package transports

import (
	"net/http"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

func TestToHTTPStatus(t *testing.T) {
	tests := []struct {
		code     catena.StatusCode
		expected int
	}{
		{catena.StatusCodeOk, http.StatusOK},
		{catena.StatusCodeCancelled, 499},
		{catena.StatusCodeUnknown, http.StatusInternalServerError},
		{catena.StatusCodeInvalidArgument, http.StatusBadRequest},
		{catena.StatusCodeDeadlineExceeded, http.StatusGatewayTimeout},
		{catena.StatusCodeNotFound, http.StatusNotFound},
		{catena.StatusCodeAlreadyExists, http.StatusConflict},
		{catena.StatusCodePermissionDenied, http.StatusForbidden},
		{catena.StatusCodeResourceExhausted, http.StatusTooManyRequests},
		{catena.StatusCodeFailedPrecondition, http.StatusBadRequest},
		{catena.StatusCodeAborted, http.StatusConflict},
		{catena.StatusCodeOutOfRange, http.StatusBadRequest},
		{catena.StatusCodeUnimplemented, http.StatusNotImplemented},
		{catena.StatusCodeInternal, http.StatusInternalServerError},
		{catena.StatusCodeUnavailable, http.StatusServiceUnavailable},
		{catena.StatusCodeDataLoss, http.StatusInternalServerError},
		{catena.StatusCodeUnauthenticated, http.StatusUnauthorized},
	}

	for i, tt := range tests {
		t.Run(http.StatusText(tt.expected), func(t *testing.T) {
			result := ToHTTPStatus(tt.code)
			if result != tt.expected {
				t.Errorf("test %d: ToHTTPStatus(%d) = %d, want %d", i, tt.code, result, tt.expected)
			}
		})
	}
}

func TestToHTTPStatus_Unknown(t *testing.T) {
	unknownCode := catena.StatusCode(999)
	result := ToHTTPStatus(unknownCode)
	if result != http.StatusInternalServerError {
		t.Errorf("ToHTTPStatus(999) = %d, want %d", result, http.StatusInternalServerError)
	}
}
