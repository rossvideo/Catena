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
 * @brief Round-trip tests for wrapper structs with exported Proto fields.
 * @file roundtrip_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"testing"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// TestDevice_ProtoRoundtrip builds a Device directly via the exported Proto
// field and verifies it feeds serialization and ParamInfosForRequest correctly.
func TestDevice_ProtoRoundtrip(t *testing.T) {
	inner := &protos.Device{
		Slot: 3,
		Params: map[string]*protos.Param{
			"gain": {
				Type:  protos.ParamType_INT32,
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 50}},
			},
		},
	}
	d := Device{Proto: inner}

	if d.Proto != inner {
		t.Fatal("Device.Proto is not the assigned pointer")
	}

	// The proto must serialize via protojson.
	data, err := protojson.Marshal(d.Proto)
	if err != nil {
		t.Fatalf("marshal: %v", err)
	}
	if len(data) == 0 {
		t.Error("marshal returned empty JSON")
	}

	// ParamInfosForRequest must find the "gain" param.
	stream := &sliceStream[ParamInfo]{}
	status := ParamInfosForRequest("gain", &d, false, stream)
	if status.Code != StatusCodeOk {
		t.Fatalf("ParamInfosForRequest status: %v", status)
	}
	if len(stream.Items) != 1 {
		t.Fatalf("expected 1 ParamInfo, got %d", len(stream.Items))
	}
	if stream.Items[0].GetOid() != "gain" {
		t.Errorf("expected oid 'gain', got %q", stream.Items[0].GetOid())
	}
}

// TestAsset_ProtoRoundtrip builds an Asset directly via the exported Proto
// field and verifies the struct is accessible and usable.
func TestAsset_ProtoRoundtrip(t *testing.T) {
	inner := &protos.ExternalObjectPayload{
		Cachable: true,
		Payload: &protos.DataPayload{
			Kind:            &protos.DataPayload_Payload{Payload: []byte("hello")},
			PayloadEncoding: protos.DataPayload_UNCOMPRESSED,
		},
	}
	a := Asset{Proto: inner}

	if a.Proto != inner {
		t.Fatal("Asset.Proto is not the assigned pointer")
	}
	if !a.Proto.GetCachable() {
		t.Error("expected Cachable=true")
	}

	// TranscodeAssetPayload should operate on a.Proto in-place.
	res := TranscodeAssetPayload(&a, EncodingGzip)
	if res.Code != StatusCodeOk {
		t.Fatalf("TranscodeAssetPayload: %v", res)
	}
	if Encoding(a.Proto.GetPayload().GetPayloadEncoding()) != EncodingGzip {
		t.Errorf("expected GZIP encoding after transcode, got %v", a.Proto.GetPayload().GetPayloadEncoding())
	}
}

// TestParamInfo_ProtoRoundtrip builds a ParamInfo directly via the exported
// Proto field and verifies the convenience accessors delegate to it.
func TestParamInfo_ProtoRoundtrip(t *testing.T) {
	inner := &protos.ParamInfoResponse{
		Info: &protos.ParamInfo{
			Oid:  "brightness",
			Type: protos.ParamType_INT32,
		},
		ArrayLength: 0,
	}
	pi := ParamInfo{Proto: inner}

	if pi.Proto != inner {
		t.Fatal("ParamInfo.Proto is not the assigned pointer")
	}
	if pi.GetOid() != "brightness" {
		t.Errorf("GetOid = %q, want 'brightness'", pi.GetOid())
	}
	if pi.GetParamType() != ParamTypeInt32 {
		t.Errorf("GetParamType = %v, want INT32", pi.GetParamType())
	}
	if pi.GetArrayLength() != 0 {
		t.Errorf("GetArrayLength = %d, want 0", pi.GetArrayLength())
	}
}

// TestCommandResult_ProtoRoundtrip builds a CommandResult directly via the
// exported Proto field and verifies the behavioral helpers read through it.
func TestCommandResult_ProtoRoundtrip(t *testing.T) {
	inner := &protos.CommandResponse{
		Kind: &protos.CommandResponse_Response{
			Response: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 7}},
		},
	}
	cr := CommandResult{Proto: inner}

	if cr.Proto != inner {
		t.Fatal("CommandResult.Proto is not the assigned pointer")
	}
	if cr.IsEmpty() {
		t.Error("IsEmpty should be false for a response result")
	}
	if cr.IsException() {
		t.Error("IsException should be false for a response result")
	}
	if !proto.Equal(cr.Proto.GetResponse(), inner.GetResponse()) {
		t.Error("Proto.GetResponse() mismatch")
	}
}

// TestValue_ProtoRoundtrip builds a Value directly via the exported Proto
// field and verifies it is preserved through FromProto.
func TestValue_ProtoRoundtrip(t *testing.T) {
	inner := &protos.Value{Kind: &protos.Value_StringValue{StringValue: "catena"}}
	v := Value{Proto: inner}

	if v.Proto != inner {
		t.Fatal("Value.Proto is not the assigned pointer")
	}

	native, status := FromProto(v.Proto)
	if status.Code != StatusCodeOk {
		t.Fatalf("FromProto: %v", status)
	}
	if s, ok := native.(string); !ok || s != "catena" {
		t.Errorf("FromProto = %v (%T), want string 'catena'", native, native)
	}
}
