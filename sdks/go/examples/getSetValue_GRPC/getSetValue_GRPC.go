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
 * @brief Example gRPC server implementation
 * @file main.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-25
 */

package main

import (
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	grpcServer "github.com/rossvideo/catena/sdks/go/pkg/grpc"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	deviceParams = &sync.Map{}
)

func main() {
	// Initialize SDK with prefix and app name
	cfg, err := catena.InitOptions(catena.Options{AppName: "getSetValue_GRPC"})
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize SDK: %v\n", err)
		os.Exit(1)
	}
	defer catena.Close()

	// Handle signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	// Start server in a goroutine so shutdown handling works
	port := cfg.Port

	// Initialize device parameters
	deviceParams.Store("/volume", int32(50))
	deviceParams.Store("/brightness", int32(75))
	deviceParams.Store("/contrast", int32(60))

	// Create gRPC server with slot 0
	server := grpcServer.NewServer([]uint16{0}, 100, cfg)

	// Register GetValue handler
	server.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("GetParam", "slot", slot, "fqoid", fqoid)

		value, exists := deviceParams.Load(fqoid)
		if !exists {
			logger.Error("GetParam param not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "parameter not found")
		}

		protoValue, err := catena.ToProto(value)
		if err != nil {
			logger.Error("GetParam failed to convert value", "slot", slot, "fqoid", fqoid, "error", err)
			return catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to convert value")
		}

		return catena.CatenaValue{Value: protoValue}, catena.StatusWithCode(catena.OK, "")
	})

	// Register SetValue handler
	server.RegisterSetValueHandler(0, func(value any, slot uint16, fqoid string) catena.StatusResult {
		logger.Info("SetParam", "slot", slot, "fqoid", fqoid)
		intVal, ok := value.(int32)
		if !ok {
			logger.Error("SetParam type mismatch", "slot", slot, "fqoid", fqoid)
			return catena.StatusWithCode(catena.INVALID_ARGUMENT, "value must be int32")
		}

		deviceParams.Store(fqoid, intVal)
		logger.Info("Parameter updated", "slot", slot, "fqoid", fqoid, "value", intVal)

		server.BroadcastUpdate(slot, fqoid, value)
		return catena.StatusWithCode(catena.OK, "")
	})

	// Register ExecuteCommand handler
	server.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("Command executed", "command", commandFqoid, "payload", payload)

		// Return a simple response
		response, _ := catena.ToProto("Command executed successfully")
		return catena.CatenaValue{Value: response}, catena.StatusWithCode(catena.OK, "")
	})

	logger.Info("Starting gRPC server", "port", port)
	go func() {
		if err := server.Start(port); err != nil {
			logger.Error("server failed", "error", err)
			os.Exit(1)
		}
	}()

	// Wait for shutdown signal
	<-shutdownChan
	server.Shutdown()
	logger.Info("Server shutdown complete")
}
