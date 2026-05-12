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

func TestToHTTPStatus_catenaStatusCodes(t *testing.T) {
	tests := []struct {
		code     catena.StatusCode
		expected int
	}{
		{catena.OK, http.StatusOK},
		{catena.CANCELLED, 499},
		{catena.UNKNOWN, http.StatusInternalServerError},
		{catena.INVALID_ARGUMENT, http.StatusBadRequest},
		{catena.DEADLINE_EXCEEDED, http.StatusGatewayTimeout},
		{catena.NOT_FOUND, http.StatusNotFound},
		{catena.ALREADY_EXISTS, http.StatusConflict},
		{catena.PERMISSION_DENIED, http.StatusForbidden},
		{catena.RESOURCE_EXHAUSTED, http.StatusTooManyRequests},
		{catena.FAILED_PRECONDITION, http.StatusBadRequest},
		{catena.ABORTED, http.StatusConflict},
		{catena.OUT_OF_RANGE, http.StatusBadRequest},
		{catena.UNIMPLEMENTED, http.StatusNotImplemented},
		{catena.INTERNAL, http.StatusInternalServerError},
		{catena.UNAVAILABLE, http.StatusServiceUnavailable},
		{catena.DATA_LOSS, http.StatusInternalServerError},
		{catena.UNAUTHENTICATED, http.StatusUnauthorized},
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

func TestToHTTPStatus_RESTCodes(t *testing.T) {
	tests := []struct {
		code     catena.StatusCode
		expected int
	}{
		{catena.CREATED, 201},
		{catena.ACCEPTED, 202},
		{catena.NO_CONTENT, 204},
		{catena.METHOD_NOT_ALLOWED, 405},
		{catena.CONFLICT, 409},
		{catena.UNPROCESSABLE_ENTITY, 422},
		{catena.TOO_MANY_REQUESTS, 429},
		{catena.BAD_GATEWAY, 502},
		{catena.SERVICE_UNAVAILABLE, 503},
		{catena.GATEWAY_TIMEOUT, 504},
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
