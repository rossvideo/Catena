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
 * @brief Status code handling for the Catena SDK.
 * @file status_code_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"fmt"
	"testing"
)

func TestReply_Value(t *testing.T) {
	value, _ := ToValue(int32(42))
	result, status := Reply(value)

	if status.Code != StatusCodeOk {
		t.Errorf("Reply status code = %d, want %d", status.Code, StatusCodeOk)
	}
	if status.Error != "" {
		t.Errorf("Reply status error = %q, want empty", status.Error)
	}
	if result.Value == nil {
		t.Error("Reply result value should not be nil")
	}
}

func TestReply_Device(t *testing.T) {
	device := *NewDevice(0, "Camera", "Ross Video", "1.0", "SN-12345").
		WithDetailLevel(DetailLevelFull)
	result, status := Reply(device)

	if status.Code != StatusCodeOk {
		t.Errorf("Reply status code = %d, want %d", status.Code, StatusCodeOk)
	}
	if result.ToProtoDevice() == nil {
		t.Error("Reply result device should not be nil")
	}
}

func TestReply_Asset(t *testing.T) {
	dp := DataPayload{
		Payload: []byte("test"),
	}
	asset, _ := ToAsset(dp, true)
	result, status := Reply(asset)

	if status.Code != StatusCodeOk {
		t.Errorf("Reply status code = %d, want %d", status.Code, StatusCodeOk)
	}
	if result.GetProtoAsset() == nil {
		t.Error("Reply result asset should not be nil")
	}
}

func TestReplyWithCode(t *testing.T) {
	value, _ := ToValue(int32(42))
	result, status := ReplyWithCode(value, StatusCodeNotFound)

	if status.Code != StatusCodeNotFound {
		t.Errorf("ReplyWithCode status code = %d, want %d", status.Code, StatusCodeNotFound)
	}
	if status.Error != "" {
		t.Errorf("ReplyWithCode status error = %q, want empty", status.Error)
	}
	if result.Value == nil {
		t.Error("ReplyWithCode result value should not be nil")
	}
}

func TestReplyError_Value(t *testing.T) {
	result, status := ReplyError[Value](StatusCodeNotFound, "resource not found")

	if status.Code != StatusCodeNotFound {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, StatusCodeNotFound)
	}
	if status.Error != "resource not found" {
		t.Errorf("ReplyError status error = %q, want 'resource not found'", status.Error)
	}
	if result.Value != nil {
		t.Error("ReplyError result value should be nil (zero value)")
	}
}

func TestReplyError_Device(t *testing.T) {
	result, status := ReplyError[Device](StatusCodeInternal, "internal error")

	if status.Code != StatusCodeInternal {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, StatusCodeInternal)
	}
	if status.Error != "internal error" {
		t.Errorf("ReplyError status error = %q, want 'internal error'", status.Error)
	}
	if result.ToProtoDevice() != nil {
		t.Error("ReplyError result device should be nil (zero value)")
	}
}

func TestReplyError_Asset(t *testing.T) {
	result, status := ReplyError[Asset](StatusCodeUnavailable, "service unavailable")

	if status.Code != StatusCodeUnavailable {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, StatusCodeUnavailable)
	}
	if status.Error != "service unavailable" {
		t.Errorf("ReplyError status error = %q, want 'service unavailable'", status.Error)
	}
	if result.GetProtoAsset() != nil {
		t.Error("ReplyError result asset should be nil (zero value)")
	}
}

func TestStatusWithCode(t *testing.T) {
	status := StatusWithCode(StatusCodeInvalidArgument, "bad input")

	if status.Code != StatusCodeInvalidArgument {
		t.Errorf("StatusWithCode code = %d, want %d", status.Code, StatusCodeInvalidArgument)
	}
	if status.Error != "bad input" {
		t.Errorf("StatusWithCode error = %q, want 'bad input'", status.Error)
	}
}

func TestStatusWithCode_NoMessage(t *testing.T) {
	status := StatusWithCode(StatusCodeOk, "")

	if status.Code != StatusCodeOk {
		t.Errorf("StatusWithCode code = %d, want %d", status.Code, StatusCodeOk)
	}
	if status.Error != "" {
		t.Errorf("StatusWithCode error = %q, want empty", status.Error)
	}
}

func TestStatusResult_Fields(t *testing.T) {
	result := StatusResult{
		Code:  StatusCodeNotFound,
		Error: "not found",
	}

	if result.Code != StatusCodeNotFound {
		t.Errorf("StatusResult.Code = %d, want %d", result.Code, StatusCodeNotFound)
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
		{StatusCodeOk, 0},
		{StatusCodeNotFound, 5},
		{StatusCodeInternal, 13},
		{StatusCodeInvalidArgument, 3},
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
	var _ func(Value) (Value, StatusResult) = Reply[Value]
	var _ func(Device) (Device, StatusResult) = Reply[Device]
	var _ func(Asset) (Asset, StatusResult) = Reply[Asset]
}

func TestReplyError_AllStatusCodes(t *testing.T) {
	codes := []StatusCode{
		StatusCodeCancelled, StatusCodeUnknown, StatusCodeInvalidArgument, StatusCodeDeadlineExceeded,
		StatusCodeNotFound, StatusCodeAlreadyExists, StatusCodePermissionDenied, StatusCodeResourceExhausted,
		StatusCodeFailedPrecondition, StatusCodeAborted, StatusCodeOutOfRange, StatusCodeUnimplemented,
		StatusCodeInternal, StatusCodeUnavailable, StatusCodeDataLoss, StatusCodeUnauthenticated,
	}

	for i, code := range codes {
		t.Run(fmt.Sprintf("StatusCode_%d", code), func(t *testing.T) {
			result, status := ReplyError[Value](code, "test error")
			if status.Code != code {
				t.Errorf("test %d: ReplyError code = %d, want %d", i, status.Code, code)
			}
			if result.Value != nil {
				t.Error("ReplyError should return zero value")
			}
		})
	}
}
