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

package catena

import (
	"net/http"
	"testing"
)

func TestStatusCode_ToHTTPStatus_gRPCCodes(t *testing.T) {
	tests := []struct {
		code     StatusCode
		expected int
	}{
		{OK, http.StatusOK},
		{CANCELLED, 499},
		{UNKNOWN, http.StatusInternalServerError},
		{INVALID_ARGUMENT, http.StatusBadRequest},
		{DEADLINE_EXCEEDED, http.StatusGatewayTimeout},
		{NOT_FOUND, http.StatusNotFound},
		{ALREADY_EXISTS, http.StatusConflict},
		{PERMISSION_DENIED, http.StatusForbidden},
		{RESOURCE_EXHAUSTED, http.StatusTooManyRequests},
		{FAILED_PRECONDITION, http.StatusBadRequest},
		{ABORTED, http.StatusConflict},
		{OUT_OF_RANGE, http.StatusBadRequest},
		{UNIMPLEMENTED, http.StatusNotImplemented},
		{INTERNAL, http.StatusInternalServerError},
		{UNAVAILABLE, http.StatusServiceUnavailable},
		{DATA_LOSS, http.StatusInternalServerError},
		{UNAUTHENTICATED, http.StatusUnauthorized},
	}

	for i, tt := range tests {
		t.Run(http.StatusText(tt.expected), func(t *testing.T) {
			result := tt.code.ToHTTPStatus()
			if result != tt.expected {
				t.Errorf("test %d: StatusCode(%d).ToHTTPStatus() = %d, want %d", i, tt.code, result, tt.expected)
			}
		})
	}
}

func TestStatusCode_ToHTTPStatus_RESTCodes(t *testing.T) {
	tests := []struct {
		code     StatusCode
		expected int
	}{
		{CREATED, 201},
		{ACCEPTED, 202},
		{NO_CONTENT, 204},
		{METHOD_NOT_ALLOWED, 405},
		{CONFLICT, 409},
		{UNPROCESSABLE_ENTITY, 422},
		{TOO_MANY_REQUESTS, 429},
		{BAD_GATEWAY, 502},
		{SERVICE_UNAVAILABLE, 503},
		{GATEWAY_TIMEOUT, 504},
	}

	for i, tt := range tests {
		t.Run(http.StatusText(tt.expected), func(t *testing.T) {
			result := tt.code.ToHTTPStatus()
			if result != tt.expected {
				t.Errorf("test %d: StatusCode(%d).ToHTTPStatus() = %d, want %d", i, tt.code, result, tt.expected)
			}
		})
	}
}

func TestStatusCode_ToHTTPStatus_Unknown(t *testing.T) {
	// Unknown codes should default to InternalServerError
	unknownCode := StatusCode(999)
	result := unknownCode.ToHTTPStatus()
	if result != http.StatusInternalServerError {
		t.Errorf("Unknown StatusCode.ToHTTPStatus() = %d, want %d", result, http.StatusInternalServerError)
	}
}

func TestStatusCodeConstants(t *testing.T) {
	// Verify gRPC-compatible codes are in the expected range (0-16)
	grpcCodes := []StatusCode{
		OK, CANCELLED, UNKNOWN, INVALID_ARGUMENT, DEADLINE_EXCEEDED,
		NOT_FOUND, ALREADY_EXISTS, PERMISSION_DENIED, RESOURCE_EXHAUSTED,
		FAILED_PRECONDITION, ABORTED, OUT_OF_RANGE, UNIMPLEMENTED,
		INTERNAL, UNAVAILABLE, DATA_LOSS, UNAUTHENTICATED,
	}

	for _, code := range grpcCodes {
		if code < 0 || code > 16 {
			t.Errorf("gRPC code %d is outside valid range 0-16", code)
		}
	}

	// Verify REST codes are in the expected HTTP range
	restCodes := []StatusCode{
		CREATED, ACCEPTED, NO_CONTENT, METHOD_NOT_ALLOWED,
		CONFLICT, UNPROCESSABLE_ENTITY, TOO_MANY_REQUESTS,
		BAD_GATEWAY, SERVICE_UNAVAILABLE, GATEWAY_TIMEOUT,
	}

	for _, code := range restCodes {
		if code < 200 {
			t.Errorf("REST code %d should be >= 200", code)
		}
	}
}

func TestReply_CatenaValue(t *testing.T) {
	value, _ := ToCatenaValue(int32(42))
	result, status := Reply(value)

	if status.Code != OK {
		t.Errorf("Reply status code = %d, want %d", status.Code, OK)
	}
	if status.Error != "" {
		t.Errorf("Reply status error = %q, want empty", status.Error)
	}
	if result.Value == nil {
		t.Error("Reply result value should not be nil")
	}
}

func TestReply_CatenaDevice(t *testing.T) {
	deviceMap := map[string]any{
		"slot":         uint32(0),
		"detail_level": DetailLevelFull,
	}
	device, _ := ToCatenaDevice(deviceMap)
	result, status := Reply(device)

	if status.Code != OK {
		t.Errorf("Reply status code = %d, want %d", status.Code, OK)
	}
	if result.GetProtoDevice() == nil {
		t.Error("Reply result device should not be nil")
	}
}

func TestReply_CatenaAsset(t *testing.T) {
	dp := DataPayload{
		Payload: []byte("test"),
	}
	asset, _ := ToCatenaAsset(dp, true)
	result, status := Reply(asset)

	if status.Code != OK {
		t.Errorf("Reply status code = %d, want %d", status.Code, OK)
	}
	if result.GetProtoAsset() == nil {
		t.Error("Reply result asset should not be nil")
	}
}

func TestReplyWithCode(t *testing.T) {
	value, _ := ToCatenaValue(int32(42))
	result, status := ReplyWithCode(value, CREATED)

	if status.Code != CREATED {
		t.Errorf("ReplyWithCode status code = %d, want %d", status.Code, CREATED)
	}
	if status.Error != "" {
		t.Errorf("ReplyWithCode status error = %q, want empty", status.Error)
	}
	if result.Value == nil {
		t.Error("ReplyWithCode result value should not be nil")
	}
}

func TestReplyError_CatenaValue(t *testing.T) {
	result, status := ReplyError[CatenaValue](NOT_FOUND, "resource not found")

	if status.Code != NOT_FOUND {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, NOT_FOUND)
	}
	if status.Error != "resource not found" {
		t.Errorf("ReplyError status error = %q, want 'resource not found'", status.Error)
	}
	if result.Value != nil {
		t.Error("ReplyError result value should be nil (zero value)")
	}
}

func TestReplyError_CatenaDevice(t *testing.T) {
	result, status := ReplyError[CatenaDevice](INTERNAL, "internal error")

	if status.Code != INTERNAL {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, INTERNAL)
	}
	if status.Error != "internal error" {
		t.Errorf("ReplyError status error = %q, want 'internal error'", status.Error)
	}
	if result.GetProtoDevice() != nil {
		t.Error("ReplyError result device should be nil (zero value)")
	}
}

func TestReplyError_CatenaAsset(t *testing.T) {
	result, status := ReplyError[CatenaAsset](UNAVAILABLE, "service unavailable")

	if status.Code != UNAVAILABLE {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, UNAVAILABLE)
	}
	if status.Error != "service unavailable" {
		t.Errorf("ReplyError status error = %q, want 'service unavailable'", status.Error)
	}
	if result.GetProtoAsset() != nil {
		t.Error("ReplyError result asset should be nil (zero value)")
	}
}

func TestStatusWithCode(t *testing.T) {
	status := StatusWithCode(INVALID_ARGUMENT, "bad input")

	if status.Code != INVALID_ARGUMENT {
		t.Errorf("StatusWithCode code = %d, want %d", status.Code, INVALID_ARGUMENT)
	}
	if status.Error != "bad input" {
		t.Errorf("StatusWithCode error = %q, want 'bad input'", status.Error)
	}
}

func TestStatusWithCode_NoMessage(t *testing.T) {
	status := StatusWithCode(OK, "")

	if status.Code != OK {
		t.Errorf("StatusWithCode code = %d, want %d", status.Code, OK)
	}
	if status.Error != "" {
		t.Errorf("StatusWithCode error = %q, want empty", status.Error)
	}
}

func TestStatusResult_Fields(t *testing.T) {
	result := StatusResult{
		Code:  NOT_FOUND,
		Error: "not found",
	}

	if result.Code != NOT_FOUND {
		t.Errorf("StatusResult.Code = %d, want %d", result.Code, NOT_FOUND)
	}
	if result.Error != "not found" {
		t.Errorf("StatusResult.Error = %q, want 'not found'", result.Error)
	}
}

// TestStatusCode_Values verifies status codes have expected integer values
func TestStatusCode_Values(t *testing.T) {
	tests := []struct {
		code     StatusCode
		expected int
	}{
		{OK, 0},
		{NOT_FOUND, 5},
		{INTERNAL, 13},
		{INVALID_ARGUMENT, 3},
	}

	for _, tt := range tests {
		if int(tt.code) != tt.expected {
			t.Errorf("StatusCode %d expected, got %d", tt.expected, tt.code)
		}
	}
}

// TestResponseType_Constraint verifies the generic constraint works
func TestResponseType_Constraint(t *testing.T) {
	// These should all compile and work
	var _ func(CatenaValue) (CatenaValue, StatusResult) = Reply[CatenaValue]
	var _ func(CatenaDevice) (CatenaDevice, StatusResult) = Reply[CatenaDevice]
	var _ func(CatenaAsset) (CatenaAsset, StatusResult) = Reply[CatenaAsset]
}

func TestReplyError_AllStatusCodes(t *testing.T) {
	codes := []StatusCode{
		CANCELLED, UNKNOWN, INVALID_ARGUMENT, DEADLINE_EXCEEDED,
		NOT_FOUND, ALREADY_EXISTS, PERMISSION_DENIED, RESOURCE_EXHAUSTED,
		FAILED_PRECONDITION, ABORTED, OUT_OF_RANGE, UNIMPLEMENTED,
		INTERNAL, UNAVAILABLE, DATA_LOSS, UNAUTHENTICATED,
	}

	for i, code := range codes {
		t.Run(http.StatusText(code.ToHTTPStatus()), func(t *testing.T) {
			result, status := ReplyError[CatenaValue](code, "test error")
			if status.Code != code {
				t.Errorf("test %d: ReplyError code = %d, want %d", i, status.Code, code)
			}
			if result.Value != nil {
				t.Error("ReplyError should return zero value")
			}
		})
	}
}
