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
 * @brief Command response types for the Catena SDK.
 * @file command.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-10
 */

package st2138

import (
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"google.golang.org/protobuf/proto"
)

// CommandResponse wraps protos.CommandResponse, representing the three possible
// outcomes of ExecuteCommand: no_response, response, or exception.
// Proto is the underlying proto message; it may be read or replaced directly.
type CommandResponse struct {
	Proto *protos.CommandResponse
}

// ensure CommandResponse implements the Message interface.
var _ Message = CommandResponse{}

func (r CommandResponse) Wire() proto.Message {
	return r.Proto
}

// IsEmpty returns true if this is a no_response result.
func (r CommandResponse) IsEmpty() bool {
	return r.Proto == nil || r.Proto.GetNoResponse() != nil
}

// IsException returns true if this is an exception result.
func (r CommandResponse) IsException() bool {
	return r.Proto != nil && r.Proto.GetException() != nil
}

// GetException returns the underlying proto Exception.
// Only valid when IsException() is true.
func (r CommandResponse) GetException() *protos.Exception {
	return r.Proto.GetException()
}

// CommandValue returns a successful command response wrapping a value.
func CommandValue(value Value) CommandResponse {
	return CommandResponse{
		Proto: &protos.CommandResponse{
			Kind: &protos.CommandResponse_Response{Response: value.Proto},
		},
	}
}

// CommandNoResponse returns an empty command response (no_response).
func CommandNoResponse() CommandResponse {
	return CommandResponse{
		Proto: &protos.CommandResponse{
			Kind: &protos.CommandResponse_NoResponse{NoResponse: &protos.Empty{}},
		},
	}
}

// CommandException returns a command exception response.
// exType is the exception type, details provides additional context,
// and errorMessage is a PolyglotText map of language code to display string (may be nil).
func CommandException(exType, details string, errorMessage PolyglotText) CommandResponse {
	exc := &protos.Exception{
		Type:    exType,
		Details: details,
	}
	if errorMessage != nil {
		exc.ErrorMessage = &protos.PolyglotText{DisplayStrings: errorMessage}
	}
	return CommandResponse{
		Proto: &protos.CommandResponse{
			Kind: &protos.CommandResponse_Exception{Exception: exc},
		},
	}
}
