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
	"net/http"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/internal"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

type FallbackHandler func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult)

var _ catena.CatenaServer = (*Server)(nil)

// Server provides REST API endpoints for Catena devices
type Server struct {
	baseServer      *internal.BaseServer
	mux             *http.ServeMux
	fallbackHandler FallbackHandler
}

// writeHTTPResult is a unified function that handles writing different response types
func writeHTTPResult(w http.ResponseWriter, result catena.StatusResult, value interface{}) {
	httpStatus := ToHTTPStatus(result.Code)

	if result.Error != "" {
		// Set status code BEFORE writing error body
		w.WriteHeader(httpStatus)

		// Only return detailed error messages in dev mode
		if catena.IsDev() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
		return
	}

	// Handle different value types (no error case)
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
	httpStatus := ToHTTPStatus(result.Code)
	logger.Info("writeHTTPStatusResult", "httpStatus", httpStatus, "error", result.Error, "code", result.Code)

	// Set status code BEFORE writing body
	w.WriteHeader(httpStatus)

	if result.Error != "" {
		// Only return detailed error messages in dev mode
		if catena.IsDev() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
	}
}

// writeValueResult writes a CatenaValue as JSON
func writeValueResult(w http.ResponseWriter, value catena.CatenaValue, httpStatus int) {
	protoValue := value.Value
	if protoValue == nil {
		w.WriteHeader(httpStatus)
		return
	}

	if err := WriteResponseJSON(w, protoValue, httpStatus); err != nil {
		logger.Error("failed to write value response", "error", err)
		w.WriteHeader(http.StatusInternalServerError)
	}
}

// writeDeviceResult writes a CatenaDevice as JSON
func writeDeviceResult(w http.ResponseWriter, device catena.CatenaDevice, httpStatus int) {
	if device.GetProtoDevice() == nil {
		w.WriteHeader(httpStatus)
		return
	}

	b, err := MarshalDeviceJSON(device.GetProtoDevice())
	if err != nil {
		logger.Error("failed to marshal device response", "error", err)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusInternalServerError)
		json.NewEncoder(w).Encode(map[string]string{"error": "failed to marshal device response"})
		return
	}

	w.Header().Set("Content-Type", "application/json")
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

	jsonData, err := MarshalAssetJSON(protoAsset)
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
func NewServer(slots []uint16, maxConnections int) *Server {
	s := &Server{
		baseServer: internal.NewBaseServer(slots, maxConnections),
		mux:        http.NewServeMux(),
	}

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

func (s *Server) RegisterFallbackHandler(handler FallbackHandler) {
	s.baseServer.Mu.Lock()
	defer s.baseServer.Mu.Unlock()
	s.fallbackHandler = handler
}

// Shutdown gracefully shuts down the server by closing all SSE connections
// and waiting for their goroutines to finish.
func (s *Server) Shutdown() {
	s.baseServer.ShutdownConnections()
}

// For unit tests to override the default functions
var marshalSSEFunc = MarshalProtoJSON

// sendSSEEvent writes a single SSE event to the response writer,
// serializing the proto PushUpdates message via MarshalProtoJSON.
func (s *Server) sendSSEEvent(w http.ResponseWriter, flusher http.Flusher, update *protos.PushUpdates) error {
	data, err := marshalSSEFunc(update)
	if err != nil {
		return err
	}
	data = injectJSONField(data, "slot", update.GetSlot())
	_, err = fmt.Fprintf(w, "data: %s\n\n", data)
	if err != nil {
		return err
	}
	flusher.Flush()
	return nil
}

// handleConnect handles GET /st2138-api/v1/connect (SSE streaming)
func (s *Server) handleConnect(w http.ResponseWriter, r *http.Request) {
	// Check if the request method is GET
	if r.Method != http.MethodGet {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
		writeHTTPResult(w, res, val)
		return
	}

	// Check if SSE streaming is supported
	flusher, ok := w.(http.Flusher)
	if !ok {
		val, res := catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "streaming not supported")
		writeHTTPResult(w, res, val)
		return
	}

	// Read request headers (not used yet)
	_ = r.Header.Get("Detail-Level")
	_ = r.Header.Get("User-Agent")
	_ = r.Header.Get("Authorization")

	// Register this connection in the connectionQueue
	connID, conn := s.baseServer.RegisterConnection()
	if connID < 0 {
		val, res := catena.ReplyError[catena.CatenaValue](catena.RESOURCE_EXHAUSTED, "Too many connections to service")
		writeHTTPResult(w, res, val)
		return
	}
	defer s.baseServer.DeregisterConnection(connID)

	// Set SSE headers
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")
	origin := r.Header.Get("Origin")
	if origin != "" {
		w.Header().Set("Access-Control-Allow-Origin", origin)
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept, Origin, X-Requested-With, Language, Detail-Level")
		w.Header().Set("Access-Control-Allow-Credentials", "true")
	}
	w.WriteHeader(http.StatusOK)
	flusher.Flush()

	// Send initial update with populated slots using proto PushUpdates format
	populatedSlots := s.baseServer.Slots
	initialEvent := &protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: populatedSlots,
			},
		},
	}
	if err := s.sendSSEEvent(w, flusher, initialEvent); err != nil {
		logger.Error("failed to send initial SSE event", "error", err)
		return
	}

	logger.Info("SSE Connect started", "connID", connID)

	// Each connection's goroutine listens for setValue updates and server shutdown signals
	ctx := r.Context()
	for {
		select {
		case <-ctx.Done():
			logger.Info("SSE client disconnected", "connID", connID)
			return
		case <-conn.Done:
			logger.Info("SSE connection shut down by server", "connID", connID)
			return
		case update := <-conn.Updates:
			if err := s.sendSSEEvent(w, flusher, update); err != nil {
				logger.Error("failed to send SSE event", "connID", connID, "error", err)
				return
			}
		}
	}
}

// RegisterRoutes sets up all HTTP routes
func (s *Server) RegisterRoutes() {
	// Device endpoint: GET /st2138-api/v1/{slot}
	s.mux.HandleFunc("/st2138-api/v1/", func(w http.ResponseWriter, r *http.Request) {
		logger.Info("Device endpoint", "path", r.URL.Path, "method", r.Method)
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid path format")
			writeHTTPResult(w, res, val)
			return
		}

		slotStr := parts[2]
		slot, err := s.baseServer.ValidateSlotString(slotStr)
		if err.Code != catena.OK {
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid slot number")
			writeHTTPResult(w, res, val)
			return
		}

		// Route based on path structure
		if len(parts) == 3 && r.Method == http.MethodGet {
			// GET /st2138-api/v1/{slot} - Get device info
			handler := s.baseServer.LookupGetDeviceHandler(slot)
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
			case "param-info":
				s.handleParamInfoEndpoint(w, r, slot, parts[4:])
			default:
				val, res := catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "unknown endpoint")
				writeHTTPResult(w, res, val)
			}
			return
		}

		val, res := catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found")
		writeHTTPResult(w, res, val)
	})

	// Connect endpoint: GET /st2138-api/v1/connect (SSE streaming)
	s.mux.HandleFunc("/st2138-api/v1/connect", s.handleConnect)

	// Devices endpoint: GET /st2138-api/v1/devices (returns populated slots)
	s.mux.HandleFunc("/st2138-api/v1/devices", s.handleGetPopulatedSlots)

	// Catch-all for 404 - must be registered last
	s.mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		logger.Info("Fallback handler", "path", r.URL.Path, "method", r.Method)
		if s.fallbackHandler != nil {
			val, res := s.fallbackHandler(w, r)
			writeHTTPResult(w, res, val)
			return
		}
		val, res := catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found")
		writeHTTPResult(w, res, val)
	})

	s.mux.HandleFunc("/st2138-api/v1", func(w http.ResponseWriter, r *http.Request) {
		writeHTTPStatusResult(w, catena.StatusWithCode(catena.NOT_FOUND, "endpoint not found"))
	})
}

// handleGetPopulatedSlots handles GET /st2138-api/v1/devices
// Returns the list of populated slots
func (s *Server) handleGetPopulatedSlots(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
		writeHTTPResult(w, res, val)
		return
	}

	logger.Info("GetPopulatedSlots")
	slots := s.baseServer.Slots

	response := map[string][]uint32{
		"slots": slots,
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	if err := json.NewEncoder(w).Encode(response); err != nil {
		logger.Error("failed to write slots response", "error", err)
	}
}

func (s *Server) handleValueEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	fqoid := strings.Join(pathParts, "/")

	switch r.Method {
	case http.MethodGet:
		handler := s.baseServer.LookupGetValueHandler(slot)
		val, res := handler(slot, fqoid)
		writeHTTPResult(w, res, val)

	case http.MethodPut:
		// Read request body
		reqValue, err := ReadRequestJSON(r)
		if err.Code != catena.OK {
			logger.Error("failed to read request", "error", err)
			writeHTTPStatusResult(w, catena.StatusWithCode(catena.INVALID_ARGUMENT, "invalid request body"))
			return
		}

		// Convert proto value to native Go type
		nativeValue, errProto := catena.FromProto(reqValue)
		if errProto.Code != catena.OK {
			logger.Error("failed to convert proto value to native Go type", "error", errProto.Error)
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid request body")
			writeHTTPResult(w, res, val)
			return
		}

		handler := s.baseServer.LookupSetValueHandler(slot)
		res := handler(nativeValue, slot, fqoid)
		writeHTTPStatusResult(w, res)

	default:
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET, PUT, PATCH allowed")
		writeHTTPResult(w, res, val)
	}
}

func (s *Server) handleAssetEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodGet {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
		writeHTTPResult(w, res, val)
		return
	}

	fqoid := strings.Join(pathParts, "/")
	handler := s.baseServer.LookupGetAssetHandler(slot)
	asset, res := handler(slot, fqoid)

	if res.Error == "" {
		if compressionStr := r.URL.Query().Get("compression"); compressionStr != "" {
			targetEncoding, encRes := catena.ParsePayloadEncoding(compressionStr)
			if encRes.Code != catena.OK {
				_, errRes := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, encRes.Error)
				writeHTTPResult(w, errRes, catena.CatenaValue{})
				return
			}
			if tcRes := catena.TranscodeAssetPayload(&asset, targetEncoding); tcRes.Code != catena.OK {
				logger.Error("failed to transcode asset payload", "error", tcRes.Error)
				_, errRes := catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "failed to transcode payload: "+tcRes.Error)
				writeHTTPResult(w, errRes, catena.CatenaValue{})
				return
			}
		}
	}

	writeHTTPResult(w, res, asset)
}

// handleParamInfoEndpoint dispatches /st2138-api/v1/{slot}/param-info/... requests.
// Supported routes (mirroring the C++ REST controller):
//   - GET /{slot}/param-info/{fqoid}         -> unary: returns a single ParamInfoResponse JSON
//   - GET /{slot}/param-info/{fqoid}/stream  -> SSE stream rooted at fqoid; ?recursive to recurse
//   - GET /{slot}/param-info/stream          -> SSE stream of all top-level params; ?recursive to recurse
//
// Per the OpenAPI spec and the C++ reference, the recursive flag is determined
// by presence of the `recursive` query parameter alone — any value (including
// "false") enables recursion.
func (s *Server) handleParamInfoEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodGet {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only GET allowed")
		writeHTTPResult(w, res, val)
		return
	}

	recursive := r.URL.Query().Has("recursive")

	// Parse trailing path segments. The "stream" suffix selects SSE.
	streaming := false
	fqoidParts := pathParts
	if len(fqoidParts) > 0 && fqoidParts[len(fqoidParts)-1] == "stream" {
		streaming = true
		fqoidParts = fqoidParts[:len(fqoidParts)-1]
	}

	// Match the C++ REST controller: the fqoid is built by prepending "/" to
	// every path segment after the endpoint, yielding e.g. "/parent/child".
	// An empty fqoid means "all top-level parameters".
	oidPrefix := ""
	for _, p := range fqoidParts {
		oidPrefix += "/" + p
	}

	// Validate unary-mode invariants up front, mirroring the C++ controller.
	if !streaming {
		if recursive {
			writeHTTPStatusResult(w, catena.StatusWithCode(catena.INVALID_ARGUMENT, "Recursive parameter info request is not supported with unary response"))
			return
		}
		if oidPrefix == "" {
			writeHTTPStatusResult(w, catena.StatusWithCode(catena.INVALID_ARGUMENT, "Unary request must include fqoid"))
			return
		}
	}

	handler := s.baseServer.LookupGetParamInfoHandler(slot)
	infos, res := handler(slot, oidPrefix, recursive)
	if res.Code != catena.OK {
		writeHTTPStatusResult(w, res)
		return
	}

	if len(infos) == 0 {
		msg := "Parameter not found: " + oidPrefix
		if oidPrefix == "" {
			msg = "No top-level parameters found"
		}
		writeHTTPStatusResult(w, catena.StatusWithCode(catena.NOT_FOUND, msg))
		return
	}

	if streaming {
		s.writeParamInfoStream(w, r, infos)
		return
	}

	if err := WriteParamInfoJSON(w, infos[0].GetProtoResponse(), http.StatusOK); err != nil {
		logger.Error("failed to write param info response", "error", err)
	}
}

// writeParamInfoStream streams the param info entries to the client as Server-Sent Events.
func (s *Server) writeParamInfoStream(w http.ResponseWriter, r *http.Request, infos []catena.CatenaParamInfo) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		val, res := catena.ReplyError[catena.CatenaValue](catena.INTERNAL, "streaming not supported")
		writeHTTPResult(w, res, val)
		return
	}

	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")
	w.WriteHeader(http.StatusOK)
	flusher.Flush()

	ctx := r.Context()
	for _, info := range infos {
		select {
		case <-ctx.Done():
			logger.Info("param-info stream client disconnected")
			return
		default:
		}
		protoResp := info.GetProtoResponse()
		if protoResp == nil {
			continue
		}
		data, err := MarshalProtoJSON(protoResp)
		if err != nil {
			logger.Error("failed to marshal param info entry", "error", err)
			return
		}
		if _, err := fmt.Fprintf(w, "data: %s\n\n", data); err != nil {
			logger.Error("failed to write param info SSE event", "error", err)
			return
		}
		flusher.Flush()
	}
}

func (s *Server) handleCommandEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodPost {
		val, res := catena.ReplyError[catena.CatenaValue](catena.METHOD_NOT_ALLOWED, "only POST allowed")
		writeHTTPResult(w, res, val)
		return
	}

	commandFqoid := strings.Join(pathParts, "/")

	respond := true
	if r.URL.Query().Get("respond") == "false" {
		respond = false
	}

	// Read command payload
	var payload any
	if r.ContentLength > 0 {
		reqValue, err := ReadRequestJSON(r)
		if err.Code != catena.OK {
			logger.Error("failed to read command payload", "error", err)
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid command payload")
			writeHTTPResult(w, res, val)
			return
		}
		var errProto catena.StatusResult
		payload, errProto = catena.FromProto(reqValue)
		if errProto.Code != catena.OK {
			logger.Error("failed to convert proto value to native Go type", "error", errProto.Error)
			val, res := catena.ReplyError[catena.CatenaValue](catena.INVALID_ARGUMENT, "invalid command payload")
			writeHTTPResult(w, res, val)
			return
		}
	}

	handler := s.baseServer.LookupExecuteCommandHandler(slot)
	cmdResult, res := handler(slot, commandFqoid, payload)
	if res.Code != catena.OK {
		writeHTTPResult(w, res, catena.CatenaValue{})
		return
	}

	if !respond {
		cmdResult, _ = catena.CommandNoResponse()
	}

	_ = WriteCommandResponseJSON(w, cmdResult.GetProtoResponse(), http.StatusOK)
}

// ToHTTPStatus converts a StatusCode to an HTTP status code.
// gRPC codes are mapped to their closest HTTP equivalents.
func ToHTTPStatus(s catena.StatusCode) int {
	switch s {
	case catena.OK:
		return http.StatusOK
	case catena.CANCELLED:
		return 499 // Client Closed Request (nginx convention)
	case catena.UNKNOWN:
		return http.StatusInternalServerError
	case catena.INVALID_ARGUMENT:
		return http.StatusBadRequest
	case catena.DEADLINE_EXCEEDED:
		return http.StatusGatewayTimeout
	case catena.NOT_FOUND:
		return http.StatusNotFound
	case catena.ALREADY_EXISTS:
		return http.StatusConflict
	case catena.PERMISSION_DENIED:
		return http.StatusForbidden
	case catena.RESOURCE_EXHAUSTED:
		return http.StatusTooManyRequests
	case catena.FAILED_PRECONDITION:
		return http.StatusBadRequest
	case catena.ABORTED:
		return http.StatusConflict
	case catena.OUT_OF_RANGE:
		return http.StatusBadRequest
	case catena.UNIMPLEMENTED:
		return http.StatusNotImplemented
	case catena.INTERNAL:
		return http.StatusInternalServerError
	case catena.UNAVAILABLE:
		return http.StatusServiceUnavailable
	case catena.DATA_LOSS:
		return http.StatusInternalServerError
	case catena.UNAUTHENTICATED:
		return http.StatusUnauthorized
	// REST codes map directly
	case catena.CREATED, catena.ACCEPTED, catena.NO_CONTENT, catena.METHOD_NOT_ALLOWED, catena.CONFLICT,
		catena.UNPROCESSABLE_ENTITY, catena.TOO_MANY_REQUESTS, catena.BAD_GATEWAY,
		catena.SERVICE_UNAVAILABLE, catena.GATEWAY_TIMEOUT:
		return int(s)
	default:
		return http.StatusInternalServerError
	}
}

func (s *Server) RegisterGetDeviceHandler(slot uint16, handler catena.DeviceHandler) {
	s.baseServer.RegisterGetDeviceHandler(slot, handler)
}

func (s *Server) RegisterGetValueHandler(slot uint16, handler catena.GetValueHandler) {
	s.baseServer.RegisterGetValueHandler(slot, handler)
}

func (s *Server) RegisterSetValueHandler(slot uint16, handler catena.SetValueHandler) {
	s.baseServer.RegisterSetValueHandler(slot, handler)
}

func (s *Server) RegisterGetAssetHandler(slot uint16, handler catena.GetAssetHandler) {
	s.baseServer.RegisterGetAssetHandler(slot, handler)
}

func (s *Server) RegisterExecuteCommandHandler(slot uint16, handler catena.ExecuteCommandHandler) {
	s.baseServer.RegisterExecuteCommandHandler(slot, handler)
}

func (s *Server) RegisterGetParamInfoHandler(slot uint16, handler catena.GetParamInfoHandler) {
	s.baseServer.RegisterGetParamInfoHandler(slot, handler)
}

func (s *Server) LookupGetDeviceHandler(slot uint16) catena.DeviceHandler {
	return s.baseServer.LookupGetDeviceHandler(slot)
}

func (s *Server) LookupGetValueHandler(slot uint16) catena.GetValueHandler {
	return s.baseServer.LookupGetValueHandler(slot)
}

func (s *Server) LookupSetValueHandler(slot uint16) catena.SetValueHandler {
	return s.baseServer.LookupSetValueHandler(slot)
}

func (s *Server) LookupGetAssetHandler(slot uint16) catena.GetAssetHandler {
	return s.baseServer.LookupGetAssetHandler(slot)
}

func (s *Server) LookupExecuteCommandHandler(slot uint16) catena.ExecuteCommandHandler {
	return s.baseServer.LookupExecuteCommandHandler(slot)
}

func (s *Server) LookupGetParamInfoHandler(slot uint16) catena.GetParamInfoHandler {
	return s.baseServer.LookupGetParamInfoHandler(slot)
}

func (s *Server) BroadcastUpdate(slot uint16, fqoid string, value any) {
	s.baseServer.BroadcastUpdate(slot, fqoid, value)
}

func (s *Server) SetMaxConnections(max int) {
	s.baseServer.SetMaxConnections(max)
}

func (s *Server) ConnectionCount() int {
	return s.baseServer.ConnectionCount()
}
