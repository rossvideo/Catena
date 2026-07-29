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
 * @brief Tests for SDK-managed product struct dispatch in the server.
 * @file product_server_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"testing"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func testProduct() ProductStruct {
	return ProductStruct{
		Name:         "Camera",
		Vendor:       "Ross Video",
		Version:      "1.0",
		SerialNumber: "SN-12345",
	}
}

// expectedProductParam is the golden proto for the SDK-managed product param
// built from testProduct(): a read-only STRUCT with STRING field descriptors and
// the field values carried in the struct's Value. Because the struct itself is
// read-only, its field descriptors inherit read_only too.
func expectedProductParam() *protos.Param {
	return &protos.Param{
		Type:        protos.ParamType_STRUCT,
		ReadOnly:    true,
		AccessScope: st2138.ScopeMon,
		Params: map[string]*protos.Param{
			ProductOidName:             {Type: protos.ParamType_STRING, ReadOnly: true},
			ProductOidVendor:           {Type: protos.ParamType_STRING, ReadOnly: true},
			ProductOidVersion:          {Type: protos.ParamType_STRING, ReadOnly: true},
			ProductOidSerialNumber:     {Type: protos.ParamType_STRING, ReadOnly: true},
			ProductOidCatenaSDKVersion: {Type: protos.ParamType_STRING, ReadOnly: true},
			ProductOidCatenaSDK:        {Type: protos.ParamType_STRING, ReadOnly: true},
		},
		Value: &protos.Value{
			Kind: &protos.Value_StructValue{
				StructValue: &protos.StructValue{
					Fields: map[string]*protos.Value{
						ProductOidName:             {Kind: &protos.Value_StringValue{StringValue: "Camera"}},
						ProductOidVendor:           {Kind: &protos.Value_StringValue{StringValue: "Ross Video"}},
						ProductOidVersion:          {Kind: &protos.Value_StringValue{StringValue: "1.0"}},
						ProductOidSerialNumber:     {Kind: &protos.Value_StringValue{StringValue: "SN-12345"}},
						ProductOidCatenaSDKVersion: {Kind: &protos.Value_StringValue{StringValue: SDKVersion}},
						ProductOidCatenaSDK:        {Kind: &protos.Value_StringValue{StringValue: CatenaSDKURL}},
					},
				},
			},
		},
	}
}

// TestServer_GetDevice_InjectsProduct verifies GetDevice overwrites the product
// param in whatever the business logic returned with the SDK-managed product.
func TestServer_GetDevice_InjectsProduct(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (st2138.Device, StatusResult) {
		// Business logic returns a device with a bogus product that must be
		// overwritten, plus its own param that must be preserved.
		return Reply(*st2138.NewDevice(0).
			WithParam("product", st2138.NewParamString("wrong")).
			WithParam("brightness", st2138.NewParamInt32(50)))
	})

	device, res := srv.InvokeGetDeviceHandler(0, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}

	params := device.Proto.GetParams()
	if _, ok := params["brightness"]; !ok {
		t.Error("expected business-logic 'brightness' param to be preserved")
	}
	product, ok := params["product"]
	if !ok {
		t.Fatal("expected SDK-injected 'product' param")
	}
	if !proto.Equal(product, expectedProductParam()) {
		t.Errorf("SDK-injected product param does not match expected, got %v", product)
	}
}

// TestServer_GetDevice_NoProductRegistered verifies the device is passed through
// untouched when no product is registered for the slot.
func TestServer_GetDevice_NoProductRegistered(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (st2138.Device, StatusResult) {
		return Reply(*st2138.NewDevice(0).WithParam("brightness", st2138.NewParamInt32(50)))
	})

	device, res := srv.InvokeGetDeviceHandler(0, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if _, ok := device.Proto.GetParams()["product"]; ok {
		t.Error("did not expect a product param when none is registered")
	}
}

// TestServer_GetValue_Product verifies product/* reads are answered by the SDK
// without invoking the business-logic GetValue handler.
func TestServer_GetValue_Product(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (st2138.Value, StatusResult) {
		t.Errorf("business-logic GetValue should not be called for %q", fqoid)
		return ReplyError[st2138.Value](StatusCodeInternal, "should not happen")
	})

	cases := map[string]string{
		"product/name":               "Camera",
		"product/serial_number":      "SN-12345",
		"product/catena_sdk":         CatenaSDKURL,
		"product/catena_sdk_version": SDKVersion,
	}
	for fqoid, want := range cases {
		value, res := srv.InvokeGetValueHandler(0, fqoid, TransportContext{})
		if res.Code != StatusCodeOk {
			t.Fatalf("%s: expected OK, got %v: %s", fqoid, res.Code, res.Error)
		}
		if got := value.Proto.GetStringValue(); got != want {
			t.Errorf("%s = %q, want %q", fqoid, got, want)
		}
	}

	// The whole struct is returned for the bare "product" oid.
	value, res := srv.InvokeGetValueHandler(0, "product", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("product: expected OK, got %v: %s", res.Code, res.Error)
	}
	if !proto.Equal(value.Proto, expectedProductParam().GetValue()) {
		t.Errorf("bare 'product' value does not match expected, got %v", value.Proto)
	}
}

// TestServer_GetValue_ProductUnknownField verifies an unknown product sub-field
// is NotFound.
func TestServer_GetValue_ProductUnknownField(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())

	_, res := srv.InvokeGetValueHandler(0, "product/nope", TransportContext{})
	if res.Code != StatusCodeNotFound {
		t.Fatalf("expected NOT_FOUND, got %v: %s", res.Code, res.Error)
	}
}

// TestServer_GetValue_NonProductCallsHandler verifies non-product reads still go
// to the business-logic handler.
func TestServer_GetValue_NonProductCallsHandler(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	called := false
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (st2138.Value, StatusResult) {
		called = true
		v, _ := st2138.ToValue(int32(7))
		return Reply(v)
	})

	value, res := srv.InvokeGetValueHandler(0, "brightness", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if !called {
		t.Error("expected business-logic handler to be called for non-product oid")
	}
	if value.Proto.GetInt32Value() != 7 {
		t.Errorf("brightness = %d, want 7", value.Proto.GetInt32Value())
	}
}

// TestServer_GetParam_Product verifies product/* GetParam requests are answered
// by the SDK without invoking the business-logic GetParam handler.
func TestServer_GetParam_Product(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterGetParamHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (st2138.Param, StatusResult) {
		t.Errorf("business-logic GetParam should not be called for %q", fqoid)
		return st2138.Param{}, StatusWithCode(StatusCodeInternal, "should not happen")
	})

	// Bare "product" returns the full golden proto.
	param, res := srv.InvokeGetParamHandler(0, "product", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("product: expected OK, got %v: %s", res.Code, res.Error)
	}
	if !proto.Equal(param.Proto, expectedProductParam()) {
		t.Errorf("bare 'product' param does not match expected, got %v", param.Proto)
	}

	// A single field returns a STRING param carrying that field's value.
	param, res = srv.InvokeGetParamHandler(0, "product/name", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("product/name: expected OK, got %v: %s", res.Code, res.Error)
	}
	if param.Proto.GetType() != st2138.ParamTypeString {
		t.Errorf("product/name type = %v, want STRING", param.Proto.GetType())
	}
	if got := param.Proto.GetValue().GetStringValue(); got != "Camera" {
		t.Errorf("product/name = %q, want %q", got, "Camera")
	}

	// An unknown field is NotFound.
	_, res = srv.InvokeGetParamHandler(0, "product/nope", TransportContext{})
	if res.Code != StatusCodeNotFound {
		t.Fatalf("product/nope: expected NOT_FOUND, got %v: %s", res.Code, res.Error)
	}
}

// TestServer_GetParam_NoProductRegistered verifies product/* GetParam falls
// through to the business-logic handler when no product is registered.
func TestServer_GetParam_NoProductRegistered(t *testing.T) {
	srv := newTestServer(t, false)
	called := false
	srv.RegisterGetParamHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (st2138.Param, StatusResult) {
		called = true
		return *st2138.NewParamString("bl"), StatusWithCode(StatusCodeOk, "")
	})

	_, res := srv.InvokeGetParamHandler(0, "product", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if !called {
		t.Error("expected business-logic GetParam to be called when no product registered")
	}
}

// TestServer_SetValue_ProductRejected verifies writes to product/* are rejected
// with PermissionDenied when a product is registered.
func TestServer_SetValue_ProductRejected(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterSetValueHandler(0, func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
		t.Error("business-logic SetValue should not be called for product writes")
		return StatusWithCode(StatusCodeInternal, "should not happen")
	})

	res := srv.InvokeSetValueHandler(0, []SetValueEntry{{Fqoid: "product/name", Value: "hacked"}}, TransportContext{})
	if res.Code != StatusCodePermissionDenied {
		t.Fatalf("expected PERMISSION_DENIED, got %v: %s", res.Code, res.Error)
	}
}

// TestServer_SetValue_ProductRejectedWithoutRegistration verifies writes to
// product/* are rejected even when no product is registered, since the product
// struct is always read-only regardless of who manages it.
func TestServer_SetValue_ProductRejectedWithoutRegistration(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterSetValueHandler(0, func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
		t.Error("business-logic SetValue should not be called for product writes")
		return StatusWithCode(StatusCodeInternal, "should not happen")
	})

	res := srv.InvokeSetValueHandler(0, []SetValueEntry{{Fqoid: "product/name", Value: "hacked"}}, TransportContext{})
	if res.Code != StatusCodePermissionDenied {
		t.Fatalf("expected PERMISSION_DENIED, got %v: %s", res.Code, res.Error)
	}
}

// TestServer_SetValue_NonProductAllowed verifies non-product writes reach the
// business-logic handler even when a product is registered.
func TestServer_SetValue_NonProductAllowed(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	called := false
	srv.RegisterSetValueHandler(0, func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult {
		called = true
		return StatusWithCode(StatusCodeOk, "")
	})

	res := srv.InvokeSetValueHandler(0, []SetValueEntry{{Fqoid: "brightness", Value: int32(1)}}, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if !called {
		t.Error("expected business-logic SetValue to be called for non-product write")
	}
}

// TestServer_ParamInfo_Product verifies product/* ParamInfo is answered by the
// SDK with the full product subtree.
func TestServer_ParamInfo_Product(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext, stream Stream[st2138.ParamInfo]) StatusResult {
		t.Errorf("business-logic ParamInfo should not be called for %q", oidPrefix)
		return StatusWithCode(StatusCodeInternal, "should not happen")
	})

	stream := &sliceStream[st2138.ParamInfo]{}
	res := srv.InvokeParamInfoHandler(0, "product", true, stream, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	// product + its 6 sub-params.
	if len(stream.Items) != 7 {
		t.Fatalf("expected 7 param infos, got %d", len(stream.Items))
	}
	if stream.Items[0].GetOid() != "product" {
		t.Errorf("expected first oid 'product', got %q", stream.Items[0].GetOid())
	}
	if stream.Items[0].GetParamType() != st2138.ParamTypeStruct {
		t.Errorf("expected product STRUCT, got %v", stream.Items[0].GetParamType())
	}
}

// monScopeContext is an authz-enabled HandlerContext carrying only the monitor
// read scope, which is what the product param requires.
func monScopeContext() HandlerContext {
	return HandlerContext{readScopes: map[string]struct{}{st2138.ScopeMon: {}}, authzEnabled: true}
}

// nonMonScopeContext is an authz-enabled HandlerContext holding a read scope
// other than monitor, so product reads must be denied.
func nonMonScopeContext() HandlerContext {
	return HandlerContext{readScopes: map[string]struct{}{st2138.ScopeOp: {}}, authzEnabled: true}
}

// TestServer_GetDevice_ProductScope verifies GetDevice injects the product param
// only for callers holding the monitor scope, and strips any product param for
// callers without it, while preserving the rest of the device.
func TestServer_GetDevice_ProductScope(t *testing.T) {
	newDeviceServer := func(t *testing.T, ctx HandlerContext) (*server, *bool) {
		t.Helper()
		srv := newTestServer(t, true)
		srv.RegisterProductStruct(0, testProduct())
		called := false
		srv.RegisterGetDeviceHandler(0, func(slot uint16, hctx HandlerContext) (st2138.Device, StatusResult) {
			called = true
			return Reply(*st2138.NewDevice(0).
				WithParam("product", st2138.NewParamString("wrong")).
				WithParam("brightness", st2138.NewParamInt32(50)))
		})
		mockInvokeGateFn(t, srv, EndpointGetDevice, false, ctx, StatusWithCode(StatusCodeOk, ""))
		return srv, &called
	}

	t.Run("MonInjectsProduct", func(t *testing.T) {
		srv, called := newDeviceServer(t, monScopeContext())
		device, res := srv.InvokeGetDeviceHandler(0, TransportContext{})
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
		}
		if !*called {
			t.Error("expected business-logic GetDevice to be called")
		}
		params := device.Proto.GetParams()
		if _, ok := params["brightness"]; !ok {
			t.Error("expected business-logic 'brightness' param to be preserved")
		}
		product, ok := params["product"]
		if !ok {
			t.Fatal("expected SDK-injected 'product' param for mon caller")
		}
		if !proto.Equal(product, expectedProductParam()) {
			t.Errorf("SDK-injected product param does not match expected, got %v", product)
		}
	})

	t.Run("NonMonStripsProduct", func(t *testing.T) {
		srv, called := newDeviceServer(t, nonMonScopeContext())
		device, res := srv.InvokeGetDeviceHandler(0, TransportContext{})
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
		}
		if !*called {
			t.Error("expected business-logic GetDevice to be called")
		}
		params := device.Proto.GetParams()
		if _, ok := params["brightness"]; !ok {
			t.Error("expected business-logic 'brightness' param to be preserved")
		}
		if _, ok := params["product"]; ok {
			t.Error("did not expect a product param for non-mon caller")
		}
	})
}

// TestServer_GetValue_ProductScope verifies product/* reads succeed with the
// monitor scope and are denied without it, without invoking business logic on
// the denied path.
func TestServer_GetValue_ProductScope(t *testing.T) {
	for _, fqoid := range []string{"product", "product/name"} {
		t.Run("Mon/"+fqoid, func(t *testing.T) {
			srv := newTestServer(t, true)
			srv.RegisterProductStruct(0, testProduct())
			srv.RegisterGetValueHandler(0, func(slot uint16, oid string, ctx HandlerContext) (st2138.Value, StatusResult) {
				t.Errorf("business-logic GetValue should not be called for %q", oid)
				return ReplyError[st2138.Value](StatusCodeInternal, "should not happen")
			})
			mockInvokeGateFn(t, srv, EndpointGetValue, false, monScopeContext(), StatusWithCode(StatusCodeOk, ""))
			_, res := srv.InvokeGetValueHandler(0, fqoid, TransportContext{})
			if res.Code != StatusCodeOk {
				t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
			}
		})

		t.Run("NonMon/"+fqoid, func(t *testing.T) {
			srv := newTestServer(t, true)
			srv.RegisterProductStruct(0, testProduct())
			srv.RegisterGetValueHandler(0, func(slot uint16, oid string, ctx HandlerContext) (st2138.Value, StatusResult) {
				t.Errorf("business-logic GetValue should not be called for %q", oid)
				return ReplyError[st2138.Value](StatusCodeInternal, "should not happen")
			})
			mockInvokeGateFn(t, srv, EndpointGetValue, false, nonMonScopeContext(), StatusWithCode(StatusCodeOk, ""))
			_, res := srv.InvokeGetValueHandler(0, fqoid, TransportContext{})
			if res.Code != StatusCodePermissionDenied {
				t.Fatalf("expected PERMISSION_DENIED, got %v: %s", res.Code, res.Error)
			}
		})
	}
}

// TestServer_GetParam_ProductScope verifies product/* GetParam requests succeed
// with the monitor scope and are denied without it.
func TestServer_GetParam_ProductScope(t *testing.T) {
	for _, fqoid := range []string{"product", "product/name"} {
		t.Run("Mon/"+fqoid, func(t *testing.T) {
			srv := newTestServer(t, true)
			srv.RegisterProductStruct(0, testProduct())
			srv.RegisterGetParamHandler(0, func(slot uint16, oid string, ctx HandlerContext) (st2138.Param, StatusResult) {
				t.Errorf("business-logic GetParam should not be called for %q", oid)
				return st2138.Param{}, StatusWithCode(StatusCodeInternal, "should not happen")
			})
			mockInvokeGateFn(t, srv, EndpointGetParam, false, monScopeContext(), StatusWithCode(StatusCodeOk, ""))
			_, res := srv.InvokeGetParamHandler(0, fqoid, TransportContext{})
			if res.Code != StatusCodeOk {
				t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
			}
		})

		t.Run("NonMon/"+fqoid, func(t *testing.T) {
			srv := newTestServer(t, true)
			srv.RegisterProductStruct(0, testProduct())
			srv.RegisterGetParamHandler(0, func(slot uint16, oid string, ctx HandlerContext) (st2138.Param, StatusResult) {
				t.Errorf("business-logic GetParam should not be called for %q", oid)
				return st2138.Param{}, StatusWithCode(StatusCodeInternal, "should not happen")
			})
			mockInvokeGateFn(t, srv, EndpointGetParam, false, nonMonScopeContext(), StatusWithCode(StatusCodeOk, ""))
			_, res := srv.InvokeGetParamHandler(0, fqoid, TransportContext{})
			if res.Code != StatusCodePermissionDenied {
				t.Fatalf("expected PERMISSION_DENIED, got %v: %s", res.Code, res.Error)
			}
		})
	}
}

// TestServer_ParamInfo_ProductScope verifies product/* ParamInfo succeeds with
// the monitor scope and is denied without it.
func TestServer_ParamInfo_ProductScope(t *testing.T) {
	t.Run("Mon", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterProductStruct(0, testProduct())
		srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext, stream Stream[st2138.ParamInfo]) StatusResult {
			t.Errorf("business-logic ParamInfo should not be called for %q", oidPrefix)
			return StatusWithCode(StatusCodeInternal, "should not happen")
		})
		mockInvokeGateFn(t, srv, EndpointParamInfo, false, monScopeContext(), StatusWithCode(StatusCodeOk, ""))
		stream := &sliceStream[st2138.ParamInfo]{}
		res := srv.InvokeParamInfoHandler(0, "product", true, stream, TransportContext{})
		if res.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
		}
		if len(stream.Items) != 7 {
			t.Fatalf("expected 7 param infos, got %d", len(stream.Items))
		}
	})

	t.Run("NonMon", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterProductStruct(0, testProduct())
		srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext, stream Stream[st2138.ParamInfo]) StatusResult {
			t.Errorf("business-logic ParamInfo should not be called for %q", oidPrefix)
			return StatusWithCode(StatusCodeInternal, "should not happen")
		})
		mockInvokeGateFn(t, srv, EndpointParamInfo, false, nonMonScopeContext(), StatusWithCode(StatusCodeOk, ""))
		stream := &sliceStream[st2138.ParamInfo]{}
		res := srv.InvokeParamInfoHandler(0, "product", true, stream, TransportContext{})
		if res.Code != StatusCodePermissionDenied {
			t.Fatalf("expected PERMISSION_DENIED, got %v: %s", res.Code, res.Error)
		}
	})
}

// TestServer_GetValue_ProductScopeUnregistered verifies that when no product
// struct is registered for a slot, a product/* read is not gated by the SDK's
// mon scope check and instead falls through to the business-logic handler,
// which does its own scoping.
func TestServer_GetValue_ProductScopeUnregistered(t *testing.T) {
	srv := newTestServer(t, true)
	handlerCalled := false
	srv.RegisterGetValueHandler(0, func(slot uint16, oid string, ctx HandlerContext) (st2138.Value, StatusResult) {
		handlerCalled = true
		value, _ := st2138.ToValue("from business logic")
		return Reply(value)
	})
	mockInvokeGateFn(t, srv, EndpointGetValue, false, nonMonScopeContext(), StatusWithCode(StatusCodeOk, ""))

	_, res := srv.InvokeGetValueHandler(0, "product/name", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK from handler fallback, got %v: %s", res.Code, res.Error)
	}
	if !handlerCalled {
		t.Error("expected business-logic GetValue handler to be called when no product struct is registered")
	}
}
