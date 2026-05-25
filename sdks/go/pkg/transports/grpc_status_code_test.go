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
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"google.golang.org/grpc/codes"
)

func TestToGRPCCode(t *testing.T) {
	tests := []struct {
		code     catena.StatusCode
		expected codes.Code
	}{
		{catena.StatusCodeOk, codes.OK},
		{catena.StatusCodeCanceled, codes.Canceled},
		{catena.StatusCodeUnknown, codes.Unknown},
		{catena.StatusCodeInvalidArgument, codes.InvalidArgument},
		{catena.StatusCodeDeadlineExceeded, codes.DeadlineExceeded},
		{catena.StatusCodeNotFound, codes.NotFound},
		{catena.StatusCodeAlreadyExists, codes.AlreadyExists},
		{catena.StatusCodePermissionDenied, codes.PermissionDenied},
		{catena.StatusCodeResourceExhausted, codes.ResourceExhausted},
		{catena.StatusCodeFailedPrecondition, codes.FailedPrecondition},
		{catena.StatusCodeAborted, codes.Aborted},
		{catena.StatusCodeOutOfRange, codes.OutOfRange},
		{catena.StatusCodeUnimplemented, codes.Unimplemented},
		{catena.StatusCodeInternal, codes.Internal},
		{catena.StatusCodeUnavailable, codes.Unavailable},
		{catena.StatusCodeDataLoss, codes.DataLoss},
		{catena.StatusCodeUnauthenticated, codes.Unauthenticated},
	}

	for i, tt := range tests {
		t.Run(tt.expected.String(), func(t *testing.T) {
			result := ToGRPCCode(tt.code)
			if result != tt.expected {
				t.Errorf("test %d: ToGRPCCode(%d) = %v, want %v", i, tt.code, result, tt.expected)
			}
		})
	}
}

func TestToGRPCCode_Unknown(t *testing.T) {
	unknownCode := catena.StatusCode(999)
	result := ToGRPCCode(unknownCode)
	if result != codes.Unknown {
		t.Errorf("ToGRPCCode(999) = %v, want %v", result, codes.Unknown)
	}
}

func TestToGRPCCode_ValidRange(t *testing.T) {
	grpcCodes := []catena.StatusCode{
		catena.StatusCodeOk, catena.StatusCodeCanceled, catena.StatusCodeUnknown, catena.StatusCodeInvalidArgument, catena.StatusCodeDeadlineExceeded,
		catena.StatusCodeNotFound, catena.StatusCodeAlreadyExists, catena.StatusCodePermissionDenied, catena.StatusCodeResourceExhausted,
		catena.StatusCodeFailedPrecondition, catena.StatusCodeAborted, catena.StatusCodeOutOfRange, catena.StatusCodeUnimplemented,
		catena.StatusCodeInternal, catena.StatusCodeUnavailable, catena.StatusCodeDataLoss, catena.StatusCodeUnauthenticated,
	}

	for _, code := range grpcCodes {
		result := ToGRPCCode(code)
		if int(result) < 0 || int(result) > 16 {
			t.Errorf("ToGRPCCode(%d) = %v (%d) is outside valid gRPC code range 0-16", code, result, int(result))
		}
	}
}

