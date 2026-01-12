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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief ExecuteCommand REST example.
 * @file executeCommand_REST.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-12
 */

package main

import (
	"fmt"
	"net/http"
	"os"
	"strconv"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// CommandHandler processes a command and returns a response
type CommandHandler func(payload any) catena.StatusResult

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "command_example"

	if err := logger.Init(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
	defer logger.Close()

	portStr := envOr("CATENA_PORT", "6254")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		logger.Error("invalid CATENA_PORT", "error", err)
		os.Exit(1)
	}

	// Device state (shared across commands)
	deviceState := &sync.Map{}
	deviceState.Store("running", false)
	deviceState.Store("counter", 0)

	// ==========================================================================
	// Slot 0: Basic Commands (simple request/response)
	// ==========================================================================
	basicCommands := map[string]CommandHandler{
		// Simple echo command - returns the input
		"echo": func(payload any) catena.StatusResult {
			logger.Info("Executing echo command", "payload", payload)
			return catena.OK(map[string]any{
				"response": map[string]any{
					"echo": payload,
				},
			})
		},

		// Ping command - no input, simple response
		"ping": func(payload any) catena.StatusResult {
			logger.Info("Executing ping command")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"string_value": "pong",
					"timestamp":    time.Now().Unix(),
				},
			})
		},

		// Get status command - returns device state
		"get_status": func(payload any) catena.StatusResult {
			running, _ := deviceState.Load("running")
			counter, _ := deviceState.Load("counter")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"running": running,
					"counter": counter,
					"uptime":  time.Now().Unix(),
				},
			})
		},
	}

	// ==========================================================================
	// Slot 1: Action Commands (commands with side effects)
	// ==========================================================================
	actionCommands := map[string]CommandHandler{
		// Start command - starts a process
		"start": func(payload any) catena.StatusResult {
			running, _ := deviceState.Load("running")
			if running.(bool) {
				return catena.OK(map[string]any{
					"exception": map[string]any{
						"error_code":    1001,
						"error_message": "Already running",
					},
				})
			}
			deviceState.Store("running", true)
			logger.Info("Device started")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"string_value": "started",
				},
			})
		},

		// Stop command - stops a process
		"stop": func(payload any) catena.StatusResult {
			running, _ := deviceState.Load("running")
			if !running.(bool) {
				return catena.OK(map[string]any{
					"exception": map[string]any{
						"error_code":    1002,
						"error_message": "Not running",
					},
				})
			}
			deviceState.Store("running", false)
			logger.Info("Device stopped")
			return catena.OK(map[string]any{
				"response": map[string]any{
					"string_value": "stopped",
				},
			})
		},

		// Increment counter command
		"increment": func(payload any) catena.StatusResult {
			counter, _ := deviceState.Load("counter")
			newCounter := counter.(int) + 1
			deviceState.Store("counter", newCounter)
			return catena.OK(map[string]any{
				"response": map[string]any{
					"int32_value": newCounter,
				},
			})
		},

		// Reset command - resets device state
		"reset": func(payload any) catena.StatusResult {
			deviceState.Store("running", false)
			deviceState.Store("counter", 0)
			logger.Info("Device reset")
			return catena.OK(map[string]any{
				"no_response": map[string]any{},
			})
		},
	}

	// ==========================================================================
	// Slot 2: Complex Commands (structured inputs/outputs)
	// ==========================================================================
	complexCommands := map[string]CommandHandler{
		// Route command - takes structured input
		"route": func(payload any) catena.StatusResult {
			logger.Info("Executing route command", "payload", payload)
			// Expect payload like: {"source": 1, "destination": 2}
			if payloadMap, ok := payload.(map[string]any); ok {
				source := payloadMap["source"]
				dest := payloadMap["destination"]
				return catena.OK(map[string]any{
					"response": map[string]any{
						"routed":      true,
						"source":      source,
						"destination": dest,
					},
				})
			}
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"error_code":    2001,
					"error_message": "Invalid payload format",
				},
			})
		},

		// Batch route command - takes array of routes
		"batch_route": func(payload any) catena.StatusResult {
			logger.Info("Executing batch_route command", "payload", payload)
			if routes, ok := payload.([]any); ok {
				results := make([]map[string]any, 0, len(routes))
				for i, route := range routes {
					if routeMap, ok := route.(map[string]any); ok {
						results = append(results, map[string]any{
							"index":  i,
							"source": routeMap["source"],
							"dest":   routeMap["destination"],
							"status": "routed",
						})
					}
				}
				return catena.OK(map[string]any{
					"response": map[string]any{
						"routes_processed": len(results),
						"results":          results,
					},
				})
			}
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"error_code":    2002,
					"error_message": "Expected array of routes",
				},
			})
		},

		// Configure command - applies a complex configuration
		"configure": func(payload any) catena.StatusResult {
			logger.Info("Executing configure command", "payload", payload)
			// Simulate configuration validation and application
			if config, ok := payload.(map[string]any); ok {
				// Validate required fields
				if _, hasVideo := config["video"]; !hasVideo {
					return catena.OK(map[string]any{
						"exception": map[string]any{
							"error_code":    2003,
							"error_message": "Missing 'video' configuration",
						},
					})
				}
				return catena.OK(map[string]any{
					"response": map[string]any{
						"configured": true,
						"applied":    config,
					},
				})
			}
			return catena.OK(map[string]any{
				"exception": map[string]any{
					"error_code":    2004,
					"error_message": "Invalid configuration format",
				},
			})
		},
	}

	slotList := []int{0, 1, 2}
	srv := rest.NewServer(slotList)

	commandMaps := map[int]map[string]CommandHandler{
		0: basicCommands,
		1: actionCommands,
		2: complexCommands,
	}

	slotDescriptions := map[int]string{
		0: "Basic Commands (echo, ping, get_status)",
		1: "Action Commands (start, stop, increment, reset)",
		2: "Complex Commands (route, batch_route, configure)",
	}

	for slot, commands := range commandMaps {
		slot := slot
		commands := commands
		desc := slotDescriptions[slot]

		srv.RegisterExecuteCommandHandler(slot, func(w http.ResponseWriter, r *http.Request, slotNum int, commandFqoid string, payload any) catena.StatusResult {
			logger.Info("ExecuteCommand", "slot", slotNum, "command", commandFqoid, "type", desc)

			handler, ok := commands[commandFqoid]
			if !ok {
				logger.Warning("Command not found", "slot", slotNum, "command", commandFqoid)
				return catena.OK(map[string]any{
					"exception": map[string]any{
						"error_code":    404,
						"error_message": "Command not found: " + commandFqoid,
					},
				})
			}

			return handler(payload)
		})
	}

	// Not found handler
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	logger.Info("Command Example listening", "port", port)
	logger.Info("Available slots and commands:")
	for slot, desc := range slotDescriptions {
		logger.Info("  ", "slot", slot, "description", desc)
	}
	logger.Info("Example requests:")
	logger.Info("  POST /st2138-api/v1/0/command/ping")
	logger.Info("  POST /st2138-api/v1/1/command/start")
	logger.Info("  POST /st2138-api/v1/2/command/route {\"source\":1,\"destination\":2}")

	srv.StartHTTPServer(port)
	select {}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

