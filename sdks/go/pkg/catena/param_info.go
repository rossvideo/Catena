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
 * @brief ParamInfo tree-walking business logic for the Catena SDK.
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-12
 * @file param_info.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"fmt"
	"sort"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// ParamInfosForRequest streams ParamInfo responses for the requested FQOID from
// a Device's params subtree, emitting each descriptor into stream as the tree
// is walked. It is a business-logic helper a ParamInfo handler can delegate to
// once it can produce a Device definition; the returned StatusResult is the
// terminal status a handler returns directly: Ok once every descriptor has been
// sent, NotFound for an unknown oid, or Internal if a Send fails (a failed Send
// stops the walk immediately).
func ParamInfosForRequest(fqoid string, device *st2138.Device, recursive bool, stream Stream[st2138.ParamInfo]) StatusResult {
	if device == nil || device.Proto == nil {
		return StatusWithCode(StatusCodeInternal, "invalid device")
	}
	params := device.Proto.GetParams()

	if fqoid == "" {
		if err := sendParamInfos(params, "", recursive, stream); err != nil {
			return StatusWithCode(StatusCodeInternal, err.Error())
		}
		return StatusResult{Code: StatusCodeOk}
	}

	param, ok := findParamDescriptor(params, fqoid)
	if !ok {
		return StatusWithCode(StatusCodeNotFound, fmt.Sprintf("param not found: %s", fqoid))
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

	return StatusResult{Code: StatusCodeOk}
}

func paramDisplayName(param *protos.Param) st2138.PolyglotText {
	displayStrings := param.GetName().GetDisplayStrings()
	if len(displayStrings) == 0 {
		return nil
	}
	return st2138.PolyglotText(displayStrings)
}

func paramArrayLength(param *protos.Param) uint32 {
	value := param.GetValue()
	switch param.GetType() {
	case st2138.ParamTypeInt32Array:
		return uint32(len(value.GetInt32ArrayValues().GetInts()))
	case st2138.ParamTypeFloat32Array:
		return uint32(len(value.GetFloat32ArrayValues().GetFloats()))
	case st2138.ParamTypeStringArray:
		return uint32(len(value.GetStringArrayValues().GetStrings()))
	case st2138.ParamTypeStructArray:
		return uint32(len(value.GetStructArrayValues().GetStructValues()))
	case st2138.ParamTypeStructVariantArray:
		return uint32(len(value.GetStructVariantArrayValues().GetStructVariants()))
	default:
		return 0
	}
}

func newParamInfoFromDescriptor(oid string, param *protos.Param) st2138.ParamInfo {
	return st2138.NewParamInfo(oid, paramDisplayName(param), param.GetType(), param.GetTemplateOid(), paramArrayLength(param))
}

// sendParamInfos walks params in sorted-key order, sending a descriptor for each
// (and, when recursive, its subtree) into stream. It stops and returns the first
// Send error encountered.
func sendParamInfos(params map[string]*protos.Param, prefix string, recursive bool, stream Stream[st2138.ParamInfo]) error {
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
