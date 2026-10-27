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
 * @brief Register transports selected by RuntimeOptions.
 * @file register.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-10-15
 */

package transports

import (
	"errors"
	"fmt"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/grpc"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/rest"
)

// ErrNoTransportEnabled is returned by RegisterFrom when neither the gRPC nor
// the REST transport is enabled.
var ErrNoTransportEnabled = errors.New("at least one of gRPC or REST transport must be enabled")

// RegisterFrom constructs and registers every transport enabled by opts.
//
// UseGrpc registers grpc.NewTransport(opts.Grpc). UseRest registers
// rest.NewTransport(opts.Rest), including any REST fallback set on opts.Rest.
// When both are enabled, gRPC is registered first. A registration error names
// the transport that failed and wraps the error from Server.RegisterTransport.
// Does not return transport references, register manually if you must
// explicitly deregister or require dynamic transports.
func RegisterFrom(srv catena.Server, opts config.RuntimeOptions) error {
	if !opts.UseGrpc && !opts.UseRest {
		return ErrNoTransportEnabled
	}
	if opts.UseGrpc {
		if err := srv.RegisterTransport(grpc.NewTransport(opts.Grpc)); err != nil {
			return fmt.Errorf("register gRPC transport: %w", err)
		}
	}
	if opts.UseRest {
		if err := srv.RegisterTransport(rest.NewTransport(opts.Rest)); err != nil {
			return fmt.Errorf("register REST transport: %w", err)
		}
	}
	return nil
}
