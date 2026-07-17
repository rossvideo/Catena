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
 * @brief Tests for ParamInfo handling.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-12
 * @file param_info_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package st2138

import (
	"errors"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewParamInfo_Basic(t *testing.T) {
	pi := NewParamInfo("text_box", NewPolyglotText("en", "Text Box"), ParamTypeString, "tpl-1", 0)

	if pi.GetOid() != "text_box" {
		t.Errorf("expected oid 'text_box', got %s", pi.GetOid())
	}
	if pi.GetParamType() != ParamTypeString {
		t.Errorf("expected type STRING, got %v", pi.GetParamType())
	}
	if pi.GetTemplateOid() != "tpl-1" {
		t.Errorf("expected template_oid 'tpl-1', got %s", pi.GetTemplateOid())
	}
	if pi.GetArrayLength() != 0 {
		t.Errorf("expected array_length 0, got %d", pi.GetArrayLength())
	}
	if pi.Proto == nil {
		t.Fatal("expected non-nil proto response")
	}
	if pi.Proto.GetInfo() == nil {
		t.Fatal("expected non-nil proto info")
	}
	name := pi.Proto.GetInfo().GetName()
	if name == nil || name.GetDisplayStrings()["en"] != "Text Box" {
		t.Errorf("expected display name 'Text Box' for en, got %v", name)
	}
}

func TestNewParamInfo_NilName(t *testing.T) {
	pi := NewParamInfo("foo", nil, ParamTypeInt32, "", 0)
	if pi.Proto.GetInfo().GetName() != nil {
		t.Error("expected nil name when none was provided")
	}
}

func TestNewParamInfo_ArrayLength(t *testing.T) {
	pi := NewParamInfo("arr", nil, ParamTypeStringArray, "", 7)
	if pi.GetArrayLength() != 7 {
		t.Errorf("expected array_length 7, got %d", pi.GetArrayLength())
	}
}

func TestParamInfo_ZeroValue(t *testing.T) {
	var pi ParamInfo
	if pi.Proto != nil {
		t.Error("expected nil proto response for zero value")
	}
	if pi.Proto.GetInfo() != nil {
		t.Error("expected nil proto info for zero value")
	}
	if pi.GetOid() != "" {
		t.Errorf("expected empty oid, got %q", pi.GetOid())
	}
	if pi.GetArrayLength() != 0 {
		t.Errorf("expected array_length 0, got %d", pi.GetArrayLength())
	}
}

func TestParamInfosForRequest(t *testing.T) {
	t.Run("RootNonRecursive", func(t *testing.T) {
		stream := &sliceStream[ParamInfo]{}
		res := ParamInfosForRequest("", testDeviceDefinition(), false, stream)
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
		}

		assertParamInfoOids(t, stream.Items, []string{"alpha", "floats", "numbers", "parent", "strings", "structs", "variants"})
	})

	t.Run("RootRecursive", func(t *testing.T) {
		stream := &sliceStream[ParamInfo]{}
		res := ParamInfosForRequest("", testDeviceDefinition(), true, stream)
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
		}

		// each node is emitted immediately followed by its subtree, so parent/child
		// appears right after parent, before the alphabetically-later siblings.
		assertParamInfoOids(t, stream.Items, []string{"alpha", "floats", "numbers", "parent", "parent/child", "strings", "structs", "variants"})
	})

	t.Run("NestedRecursive", func(t *testing.T) {
		stream := &sliceStream[ParamInfo]{}
		res := ParamInfosForRequest("parent", testDeviceDefinition(), true, stream)
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
		}

		assertParamInfoOids(t, stream.Items, []string{"parent", "parent/child"})
		if stream.Items[0].GetParamType() != ParamTypeStruct {
			t.Errorf("expected parent type STRUCT, got %v", stream.Items[0].GetParamType())
		}
		if stream.Items[0].GetArrayLength() != 0 {
			t.Errorf("expected non-array parent array_length 0, got %d", stream.Items[0].GetArrayLength())
		}
	})

	t.Run("ArrayLengthFromValue", func(t *testing.T) {
		// One case per array ParamType so paramArrayLength's whole switch is walked;
		// each length is derived from the value the shared device carries.
		cases := []struct {
			oid  string
			want uint32
		}{
			{"numbers", 3},
			{"floats", 2},
			{"strings", 4},
			{"structs", 2},
			{"variants", 1},
		}
		for _, c := range cases {
			stream := &sliceStream[ParamInfo]{}
			res := ParamInfosForRequest(c.oid, testDeviceDefinition(), false, stream)
			if res.Code != StatusCodeOk {
				t.Fatalf("%s: expected OK, got %v: %s", c.oid, res.Code, res.Error)
			}
			if got := stream.Items[0].GetArrayLength(); got != c.want {
				t.Errorf("%s: expected array_length %d from value length, got %d", c.oid, c.want, got)
			}
		}
	})

	t.Run("MissingParam", func(t *testing.T) {
		stream := &sliceStream[ParamInfo]{}
		res := ParamInfosForRequest("missing", testDeviceDefinition(), false, stream)
		if res.Code != StatusCodeNotFound {
			t.Fatalf("expected NOT_FOUND, got %v: %s", res.Code, res.Error)
		}
		if len(stream.Items) != 0 {
			t.Fatalf("expected no infos, got %d", len(stream.Items))
		}
	})

	t.Run("NilDevice", func(t *testing.T) {
		stream := &sliceStream[ParamInfo]{}
		res := ParamInfosForRequest("", nil, false, stream)
		if res.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL, got %v: %s", res.Code, res.Error)
		}
		if len(stream.Items) != 0 {
			t.Fatalf("expected no infos, got %d", len(stream.Items))
		}
	})

	t.Run("RootSendError", func(t *testing.T) {
		// FailAfter 0 fails the very first Send while walking the whole tree.
		stream := &sliceStream[ParamInfo]{Err: errors.New("boom"), FailAfter: 0}
		res := ParamInfosForRequest("", testDeviceDefinition(), false, stream)
		if res.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL from a failed Send, got %v: %s", res.Code, res.Error)
		}
	})

	t.Run("SpecificSendError", func(t *testing.T) {
		// FailAfter 0 fails the single descriptor Send on the specific-oid path.
		stream := &sliceStream[ParamInfo]{Err: errors.New("boom"), FailAfter: 0}
		res := ParamInfosForRequest("alpha", testDeviceDefinition(), false, stream)
		if res.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL from a failed Send, got %v: %s", res.Code, res.Error)
		}
	})

	t.Run("RecursiveChildSendError", func(t *testing.T) {
		// The parent descriptor is accepted (FailAfter 1); the Send while recursing
		// into its children then fails.
		stream := &sliceStream[ParamInfo]{Err: errors.New("boom"), FailAfter: 1}
		res := ParamInfosForRequest("parent", testDeviceDefinition(), true, stream)
		if res.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL from a failed child Send, got %v: %s", res.Code, res.Error)
		}
		if len(stream.Items) != 1 {
			t.Fatalf("expected only the parent to be recorded before the failure, got %d", len(stream.Items))
		}
	})

	t.Run("RootRecursiveNestedSendError", func(t *testing.T) {
		// Root-recursive walk: alpha, floats, numbers, parent are accepted
		// (FailAfter 4), then the nested recursion into parent's child fails,
		// exercising the error return inside the recursive sendParamInfos call.
		stream := &sliceStream[ParamInfo]{Err: errors.New("boom"), FailAfter: 4}
		res := ParamInfosForRequest("", testDeviceDefinition(), true, stream)
		if res.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL from a failed nested Send, got %v: %s", res.Code, res.Error)
		}
	})
}

func TestSendParamInfos_SkipsNilParam(t *testing.T) {
	// A nil param descriptor in the map must be skipped, not sent or dereferenced.
	stream := &sliceStream[ParamInfo]{}
	if err := sendParamInfos(map[string]*protos.Param{"ghost": nil}, "", true, stream); err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if len(stream.Items) != 0 {
		t.Errorf("expected nil param to be skipped, got %d infos", len(stream.Items))
	}
}

func TestFindParamDescriptor_EmptyOid(t *testing.T) {
	// An empty oid has no descriptor to resolve; the guard returns (nil, false).
	params := testDeviceDefinition().Proto.GetParams()
	if _, ok := findParamDescriptor(params, ""); ok {
		t.Error("expected findParamDescriptor to report not-found for an empty oid")
	}
}

func testDeviceDefinition() *Device {
	return NewDevice(0).
		WithParam("parent", NewParamStruct(nil).
			WithName(NewPolyglotText("en", "Parent")).
			WithParam("child", NewParamString(""))).
		WithParam("alpha", NewParamInt32(0).
			WithName(NewPolyglotText("en", "Alpha"))).
		WithParam("numbers", NewParamInt32Array([]int32{1, 2, 3})).
		WithParam("floats", NewParamFloat32Array([]float32{1.5, 2.5})).
		WithParam("strings", NewParamStringArray([]string{"a", "b", "c", "d"})).
		WithParam("structs", NewParamStructArray([]map[string]any{{"x": int32(1)}, {"x": int32(2)}})).
		WithParam("variants", NewParamStructVariantArray([]StructVariantValue{{StructVariantType: "type_a", Value: int32(9)}}))
}

func assertParamInfoOids(t *testing.T, infos []ParamInfo, expected []string) {
	t.Helper()

	if len(infos) != len(expected) {
		t.Fatalf("expected %d infos, got %d", len(expected), len(infos))
	}
	for i, oid := range expected {
		if infos[i].GetOid() != oid {
			t.Errorf("expected oid[%d] %q, got %q", i, oid, infos[i].GetOid())
		}
	}
}

func TestParamInfo_Wire(t *testing.T) {
	info := NewParamInfo("gain", nil, ParamTypeInt32, "", 3)

	wire := info.Wire()
	if wire == nil {
		t.Fatal("Wire() returned nil")
	}
	if wire != info.Proto {
		t.Error("Wire() should return the same ParamInfoResponse as GetProtoResponse()")
	}
}
