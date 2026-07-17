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
 * @brief SDK-managed product struct for the Catena SDK.
 * @file product.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package catena

import (
	"runtime/debug"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// sdkModulePath is the SDK's Go module path, used to locate the SDK's resolved
// version in the running binary's build info.
const sdkModulePath = "github.com/rossvideo/catena/sdks/go"

// SDKVersion is the Catena Go SDK version reported in the SDK-managed product
// struct (the "catena_sdk_version" field). It is resolved at startup from the
// binary's build info (the git tag or pseudo-version Go recorded for the SDK
// module), so it stays accurate without a hand-maintained constant.
var SDKVersion = sdkVersion()

// sdkVersion resolves the SDK's version from the running binary's build info.
// It returns the version Go recorded for the SDK module, falling back to
// "(devel)" for unversioned local builds and "(unknown)" when build info is
// unavailable.
func sdkVersion() string {
	bi, ok := debug.ReadBuildInfo()
	if !ok {
		return "(unknown)"
	}
	// When the SDK is imported as a dependency, its resolved version lives in
	// Deps. When the SDK module is itself the main module (e.g. running its own
	// tests or examples), the version lives on Main instead.
	if bi.Main.Path == sdkModulePath && bi.Main.Version != "" {
		return bi.Main.Version
	}
	for _, d := range bi.Deps {
		if d.Path == sdkModulePath {
			return d.Version // resolved from the git tag / pseudo-version
		}
	}
	return "(devel)"
}

// CatenaSDKURL identifies the Catena SDK in the SDK-managed product struct (the
// "catena_sdk" field).
const CatenaSDKURL = "https://github.com/rossvideo/Catena"

// ProductStruct carries the mandatory device product identity fields. The
// catena_sdk and catena_sdk_version fields are managed by the SDK
// (SDKVersion / CatenaSDKURL), so callers do not supply them. Register one per
// slot with Server.RegisterProductStruct and the SDK serves it on GetDevice,
// GetValue, and ParamInfo, and rejects writes to it on SetValue.
type ProductStruct struct {
	Name         string
	Vendor       string
	Version      string
	SerialNumber string
}

// ProductOid is the OID of the mandatory product struct param, and the
// following are the sub-OIDs of its fields.
const (
	ProductOid                 = "product"
	ProductOidName             = "name"
	ProductOidVendor           = "vendor"
	ProductOidVersion          = "version"
	ProductOidSerialNumber     = "serial_number"
	ProductOidCatenaSDKVersion = "catena_sdk_version"
	ProductOidCatenaSDK        = "catena_sdk"
)

// ProductParam builds the mandatory read-only "product" STRUCT param from p,
// including the SDK-managed catena_sdk and catena_sdk_version fields. The field
// values live in the struct's Value; the sub-params carry only the field
// descriptors (their STRING type). The SDK uses this to seed the product param
// into the device on GetDevice.
func ProductParam(p ProductStruct) *st2138.Param {
	values := ProductValues(p)
	param := st2138.NewParamStruct(values).
		WithReadOnly(true).
		WithAccessScope(st2138.ScopeMon)
	for oid := range values {
		param.WithParam(oid, productFieldDescriptor())
	}
	return param
}

// productFieldDescriptor is a valueless STRING sub-param used to describe a
// product field. The field's value lives in the parent struct's Value.
func productFieldDescriptor() *st2138.Param {
	return &st2138.Param{Proto: &protos.Param{Type: protos.ParamType_STRING}}
}

// ProductValues returns the product field values keyed by their sub-OID,
// matching the sub-params built by ProductParam (including the SDK-managed
// catena_sdk and catena_sdk_version fields).
func ProductValues(p ProductStruct) map[string]any {
	return map[string]any{
		ProductOidName:             p.Name,
		ProductOidVendor:           p.Vendor,
		ProductOidVersion:          p.Version,
		ProductOidSerialNumber:     p.SerialNumber,
		ProductOidCatenaSDKVersion: SDKVersion,
		ProductOidCatenaSDK:        CatenaSDKURL,
	}
}

// isProductOid reports whether fqoid targets the product struct or one of its
// sub-fields (e.g. "product" or "product/name").
func isProductOid(fqoid string) bool {
	return fqoid == ProductOid || strings.HasPrefix(fqoid, ProductOid+"/")
}

// productValueForOid resolves a product FQOID (the whole struct for "product",
// or a single field for "product/<field>") to a Value from p.
func productValueForOid(p ProductStruct, fqoid string) (st2138.Value, StatusResult) {
	values := ProductValues(p)

	var native any
	if fqoid == ProductOid {
		native = values
	} else {
		field := strings.TrimPrefix(fqoid, ProductOid+"/")
		value, ok := values[field]
		if !ok {
			return ReplyError[st2138.Value](StatusCodeNotFound, "parameter not found: "+fqoid)
		}
		native = value
	}

	value, res := st2138.ToValue(native)
	if res.Code != StatusCodeOk {
		return ReplyError[st2138.Value](StatusCodeInternal, "failed to convert product value")
	}
	return Reply(value)
}

// productParamForOid resolves a product FQOID to a Param: the whole struct for
// "product", or a single STRING field descriptor (with its value) for
// "product/<field>".
func productParamForOid(p ProductStruct, fqoid string) (st2138.Param, StatusResult) {
	if fqoid == ProductOid {
		return *ProductParam(p), StatusWithCode(StatusCodeOk, "")
	}

	field := strings.TrimPrefix(fqoid, ProductOid+"/")
	value, ok := ProductValues(p)[field]
	if !ok {
		return st2138.Param{}, StatusWithCode(StatusCodeNotFound, "parameter not found: "+fqoid)
	}

	pv, res := st2138.ToProto(value)
	if res.Code != StatusCodeOk {
		return st2138.Param{}, StatusWithCode(StatusCodeInternal, "failed to convert product value")
	}
	return st2138.Param{Proto: &protos.Param{Type: protos.ParamType_STRING, Value: pv}}, StatusWithCode(StatusCodeOk, "")
}

// productParamInfosForOid builds ParamInfo responses for a product FQOID by
// reusing the standard params-subtree walker over the SDK-built product param.
func productParamInfosForOid(p ProductStruct, oidPrefix string, recursive bool, stream Stream[st2138.ParamInfo]) StatusResult {
	device := &st2138.Device{Proto: &protos.Device{
		Params: map[string]*protos.Param{ProductOid: ProductParam(p).Proto},
	}}
	return st2138.ParamInfosForRequest(oidPrefix, device, recursive, stream)
}
