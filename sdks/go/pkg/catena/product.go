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
 * @brief Product struct helpers for the Catena SDK.
 * @file product.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package catena

// Product carries the mandatory device product identity fields. The
// catena_sdk and catena_sdk_version fields are managed by the SDK
// (SDKVersion / CatenaSDKURL), so callers do not supply them.
type Product struct {
	Name         string
	Vendor       string
	Version      string
	SerialNumber string
}

// Sub-OIDs of the mandatory product struct param.
const (
	ProductOidName             = "name"
	ProductOidVendor           = "vendor"
	ProductOidVersion          = "version"
	ProductOidSerialNumber     = "serial_number"
	ProductOidCatenaSDKVersion = "catena_sdk_version"
	ProductOidCatenaSDK        = "catena_sdk"
)

// ProductParam builds the mandatory read-only "product" STRUCT param from p,
// including the SDK-managed catena_sdk and catena_sdk_version sub-params.
// NewDevice uses this to seed every device; call it directly only if you need
// the param on its own.
func ProductParam(p Product) *Param {
	return NewParamStruct().
		WithReadOnly(true).
		WithAccessScope(ScopeMon).
		WithParam(ProductOidName, NewParamString(p.Name)).
		WithParam(ProductOidVendor, NewParamString(p.Vendor)).
		WithParam(ProductOidVersion, NewParamString(p.Version)).
		WithParam(ProductOidSerialNumber, NewParamString(p.SerialNumber)).
		WithParam(ProductOidCatenaSDKVersion, NewParamString(SDKVersion)).
		WithParam(ProductOidCatenaSDK, NewParamString(CatenaSDKURL))
}

// ProductValues returns the product field values keyed by their sub-OID,
// matching the sub-params built by ProductParam (including the SDK-managed
// catena_sdk and catena_sdk_version fields). Use it in GetValue handlers so the
// values reported for product/* stay in sync with the device descriptor.
func ProductValues(p Product) map[string]any {
	return map[string]any{
		ProductOidName:             p.Name,
		ProductOidVendor:           p.Vendor,
		ProductOidVersion:          p.Version,
		ProductOidSerialNumber:     p.SerialNumber,
		ProductOidCatenaSDKVersion: SDKVersion,
		ProductOidCatenaSDK:        CatenaSDKURL,
	}
}
