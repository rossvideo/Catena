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
 * @brief ExecuteCommand gRPC example - Counter Demo.
 * @file executeCommand_GRPC.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-04-01
 */

package main

import (
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	grpcServer "github.com/rossvideo/catena/sdks/go/pkg/grpc"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// CommandHandler processes a command and returns a CommandResult
type CommandHandler func(payload any) (catena.CommandResult, catena.StatusResult)

// Global state for graceful shutdown
var shutdownChan = make(chan struct{})

// CounterState holds the counter value and running state with thread-safe access.
type CounterState struct {
	mu      sync.RWMutex
	value   int32
	running bool
}

func (c *CounterState) GetValue() int32 {
	c.mu.RLock()
	defer c.mu.RUnlock()
	return c.value
}

func (c *CounterState) IsRunning() bool {
	c.mu.RLock()
	defer c.mu.RUnlock()
	return c.running
}

func (c *CounterState) Start() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.running = true
}

func (c *CounterState) Stop() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.running = false
}

func (c *CounterState) Add(n int32) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.value += n
}

func (c *CounterState) Reset() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.value = 0
}

func (c *CounterState) Increment() {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.running {
		c.value++
	}
}

func main() {
	cfg, err := catena.InitOptions(catena.Options{AppName: "executeCommand_GRPC"})
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize SDK: %v\n", err)
		os.Exit(1)
	}
	defer catena.Close()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	port := cfg.Port

	counter := &CounterState{}

	go func() {
		ticker := time.NewTicker(1 * time.Second)
		defer ticker.Stop()
		for {
			select {
			case <-ticker.C:
				if counter.IsRunning() {
					counter.Increment()
					logger.Info("Counter tick", "value", counter.GetValue())
				}
			case <-shutdownChan:
				return
			}
		}
	}()

	buildResponse := func() map[string]any {
		var runningVal int32 = 0
		if counter.IsRunning() {
			runningVal = 1
		}
		return map[string]any{
			"counter": int32(counter.GetValue()),
			"running": runningVal,
		}
	}

	// ==========================================================================
	// Commands
	// ==========================================================================
	commands := map[string]CommandHandler{
		"start": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if counter.IsRunning() {
				logger.Info("Start command - already running")
				val, _ := catena.ToCatenaValue(buildResponse())
				return catena.CommandReply(val)
			}
			counter.Start()
			logger.Info("Counter started", "value", counter.GetValue())
			val, _ := catena.ToCatenaValue(buildResponse())
			return catena.CommandReply(val)
		},

		"stop": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
				val, _ := catena.ToCatenaValue(buildResponse())
				return catena.CommandReply(val)
			}
			counter.Stop()
			logger.Info("Counter stopped", "value", counter.GetValue())
			val, _ := catena.ToCatenaValue(buildResponse())
			return catena.CommandReply(val)
		},

		"add10": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			val, _ := catena.ToCatenaValue(buildResponse())
			return catena.CommandReply(val)
		},

		"reset": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Reset()
			logger.Info("Counter reset")
			return catena.CommandNoResponse()
		},

		"error": func(payload any) (catena.CommandResult, catena.StatusResult) {
			logger.Info("Error command executed")
			return catena.CommandExceptionResult(
				"DemoError",
				"This is a demonstration of a command exception",
				catena.NewPolyglotText("en", "An example error occurred"),
			)
		},
	}

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []uint16{0}
	server := grpcServer.NewServer(slotList, 100, cfg)

	server.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid == "counter" {
			val, _ := catena.ToCatenaValue(buildResponse())
			return catena.Reply(val)
		}
		return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "parameter not found: "+fqoid)
	})

	server.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

		handler, ok := commands[commandFqoid]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.CommandError(catena.NOT_FOUND, "Command not found: "+commandFqoid)
		}

		return handler(payload)
	})

	// ==========================================================================
	// Start Server
	// ==========================================================================
	logger.Info("=======================================================")
	logger.Info("ExecuteCommand gRPC Example")
	logger.Info("=======================================================")
	logger.Info("gRPC server starting", "port", port)
	logger.Info("")
	logger.Info("Available commands: start, stop, add10, reset, error")
	logger.Info("")
	logger.Info("Test with grpcurl:")
	logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid\": \"start\", \"respond\": true}' localhost:%d st2138.CatenaService/ExecuteCommand", port))
	logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid\": \"stop\", \"respond\": true}' localhost:%d st2138.CatenaService/ExecuteCommand", port))
	logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid\": \"add10\", \"respond\": true}' localhost:%d st2138.CatenaService/ExecuteCommand", port))
	logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid\": \"reset\", \"respond\": true}' localhost:%d st2138.CatenaService/ExecuteCommand", port))
	logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid\": \"error\", \"respond\": true}' localhost:%d st2138.CatenaService/ExecuteCommand", port))
	logger.Info("")
	logger.Info("Get counter value:")
	logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid\": \"counter\"}' localhost:%d st2138.CatenaService/GetValue", port))
	logger.Info("=======================================================")

	go func() {
		if err := server.Start(port); err != nil {
			logger.Error("server failed", "error", err)
			os.Exit(1)
		}
	}()

	<-shutdownChan
	server.Shutdown()
	logger.Info("Server shutdown complete")
}
