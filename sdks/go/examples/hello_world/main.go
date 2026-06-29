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
 * @brief Minimal Catena hello world example with REST and gRPC transports.
 * @file main.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package main

import (
	"context"
	"os"
	"os/signal"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/transports"
)

const (
	slot          uint16 = 0
	helloWorldOID        = "hello_world"
)

type helloWorldState struct {
	mu    sync.RWMutex
	value string
}

var helloWorld = &helloWorldState{value: "Hello, World!"}

func (s *helloWorldState) Get() string {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.value
}

func (s *helloWorldState) Set(value string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.value = value
}

func buildDevice() map[string]any {
	return map[string]any{
		"slot":              uint32(slot),
		"detail_level":      catena.DetailLevelFull,
		"multi_set_enabled": true,
		"subscriptions":     true,
		"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
		"default_scope":     "st2138:op",
		"params": map[string]any{
			helloWorldOID: map[string]any{
				"name": map[string]any{
					"display_strings": map[string]string{
						"en": "Hello World",
					},
				},
				"type": catena.ParamTypeString,
				"value": map[string]any{
					"string_value": helloWorld.Get(),
				},
			},
		},
	}
}

func main() {
	srv, err := catena.NewServer(catena.ServerOptions{
		MaxConnections: 100,
		AuthzEnabled:   false,
	})
	if err != nil {
		logger.Error("Failed to create Catena server", "error", err)
		os.Exit(1)
	}

	srv.RegisterGetDeviceHandler(slot, func(slot uint16, ctx catena.HandlerContext) (catena.Device, catena.StatusResult) {
		logger.Info("GetDevice", "slot", slot)
		device, err := catena.ToDevice(buildDevice())
		if err != nil {
			logger.Error("Failed to convert device", "slot", slot, "error", err)
			return catena.ReplyError[catena.Device](catena.StatusCodeInternal, "failed to convert device")
		}
		return catena.Reply(device)
	})

	srv.RegisterGetValueHandler(slot, func(slot uint16, fqoid string, ctx catena.HandlerContext) (catena.Value, catena.StatusResult) {
		logger.Info("GetValue", "slot", slot, "fqoid", fqoid)
		if fqoid != helloWorldOID {
			return catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "parameter not found: "+fqoid)
		}

		value, res := catena.ToValue(helloWorld.Get())
		if res.Code != catena.StatusCodeOk {
			logger.Error("Failed to convert hello_world value", "error", res.Error)
			return catena.ReplyError[catena.Value](catena.StatusCodeInternal, "failed to convert value")
		}
		return catena.Reply(value)
	})

	srv.RegisterSetValueHandler(slot, func(slot uint16, entries []catena.SetValueEntry, ctx catena.HandlerContext) catena.StatusResult {
		logger.Info("SetValue", "slot", slot, "count", len(entries))

		for _, entry := range entries {
			if entry.Fqoid != helloWorldOID {
				return catena.StatusWithCode(catena.StatusCodeNotFound, "parameter not found: "+entry.Fqoid)
			}
			if entry.Value == nil {
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "nil value for "+entry.Fqoid)
			}
			if _, ok := entry.Value.(string); !ok {
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "hello_world requires a string value")
			}
		}

		for _, entry := range entries {
			stringValue := entry.Value.(string)
			helloWorld.Set(stringValue)
			srv.BroadcastUpdate(slot, entry.Fqoid, stringValue, catena.ScopeMon)
		}
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	})

	if err := srv.RegisterTransport(transports.NewRestTransport(config.DefaultRestOptions())); err != nil {
		logger.Error("Failed to register REST transport", "error", err)
		os.Exit(1)
	}
	if err := srv.RegisterTransport(transports.NewGrpcTransport(config.DefaultGrpcOptions())); err != nil {
		logger.Error("Failed to register gRPC transport", "error", err)
		os.Exit(1)
	}

	logger.Info("=======================================================")
	logger.Info("Hello World Example")
	logger.Info("=======================================================")
	logger.Info("REST transport listening on default port 9080")
	logger.Info("gRPC transport listening on default port 6254")
	logger.Info("Try REST:")
	logger.Info("  GET  http://localhost:9080/st2138-api/v1/0/value/hello_world")
	logger.Info("Try gRPC:")
	logger.Info("  grpcurl -plaintext -import-path ~/Catena/smpte/interface/proto -proto service.proto -d '{\"slot\":0,\"oid\":\"hello_world\"}' localhost:6254 st2138.CatenaService/GetValue")

	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()

	<-ctx.Done()
	logger.Info("Shutting down server...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), time.Second)
	defer shutdownCancel()
	srv.Shutdown(shutdownCtx)
	logger.Info("Server shutdown complete")
}
