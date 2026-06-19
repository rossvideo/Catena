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
 * @brief Redeclare types from sub packages for easier access
 * @file types.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-06-08
 */

package catena

import "github.com/rossvideo/catena/sdks/go/pkg/config"

type RuntimeOptions = config.RuntimeOptions
type ServerOptions = config.ServerOptions
type JwtValidationOptions = config.JwtValidationOptions
type DashboardOptions = config.DashboardOptions
type ConnectionProtocol = config.ConnectionProtocol
type GrpcOptions = config.GrpcOptions
type RestOptions = config.RestOptions

const (
	ProtocolST2138Rest   = config.ProtocolST2138Rest
	ProtocolST2138Grpc   = config.ProtocolST2138Grpc
	ProtocolST2138Catena = config.ProtocolST2138Catena
)

var (
	DefaultServerOptions        = config.DefaultServerOptions
	DefaultJwtValidationOptions = config.DefaultJwtValidationOptions
	DefaultDashboardOptions     = config.DefaultDashboardOptions
	DefaultGrpcOptions          = config.DefaultGrpcOptions
	DefaultRestOptions          = config.DefaultRestOptions
)
