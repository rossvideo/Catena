package main

import (
	"fmt"
	"net/http"
	"os"
	"reflect"
	"strconv"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// Implementation for registering parameter handlers for every fqoid on a given slot
func registerBasicParamHandlers(srv *rest.Server, params *sync.Map, slot int) {
	srv.RegisterSetValueHandler(slot, func(value any, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("SetParam", "slot", slot, "fqoid", fqoid)
		val, ok := params.Load(fqoid)
		if !ok {
			logger.Error("SetParam param not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyNotFound("param not found")
		}
		if reflect.TypeOf(val) != reflect.TypeOf(value) {
			logger.Error("SetParam type mismatch", "slot", slot, "fqoid", fqoid)
			return catena.ReplyBadRequest("type mismatch")
		}
		params.Store(fqoid, value)
		return catena.ReplyNoContent()
	})

	srv.RegisterGetValueHandler(slot, func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("GetParam", "slot", slot, "fqoid", fqoid)
		v, ok := params.Load(fqoid)
		if !ok {
			logger.Error("GetParam param not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyNotFound("param not found")
		}
		catenaVal, err := catena.ToCatenaValue(v)
		if err != nil {
			logger.Error("GetParam failed to convert value", "slot", slot, "fqoid", fqoid, "error", err)
			return catena.ReplyInternalError("failed to convert value")
		}
		return catena.ReplyOK(catenaVal)
	})
}

// For a given slot implement param handlers for a specific param oid only
func registerSpecificParamHandlers(srv *rest.Server, params *sync.Map, fqoid string, slot int) {
	srv.RegisterSetValueHandler(slot, func(value any, slot int, fqoid_ string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid_ != fqoid {
			return catena.ReplyNotImplemented("no handler for fqoid " + fqoid_)
		}
		logger.Info("SetSpecificParam", "slot", slot, "fqoid", fqoid_)
		val, ok := params.Load(fqoid_)
		if !ok {
			logger.Error("SetSpecificParam param not found", "slot", slot, "fqoid", fqoid_)
			return catena.ReplyNotFound("param not found")
		}
		if reflect.TypeOf(val) != reflect.TypeOf(value) {
			logger.Error("SetSpecificParam type mismatch", "slot", slot, "fqoid", fqoid_)
			return catena.ReplyBadRequest("type mismatch")
		}
		params.Store(fqoid_, value)
		return catena.ReplyNoContent()
	})

	srv.RegisterGetValueHandler(slot, func(slot int, fqoid_ string) (catena.CatenaValue, catena.StatusResult) {
		if fqoid_ != fqoid {
			return catena.ReplyNotImplemented("no handler for fqoid " + fqoid_)
		}
		logger.Info("GetSpecificParam", "slot", slot, "fqoid", fqoid_)
		v, ok := params.Load(fqoid_)
		if !ok {
			logger.Error("GetSpecificParam param not found", "slot", slot, "fqoid", fqoid_)
			return catena.ReplyNotFound("param not found")
		}
		catenaVal, err := catena.ToCatenaValue(v)
		if err != nil {
			logger.Error("GetSpecificParam failed to convert value", "slot", slot, "fqoid", fqoid_, "error", err)
			return catena.ReplyInternalError("failed to convert value")
		}
		return catena.ReplyOK(catenaVal)
	})
}

func main() {
	// Parse config from environment variables with CATENA prefix
	// and apply CLI verbosity flags (-v, -vv) if specified
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "basic_device_bl"

	err := logger.Init(cfg)
	if err != nil {
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
	logger.Info("Starting Dummy BaseServer", "port", port)

	var params0 = &sync.Map{}
	params0.Store("alpha", "alpha")
	params0.Store("beta", int32(42))
	params0.Store("counter", int32(0))

	var params1 = &sync.Map{}
	params1.Store("alpha", "default")
	params1.Store("beta", float32(3.14))
	params1.Store("status", "idle")

	var params2 = &sync.Map{}
	params2.Store("struct", map[string]any{"field1": int32(1), "field2": "two"})
	params2.Store("struct_array", []map[string]any{
		{"item": "one", "field": int32(1)},
		{"item": "two", "field": int32(2)},
		{"item": "three", "field": int32(3)},
	})
	params2.Store("struct_variant", catena.StructVariantValue{
		StructVariantType: "typeA",
		Value:             "some value",
	})
	params2.Store("struct_variant_array", []catena.StructVariantValue{
		{
			StructVariantType: "typeA",
			Value:             "value A1",
		},
		{
			StructVariantType: "typeB",
			Value:             int32(123),
		},
	})
	params2.Store("int_list", []int32{1, 2, 3, 4, 5})
	params2.Store("float_list", []float32{1.1, 2.2, 3.3})

	slotList := []int{0, 1, 2}

	// Build a BaseServer (decoupled from catena).
	srv := rest.NewServer(slotList)

	// Register param handlers for each device.
	// Device 0: basic param handlers for all params.
	registerBasicParamHandlers(srv, params0, 0)
	// Device 1: specific param handler for "alpha" only.
	registerSpecificParamHandlers(srv, params1, "alpha", 1)
	// Device 2: basic param handlers for all params.
	registerBasicParamHandlers(srv, params2, 2)

	// Register global connect handler (SSE streaming endpoint)
	srv.RegisterConnectHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		logger.Info("Connect - SSE stream requested")
		// Set SSE headers for streaming
		w.Header().Set("Content-Type", "text/event-stream")
		w.Header().Set("Cache-Control", "no-cache")
		w.Header().Set("Connection", "keep-alive")
		// For this example, just acknowledge the connection
		// A real implementation would keep the connection open and push updates
		catenaVal, err := catena.ToCatenaValue("connected")
		if err != nil {
			return catena.ReplyInternalError("failed to create response")
		}
		return catena.ReplyOK(catenaVal)
	})

	// Register command handlers for each slot
	for _, slot := range slotList {
		slot := slot // capture loop variable
		srv.RegisterExecuteCommandHandler(slot, func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
			logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid, "payload", payload)
			// Return schema-compliant command_response format
			// Options: {response: value}, {no_response: {}}, or {exception: {...}}
			response := map[string]any{
				"response": map[string]any{
					"string_value": "Command " + commandFqoid + " executed successfully",
				},
			}
			catenaVal, err := catena.ToCatenaValue(response)
			if err != nil {
				return catena.ReplyInternalError("failed to create command response")
			}
			return catena.ReplyOK(catenaVal)
		})
	}

	// Define device models for each slot
	devices := map[int]map[string]any{
		0: {
			"slot":              int32(0),
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg", "st2138:adm"},
			"default_scope":     "st2138:op",
		},
		1: {
			"slot":              int32(1),
			"multi_set_enabled": false,
			"subscriptions":     false,
			"access_scopes":     []string{"st2138:op"},
			"default_scope":     "st2138:op",
		},
		2: {
			"slot":              int32(2),
			"multi_set_enabled": true,
			"subscriptions":     true,
			"access_scopes":     []string{"st2138:mon", "st2138:op", "st2138:cfg"},
			"default_scope":     "st2138:cfg",
		},
	}

	// Register device handler (parses slot from URL path)
	for slot, deviceInfo := range devices {
		slot := slot             // capture loop variable
		deviceInfo := deviceInfo // capture loop variable
		srv.RegisterGetDeviceHandler(slot, func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
			logger.Info("GetDevice", "slot", slot)
			catenaVal, err := catena.ToCatenaValue(deviceInfo)
			if err != nil {
				return catena.ReplyInternalError("failed to create device info")
			}
			return catena.ReplyOK(catenaVal)
		})
	}

	// Register handler for non-existent endpoints
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		logger.Error("Endpoint not found", "method", r.Method, "path", r.URL.Path)
		return catena.ReplyNotFound("endpoint not found: " + r.URL.Path)
	})

	addr := ":" + strconv.Itoa(port)
	logger.Info("Dummy BaseServer listening", "address", addr)

	if err := rest.StartHTTPServer(port); err != nil {
		logger.Error("server failed", "error", err)
		os.Exit(1)
	}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
