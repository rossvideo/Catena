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
 * @brief Shared example lifecycle utilities for REST and gRPC examples.
 * @file exampleutil.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package exampleutil

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/transports"
)

// RunConfig holds all parameters for the shared example lifecycle.
type RunConfig struct {
	AppName          string
	Slots            []uint16
	RegisterHandlers func(srv catena.Server)
	OnReady          func(cfg catena.Config)
}

// RunExample encapsulates the full example lifecycle:
// SDK init, signal handling, server creation, handler registration,
// transport setup (based on config), and graceful shutdown.
func RunExample(rc RunConfig) {
	cfg, err := catena.InitOptions(catena.Options{AppName: rc.AppName})
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize SDK: %v\n", err)
		os.Exit(1)
	}
	defer catena.Close()

	srv := catena.NewServer(100)
	rc.RegisterHandlers(srv)

	if !cfg.UseGrpc && !cfg.UseRest {
		logger.Error("No transports enabled", "error", "at least one of gRPC or REST transport must be enabled in config")
		os.Exit(1)
	}

	if cfg.UseGrpc {
		if err := srv.RegisterTransport(transports.NewDefaultGrpcTransport()); err != nil {
			logger.Error("Failed to register gRPC transport", "error", err)
			os.Exit(1)
		}
	}

	if cfg.UseRest {
		if err := srv.RegisterTransport(transports.NewDefaultRestTransport()); err != nil {
			logger.Error("Failed to register REST transport", "error", err)
			os.Exit(1)
		}
	}

	if rc.OnReady != nil {
		rc.OnReady(cfg)
	}

	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()

	<-ctx.Done()
	logger.Info("Shutting down server...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), time.Second)
	defer shutdownCancel()
	srv.Shutdown(shutdownCtx)
	logger.Info("Server shutdown complete")
}
