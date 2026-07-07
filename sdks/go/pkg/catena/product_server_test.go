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

import "testing"

func testProduct() ProductStruct {
	return ProductStruct{
		Name:         "Camera",
		Vendor:       "Ross Video",
		Version:      "1.0",
		SerialNumber: "SN-12345",
	}
}

// TestServer_GetDevice_InjectsProduct verifies GetDevice overwrites the product
// param in whatever the business logic returned with the SDK-managed product.
func TestServer_GetDevice_InjectsProduct(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
		// Business logic returns a device with a bogus product that must be
		// overwritten, plus its own param that must be preserved.
		return Reply(*NewDevice(0).
			WithParam("product", NewParamString("wrong")).
			WithParam("brightness", NewParamInt32(50)))
	})

	device, res := srv.InvokeGetDeviceHandler(0, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}

	params := device.ToProtoDevice().GetParams()
	if _, ok := params["brightness"]; !ok {
		t.Error("expected business-logic 'brightness' param to be preserved")
	}
	product, ok := params["product"]
	if !ok {
		t.Fatal("expected SDK-injected 'product' param")
	}
	if product.GetType() != ParamTypeStruct {
		t.Errorf("expected product STRUCT, got %v", product.GetType())
	}
	if got := product.GetParams()["name"].GetValue().GetStringValue(); got != "Camera" {
		t.Errorf("product/name = %q, want %q", got, "Camera")
	}
	if got := product.GetParams()["catena_sdk"].GetValue().GetStringValue(); got != CatenaSDKURL {
		t.Errorf("product/catena_sdk = %q, want %q", got, CatenaSDKURL)
	}
}

// TestServer_GetDevice_NoProductRegistered verifies the device is passed through
// untouched when no product is registered for the slot.
func TestServer_GetDevice_NoProductRegistered(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterGetDeviceHandler(0, func(slot uint16, ctx HandlerContext) (Device, StatusResult) {
		return Reply(*NewDevice(0).WithParam("brightness", NewParamInt32(50)))
	})

	device, res := srv.InvokeGetDeviceHandler(0, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if _, ok := device.ToProtoDevice().GetParams()["product"]; ok {
		t.Error("did not expect a product param when none is registered")
	}
}

// TestServer_GetValue_Product verifies product/* reads are answered by the SDK
// without invoking the business-logic GetValue handler.
func TestServer_GetValue_Product(t *testing.T) {
	srv := newTestServer(t, false)
	srv.RegisterProductStruct(0, testProduct())
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		t.Errorf("business-logic GetValue should not be called for %q", fqoid)
		return ReplyError[Value](StatusCodeInternal, "should not happen")
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
		if got := value.Value.GetStringValue(); got != want {
			t.Errorf("%s = %q, want %q", fqoid, got, want)
		}
	}

	// The whole struct is returned for the bare "product" oid.
	value, res := srv.InvokeGetValueHandler(0, "product", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("product: expected OK, got %v: %s", res.Code, res.Error)
	}
	if value.Value.GetStructValue() == nil {
		t.Error("expected struct value for bare 'product' oid")
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
	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult) {
		called = true
		v, _ := ToValue(int32(7))
		return Reply(v)
	})

	value, res := srv.InvokeGetValueHandler(0, "brightness", TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	if !called {
		t.Error("expected business-logic handler to be called for non-product oid")
	}
	if value.Value.GetInt32Value() != 7 {
		t.Errorf("brightness = %d, want 7", value.Value.GetInt32Value())
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
	srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext) ([]ParamInfo, StatusResult) {
		t.Errorf("business-logic ParamInfo should not be called for %q", oidPrefix)
		return nil, StatusWithCode(StatusCodeInternal, "should not happen")
	})

	infos, res := srv.InvokeParamInfoHandler(0, "product", true, TransportContext{})
	if res.Code != StatusCodeOk {
		t.Fatalf("expected OK, got %v: %s", res.Code, res.Error)
	}
	// product + its 6 sub-params.
	if len(infos) != 7 {
		t.Fatalf("expected 7 param infos, got %d", len(infos))
	}
	if infos[0].GetOid() != "product" {
		t.Errorf("expected first oid 'product', got %q", infos[0].GetOid())
	}
	if infos[0].GetParamType() != ParamTypeStruct {
		t.Errorf("expected product STRUCT, got %v", infos[0].GetParamType())
	}
}
