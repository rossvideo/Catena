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
 * @brief Package documentation for the catena-healthcheck command.
 * @file doc.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-10-26
 */

// Command catena-healthcheck probes a running Catena Go SDK device and exits 0
// only if every enabled transport is healthy. It is a single static binary
// with no shell or other tools needed, so it works as a container HEALTHCHECK
// or Kubernetes exec probe in distroless images.
//
// # Configuration
//
// The probe reads the same environment variables and flags as the SDK's
// config package, so the environment or flags used to start the device work
// unchanged on the probe. Only the options relevant to probing are accepted:
//
//	PREFIX_USE_GRPC, PREFIX_USE_REST      --use-grpc, --use-rest
//	PREFIX_GRPC_PORT, PREFIX_REST_PORT    --grpc-port, --rest-port
//	PREFIX_GRPC_TLS_*, PREFIX_REST_TLS_*  --grpc-tls-*, --rest-tls-*
//
// PREFIX defaults to CATENA. Flags override environment variables. Each
// enabled transport is checked concurrently, and the overall result is healthy
// only if all of them are. If no transport is enabled the probe fails.
//
// The probe has a few options of its own:
//
//	--prefix        env var prefix of the SDK config (HEALTHCHECK_PREFIX, default CATENA)
//	--host          host to probe (HEALTHCHECK_HOST, default localhost)
//	--timeout       per-transport timeout (HEALTHCHECK_TIMEOUT, default 3s)
//	--verify-tls    verify the server certificate (HEALTHCHECK_VERIFY_TLS)
//	--tls-ca-file   CA bundle used for verification; implies --verify-tls (HEALTHCHECK_TLS_CA_FILE)
//
// Run with --help for the full list.
//
// # What is checked
//
// REST: GET /st2138-api/v1/health must return a 2xx status.
//
// gRPC: grpc.health.v1 Health/Check must report SERVING. The SDK's gRPC
// transport registers this service by default; if it has been disabled with
// --grpc-disable-health the server answers Unimplemented, which is accepted
// because it still shows the listener, TLS and HTTP/2 are working.
//
// # TLS
//
// If a transport has TLS enabled (PREFIX_GRPC_TLS_ENABLED or
// PREFIX_REST_TLS_ENABLED) the probe connects with TLS. Because it dials the
// local device, whose certificate is usually issued for another name, the
// server certificate is not verified unless --verify-tls or --tls-ca-file is
// given. With mutual TLS enabled, the transport's own certificate and key
// (PREFIX_*_TLS_CERT_FILE and PREFIX_*_TLS_KEY_FILE) are presented as the
// client certificate, so they must be valid for client authentication.
//
// # Exit status
//
// 0 if every enabled transport is healthy, 1 if any is unhealthy or none is
// enabled, 2 for invalid flags or configuration.
//
// # Installing
//
// From a project that depends on the SDK, add it as a tool so it is pinned to
// the same SDK version as the device (Go 1.24 or later):
//
//	go get -tool github.com/rossvideo/catena/sdks/go/cmd/catena-healthcheck
//	go tool catena-healthcheck --use-rest
//
// To build the binary for a container image:
//
//	CGO_ENABLED=0 go build -o /out/catena-healthcheck \
//	    github.com/rossvideo/catena/sdks/go/cmd/catena-healthcheck
//
// and then, for example:
//
//	FROM gcr.io/distroless/static
//	COPY --from=build /out/catena-healthcheck /catena-healthcheck
//	HEALTHCHECK CMD ["/catena-healthcheck"]
package main
