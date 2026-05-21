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
 * @brief Shared ExecuteCommand example logic for REST and gRPC.
 * @file executecommand.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package executecommand

import (
	"fmt"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/examples/exampleutil"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

type CommandHandler func(payload any) (catena.CommandResult, catena.StatusResult)

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

var SlotList = []uint16{0}

func BuildResponse(counter *CounterState) map[string]any {
	var runningVal int32 = 0
	if counter.IsRunning() {
		runningVal = 1
	}
	return map[string]any{
		"counter": int32(counter.GetValue()),
		"running": runningVal,
	}
}

// RegisterHandlers registers the GetValue and ExecuteCommand handlers for the
// counter demo. It also starts a background goroutine that increments the
// counter every second while running.
func RegisterHandlers(srv catena.Server) {
	counter := &CounterState{}

	go func() {
		ticker := time.NewTicker(1 * time.Second)
		defer ticker.Stop()
		for range ticker.C {
			if counter.IsRunning() {
				counter.Increment()
				logger.Info("Counter tick", "value", counter.GetValue())
			}
		}
	}()

	commands := map[string]CommandHandler{
		"start": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if counter.IsRunning() {
				logger.Info("Start command - already running")
				val, _ := catena.ToValue(BuildResponse(counter))
				return catena.CommandReply(val)
			}
			counter.Start()
			logger.Info("Counter started", "value", counter.GetValue())
			val, _ := catena.ToValue(BuildResponse(counter))
			return catena.CommandReply(val)
		},

		"stop": func(payload any) (catena.CommandResult, catena.StatusResult) {
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
				val, _ := catena.ToValue(BuildResponse(counter))
				return catena.CommandReply(val)
			}
			counter.Stop()
			logger.Info("Counter stopped", "value", counter.GetValue())
			val, _ := catena.ToValue(BuildResponse(counter))
			return catena.CommandReply(val)
		},

		"add10": func(payload any) (catena.CommandResult, catena.StatusResult) {
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			val, _ := catena.ToValue(BuildResponse(counter))
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

	srv.RegisterGetValueHandler(0, func(slot uint16, fqoid string) (catena.Value, catena.StatusResult) {
		if fqoid == "counter" {
			val, _ := catena.ToValue(BuildResponse(counter))
			return catena.Reply(val)
		}
		return catena.ReplyError[catena.Value](catena.NOT_FOUND, "parameter not found: "+fqoid)
	})

	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

		handler, ok := commands[commandFqoid]
		if !ok {
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.CommandError(catena.NOT_FOUND, "Command not found: "+commandFqoid)
		}

		return handler(payload)
	})
}

// RunExample encapsulates the full example lifecycle:
// SDK init, signal handling, server creation, handler registration, and graceful shutdown.
func RunExample(appName string) {
	exampleutil.RunExample(exampleutil.RunConfig{
		AppName:          appName,
		Slots:            SlotList,
		RegisterHandlers: RegisterHandlers,
		OnReady: func(cfg catena.Config) {
			port := cfg.Port
			logger.Info("=======================================================")
			logger.Info("ExecuteCommand Example", "transport", appName)
			logger.Info("=======================================================")
			logger.Info("Listening", "port", port)
			logger.Info("")
			logger.Info("Web UI available at:")
			logger.Info(fmt.Sprintf("  http://localhost:%d/", port))
			logger.Info("")
			logger.Info("=======================================================")
		},
	})
}
