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
 * @brief Server interface and handler types for the Catena SDK.
 * @file server.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package catena

// Handler function types used by both REST and gRPC servers.
type DeviceHandler func() (CatenaDevice, StatusResult)
type GetValueHandler func(slot uint16, fqoid string) (CatenaValue, StatusResult)
type SetValueHandler func(value any, slot uint16, fqoid string) StatusResult
type GetAssetHandler func(slot uint16, fqoid string) (CatenaAsset, StatusResult)
type ExecuteCommandHandler func(slot uint16, commandFqoid string, payload any) (CommandResult, StatusResult)

// GetParamInfoHandler returns parameter info entries matching oidPrefix at the
// given slot. If oidPrefix is "", all top-level parameters should be returned.
// If recursive is true, child parameters should also be included.
type GetParamInfoHandler func(slot uint16, oidPrefix string, recursive bool) ([]CatenaParamInfo, StatusResult)

// CatenaServer is the transport-agnostic interface satisfied by both
// rest.Server and grpc.Server, enabling shared handler registration code.
type CatenaServer interface {
	RegisterGetDeviceHandler(slot uint16, handler DeviceHandler)
	RegisterGetValueHandler(slot uint16, handler GetValueHandler)
	RegisterSetValueHandler(slot uint16, handler SetValueHandler)
	RegisterGetAssetHandler(slot uint16, handler GetAssetHandler)
	RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler)
	RegisterGetParamInfoHandler(slot uint16, handler GetParamInfoHandler)
	BroadcastUpdate(slot uint16, fqoid string, value any)
	Start(port int) error
	Shutdown()
}
