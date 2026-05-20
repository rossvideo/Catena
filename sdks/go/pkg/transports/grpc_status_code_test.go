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

func TestToGRPCCode_gRPCCodes(t *testing.T) {
	tests := []struct {
		code     catena.StatusCode
		expected codes.Code
	}{
		{catena.OK, codes.OK},
		{catena.CANCELLED, codes.Canceled},
		{catena.UNKNOWN, codes.Unknown},
		{catena.INVALID_ARGUMENT, codes.InvalidArgument},
		{catena.DEADLINE_EXCEEDED, codes.DeadlineExceeded},
		{catena.NOT_FOUND, codes.NotFound},
		{catena.ALREADY_EXISTS, codes.AlreadyExists},
		{catena.PERMISSION_DENIED, codes.PermissionDenied},
		{catena.RESOURCE_EXHAUSTED, codes.ResourceExhausted},
		{catena.FAILED_PRECONDITION, codes.FailedPrecondition},
		{catena.ABORTED, codes.Aborted},
		{catena.OUT_OF_RANGE, codes.OutOfRange},
		{catena.UNIMPLEMENTED, codes.Unimplemented},
		{catena.INTERNAL, codes.Internal},
		{catena.UNAVAILABLE, codes.Unavailable},
		{catena.DATA_LOSS, codes.DataLoss},
		{catena.UNAUTHENTICATED, codes.Unauthenticated},
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
		catena.OK, catena.CANCELLED, catena.UNKNOWN, catena.INVALID_ARGUMENT, catena.DEADLINE_EXCEEDED,
		catena.NOT_FOUND, catena.ALREADY_EXISTS, catena.PERMISSION_DENIED, catena.RESOURCE_EXHAUSTED,
		catena.FAILED_PRECONDITION, catena.ABORTED, catena.OUT_OF_RANGE, catena.UNIMPLEMENTED,
		catena.INTERNAL, catena.UNAVAILABLE, catena.DATA_LOSS, catena.UNAUTHENTICATED,
	}

	for _, code := range grpcCodes {
		result := ToGRPCCode(code)
		if int(result) < 0 || int(result) > 16 {
			t.Errorf("ToGRPCCode(%d) = %v (%d) is outside valid gRPC code range 0-16", code, result, int(result))
		}
	}
}

func TestToGRPCCode_RESTCodesValidRange(t *testing.T) {
	restCodes := []catena.StatusCode{
		catena.CREATED, catena.ACCEPTED, catena.NO_CONTENT, catena.METHOD_NOT_ALLOWED, catena.CONFLICT,
		catena.UNPROCESSABLE_ENTITY, catena.TOO_MANY_REQUESTS, catena.BAD_GATEWAY,
		catena.SERVICE_UNAVAILABLE, catena.GATEWAY_TIMEOUT,
	}

	for _, code := range restCodes {
		result := ToGRPCCode(code)
		if int(result) < 0 || int(result) > 16 {
			t.Errorf("ToGRPCCode(%d) = %v (%d) is outside valid gRPC code range 0-16", code, result, int(result))
		}
	}
}
