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

	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestReply_Value(t *testing.T) {
	value, _ := st2138.ToValue(int32(42))
	result, status := Reply(value)

	if status.Code != StatusCodeOk {
		t.Errorf("Reply status code = %d, want %d", status.Code, StatusCodeOk)
	}
	if status.Error != "" {
		t.Errorf("Reply status error = %q, want empty", status.Error)
	}
	if result.Proto == nil {
		t.Error("Reply result value should not be nil")
	}
}

func TestReply_Device(t *testing.T) {
	device := *st2138.NewDevice(0).
		WithDetailLevel(st2138.DetailLevelFull)
	result, status := Reply(device)

	if status.Code != StatusCodeOk {
		t.Errorf("Reply status code = %d, want %d", status.Code, StatusCodeOk)
	}
	if result.Proto == nil {
		t.Error("Reply result device should not be nil")
	}
}

func TestReply_Asset(t *testing.T) {
	dp := st2138.DataPayload{
		Payload: []byte("test"),
	}
	asset, _ := st2138.ToAsset(dp, true)
	result, status := Reply(asset)

	if status.Code != StatusCodeOk {
		t.Errorf("Reply status code = %d, want %d", status.Code, StatusCodeOk)
	}
	if result.Proto == nil {
		t.Error("Reply result asset should not be nil")
	}
}

func TestReplyWithCode(t *testing.T) {
	value, _ := st2138.ToValue(int32(42))
	result, status := ReplyWithCode(value, StatusCodeNotFound)

	if status.Code != StatusCodeNotFound {
		t.Errorf("ReplyWithCode status code = %d, want %d", status.Code, StatusCodeNotFound)
	}
	if status.Error != "" {
		t.Errorf("ReplyWithCode status error = %q, want empty", status.Error)
	}
	if result.Proto == nil {
		t.Error("ReplyWithCode result value should not be nil")
	}
}

func TestReplyError_Value(t *testing.T) {
	result, status := ReplyError[st2138.Value](StatusCodeNotFound, "resource not found")

	if status.Code != StatusCodeNotFound {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, StatusCodeNotFound)
	}
	if status.Error != "resource not found" {
		t.Errorf("ReplyError status error = %q, want 'resource not found'", status.Error)
	}
	if result.Proto != nil {
		t.Error("ReplyError result value should be nil (zero value)")
	}
}

func TestReplyError_Device(t *testing.T) {
	result, status := ReplyError[st2138.Device](StatusCodeInternal, "internal error")

	if status.Code != StatusCodeInternal {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, StatusCodeInternal)
	}
	if status.Error != "internal error" {
		t.Errorf("ReplyError status error = %q, want 'internal error'", status.Error)
	}
	if result.Proto != nil {
		t.Error("ReplyError result device should be nil (zero value)")
	}
}

func TestReplyError_Asset(t *testing.T) {
	result, status := ReplyError[st2138.Asset](StatusCodeUnavailable, "service unavailable")

	if status.Code != StatusCodeUnavailable {
		t.Errorf("ReplyError status code = %d, want %d", status.Code, StatusCodeUnavailable)
	}
	if status.Error != "service unavailable" {
		t.Errorf("ReplyError status error = %q, want 'service unavailable'", status.Error)
	}
	if result.Proto != nil {
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

func TestStatusOk(t *testing.T) {
	status := StatusOk()

	if status.Code != StatusCodeOk {
		t.Errorf("StatusOk code = %d, want %d", status.Code, StatusCodeOk)
	}
	if status.Error != "" {
		t.Errorf("StatusOk error = %q, want empty", status.Error)
	}
}

func TestStatusError(t *testing.T) {
	err := fmt.Errorf("something went wrong")
	status := StatusError(err)

	if status.Code != StatusCodeInternal {
		t.Errorf("StatusError code = %d, want %d", status.Code, StatusCodeInternal)
	}
	if status.Error != "something went wrong" {
		t.Errorf("StatusError error = %q, want 'something went wrong'", status.Error)
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
	var _ func(st2138.Value) (st2138.Value, StatusResult) = Reply[st2138.Value]
	var _ func(st2138.Device) (st2138.Device, StatusResult) = Reply[st2138.Device]
	var _ func(st2138.Asset) (st2138.Asset, StatusResult) = Reply[st2138.Asset]
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
			result, status := ReplyError[st2138.Value](code, "test error")
			if status.Code != code {
				t.Errorf("test %d: ReplyError code = %d, want %d", i, status.Code, code)
			}
			if result.Proto != nil {
				t.Error("ReplyError should return zero value")
			}
		})
	}
}
