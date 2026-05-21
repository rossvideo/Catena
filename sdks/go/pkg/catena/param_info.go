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
 * @brief ParamInfo handling for the Catena SDK.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-12
 * @file param_info.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"encoding/json"
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// ParamInfo wraps protos.ParamInfoResponse for parameter info handling.
// It carries both a ParamInfo descriptor and, for array parameters, the
// current array length.
type ParamInfo struct {
	response *protos.ParamInfoResponse
}

// NewParamInfo creates a ParamInfo with the specified fields.
// name may be nil if no display name is required.
// arrayLength should be 0 for non-array parameters.
func NewParamInfo(oid string, name PolyglotText, paramType ParamType, templateOid string, arrayLength uint32) ParamInfo {
	info := &protos.ParamInfo{
		Oid:         oid,
		Type:        paramType,
		TemplateOid: templateOid,
	}
	if name != nil {
		info.Name = &protos.PolyglotText{DisplayStrings: name}
	}
	return ParamInfo{
		response: &protos.ParamInfoResponse{
			Info:        info,
			ArrayLength: arrayLength,
		},
	}
}

// ToParamInfo converts a Go map to a ParamInfo by marshalling
// through protojson. The map keys mirror the protos.ParamInfoResponse schema
func ToParamInfo(m map[string]any) (ParamInfo, error) {
	jsonData, err := json.Marshal(m)
	if err != nil {
		return ParamInfo{}, fmt.Errorf("ToParamInfo: marshal map: %w", err)
	}

	resp := &protos.ParamInfoResponse{}
	if err := protojson.Unmarshal(jsonData, resp); err != nil {
		return ParamInfo{}, fmt.Errorf("ToParamInfo: unmarshal to proto: %w", err)
	}
	return ParamInfo{response: resp}, nil
}

// GetProtoResponse returns the underlying protos.ParamInfoResponse.
func (p ParamInfo) GetProtoResponse() *protos.ParamInfoResponse {
	return p.response
}

// GetProtoInfo returns the underlying protos.ParamInfo, or nil if unset.
func (p ParamInfo) GetProtoInfo() *protos.ParamInfo {
	if p.response == nil {
		return nil
	}
	return p.response.GetInfo()
}

// GetOid returns the parameter's OID, or "" if unset.
func (p ParamInfo) GetOid() string {
	return p.GetProtoInfo().GetOid()
}

// GetParamType returns the parameter's type, or UNDEFINED if unset.
func (p ParamInfo) GetParamType() ParamType {
	return p.GetProtoInfo().GetType()
}

// GetTemplateOid returns the template OID, or "" if unset.
func (p ParamInfo) GetTemplateOid() string {
	return p.GetProtoInfo().GetTemplateOid()
}

// GetArrayLength returns the array length for array parameters, or 0 otherwise.
func (p ParamInfo) GetArrayLength() uint32 {
	if p.response == nil {
		return 0
	}
	return p.response.GetArrayLength()
}
