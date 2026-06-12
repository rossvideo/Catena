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
 * @brief Helpers to derive transport configs from the top-level config.RuntimeOptions.
 * @file config.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-26
 */

package transports

import (
	"fmt"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

// maxPort is the highest valid TCP port number.
const maxPort = 65535

// validatePort ensures a configured port is within the valid TCP range so that
// out-of-range values fail loudly instead of silently wrapping when narrowed.
func validatePort(name string, port int) error {
	if port < 0 || port > maxPort {
		return fmt.Errorf("%s %d is out of range (must be between 0 and %d)", name, port, maxPort)
	}
	return nil
}

// RestConfigFromOptions builds a RestConfig from the top-level config.RuntimeOptions.
func RestConfigFromOptions(opts config.RuntimeOptions) (RestConfig, error) {
	if err := validatePort("rest port", opts.RestPort); err != nil {
		return RestConfig{}, err
	}
	return RestConfig{
		Port: opts.RestPort,
	}, nil
}

// GrpcConfigFromOptions builds a GrpcConfig from the top-level config.RuntimeOptions.
func GrpcConfigFromOptions(opts config.RuntimeOptions) (GrpcConfig, error) {
	if err := validatePort("grpc port", opts.GrpcPort); err != nil {
		return GrpcConfig{}, err
	}
	return GrpcConfig{
		Port:       uint16(opts.GrpcPort),
		Reflection: opts.GrpcReflection,
	}, nil
}
