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
 * @file rest_transport.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-14
 */

package rest

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"maps"
	"net"
	"net/http"
	"strings"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
	"google.golang.org/protobuf/encoding/protojson"
)

type FallbackHandler func(w http.ResponseWriter, r *http.Request) (st2138.Value, catena.StatusResult)

type Transport struct {
	mu              sync.Mutex
	server          *http.Server
	mux             *http.ServeMux
	runtime         catena.ServerRuntime
	fallbackHandler FallbackHandler

	port int
}

var _ catena.Transport = (*Transport)(nil)

// NewTransport creates a new REST transport with the given configuration.
func NewTransport(cfg Options) *Transport {
	t := &Transport{
		port: cfg.Port,
		mux:  http.NewServeMux(),
	}
	t.registerRoutes()
	return t
}

// Start starts the HTTP server on the specified port using this server's mux
func (t *Transport) Start(ctx context.Context, runtime catena.ServerRuntime) error {
	addr := fmt.Sprintf(":%d", t.port)
	// Bind synchronously so that startup errors (privileged port, address
	// already in use, invalid address, etc.) are returned to the caller
	// instead of only being logged asynchronously after Start has already
	// reported success. This mirrors ConnectionProps.Start.
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		logger.Error("REST Transport failed to listen", "address", addr, "error", err)
		return fmt.Errorf("REST transport failed to listen on %s: %w", addr, err)
	}

	t.server = &http.Server{
		Addr:    addr,
		Handler: t.mux,
	}
	t.runtime = runtime

	// http server does not use context
	// serve blocks so do it in a goroutine
	go func() {
		logger.Info("REST Transport listening", "address", addr)
		err := t.server.Serve(listener)
		if err != nil && err != http.ErrServerClosed {
			logger.Error("HTTP server error", "error", err)
		}
	}()
	return nil
}

func (t *Transport) Shutdown(ctx context.Context) error {
	// Ordering is intentional:
	// 1) start HTTP shutdown first so the listener stops accepting new requests/connections,
	// 2) then signal runtime-owned streaming connections (SSE) to drain,
	// 3) finally return the HTTP shutdown result.
	//
	// For HTTP, server.Shutdown(ctx) is context-aware and could be called synchronously.
	// We still run it in a goroutine so we can overlap "stop accepting new traffic" with
	// runtime connection draining. This mirrors the broader transport shutdown pattern where
	// the transport begins its own graceful stop and then requests stream shutdown.

	errCh := make(chan error, 1)
	go func() {
		if t.server == nil {
			errCh <- nil
			return
		}
		err := t.server.Shutdown(ctx)
		if err != nil && ctx.Err() != nil {
			// Graceful shutdown timed out/cancelled; force close as a best-effort fallback
			// to ensure this transport does not leave active HTTP connections behind.
			logger.Warning("HTTP server shutdown timed out, forcing close", "error", err)
			closeErr := t.server.Close()
			if closeErr != nil {
				logger.Error("failed to force close HTTP server", "error", closeErr)
			}
		}
		// Return the graceful shutdown result as the primary status. Any force-close
		// error is logged for diagnostics, but does not replace the main shutdown result.
		errCh <- err
	}()

	if t.runtime != nil {
		// Drain runtime-managed streams after HTTP starts draining, so long-lived SSE
		// handlers are signaled to exit and Shutdown can complete.
		t.runtime.ShutdownTransportConnections(ctx, t)
	}

	// Wait for HTTP shutdown to complete and return its result. By this point, the runtime
	// should have signaled all active connections to shut down, so this should complete in
	// a timely manner.
	return <-errCh
}

func (t *Transport) RegisterFallbackHandler(handler FallbackHandler) {
	t.mu.Lock()
	defer t.mu.Unlock()
	t.fallbackHandler = handler
}

func (t *Transport) retrieveMetadataFromRequest(r *http.Request) catena.TransportContext {
	transportContext := catena.TransportContext{
		AccessToken: r.Header.Get("Authorization"),
		Metadata:    maps.Clone(r.Header), // include all headers as metadata for now; could be filtered in the future if needed
		Ctx:         r.Context(),
	}
	return transportContext
}

func (t *Transport) isDevMode() bool {
	return t != nil && t.runtime != nil && t.runtime.IsDev()
}

// writeHTTPResult is a unified function that handles writing different response types
func (t *Transport) writeHTTPResult(w http.ResponseWriter, result catena.StatusResult, value interface{}) {
	httpStatus := ToHTTPStatus(result.Code)

	if result.IsError() {
		// Set status code BEFORE writing error body
		w.WriteHeader(httpStatus)

		// Only return detailed error messages in dev mode
		if t.isDevMode() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
		return
	}

	// Handle different value types (no error case)
	switch v := value.(type) {
	case st2138.Value:
		writeValueResult(w, v, httpStatus)
	case st2138.Device:
		writeDeviceResult(w, v, httpStatus)
	case st2138.Asset:
		writeAssetResult(w, v, httpStatus)
	default:
		w.WriteHeader(httpStatus)
	}
}

// writeHTTPMethodNotAllowed writes HTTP 405 with a JSON error body. Method
// enforcement is a transport-only concern, so it bypasses StatusCode entirely.
// 405 is not in ST 2138-12, but is emitted here for HTTP correctness when a
// route is reached with the wrong method.
func (t *Transport) writeHTTPMethodNotAllowed(w http.ResponseWriter, msg string) {
	w.WriteHeader(http.StatusMethodNotAllowed)
	if t.isDevMode() {
		_ = json.NewEncoder(w).Encode(map[string]string{"error": msg})
	} else {
		_ = json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(http.StatusMethodNotAllowed)})
	}
}

// writeHTTPStatusResultNoBody writes a StatusResult to the HTTP response,
// emitting HTTP 204 No Content on success instead of 200. Used by routes
// whose successful response has no body per ST 2138-12 §7.7.2, §7.8.4-5,
// and §7.11.3-5 (SetValue, SetValues, language-pack mutations). 204 vs 200
// is a route-level choice, not a handler outcome, so it lives here in the
// transport rather than in StatusCode.
func (t *Transport) writeHTTPStatusResultNoBody(w http.ResponseWriter, result catena.StatusResult) {
	if result.IsOk() {
		w.WriteHeader(http.StatusNoContent)
		return
	}
	t.writeHTTPStatusResult(w, result)
}

// t.writeHTTPStatusResult writes a StatusResult to the HTTP response (no value).
func (t *Transport) writeHTTPStatusResult(w http.ResponseWriter, result catena.StatusResult) {
	httpStatus := ToHTTPStatus(result.Code)
	logger.Info("t.writeHTTPStatusResult", "httpStatus", httpStatus, "error", result.Error, "code", result.Code)

	// Set status code BEFORE writing body
	w.WriteHeader(httpStatus)

	if result.IsError() {
		// Only return detailed error messages in dev mode
		if t.isDevMode() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
	}
}

// writeValueResult writes a Value as JSON
func writeValueResult(w http.ResponseWriter, value st2138.Value, httpStatus int) {
	protoValue := value.Proto
	if protoValue == nil {
		w.WriteHeader(httpStatus)
		return
	}

	if err := WriteProtoJSON(w, protoValue, httpStatus); err != nil {
		logger.Error("failed to write value response", "error", err)
		w.WriteHeader(http.StatusInternalServerError)
	}
}

// writeDeviceResult writes a Device as JSON
func writeDeviceResult(w http.ResponseWriter, device st2138.Device, httpStatus int) {
	if device.Proto == nil {
		w.WriteHeader(httpStatus)
		return
	}

	b, err := MarshalDeviceJSON(device.Proto)
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

// writeAssetResult writes an Asset as JSON-encoded ExternalObjectPayload
func writeAssetResult(w http.ResponseWriter, asset st2138.Asset, httpStatus int) {
	protoAsset := asset.Proto
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

// For unit tests to override the default functions
var marshalSSEFunc = MarshalProtoJSON

// sendSSEEvent writes a single SSE event to the response writer,
// serializing the proto PushUpdates message via MarshalProtoJSON.
func (t *Transport) sendSSEEvent(w http.ResponseWriter, flusher http.Flusher, update *protos.PushUpdates) error {
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
func (t *Transport) handleConnect(w http.ResponseWriter, r *http.Request) {
	// Check if the request method is GET
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	// Check if SSE streaming is supported
	flusher, ok := w.(http.Flusher)
	if !ok {
		val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInternal, "streaming not supported")
		t.writeHTTPResult(w, res, val)
		return
	}

	// Read request headers (not used yet)
	_ = r.Header.Get("Detail-Level")
	_ = r.Header.Get("User-Agent")
	_ = r.Header.Get("Authorization")

	// Register this connection with the runtime using this transport as owner.
	// Owner association allows targeted cleanup (ShutdownTransportConnections(owner))
	// so one transport can shut down without impacting streams owned by others.
	transportContext := t.retrieveMetadataFromRequest(r)

	conn, res := t.runtime.RegisterTransportConnection(t, transportContext)
	if res.Code != catena.StatusCodeOk {
		val, res := catena.ReplyError[st2138.Value](res.Code, res.Error)
		t.writeHTTPResult(w, res, val)
		return
	}
	defer t.runtime.DeregisterConnection(conn.ID)

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

	logger.Info("SSE Connect started", "connID", conn.ID)

	// Each connection's goroutine listens for setValue updates and server shutdown signals
	ctx := r.Context()
	for {
		select {
		case <-ctx.Done():
			logger.Info("SSE client disconnected", "connID", conn.ID)
			return
		case <-conn.Done:
			logger.Info("SSE connection shut down by server", "connID", conn.ID)
			return
		case update := <-conn.Updates:
			if err := t.sendSSEEvent(w, flusher, update); err != nil {
				logger.Error("failed to send SSE event", "connID", conn.ID, "error", err)
				return
			}
		}
	}
}

// registerRoutes sets up all HTTP routes
func (t *Transport) registerRoutes() {
	// Device endpoint: GET /st2138-api/v1/{slot}
	t.mux.HandleFunc("/st2138-api/v1/", func(w http.ResponseWriter, r *http.Request) {
		logger.Info("Device endpoint", "path", r.URL.Path, "method", r.Method)
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "invalid path format")
			t.writeHTTPResult(w, res, val)
			return
		}

		slotStr := parts[2]
		slot, err := catena.ValidateSlotString(slotStr)
		if err.IsError() {
			val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "invalid slot number")
			t.writeHTTPResult(w, res, val)
			return
		}

		// Route based on path structure
		if len(parts) == 3 && r.Method == http.MethodGet {
			// GET /st2138-api/v1/{slot} - Get device info
			transportContext := t.retrieveMetadataFromRequest(r)
			device, result := t.runtime.InvokeGetDeviceHandler(slot, transportContext)
			t.writeHTTPResult(w, result, device)
			return
		}

		if len(parts) >= 4 {
			endpoint := parts[3]
			switch endpoint {
			case "value":
				t.handleValueEndpoint(w, r, slot, parts[4:])
			case "values":
				t.handleValuesEndpoint(w, r, slot)
			case "asset":
				t.handleAssetEndpoint(w, r, slot, parts[4:])
			case "command":
				t.handleCommandEndpoint(w, r, slot, parts[4:])
			case "param":
				t.handleParamEndpoint(w, r, slot, parts[4:])
			case "param-info":
				t.handleParamInfoEndpoint(w, r, slot, parts[4:])
			case "language-pack":
				t.handleLanguagePackEndpoint(w, r, slot, parts[4:])
			case "languages":
				t.handleLanguagesEndpoint(w, r, slot)
			default:
				val, res := catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "unknown endpoint")
				t.writeHTTPResult(w, res, val)
			}
			return
		}

		val, res := catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "endpoint not found")
		t.writeHTTPResult(w, res, val)
	})

	// Health endpoint: GET /st2138-api/v1/health
	t.mux.HandleFunc("/st2138-api/v1/health", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			t.writeHTTPMethodNotAllowed(w, "only GET allowed")
			return
		}
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeOk, ""))
	})

	// Connect endpoint: GET /st2138-api/v1/connect (SSE streaming)
	t.mux.HandleFunc("/st2138-api/v1/connect", t.handleConnect)

	// Devices endpoint: GET /st2138-api/v1/devices (returns populated slots)
	t.mux.HandleFunc("/st2138-api/v1/devices", t.handleGetPopulatedSlots)

	// Catch-all for 404 - must be registered last
	t.mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		logger.Info("Fallback handler", "path", r.URL.Path, "method", r.Method)
		if t.fallbackHandler != nil {
			val, res := t.fallbackHandler(w, r)
			t.writeHTTPResult(w, res, val)
			return
		}
		val, res := catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "endpoint not found")
		t.writeHTTPResult(w, res, val)
	})

	t.mux.HandleFunc("/st2138-api/v1", func(w http.ResponseWriter, r *http.Request) {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeNotFound, "endpoint not found"))
	})
}

// handleGetPopulatedSlots handles GET /st2138-api/v1/devices
// Returns the list of populated slots
func (t *Transport) handleGetPopulatedSlots(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	logger.Info("GetPopulatedSlots")
	transportContext := t.retrieveMetadataFromRequest(r)
	slots, result := t.runtime.GetSlots(transportContext)
	if result.IsError() {
		t.writeHTTPResult(w, result, nil)
		return
	}
	uint32Slots := make([]uint32, len(slots))
	for i, slot := range slots {
		uint32Slots[i] = uint32(slot)
	}

	response := map[string][]uint32{
		"slots": uint32Slots,
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	if err := json.NewEncoder(w).Encode(response); err != nil {
		logger.Error("failed to write slots response", "error", err)
	}
}

// handleLanguagesEndpoint handles GET /st2138-api/v1/{slot}/languages (Languages).
// Per the OpenAPI contract (GET /{slot}/languages), the body is a bare JSON array
// of language codes, e.g. ["en","fr","es","de"]. An empty result serializes as [].
func (t *Transport) handleLanguagesEndpoint(w http.ResponseWriter, r *http.Request, slot uint16) {
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	transportContext := t.retrieveMetadataFromRequest(r)
	languages, result := t.runtime.InvokeListLanguagesHandler(slot, transportContext)
	if result.IsError() {
		t.writeHTTPStatusResult(w, result)
		return
	}

	if languages == nil {
		languages = []string{}
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	if err := json.NewEncoder(w).Encode(languages); err != nil {
		logger.Error("failed to write languages response", "error", err)
	}
}

func (t *Transport) handleValueEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	fqoid := strings.Join(pathParts, "/")

	switch r.Method {
	case http.MethodGet:
		transportContext := t.retrieveMetadataFromRequest(r)
		val, res := t.runtime.InvokeGetValueHandler(slot, fqoid, transportContext)
		t.writeHTTPResult(w, res, val)

	case http.MethodPut:
		// Read request body
		reqValue, err := ReadRequestJSON(r)
		if err.Code != catena.StatusCodeOk {
			logger.Error("failed to read request", "error", err)
			t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid request body"))
			return
		}

		// Convert proto value to native Go type
		nativeValue, errProto := st2138.FromProto(reqValue)
		if errProto != nil {
			logger.Error("failed to convert proto value to native Go type", "error", errProto)
			val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "invalid request body")
			t.writeHTTPResult(w, res, val)
			return
		}

		transportContext := t.retrieveMetadataFromRequest(r)
		entries := []catena.SetValueEntry{{Fqoid: fqoid, Value: nativeValue}}
		res := t.runtime.InvokeSetValueHandler(slot, entries, transportContext)
		t.writeHTTPStatusResultNoBody(w, res)

	default:
		t.writeHTTPMethodNotAllowed(w, "only GET, PUT, PATCH allowed")
	}
}

// handleValuesEndpoint handles PUT /st2138-api/v1/{slot}/values (SetValues).
// The full set of values is applied via the runtime's SetValue handler.
// On success it returns 204
func (t *Transport) handleValuesEndpoint(w http.ResponseWriter, r *http.Request, slot uint16) {
	if r.Method != http.MethodPut {
		t.writeHTTPMethodNotAllowed(w, "only PUT allowed")
		return
	}

	entries, err := ReadMultiSetValuesRequestJSON(r)
	if err.Code != catena.StatusCodeOk {
		logger.Error("failed to read request", "error", err)
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid request body"))
		return
	}

	transportContext := t.retrieveMetadataFromRequest(r)
	res := t.runtime.InvokeSetValueHandler(slot, entries, transportContext)
	t.writeHTTPStatusResultNoBody(w, res)
}

// handleAssetEndpoint dispatches /st2138-api/v1/{slot}/asset/{fqoid} across
// the four asset operations: GET (ReadAsset), POST (CreateAsset), PUT
// (UpdateAsset) and DELETE (DeleteAsset). The three write operations return
// 204 No Content on success per the OpenAPI spec.
func (t *Transport) handleAssetEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	fqoid := strings.Join(pathParts, "/")

	switch r.Method {
	case http.MethodGet:
		t.handleReadAsset(w, r, slot, fqoid)
	case http.MethodPost:
		t.handleWriteAsset(w, r, slot, fqoid, t.runtime.InvokeCreateAssetHandler)
	case http.MethodPut:
		t.handleWriteAsset(w, r, slot, fqoid, t.runtime.InvokeUpdateAssetHandler)
	case http.MethodDelete:
		transportContext := t.retrieveMetadataFromRequest(r)
		res := t.runtime.InvokeDeleteAssetHandler(slot, fqoid, transportContext)
		t.writeHTTPStatusResultNoBody(w, res)
	default:
		t.writeHTTPMethodNotAllowed(w, "only GET, POST, PUT, DELETE allowed")
	}
}

// handleReadAsset serves GET /st2138-api/v1/{slot}/asset/{fqoid}, with optional
// payload transcoding via the ?compression= query parameter.
func (t *Transport) handleReadAsset(w http.ResponseWriter, r *http.Request, slot uint16, fqoid string) {
	transportContext := t.retrieveMetadataFromRequest(r)
	asset, result := t.runtime.InvokeReadAssetHandler(slot, fqoid, transportContext)

	if result.IsOk() {
		if compressionStr := r.URL.Query().Get("compression"); compressionStr != "" {
			targetEncoding, encErr := st2138.ParsePayloadEncoding(compressionStr)
			if encErr != nil {
				val, errRes := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, encErr.Error())
				t.writeHTTPResult(w, errRes, val)
				return
			}
			if tcErr := st2138.TranscodeAssetPayload(&asset, targetEncoding); tcErr != nil {
				logger.Error("failed to transcode asset payload", "error", tcErr)
				val, errRes := catena.ReplyError[st2138.Value](catena.StatusCodeInternal, "failed to transcode payload: "+tcErr.Error())
				t.writeHTTPResult(w, errRes, val)
				return
			}
		}
	}

	t.writeHTTPResult(w, result, asset)
}

// handleWriteAsset handles POST (CreateAsset) and PUT (UpdateAsset). Both read
// an external_object_payload body, hand it to the given invoke function, and
// return 204 No Content on success.
func (t *Transport) handleWriteAsset(
	w http.ResponseWriter,
	r *http.Request,
	slot uint16,
	fqoid string,
	invoke func(slot uint16, fqoid string, asset st2138.Asset, transportContext catena.TransportContext) catena.StatusResult,
) {
	payload, err := ReadAssetRequestJSON(r)
	if err != nil {
		logger.Error("failed to read asset request", "error", err)
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, err.Error()))
		return
	}

	transportContext := t.retrieveMetadataFromRequest(r)
	res := invoke(slot, fqoid, st2138.Asset{Proto: payload}, transportContext)
	t.writeHTTPStatusResultNoBody(w, res)
}

// handleParamEndpoint handles GET /st2138-api/v1/{slot}/param/{fqoid} (GetParam).
// It returns the full parameter (metadata + value) as a component_param object
// of the form {"oid": ..., "param": {...}}.
func (t *Transport) handleParamEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	fqoid := strings.Join(pathParts, "/")
	if fqoid == "" {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "request must include fqoid"))
		return
	}

	transportContext := t.retrieveMetadataFromRequest(r)
	param, result := t.runtime.InvokeGetParamHandler(slot, fqoid, transportContext)
	if result.IsError() {
		t.writeHTTPStatusResult(w, result)
		return
	}
	if param.Proto == nil {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInternal, "param returned nil"))
		return
	}

	component := &protos.DeviceComponent_ComponentParam{
		Oid:   fqoid,
		Param: param.Proto,
	}
	// Use the device-style marshaller so meaningful proto3 zero values survive
	// (e.g. constraint min_value:0, or a current value of 0/0.0/"").
	b, err := MarshalComponentParamJSON(component)
	if err != nil {
		logger.Error("failed to marshal param response", "error", err)
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInternal, "failed to marshal param response"))
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	if _, writeErr := w.Write(b); writeErr != nil {
		logger.Error("failed to write param response", "error", writeErr)
	}
}

// handleParamInfoEndpoint handles param info requests and streaming (SSE).
func (t *Transport) handleParamInfoEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	recursive := r.URL.Query().Has("recursive")

	// Check for "stream" suffix to enable SSE.
	streaming := false
	fqoidParts := pathParts
	if len(fqoidParts) > 0 && fqoidParts[len(fqoidParts)-1] == "stream" {
		streaming = true
		fqoidParts = fqoidParts[:len(fqoidParts)-1]
	}

	// Build OID prefix from path segments. (no leading slash per spec)
	oidPrefix := strings.Join(fqoidParts, "/")

	// Unary requests must include fqoid and cannot be recursive.
	if !streaming {
		if recursive {
			t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "Recursive parameter info request is not supported with unary response"))
			return
		}
		if oidPrefix == "" {
			t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "Unary request must include fqoid"))
			return
		}
	}

	transportContext := t.retrieveMetadataFromRequest(r)

	if streaming {
		t.streamParamInfo(w, r, slot, oidPrefix, recursive, transportContext)
		return
	}

	// Unary: the handler still streams, so collect its first message and discard the rest.
	stream := &firstStream[st2138.ParamInfo]{}
	result := t.runtime.InvokeParamInfoHandler(slot, oidPrefix, recursive, stream, transportContext)
	if result.IsError() {
		t.writeHTTPStatusResult(w, result)
		return
	}

	if !stream.has {
		// A unary request must resolve to exactly one parameter. A handler that
		// reports success yet emits nothing has violated that contract; surface
		// it as an internal error rather than inventing a NotFound over what may
		// be a genuine handler bug. Handlers own NotFound for missing oids.
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInternal, "param info handler reported success but produced no result"))
		return
	}

	if err := WriteProtoJSON(w, stream.item.Wire(), http.StatusOK); err != nil {
		logger.Error("failed to write param info response", "error", err)
	}
}

func (t *Transport) handleLanguagePackEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	// The language code is a single path segment; an empty code (if any) is
	// left for the server layer to validate.
	if len(pathParts) != 1 {
		val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "language is required")
		t.writeHTTPResult(w, res, val)
		return
	}

	language := pathParts[0]
	transportContext := t.retrieveMetadataFromRequest(r)

	switch r.Method {
	case http.MethodGet:
		languagePack, res := t.runtime.InvokeReadLanguagePackHandler(slot, language, transportContext)
		if res.Code != catena.StatusCodeOk {
			t.writeHTTPStatusResult(w, res)
			return
		}

		// A nil proto on an OK result is an internal contract violation by the
		// handler, not a not-found; surface it as an internal error rather than
		// overwriting the successful outcome with a 404.
		if languagePack.Proto == nil {
			logger.Error("language pack handler returned OK with nil proto", "slot", slot, "language", language)
			t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInternal, "language pack response was empty"))
			return
		}

		// The handler wraps the inner pack (name + words); the REST response is
		// the outer component that also carries the language tag.
		component := &protos.DeviceComponent_ComponentLanguagePack{
			Language:     language,
			LanguagePack: languagePack.Proto,
		}
		if err := WriteProtoJSON(w, component, http.StatusOK); err != nil {
			logger.Error("failed to write language pack response", "error", err)
			w.WriteHeader(http.StatusInternalServerError)
		}

	case http.MethodPost, http.MethodPut:
		body, err := io.ReadAll(r.Body)
		if err != nil {
			t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "failed to read request body"))
			return
		}

		pack := &protos.LanguagePack{}
		if err := protojson.Unmarshal(body, pack); err != nil {
			t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid language pack request body"))
			return
		}

		languagePack := catena.LanguagePack{Proto: pack}

		var res catena.StatusResult
		if r.Method == http.MethodPut {
			res = t.runtime.InvokeUpdateLanguagePackHandler(slot, language, languagePack, transportContext)
		} else {
			res = t.runtime.InvokeCreateLanguagePackHandler(slot, language, languagePack, transportContext)
		}
		if res.Code != catena.StatusCodeOk {
			t.writeHTTPStatusResult(w, res)
			return
		}
		w.WriteHeader(http.StatusNoContent)

	case http.MethodDelete:
		res := t.runtime.InvokeDeleteLanguagePackHandler(slot, language, transportContext)
		if res.Code != catena.StatusCodeOk {
			t.writeHTTPStatusResult(w, res)
			return
		}
		w.WriteHeader(http.StatusNoContent)

	default:
		t.writeHTTPMethodNotAllowed(w, "only GET, POST, PUT and DELETE allowed")
	}
}

// streamParamInfo streams param info entries to the client as Server-Sent
// Events. The restStream writes SSE headers lazily on the first chunk, so if the
// handler emits nothing before erroring this method can still report a status;
// once chunks have been sent, a later error is reported in-band as an SSE
// "error" event carrying the HTTP status code.
func (t *Transport) streamParamInfo(w http.ResponseWriter, r *http.Request, slot uint16, oidPrefix string, recursive bool, transportContext catena.TransportContext) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInternal, "streaming not supported"))
		return
	}

	stream := &restStream[st2138.ParamInfo]{
		w:       w,
		flusher: flusher,
		marshal: MarshalProtoJSON,
		ctx:     r.Context(),
		devMode: t.isDevMode(),
	}
	result := t.runtime.InvokeParamInfoHandler(slot, oidPrefix, recursive, stream, transportContext)

	if result.IsError() {
		// With nothing sent yet the HTTP status is still uncommitted, so we can
		// set a normal error status. Once any chunk has been streamed the 200 is
		// already on the wire, so the error is reported in-band as an SSE "error"
		// event carrying the status code that would have been returned.
		if stream.sent == 0 {
			t.writeHTTPStatusResult(w, result)
		} else if err := stream.sendError(result); err != nil {
			logger.Error("failed to send param-info SSE error event", "slot", slot, "oid_prefix", oidPrefix, "error", err)
		}
		return
	}

	// A successful handler that produced no chunks is a legitimate empty result
	// (e.g. a device with no top-level params), not a NotFound. Commit the SSE
	// headers so the client still receives a well-formed empty event stream.
	if stream.sent == 0 {
		stream.writeHeaders()
	}
}

func (t *Transport) handleCommandEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodPost {
		t.writeHTTPMethodNotAllowed(w, "only POST allowed")
		return
	}

	// process the path to determine if this is a streaming command or a unary command.
	// A trailing "stream" segment selects streaming; what remains is the command oid.
	streaming := false
	if len(pathParts) > 0 && pathParts[len(pathParts)-1] == "stream" {
		streaming = true
		pathParts = pathParts[:len(pathParts)-1]
	}

	if len(pathParts) == 0 {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInvalidArgument, "command FQOID is required"))
		return
	}
	if len(pathParts) > 1 {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeNotFound, "unknown command endpoint"))
		return
	}

	commandOid := pathParts[0]

	// respond defaults to false to match protobuf; only respond=true opts in.
	respond := false
	if r.URL.Query().Get("respond") == "true" {
		respond = true
	}

	// Read command payload. Per ST 2138 an empty body is valid and means "no
	// value": leave payload nil and skip all body validation, mirroring the
	// gRPC path (which keys off a nil Value). Only a non-empty body is parsed
	// and validated. We read the body directly rather than trusting
	// Content-Length, which the server reports as -1 for chunked requests.
	var payload any
	body, err := io.ReadAll(r.Body)
	r.Body.Close()
	if err != nil {
		logger.Error("failed to read command payload", "error", err)
		val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "invalid command payload")
		t.writeHTTPResult(w, res, val)
		return
	}
	if len(body) > 0 {
		reqValue, parseErr := parseValueJSON(r.Header.Get("Content-Type"), body)
		if parseErr.IsError() {
			logger.Error("failed to read command payload", "error", parseErr)
			val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "invalid command payload")
			t.writeHTTPResult(w, res, val)
			return
		}
		var errProto error
		payload, errProto = st2138.FromProto(reqValue)
		if errProto != nil {
			logger.Error("failed to convert proto value to native Go type", "error", errProto)
			val, res := catena.ReplyError[st2138.Value](catena.StatusCodeInvalidArgument, "invalid command payload")
			t.writeHTTPResult(w, res, val)
			return
		}
	}

	transportContext := t.retrieveMetadataFromRequest(r)

	if streaming {
		t.streamExecuteCommand(w, r, slot, commandOid, payload, respond, transportContext)
		return
	}

	// Unary: the handler streams CommandResponses but the HTTP reply is a single
	// response, so keep only the last Send. respond is passed through so a smart
	// handler can skip emitting responses; the server also swaps in a nullStream
	// when respond=false, so nothing reaches this stream in that case.
	stream := &lastStream[st2138.CommandResponse]{}
	result := t.runtime.InvokeExecuteCommandHandler(slot, commandOid, payload, respond, stream, transportContext)
	if result.IsError() {
		t.writeHTTPStatusResult(w, result)
		return
	}

	// Reply with the final CommandResponse the handler sent. When the caller opted
	// out (respond=false) the server gobbled every chunk, so nothing was retained
	// and we reply with an explicit no_response.
	cmdResult := st2138.CommandNoResponse()
	if stream.has {
		cmdResult = stream.item
	}

	if err := WriteProtoJSON(w, cmdResult.Proto, http.StatusOK); err != nil {
		logger.Error("failed to write command response", "error", err)
	}
}

// streamExecuteCommand streams command responses to the client as Server-Sent
// Events. The restStream writes SSE headers lazily on the first chunk, so if the
// handler emits nothing before erroring this method can still report a status.
// respond is passed through so a smart handler can skip emitting responses.
func (t *Transport) streamExecuteCommand(w http.ResponseWriter, r *http.Request, slot uint16, commandOid string, payload any, respond bool, transportContext catena.TransportContext) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeInternal, "streaming not supported"))
		return
	}

	stream := &restStream[st2138.CommandResponse]{
		w:       w,
		flusher: flusher,
		marshal: MarshalProtoJSON,
		ctx:     r.Context(),
		devMode: t.isDevMode(),
	}
	result := t.runtime.InvokeExecuteCommandHandler(slot, commandOid, payload, respond, stream, transportContext)

	if result.IsError() {
		// Once any chunk has been streamed the HTTP status is committed, so a
		// late error can only be logged. With nothing sent yet we can still set
		// the error status.
		if stream.sent == 0 {
			t.writeHTTPStatusResult(w, result)
		} else if err := stream.sendError(result); err != nil {
			logger.Error("failed to send command SSE error event", "slot", slot, "command", commandOid, "error", err)
		}
		return
	}

	// A successful handler that produced no chunks still gets a well-formed empty
	// event stream so the client receives a valid 200 response.
	if stream.sent == 0 {
		stream.writeHeaders()
	}
}

// ToHTTPStatus converts a transport-neutral StatusCode to an HTTP status code.
// See StatusCode.md for the design and ST 2138-12 §7.3 for the default REST
// failure set ({401, 403, 404, 500, 503}). The success set is {200, 204}; the
// REST handler picks 204 (no body) vs 200 (body) per the route, not from this
// mapper.
func ToHTTPStatus(s catena.StatusCode) int {
	switch s {
	case catena.StatusCodeOk:
		return http.StatusOK
	case catena.StatusCodeCancelled:
		return 499 // Client Closed Request (nginx convention; no stdlib const)
	case catena.StatusCodeUnknown:
		return http.StatusInternalServerError
	case catena.StatusCodeInvalidArgument:
		return http.StatusBadRequest
	case catena.StatusCodeDeadlineExceeded:
		return http.StatusGatewayTimeout
	case catena.StatusCodeNotFound:
		return http.StatusNotFound
	case catena.StatusCodeAlreadyExists:
		return http.StatusConflict
	case catena.StatusCodePermissionDenied:
		return http.StatusForbidden
	case catena.StatusCodeResourceExhausted:
		return http.StatusTooManyRequests
	case catena.StatusCodeFailedPrecondition:
		return http.StatusBadRequest
	case catena.StatusCodeAborted:
		return http.StatusConflict
	case catena.StatusCodeOutOfRange:
		return http.StatusBadRequest
	case catena.StatusCodeUnimplemented:
		return http.StatusNotImplemented
	case catena.StatusCodeInternal:
		return http.StatusInternalServerError
	case catena.StatusCodeUnavailable:
		return http.StatusServiceUnavailable
	case catena.StatusCodeDataLoss:
		return http.StatusInternalServerError
	case catena.StatusCodeUnauthenticated:
		return http.StatusUnauthorized
	default:
		return http.StatusInternalServerError
	}
}
