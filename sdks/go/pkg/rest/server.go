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
 * @brief REST server for the Catena SDK.
 * @file server.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-04
 */

package rest

import (
	"encoding/json"
	"fmt"
	"math"
	"net/http"
	"strconv"
	"strings"
	"sync"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/sdks/go/internal"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Handlers now return (CatenaValue, StatusResult) so the server can respond consistently.
type DeviceHandler func() (catena.CatenaDevice, catena.StatusResult)
type GetValueHandler func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult)
type SetValueHandler func(value any, slot int, fqoid string) catena.StatusResult
type GetAssetHandler func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult)
type ConnectHandler func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult)
type ExecuteCommandHandler func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult)
type FallbackHandler func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult)

// Server provides REST API endpoints for Catena devices
type Server struct {
	mu                     sync.Mutex
	mux                    *http.ServeMux
	getDeviceHandlers      map[int]DeviceHandler
	getValueHandlers       map[int]GetValueHandler
	setValueHandlers       map[int]SetValueHandler
	getAssetHandlers       map[int]GetAssetHandler
	connectHandler         ConnectHandler
	executeCommandHandlers map[int]ExecuteCommandHandler
	fallbackHandler        FallbackHandler
}

// writeHTTPResult is a unified function that handles writing different response types
func writeHTTPResult(w http.ResponseWriter, result catena.StatusResult, value interface{}) {
	httpStatus := result.Code.ToHTTPStatus()

	if result.Error != "" {
		// Only return detailed error messages in dev mode
		if catena.IsDev() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
	}

	// Handle different value types
	switch v := value.(type) {
	case catena.CatenaValue:
		writeValueResult(w, v, httpStatus)
	case catena.CatenaDevice:
		writeDeviceResult(w, v, httpStatus)
	case catena.CatenaAsset:
		writeAssetResult(w, v, httpStatus)
	default:
		w.WriteHeader(httpStatus)
	}
}

// writeHTTPStatusResult writes a StatusResult to the HTTP response (no value).
func writeHTTPStatusResult(w http.ResponseWriter, result catena.StatusResult) {
	httpStatus := result.Code.ToHTTPStatus()

	if result.Error != "" {
		// Only return detailed error messages in dev mode
		if catena.IsDev() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
	}

	w.WriteHeader(httpStatus)
}

// writeValueResult writes a CatenaValue as JSON
func writeValueResult(w http.ResponseWriter, value catena.CatenaValue, httpStatus int) {
	protoValue := value.Value
	if protoValue == nil {
		w.WriteHeader(httpStatus)
		return
	}

	if err := internal.WriteResponseJSON(w, protoValue, httpStatus); err != nil {
		logger.Error("failed to write value response", "error", err)
		w.WriteHeader(http.StatusInternalServerError)
	}
}

// writeDeviceResult writes a CatenaDevice as JSON
func writeDeviceResult(w http.ResponseWriter, device catena.CatenaDevice, httpStatus int) {
	protoDevice := device.GetProtoDevice()
	if protoDevice == nil {
		w.WriteHeader(httpStatus)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	b, err := (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(protoDevice)
	if err != nil {
		logger.Error("failed to marshal device response", "error", err)
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": "failed to marshal device response"})
		return
	}

	w.WriteHeader(httpStatus)
	if _, writeErr := w.Write(b); writeErr != nil {
		logger.Error("failed to write device response", "error", writeErr)
	}
}

// writeAssetResult writes a CatenaAsset as JSON-encoded ExternalObjectPayload
func writeAssetResult(w http.ResponseWriter, asset catena.CatenaAsset, httpStatus int) {
	protoAsset := asset.GetProtoAsset()
	if protoAsset == nil {
		w.WriteHeader(http.StatusInternalServerError)
		return
	}

	// Convert asset to JSON
	jsonData, err := asset.ToJSON()
	if err != nil {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": "Asset payload is missing"})
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(httpStatus)
	if _, writeErr := w.Write(jsonData); writeErr != nil {
		logger.Error("failed to write asset response", "error", writeErr)
	}
}

// NewServer creates a new REST server for the given device slots
func NewServer(slots []int) *Server {
	s := &Server{
		mux:                    http.NewServeMux(),
		getDeviceHandlers:      make(map[int]DeviceHandler),
		getValueHandlers:       make(map[int]GetValueHandler),
		setValueHandlers:       make(map[int]SetValueHandler),
		getAssetHandlers:       make(map[int]GetAssetHandler),
		executeCommandHandlers: make(map[int]ExecuteCommandHandler),
	}

	// Register default handlers for each slot
	for _, slot := range slots {
		s.RegisterGetDeviceHandler(slot, DefaultDeviceHandler)
		s.RegisterGetValueHandler(slot, DefaultGetValueHandler)
		s.RegisterSetValueHandler(slot, DefaultSetValueHandler)
		s.RegisterGetAssetHandler(slot, DefaultGetAssetHandler)
		s.RegisterExecuteCommandHandler(slot, DefaultExecuteCommandHandler)
	}

	// Register default connect handler
	s.RegisterConnectHandler(DefaultConnectHandler)

	// Register routes
	s.RegisterRoutes()

	return s
}

// Start starts the HTTP server on the specified port using this server's mux
func (s *Server) Start(port int) error {
	addr := fmt.Sprintf(":%d", port)
	logger.Info("Starting HTTP server", "address", addr)
	return http.ListenAndServe(addr, s.mux)
}

// Default handlers that return "not implemented"

func DefaultDeviceHandler() (catena.CatenaDevice, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "GetDevice not implemented")
}

func DefaultGetValueHandler(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaValue](catena.UNIMPLEMENTED, "GetValue not implemented")
}

func DefaultSetValueHandler(value any, slot int, fqoid string) catena.StatusResult {
	return catena.StatusWithCode(catena.UNIMPLEMENTED, "SetValue not implemented")
}

func DefaultGetAssetHandler(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "GetAsset not implemented")
}

func DefaultConnectHandler(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaValue](catena.UNIMPLEMENTED, "Connect not implemented")
}

func DefaultExecuteCommandHandler(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaValue](catena.UNIMPLEMENTED, "ExecuteCommand not implemented")
}

// Handler registration methods

func (s *Server) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getDeviceHandlers[slot] = handler
}

func (s *Server) RegisterGetValueHandler(slot int, handler GetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getValueHandlers[slot] = handler
}

func (s *Server) RegisterSetValueHandler(slot int, handler SetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.setValueHandlers[slot] = handler
}

func (s *Server) RegisterGetAssetHandler(slot int, handler GetAssetHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getAssetHandlers[slot] = handler
}

func (s *Server) RegisterConnectHandler(handler ConnectHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.connectHandler = handler
}

func (s *Server) RegisterExecuteCommandHandler(slot int, handler ExecuteCommandHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.executeCommandHandlers[slot] = handler
}

func (s *Server) RegisterFallbackHandler(handler FallbackHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.fallbackHandler = handler
}

// Lookup helper functions

func (s *Server) lookupGetValue(slot int) GetValueHandler {
	if handler, ok := s.getValueHandlers[slot]; ok {
		return handler
	}
	return DefaultGetValueHandler
}

func (s *Server) lookupSetValue(slot int) SetValueHandler {
	if handler, ok := s.setValueHandlers[slot]; ok {
		return handler
	}
	return DefaultSetValueHandler
}

func (s *Server) lookupGetAsset(slot int) GetAssetHandler {
	if handler, ok := s.getAssetHandlers[slot]; ok {
		return handler
	}
	return DefaultGetAssetHandler
}

func (s *Server) lookupConnect() ConnectHandler {
	if s.connectHandler != nil {
		return s.connectHandler
	}
	return DefaultConnectHandler
}

func (s *Server) lookupExecuteCommand(slot int) ExecuteCommandHandler {
	if handler, ok := s.executeCommandHandlers[slot]; ok {
		return handler
	}
	return DefaultExecuteCommandHandler
}

// RegisterRoutes sets up all HTTP routes
func (s *Server) RegisterRoutes() {
	// Device endpoint: GET /st2138-api/v1/{slot}
	s.mux.HandleFunc("/st2138-api/v1/", func(w http.ResponseWriter, r *http.Request) {
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid path format")
			writeHTTPResult(w, res, val)
			return
		}

		slotStr := parts[2]
		slot, err := strconv.Atoi(slotStr)
		if err != nil || slot < 0 || slot > math.MaxInt16 {
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid slot number")
			writeHTTPResult(w, res, val)
			return
		}

		// Route based on path structure
		if len(parts) == 3 && r.Method == http.MethodGet {
			// GET /st2138-api/v1/{slot} - Get device info
			handler, ok := s.getDeviceHandlers[slot]
			if !ok {
				device, res := catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "device not found")
				writeHTTPResult(w, res, device)
				return
			}
			device, res := handler()
			writeHTTPResult(w, res, device)
			return
		}

		if len(parts) >= 4 {
			endpoint := parts[3]
			switch endpoint {
			case "value":
				s.handleValueEndpoint(w, r, slot, parts[4:])
			case "asset":
				s.handleAssetEndpoint(w, r, slot, parts[4:])
			case "command":
				s.handleCommandEndpoint(w, r, slot, parts[4:])
			default:
				val, res := catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "unknown endpoint")
				writeHTTPResult(w, res, val)
			}
			return
		}

		val, res := catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found")
		writeHTTPResult(w, res, val)
	})

	// Connect endpoint: GET /st2138-api/v1/connect
	s.mux.HandleFunc("/st2138-api/v1/connect", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
			writeHTTPResult(w, res, val)
			return
		}
		handler := s.lookupConnect()
		val, res := handler(w, r)
		writeHTTPResult(w, res, val)
	})

	// Catch-all for 404 - must be registered last
	s.mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if s.fallbackHandler != nil {
			val, res := s.fallbackHandler(w, r)
			writeHTTPResult(w, res, val)
			return
		}
		val, res := catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found")
		writeHTTPResult(w, res, val)
	})
}

func (s *Server) handleValueEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	fqoid := strings.Join(pathParts, "/")

	switch r.Method {
	case http.MethodGet:
		handler := s.lookupGetValue(slot)
		val, res := handler(slot, fqoid)
		writeHTTPResult(w, res, val)

	case http.MethodPut:
		// Read request body
		reqValue, err := internal.ReadRequestJSON(r)
		if err != nil {
			logger.Error("failed to read request", "error", err)
			writeHTTPStatusResult(w, catena.StatusWithCode(catena.INVALID_ARGUMENT, "invalid request body"))
			return
		}

		// Convert proto value to native Go type
		nativeValue, err := catena.FromProto(reqValue)
		if err != nil {
			logger.Error("failed to convert proto value to native Go type", "error", err)
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid request body")
			writeHTTPResult(w, res, val)
			return
		}

		handler := s.lookupSetValue(slot)
		res := handler(nativeValue, slot, fqoid)
		writeHTTPStatusResult(w, res)

	default:
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET, PUT, PATCH allowed")
		writeHTTPResult(w, res, val)
	}
}

func (s *Server) handleAssetEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	if r.Method != http.MethodGet {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
		writeHTTPResult(w, res, val)
		return
	}

	fqoid := strings.Join(pathParts, "/")
	handler := s.lookupGetAsset(slot)
	val, res := handler(slot, fqoid)
	writeHTTPResult(w, res, val)
}

func (s *Server) handleCommandEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	if r.Method != http.MethodPost {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only POST allowed")
		writeHTTPResult(w, res, val)
		return
	}

	commandFqoid := strings.Join(pathParts, "/")

	// Read command payload
	var payload any
	if r.ContentLength > 0 {
		reqValue, err := internal.ReadRequestJSON(r)
		if err != nil {
			logger.Error("failed to read command payload", "error", err)
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid command payload")
			writeHTTPResult(w, res, val)
			return
		}
		payload, err = catena.FromProto(reqValue)
		if err != nil {
			logger.Error("failed to convert proto value to native Go type", "error", err)
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid command payload")
			writeHTTPResult(w, res, val)
			return
		}
	}

	handler := s.lookupExecuteCommand(slot)
	val, res := handler(w, r, slot, commandFqoid, payload)
	writeHTTPResult(w, res, val)
}
