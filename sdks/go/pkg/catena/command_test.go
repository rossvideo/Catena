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
 * @brief Tests for command response types.
 * @file command_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-03-10
 */

package catena

import (
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewPolyglotText(t *testing.T) {
	pt := NewPolyglotText("en", "Hello")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
}

func TestPolyglotText_With(t *testing.T) {
	pt := NewPolyglotText("en", "Hello").With("fr", "Bonjour")
	if pt["en"] != "Hello" {
		t.Errorf("expected 'Hello', got %s", pt["en"])
	}
	if pt["fr"] != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt["fr"])
	}
}

func TestPolyglotText_Get(t *testing.T) {
	pt := NewPolyglotText("en", "Hello").With("fr", "Bonjour")
	if pt.Get("fr", "en") != "Bonjour" {
		t.Errorf("expected 'Bonjour', got %s", pt.Get("fr", "en"))
	}
	if pt.Get("de", "en") != "Hello" {
		t.Errorf("expected fallback 'Hello', got %s", pt.Get("de", "en"))
	}
}

func TestPolyglotText_Get_MissingFallback(t *testing.T) {
	pt := NewPolyglotText("en", "Hello")
	if pt.Get("de", "fr") != "" {
		t.Errorf("expected empty string for missing fallback, got %s", pt.Get("de", "fr"))
	}
}

func TestCommandReply(t *testing.T) {
	val, _ := ToValue(int32(42))
	result, status := CommandReply(val)

	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if result.IsEmpty() {
		t.Error("expected non-empty result")
	}
	if result.IsException() {
		t.Error("expected non-exception result")
	}

	proto := result.GetProtoResponse()
	resp := proto.GetResponse()
	if resp == nil {
		t.Fatal("expected response in proto")
	}
	if resp.GetInt32Value() != 42 {
		t.Errorf("expected int32_value 42, got %v", resp.GetInt32Value())
	}
}

func TestCommandNoResponse(t *testing.T) {
	result, status := CommandNoResponse()

	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if !result.IsEmpty() {
		t.Error("expected empty result")
	}
	if result.IsException() {
		t.Error("expected non-exception result")
	}

	proto := result.GetProtoResponse()
	if proto.GetNoResponse() == nil {
		t.Error("expected no_response in proto")
	}
	if proto.GetResponse() != nil {
		t.Error("expected nil response in proto")
	}
	if proto.GetException() != nil {
		t.Error("expected nil exception in proto")
	}
}

func TestCommandExceptionResult(t *testing.T) {
	errorMsg := NewPolyglotText("en", "Something failed")
	result, status := CommandExceptionResult("TestError", "detailed info", errorMsg)

	if status.Code != OK {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if result.IsEmpty() {
		t.Error("expected non-empty result")
	}
	if !result.IsException() {
		t.Error("expected exception result")
	}

	exc := result.GetException()
	if exc.GetType() != "TestError" {
		t.Errorf("expected type 'TestError', got %s", exc.GetType())
	}
	if exc.GetDetails() != "detailed info" {
		t.Errorf("expected details 'detailed info', got %s", exc.GetDetails())
	}
	if exc.GetErrorMessage().GetDisplayStrings()["en"] != "Something failed" {
		t.Errorf("expected en error message 'Something failed', got %s",
			exc.GetErrorMessage().GetDisplayStrings()["en"])
	}

	proto := result.GetProtoResponse()
	protoExc := proto.GetException()
	if protoExc == nil {
		t.Fatal("expected exception in proto")
	}
	if protoExc.GetType() != "TestError" {
		t.Errorf("expected proto type 'TestError', got %s", protoExc.GetType())
	}
	if protoExc.GetDetails() != "detailed info" {
		t.Errorf("expected proto details 'detailed info', got %s", protoExc.GetDetails())
	}
	if protoExc.GetErrorMessage().GetDisplayStrings()["en"] != "Something failed" {
		t.Errorf("expected proto en display string 'Something failed', got %s",
			protoExc.GetErrorMessage().GetDisplayStrings()["en"])
	}
}

func TestCommandExceptionResult_MultipleLanguages(t *testing.T) {
	errorMsg := NewPolyglotText("en", "Command not found").With("fr", "Commande introuvable")
	result, _ := CommandExceptionResult("NotFound", "missing", errorMsg)

	exc := result.GetException()
	ds := exc.GetErrorMessage().GetDisplayStrings()
	if ds["en"] != "Command not found" {
		t.Errorf("expected en='Command not found', got %s", ds["en"])
	}
	if ds["fr"] != "Commande introuvable" {
		t.Errorf("expected fr='Commande introuvable', got %s", ds["fr"])
	}
}

func TestCommandExceptionResult_NilErrorMessage(t *testing.T) {
	result, _ := CommandExceptionResult("Error", "details", nil)

	proto := result.GetProtoResponse()
	protoExc := proto.GetException()
	if protoExc == nil {
		t.Fatal("expected exception in proto")
	}
	if protoExc.GetErrorMessage() != nil {
		t.Errorf("expected nil error_message when none provided, got %v", protoExc.GetErrorMessage())
	}
}

func TestCommandError(t *testing.T) {
	result, status := CommandError(UNIMPLEMENTED, "not implemented")

	if status.Code != UNIMPLEMENTED {
		t.Errorf("expected UNIMPLEMENTED status, got %v", status.Code)
	}
	if status.Error != "not implemented" {
		t.Errorf("expected error 'not implemented', got %s", status.Error)
	}
	if !result.IsEmpty() {
		t.Error("expected empty result for CommandError")
	}
}

func TestCommandError_NilResponse(t *testing.T) {
	result, _ := CommandError(INTERNAL, "error")
	if result.GetProtoResponse() != nil {
		t.Error("expected nil proto response for CommandError")
	}
	if result.GetException() != nil {
		t.Error("expected nil exception for CommandError")
	}
}

func TestCommandResult_GetProtoResponse_Response(t *testing.T) {
	val, _ := ToValue("hello")
	result, _ := CommandReply(val)
	proto := result.GetProtoResponse()

	if _, ok := proto.Kind.(*protos.CommandResponse_Response); !ok {
		t.Errorf("expected CommandResponse_Response, got %T", proto.Kind)
	}
	if proto.GetResponse().GetStringValue() != "hello" {
		t.Errorf("expected string_value 'hello', got %v", proto.GetResponse().GetStringValue())
	}
}

func TestCommandResult_GetProtoResponse_NoResponse(t *testing.T) {
	result, _ := CommandNoResponse()
	proto := result.GetProtoResponse()

	if _, ok := proto.Kind.(*protos.CommandResponse_NoResponse); !ok {
		t.Errorf("expected CommandResponse_NoResponse, got %T", proto.Kind)
	}
}

func TestCommandResult_GetProtoResponse_Exception(t *testing.T) {
	result, _ := CommandExceptionResult("E", "d", NewPolyglotText("fr", "erreur"))
	proto := result.GetProtoResponse()

	if _, ok := proto.Kind.(*protos.CommandResponse_Exception); !ok {
		t.Errorf("expected CommandResponse_Exception, got %T", proto.Kind)
	}
	exc := proto.GetException()
	if exc.GetType() != "E" {
		t.Errorf("expected type 'E', got %s", exc.GetType())
	}
	if exc.GetErrorMessage().GetDisplayStrings()["fr"] != "erreur" {
		t.Errorf("expected fr='erreur', got %s", exc.GetErrorMessage().GetDisplayStrings()["fr"])
	}
}
