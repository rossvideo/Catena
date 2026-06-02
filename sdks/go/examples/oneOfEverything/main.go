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
 * @brief Shared oneOfEverything example logic for REST and gRPC.
 * @file oneofeverything.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 */

package main

// oneOfEverything is a reference server that demonstrates the Catena Go SDK end
// to end. The same handler registrations serve both REST and gRPC transports.
//
// Adopter map — where to look:
//   - main.go (this file): application state, device model (buildDeviceDefinition),
//     server bootstrap, transports, embedded demo UI/assets
//   - device_handler.go:     GetDevice handler registration
//   - value_handlers.go:     GetValue / SetValue per slot (three dispatch styles)
//   - param_info_handler.go: ParamInfo per slot (manual vs device-delegated)
//   - command_handler.go:    ExecuteCommand (slot 0 counter commands)
//   - asset_handler.go:      ExternalObject / asset download
//   - access_handler.go:     coarse endpoint authorization gate
//   - heartbeat_handler.go:  periodic BroadcastUpdate for quiet slots
//
// Run: go run . --use-rest [--use-grpc]   (authz disabled here for easy local testing)

import (
	"context"
	"embed"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"sort"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/transports"
)

//go:embed static/*
var StaticFS embed.FS // binary assets served via GetAssetHandler (ExternalObjectRequest)

//go:embed webui/*
var webFS embed.FS // demo dashboard; served by REST fallback handler, not Catena API

// CounterState backs slot 0's live counter demo: a ticking value plus a
// read-only "running" flag changed only by start/stop commands.
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

// RunningInt32 returns the running state as an int32 (1 = running, 0 = stopped),
// matching the on-the-wire representation of the "running" param.
func (c *CounterState) RunningInt32() int32 {
	if c.IsRunning() {
		return 1
	}
	return 0
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

func (c *CounterState) SetValue(value int32) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.value = value
}

func (c *CounterState) Increment() {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.running {
		c.value++
	}
}

var slotList = []uint16{0, 1, 2} // every handler file iterates this to register per-slot callbacks

// ExampleState is the in-memory device model shared by all handlers. Each slot
// intentionally uses a different storage style so adopters can pick a pattern:
//   - slot 0 fields + CounterState: explicit typed fields, manual switch handlers
//   - slot 1 slotOneParams sync.Map: map-backed params, direct key lookup
//   - slot 2 typed fields + product map: prefix-based dispatch in value_handlers
//
// Slot 2's sample_* fields demonstrate the remaining Catena param types (except DATA).
type ExampleState struct {
	mu                       sync.RWMutex
	slotZeroProduct          map[string]any
	sampleIntArray           []int32
	slotOneParams            *sync.Map
	slotTwoProduct           map[string]any
	volume                   int32
	muted                    int32
	deviceName               string
	structExample            map[string]any
	sampleFloat              float32
	sampleFloatArray         []float32
	sampleStringArray        []string
	sampleBinary             []byte
	sampleStructVariant      catena.StructVariantValue
	sampleStructArray        []map[string]any
	sampleStructVariantArray []catena.StructVariantValue
}

func NewExampleState() *ExampleState {
	// Defaults here are what GetValue returns on first request and what
	// buildDeviceDefinition embeds into each param's initial "value" field.
	state := &ExampleState{
		slotZeroProduct: map[string]any{
			"name":    "Counter Slot 0 Product",
			"vendor":  "Ross Video",
			"version": "1.0.0",
		},
		sampleIntArray: []int32{1, 2, 3, 4},
		slotOneParams:  &sync.Map{},
		slotTwoProduct: map[string]any{
			"name":               "Prefix Slot 2 Product",
			"vendor":             "Ross Video",
			"version":            "1.0.0",
			"catena_sdk":         "github.com/rossvideo/catena/sdks/go",
			"catena_sdk_version": "v0.1.0",
			"serial_number":      "SN12345678",
		},
		volume:     75,
		muted:      0,
		deviceName: "Demo Device",
		structExample: map[string]any{
			"number": int32(2),
			"text":   "Slot 2 struct",
		},
		sampleFloat:       3.14,
		sampleFloatArray:  []float32{1.1, 2.2, 3.3},
		sampleStringArray: []string{"alpha", "beta", "gamma"},
		sampleBinary:      []byte{0xCA, 0xFE, 0xBA, 0xBE},
		sampleStructVariant: catena.StructVariantValue{
			StructVariantType: "int_kind",
			Value:             int32(42),
		},
		sampleStructArray: []map[string]any{
			{"label": "entry_a", "count": int32(1)},
			{"label": "entry_b", "count": int32(2)},
		},
		sampleStructVariantArray: []catena.StructVariantValue{
			{StructVariantType: "int_kind", Value: int32(10)},
			{StructVariantType: "string_kind", Value: "hello"},
		},
	}
	state.slotOneParams.Store("resolution", "1920x1080")
	state.slotOneParams.Store("brightness", int32(50))
	state.slotOneParams.Store("contrast", int32(50))
	state.slotOneParams.Store("saturation", int32(50))
	return state
}

func (s *ExampleState) sampleIntArrayValue() []int32 {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.sampleIntArray
}

func (s *ExampleState) slotZeroProductValue(fqoid string) (any, bool) {
	// Slot 0 product subtree lookup; called from value_handlers case switch.
	s.mu.RLock()
	defer s.mu.RUnlock()

	switch fqoid {
	case "product":
		return s.slotZeroProduct, true
	case "product/name":
		value, ok := s.slotZeroProduct["name"]
		return value, ok
	case "product/vendor":
		value, ok := s.slotZeroProduct["vendor"]
		return value, ok
	case "product/version":
		value, ok := s.slotZeroProduct["version"]
		return value, ok
	default:
		return nil, false
	}
}

func (s *ExampleState) slotTwoProductValue(fqoid string) (any, bool) {
	// Slot 2 product subtree lookup; prefix-dispatched from value_handlers.
	s.mu.RLock()
	defer s.mu.RUnlock()

	switch fqoid {
	case "product":
		return s.slotTwoProduct, true
	case "product/name":
		value, ok := s.slotTwoProduct["name"]
		return value, ok
	case "product/vendor":
		value, ok := s.slotTwoProduct["vendor"]
		return value, ok
	case "product/version":
		value, ok := s.slotTwoProduct["version"]
		return value, ok
	case "product/catena_sdk":
		value, ok := s.slotTwoProduct["catena_sdk"]
		return value, ok
	case "product/catena_sdk_version":
		value, ok := s.slotTwoProduct["catena_sdk_version"]
		return value, ok
	case "product/serial_number":
		value, ok := s.slotTwoProduct["serial_number"]
		return value, ok
	default:
		return nil, false
	}
}

func (s *ExampleState) sampleFloatArrayValue() []float32 {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.sampleFloatArray
}

func (s *ExampleState) sampleStringArrayValue() []string {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.sampleStringArray
}

func (s *ExampleState) sampleStructArrayValue() []map[string]any {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.sampleStructArray
}

func (s *ExampleState) sampleStructVariantArrayValue() []catena.StructVariantValue {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.sampleStructVariantArray
}

func sortedAssetIDs(assets *sync.Map) []string {
	var ids []string
	assets.Range(func(key, _ any) bool {
		if id, ok := key.(string); ok {
			ids = append(ids, id)
		}
		return true
	})
	sort.Strings(ids)
	return ids
}

func main() {
	// --- Configuration & logging ---
	// Flags/env: --use-rest, --use-grpc, --authz, log level, etc. See config.InitOptions.
	options, err := config.InitOptions("oneofeverything", os.Args[1:])
	if err != nil {
		if errors.Is(err, config.ErrHelp) {
			os.Exit(0)
		}
		os.Exit(1)
	}
	options.Server.AuthzEnabled = false // disable JWT scope checks for local demo use
	closeLogger, err := logger.Init(options.Logger)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
	defer closeLogger()

	logger.Info("Loaded Configuration", "config", options)

	srv, err := catena.NewServer(options.Server)
	if err != nil {
		logger.Error("Failed to create Catena server", "error", err)
		os.Exit(1)
	}

	// --- Application state ---
	// Handlers receive these pointers; mutations here are what Get/SetValue expose.
	counter := &CounterState{}
	state := NewExampleState()
	counterScope := catena.ScopeCfg // scope tag on counter BroadcastUpdate ticks

	// Background goroutine: increment counter every second while running and push
	// updates to SSE/gRPC subscribers. Command handler toggles running via broadcastRunning.
	go func() {
		ticker := time.NewTicker(1 * time.Second)
		defer ticker.Stop()
		for range ticker.C {
			if counter.IsRunning() {
				counter.Increment()
				logger.Info("Counter tick", "value", counter.GetValue())
				srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			}
		}
	}()
	counter.Start()

	// broadcastRunning publishes the current running flag on the "running" param
	// so subscribers (REST SSE / gRPC stream) see the state change live.
	broadcastRunning := func() {
		srv.BroadcastUpdate(0, "running", counter.RunningInt32(), catena.ScopeMon)
	}

	assets := &sync.Map{}
	payloads, err := catena.LoadPayloadsFromEmbed(StaticFS, "static")
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to load embedded assets: %v\n", err)
		os.Exit(1)
	}
	for id, payload := range payloads {
		assets.Store(id, payload)
	}
	assetIDs := sortedAssetIDs(assets)

	// --- Handler registration ---
	// Each register* function wires one Catena endpoint family. Transports call
	// these through the SDK runtime; you do not implement HTTP/gRPC routing here.
	registerAccessHandler(srv)                                           // EndpointAccess gate
	registerDeviceHandlers(srv, counter, state)                          // GetDevice → buildDeviceDefinition
	registerValueHandlers(srv, counter, state)                           // GetValue / SetValue
	registerCommandHandler(srv, counter, broadcastRunning, counterScope) // ExecuteCommand
	registerAssetHandlers(srv, assets)                                   // ExternalObjectRequest
	registerParamInfoHandlers(srv, counter, state)                       // ParamInfoRequest
	registerHeartbeatHandlers(srv, state)                                // periodic BroadcastUpdate

	srv.StartHeartbeat(5 * time.Second)

	if !options.UseGrpc && !options.UseRest {
		logger.Error("No transports enabled", "error", "at least one of gRPC or REST transport must be enabled in config")
		os.Exit(1)
	}

	// DashBoard "Detect Frame Information" connection-props HTTP server.
	// Advertises this device so DashBoard can resolve and populate the
	// connection dialog. Works for both REST and gRPC devices.
	dashboardOpts := options.Dashboard
	dashboardOpts.Protocol = catena.ProtocolST2138Rest
	if options.UseGrpc {
		dashboardOpts.Protocol = catena.ProtocolST2138Grpc
	}
	dashboardOpts.RefreshInterval = 30000
	dashboardOpts.NodeName = "One of Everything Demo"
	dashboardOpts.NodeID = "one-of-everything-a4:bb:6d:6a:6f:a3"
	connectionProps := catena.NewConnectionProps(dashboardOpts)
	connectionPropsURL := fmt.Sprintf("http://localhost:%d%s", options.Dashboard.Port, connectionProps.Endpoint())
	if err := connectionProps.Start(); err != nil {
		logger.Warning("Failed to start connection props server", "port", options.Dashboard.Port, "error", err)
		connectionPropsURL = ""
	}

	sampleAsset := ""
	if len(assetIDs) > 0 {
		sampleAsset = assetIDs[0]
	}

	// --- Transports ---
	// Register one or both; the same handlers serve REST (port 8080) and gRPC (6254).
	if options.UseGrpc {
		// Reflection enabled so grpcurl works without -proto (local demo only).
		if err := srv.RegisterTransport(transports.NewGrpcTransport(options.Grpc)); err != nil {
			logger.Error("Failed to register gRPC transport", "error", err)
			os.Exit(1)
		}
		logGrpcEndpointGuide("localhost:6254", sampleAsset)
	} else {
		logger.Info("gRPC transport disabled by config")
	}

	if options.UseRest {
		restTransport := transports.NewRestTransport(options.Rest)

		// RegisterFallbackHandler is for custom HTTP routes that are not part of
		// the Catena API. SDK endpoints are still handled by the REST transport;
		// this fallback only serves the demo UI and an asset index convenience
		// route when no SDK route matched.
		restTransport.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.Value, catena.StatusResult) {
			if r.URL.Path == "/assets-list" {
				var assetList []map[string]any
				assets.Range(func(key, value any) bool {
					payload := value.(catena.DataPayload)
					assetList = append(assetList, map[string]any{
						"id":           key.(string),
						"content_type": payload.Metadata["content-type"],
						"file_name":    payload.Metadata["file-name"],
						"size":         len(payload.Payload),
					})
					return true
				})
				w.Header().Set("Content-Type", "application/json")
				json.NewEncoder(w).Encode(assetList)
				return catena.Reply(catena.Value{})
			}

			fileMap := map[string]struct {
				path        string
				contentType string
			}{
				"/":           {"webui/index.htm", "text/html; charset=utf-8"},
				"/styles.css": {"webui/styles.css", "text/css; charset=utf-8"},
				"/script.js":  {"webui/script.js", "application/javascript; charset=utf-8"},
			}

			if file, ok := fileMap[r.URL.Path]; ok {
				data, err := webFS.ReadFile(file.path)
				if err != nil {
					return catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "file not found: "+r.URL.Path)
				}
				w.Header().Set("Content-Type", file.contentType)
				w.Write(data)
				return catena.Reply(catena.Value{})
			}
			return catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "endpoint not found: "+r.URL.Path)
		})

		if err := srv.RegisterTransport(restTransport); err != nil {
			logger.Error("Failed to register REST transport", "error", err)
			os.Exit(1)
		}

		logRestEndpointGuide(sampleAsset)
		if len(assetIDs) > 0 {
			logger.Info("")
			logger.Info("Available assets:")
			for _, id := range assetIDs {
				logger.Info(fmt.Sprintf("  %s", id))
			}
		}
	} else {
		logger.Info("REST transport disabled by config")
	}

	// Startup summary: header, then one section per transport/service with an
	// ENABLED/DISABLED indicator and its endpoints.
	status := func(on bool) string {
		if on {
			return "ENABLED"
		}
		return "DISABLED"
	}

	logger.Info("")
	logger.Info("=======================================================")
	logger.Info("One of Everything Example")
	logger.Info("=======================================================")

	logger.Info("")
	logger.Info("[ gRPC transport ]", "status", status(options.UseGrpc))
	if options.UseGrpc {
		grpcAddr := fmt.Sprintf("localhost:%d", options.Dashboard.ServicePort)
		logger.Info("    address", "value", grpcAddr)
		logger.Info("    query", "command", "grpcurl -plaintext "+grpcAddr+" list")
	}

	logger.Info("")
	logger.Info("[ REST transport ]", "status", status(options.UseRest))
	if options.UseRest {
		logger.Info("    web ui", "url", "http://localhost:9080/")
	}

	logger.Info("")
	logger.Info("[ DashBoard connection props ]", "status", status(connectionPropsURL != ""))
	if connectionPropsURL != "" {
		logger.Info("    endpoint", "url", connectionPropsURL)
	}

	logger.Info("")
	logger.Info("=======================================================")

	// Block until Ctrl+C, then shut down transports and streaming connections.
	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()

	<-ctx.Done()
	logger.Info("Shutting down server...")

	shutdownCtx, shutdownCancel := context.WithTimeout(context.Background(), time.Second)
	defer shutdownCancel()
	if err := connectionProps.Stop(shutdownCtx); err != nil {
		logger.Warning("Error stopping connection props server", "error", err)
	}
	srv.Shutdown(shutdownCtx)
	logger.Info("Server shutdown complete")
}
