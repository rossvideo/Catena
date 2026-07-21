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

package st2138

import (
	"sort"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"google.golang.org/protobuf/proto"
)

// ParamInfo wraps protos.ParamInfoResponse for parameter info handling.
// It carries both a ParamInfo descriptor and, for array parameters, the
// current array length. Proto is the underlying proto message; it may be read
// or replaced directly.
type ParamInfo struct {
	Proto *protos.ParamInfoResponse
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
		Proto: &protos.ParamInfoResponse{
			Info:        info,
			ArrayLength: arrayLength,
		},
	}
}

// paramInfoSink is the minimal chunk sink ParamInfosForRequest needs: somewhere
// to Send each ParamInfo descriptor. The handler-facing Stream type lives in the
// catena package (which would create an import cycle if referenced here), so we
// accept the narrow interface instead - any catena.Stream[ParamInfo] satisfies
// it structurally.
type paramInfoSink interface {
	Send(chunk ParamInfo) error
}

// ParamInfosForRequest streams ParamInfo responses for the requested FQOID from
// a Device's params subtree, emitting each descriptor into stream as the tree
// is walked. The returned StatusResult is the terminal status: Ok once every
// descriptor has been sent, NotFound for an unknown oid, or Internal if a Send
// fails (a failed Send stops the walk immediately).
func ParamInfosForRequest(fqoid string, device *Device, recursive bool, stream paramInfoSink) StatusResult {
	if device == nil || device.Proto == nil {
		return StatusWithCode(StatusCodeInternal, "invalid device")
	}
	params := device.Proto.GetParams()

	if fqoid == "" {
		if err := sendParamInfos(params, "", recursive, stream); err != nil {
			return StatusWithCode(StatusCodeInternal, err.Error())
		}
		return StatusWithCode(StatusCodeOk, "")
	}

	param, ok := findParamDescriptor(params, fqoid)
	if !ok {
		return StatusWithCode(StatusCodeNotFound, "param not found: "+fqoid)
	}

	if err := stream.Send(newParamInfoFromDescriptor(fqoid, param)); err != nil {
		return StatusWithCode(StatusCodeInternal, err.Error())
	}
	if recursive {
		if children := param.GetParams(); len(children) > 0 {
			if err := sendParamInfos(children, fqoid, true, stream); err != nil {
				return StatusWithCode(StatusCodeInternal, err.Error())
			}
		}
	}

	return StatusWithCode(StatusCodeOk, "")
}

func paramDisplayName(param *protos.Param) PolyglotText {
	displayStrings := param.GetName().GetDisplayStrings()
	if len(displayStrings) == 0 {
		return nil
	}
	return PolyglotText(displayStrings)
}

func paramArrayLength(param *protos.Param) uint32 {
	value := param.GetValue()
	switch param.GetType() {
	case ParamTypeInt32Array:
		return uint32(len(value.GetInt32ArrayValues().GetInts()))
	case ParamTypeFloat32Array:
		return uint32(len(value.GetFloat32ArrayValues().GetFloats()))
	case ParamTypeStringArray:
		return uint32(len(value.GetStringArrayValues().GetStrings()))
	case ParamTypeStructArray:
		return uint32(len(value.GetStructArrayValues().GetStructValues()))
	case ParamTypeStructVariantArray:
		return uint32(len(value.GetStructVariantArrayValues().GetStructVariants()))
	default:
		return 0
	}
}

func newParamInfoFromDescriptor(oid string, param *protos.Param) ParamInfo {
	return NewParamInfo(oid, paramDisplayName(param), param.GetType(), param.GetTemplateOid(), paramArrayLength(param))
}

// sendParamInfos walks params in sorted-key order, sending a descriptor for each
// (and, when recursive, its subtree) into stream. It stops and returns the first
// Send error encountered.
func sendParamInfos(params map[string]*protos.Param, prefix string, recursive bool, stream paramInfoSink) error {
	keys := make([]string, 0, len(params))
	for key := range params {
		keys = append(keys, key)
	}
	sort.Strings(keys)

	for _, key := range keys {
		param := params[key]
		if param == nil {
			continue
		}

		oid := key
		if prefix != "" {
			oid = prefix + "/" + key
		}
		if err := stream.Send(newParamInfoFromDescriptor(oid, param)); err != nil {
			return err
		}

		if recursive {
			if children := param.GetParams(); len(children) > 0 {
				if err := sendParamInfos(children, oid, true, stream); err != nil {
					return err
				}
			}
		}
	}
	return nil
}

func findParamDescriptor(params map[string]*protos.Param, oid string) (*protos.Param, bool) {
	if oid == "" {
		return nil, false
	}

	pathParts := strings.Split(oid, "/")
	var param *protos.Param
	for _, part := range pathParts {
		next, exists := params[part]
		if !exists || next == nil {
			return nil, false
		}
		param = next
		params = param.GetParams()
	}

	return param, true
}

// Ensure ParamInfo can be streamed as a Message.
var _ Message = ParamInfo{}

// Wire returns the underlying protos.ParamInfoResponse as the streamed chunk's
// wire representation. ParamInfo absorbs the ParamInfoResponse layer, so the
// response is already the complete, self-contained message for one chunk.
func (p ParamInfo) Wire() proto.Message {
	return p.Proto
}

// GetOid returns the parameter's OID, or "" if unset.
func (p ParamInfo) GetOid() string {
	return p.Proto.GetInfo().GetOid()
}

// GetParamType returns the parameter's type, or UNDEFINED if unset.
func (p ParamInfo) GetParamType() ParamType {
	return p.Proto.GetInfo().GetType()
}

// GetTemplateOid returns the template OID, or "" if unset.
func (p ParamInfo) GetTemplateOid() string {
	return p.Proto.GetInfo().GetTemplateOid()
}

// GetArrayLength returns the array length for array parameters, or 0 otherwise.
func (p ParamInfo) GetArrayLength() uint32 {
	return p.Proto.GetArrayLength()
}
