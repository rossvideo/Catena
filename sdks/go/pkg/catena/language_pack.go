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
 * @brief A parent class for REST test fixtures.
 * @author benjamin.whitten@rossvideo.com
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author keon.foster@rossvideo.com
 * @date 2026-02-19
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import "github.com/rossvideo/catena/sdks/go/pkg/protos"

// LanguagePack wraps protos.DeviceComponent_ComponentLanguagePack for language-pack handling.
type LanguagePack struct {
	response *protos.DeviceComponent_ComponentLanguagePack
}

// NewLanguagePack creates a LanguagePack response.
func NewLanguagePack(language string, pack *protos.LanguagePack) LanguagePack {
	return LanguagePack{
		response: &protos.DeviceComponent_ComponentLanguagePack{
			Language:     language,
			LanguagePack: pack,
		},
	}
}

// NewLanguagePackFromProto wraps an existing proto language-pack response.
func NewLanguagePackFromProto(response *protos.DeviceComponent_ComponentLanguagePack) LanguagePack {
	return LanguagePack{response: response}
}

// GetProtoResponse returns the underlying protos.DeviceComponent_ComponentLanguagePack.
func (lp LanguagePack) GetProtoResponse() *protos.DeviceComponent_ComponentLanguagePack {
	return lp.response
}
