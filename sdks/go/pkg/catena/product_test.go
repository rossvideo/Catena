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
 * @brief Product struct helper tests for the Catena SDK.
 * @file product_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import "testing"

// TestProductParamValuesParity guards the invariant behind the "Product SDK
// fields diverge" bug: the sub-params ProductParam advertises in the device
// descriptor must exactly match what ProductValues reports to GetValue handlers,
// including the SDK-managed catena_sdk / catena_sdk_version fields.
func TestProductParamValuesParity(t *testing.T) {
	p := Product{
		Name:         "Camera",
		Vendor:       "Ross Video",
		Version:      "1.0",
		SerialNumber: "SN-12345",
	}

	param := ProductParam(p)
	values := ProductValues(p)

	subParams := param.Proto.GetParams()
	if len(subParams) != len(values) {
		t.Fatalf("sub-param count %d != value count %d", len(subParams), len(values))
	}

	for oid, child := range subParams {
		want, ok := values[oid]
		if !ok {
			t.Errorf("descriptor advertises %q but ProductValues has no such field", oid)
			continue
		}
		wantStr, ok := want.(string)
		if !ok {
			t.Errorf("ProductValues[%q] is not a string: %T", oid, want)
			continue
		}
		if got := child.GetValue().GetStringValue(); got != wantStr {
			t.Errorf("product/%s: descriptor=%q, value=%q", oid, got, wantStr)
		}
	}

	if values[ProductOidCatenaSDK] != CatenaSDKURL {
		t.Errorf("catena_sdk = %v, want %q", values[ProductOidCatenaSDK], CatenaSDKURL)
	}
	if values[ProductOidCatenaSDKVersion] != SDKVersion {
		t.Errorf("catena_sdk_version = %v, want %q", values[ProductOidCatenaSDKVersion], SDKVersion)
	}
}
