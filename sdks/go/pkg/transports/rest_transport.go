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
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-14
 */

package transports

import (
	"context"
	"encoding/json"
	"fmt"
	"maps"
	"net/http"
	"strings"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

type FallbackHandler func(w http.ResponseWriter, r *http.Request) (catena.Value, catena.StatusResult)

type RestTransport struct {
	mu              sync.Mutex
	server          *http.Server
	mux             *http.ServeMux
	runtime         catena.ServerRuntime
	fallbackHandler FallbackHandler

	port int

	// future configs
	// tls: TlsConfig etc.
}

var _ catena.Transport = (*RestTransport)(nil)

func NewRestTransport(port int) *RestTransport {
	t := &RestTransport{
		port: port,
		mux:  http.NewServeMux(),
	}
	t.registerRoutes()
	return t
}

func NewDefaultRestTransport() *RestTransport {
	return NewRestTransport(
		8080, // port
	)
}

// Start starts the HTTP server on the specified port using this server's mux
func (t *RestTransport) Start(ctx context.Context, runtime catena.ServerRuntime) error {
	addr := fmt.Sprintf(":%d", t.port)
	t.server = &http.Server{
		Addr:    addr,
		Handler: t.mux,
	}
	t.runtime = runtime

	// http server does not use context
	// listen and serve blocks so do it in a goroutine
	go func() {
		logger.Info("REST Transport listening", "address", addr)
		err := t.server.ListenAndServe()
		if err != nil && err != http.ErrServerClosed {
			logger.Error("HTTP server error", "error", err)
		}
	}()
	return nil
}

func (t *RestTransport) Shutdown(ctx context.Context) error {
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

func (t *RestTransport) RegisterFallbackHandler(handler FallbackHandler) {
	t.mu.Lock()
	defer t.mu.Unlock()
	t.fallbackHandler = handler
}

func (t *RestTransport) retrieveMetadataFromRequest(r *http.Request) catena.TransportContext {
	transportContext := catena.TransportContext{
		AccessToken: r.Header.Get("Authorization"),
		Metadata:    maps.Clone(r.Header), // include all headers as metadata for now; could be filtered in the future if needed
	}
	return transportContext
}

func (t *RestTransport) isDevMode() bool {
	return t != nil && t.runtime != nil && t.runtime.IsDev()
}

// writeHTTPResult is a unified function that handles writing different response types
func (t *RestTransport) writeHTTPResult(w http.ResponseWriter, result catena.StatusResult, value interface{}) {
	httpStatus := ToHTTPStatus(result.Code)

	if result.Error != "" {
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
	case catena.Value:
		writeValueResult(w, v, httpStatus)
	case catena.Device:
		writeDeviceResult(w, v, httpStatus)
	case catena.Asset:
		writeAssetResult(w, v, httpStatus)
	default:
		w.WriteHeader(httpStatus)
	}
}

// writeHTTPMethodNotAllowed writes HTTP 405 with a JSON error body. Method
// enforcement is a transport-only concern, so it bypasses StatusCode entirely.
// 405 is not in ST 2138-12, but is emitted here for HTTP correctness when a
// route is reached with the wrong method.
func (t *RestTransport) writeHTTPMethodNotAllowed(w http.ResponseWriter, msg string) {
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
func (t *RestTransport) writeHTTPStatusResultNoBody(w http.ResponseWriter, result catena.StatusResult) {
	if result.Code == catena.StatusCodeOk && result.Error == "" {
		w.WriteHeader(http.StatusNoContent)
		return
	}
	t.writeHTTPStatusResult(w, result)
}

// t.writeHTTPStatusResult writes a StatusResult to the HTTP response (no value).
func (t *RestTransport) writeHTTPStatusResult(w http.ResponseWriter, result catena.StatusResult) {
	httpStatus := ToHTTPStatus(result.Code)
	logger.Info("t.writeHTTPStatusResult", "httpStatus", httpStatus, "error", result.Error, "code", result.Code)

	// Set status code BEFORE writing body
	w.WriteHeader(httpStatus)

	if result.Error != "" {
		// Only return detailed error messages in dev mode
		if t.isDevMode() {
			json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		} else {
			json.NewEncoder(w).Encode(map[string]string{"error": http.StatusText(httpStatus)})
		}
	}
}

// writeValueResult writes a Value as JSON
func writeValueResult(w http.ResponseWriter, value catena.Value, httpStatus int) {
	protoValue := value.Value
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
func writeDeviceResult(w http.ResponseWriter, device catena.Device, httpStatus int) {
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

// writeAssetResult writes an Asset as JSON-encoded ExternalObjectPayload
func writeAssetResult(w http.ResponseWriter, asset catena.Asset, httpStatus int) {
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

// For unit tests to override the default functions
var marshalSSEFunc = MarshalProtoJSON

// sendSSEEvent writes a single SSE event to the response writer,
// serializing the proto PushUpdates message via MarshalProtoJSON.
func (t *RestTransport) sendSSEEvent(w http.ResponseWriter, flusher http.Flusher, update *protos.PushUpdates) error {
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
func (t *RestTransport) handleConnect(w http.ResponseWriter, r *http.Request) {
	// Check if the request method is GET
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	// Check if SSE streaming is supported
	flusher, ok := w.(http.Flusher)
	if !ok {
		val, res := catena.ReplyError[catena.Value](catena.StatusCodeInternal, "streaming not supported")
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
		val, res := catena.ReplyError[catena.Value](res.Code, res.Error)
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
func (t *RestTransport) registerRoutes() {
	// Device endpoint: GET /st2138-api/v1/{slot}
	t.mux.HandleFunc("/st2138-api/v1/", func(w http.ResponseWriter, r *http.Request) {
		logger.Info("Device endpoint", "path", r.URL.Path, "method", r.Method)
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			val, res := catena.ReplyError[catena.Value](catena.StatusCodeInvalidArgument, "invalid path format")
			t.writeHTTPResult(w, res, val)
			return
		}

		slotStr := parts[2]
		slot, err := catena.ValidateSlotString(slotStr)
		if err.Code != catena.StatusCodeOk {
			val, res := catena.ReplyError[catena.Value](catena.StatusCodeInvalidArgument, "invalid slot number")
			t.writeHTTPResult(w, res, val)
			return
		}

		// Route based on path structure
		if len(parts) == 3 && r.Method == http.MethodGet {
			// GET /st2138-api/v1/{slot} - Get device info
			transportContext := t.retrieveMetadataFromRequest(r)
			device, res := t.runtime.InvokeGetDeviceHandler(slot, transportContext)
			t.writeHTTPResult(w, res, device)
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
			case "param-info":
				t.handleParamInfoEndpoint(w, r, slot, parts[4:])
			default:
				val, res := catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "unknown endpoint")
				t.writeHTTPResult(w, res, val)
			}
			return
		}

		val, res := catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "endpoint not found")
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
		val, res := catena.ReplyError[catena.Value](catena.StatusCodeNotFound, "endpoint not found")
		t.writeHTTPResult(w, res, val)
	})

	t.mux.HandleFunc("/st2138-api/v1", func(w http.ResponseWriter, r *http.Request) {
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeNotFound, "endpoint not found"))
	})
}

// handleGetPopulatedSlots handles GET /st2138-api/v1/devices
// Returns the list of populated slots
func (t *RestTransport) handleGetPopulatedSlots(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	logger.Info("GetPopulatedSlots")
	transportContext := t.retrieveMetadataFromRequest(r)
	slots, err := t.runtime.GetSlots(transportContext)
	if err.Code != catena.StatusCodeOk {
		val, res := catena.ReplyError[catena.Value](err.Code, err.Error)
		t.writeHTTPResult(w, res, val)
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

func (t *RestTransport) handleValueEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
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
		nativeValue, errProto := catena.FromProto(reqValue)
		if errProto.Code != catena.StatusCodeOk {
			logger.Error("failed to convert proto value to native Go type", "error", errProto.Error)
			val, res := catena.ReplyError[catena.Value](catena.StatusCodeInvalidArgument, "invalid request body")
			t.writeHTTPResult(w, res, val)
			return
		}

		transportContext := t.retrieveMetadataFromRequest(r)
		res := t.runtime.InvokeSetValueHandler(nativeValue, slot, fqoid, transportContext)
		t.writeHTTPStatusResultNoBody(w, res)

	default:
		t.writeHTTPMethodNotAllowed(w, "only GET, PUT, PATCH allowed")
	}
}

// handleValuesEndpoint handles PUT /st2138-api/v1/{slot}/values (SetValues).
// The values are applied via the runtime's MultiSetValue handler.
// On success it returns 204
func (t *RestTransport) handleValuesEndpoint(w http.ResponseWriter, r *http.Request, slot uint16) {
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
	res := t.runtime.InvokeMultiSetValueHandler(entries, slot, transportContext)
	t.writeHTTPStatusResultNoBody(w, res)
}

func (t *RestTransport) handleAssetEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodGet {
		t.writeHTTPMethodNotAllowed(w, "only GET allowed")
		return
	}

	fqoid := strings.Join(pathParts, "/")
	transportContext := t.retrieveMetadataFromRequest(r)
	asset, res := t.runtime.InvokeGetAssetHandler(slot, fqoid, transportContext)

	if res.Error == "" {
		if compressionStr := r.URL.Query().Get("compression"); compressionStr != "" {
			targetEncoding, encRes := catena.ParsePayloadEncoding(compressionStr)
			if encRes.Code != catena.StatusCodeOk {
				val, errRes := catena.ReplyError[catena.Value](catena.StatusCodeInvalidArgument, encRes.Error)
				t.writeHTTPResult(w, errRes, val)
				return
			}
			if tcRes := catena.TranscodeAssetPayload(&asset, targetEncoding); tcRes.Code != catena.StatusCodeOk {
				logger.Error("failed to transcode asset payload", "error", tcRes.Error)
				val, errRes := catena.ReplyError[catena.Value](catena.StatusCodeInternal, "failed to transcode payload: "+tcRes.Error)
				t.writeHTTPResult(w, errRes, val)
				return
			}
		}
	}

	t.writeHTTPResult(w, res, asset)
}

// handleParamInfoEndpoint handles param info requests and streaming (SSE).
func (t *RestTransport) handleParamInfoEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
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

	// Build OID prefix from path segments.
	oidPrefix := ""
	for _, p := range fqoidParts {
		oidPrefix += "/" + p
	}

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
	infos, res := t.runtime.InvokeParamInfoHandler(slot, oidPrefix, recursive, transportContext)
	if res.Code != catena.StatusCodeOk {
		t.writeHTTPStatusResult(w, res)
		return
	}

	if len(infos) == 0 {
		msg := "Parameter not found: " + oidPrefix
		if oidPrefix == "" {
			msg = "No top-level parameters found"
		}
		t.writeHTTPStatusResult(w, catena.StatusWithCode(catena.StatusCodeNotFound, msg))
		return
	}

	if streaming {
		t.writeParamInfoStream(w, r, infos)
		return
	}

	if err := WriteProtoJSON(w, infos[0].GetProtoResponse(), http.StatusOK); err != nil {
		logger.Error("failed to write param info response", "error", err)
	}
}

// writeParamInfoStream streams the param info entries to the client as Server-Sent Events.
func (t *RestTransport) writeParamInfoStream(w http.ResponseWriter, r *http.Request, infos []catena.ParamInfo) {
	flusher, ok := w.(http.Flusher)
	if !ok {
		val, res := catena.ReplyError[catena.Value](catena.StatusCodeInternal, "streaming not supported")
		t.writeHTTPResult(w, res, val)
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

func (t *RestTransport) handleCommandEndpoint(w http.ResponseWriter, r *http.Request, slot uint16, pathParts []string) {
	if r.Method != http.MethodPost {
		t.writeHTTPMethodNotAllowed(w, "only POST allowed")
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
		if err.Code != catena.StatusCodeOk {
			logger.Error("failed to read command payload", "error", err)
			val, res := catena.ReplyError[catena.Value](catena.StatusCodeInvalidArgument, "invalid command payload")
			t.writeHTTPResult(w, res, val)
			return
		}
		var errProto catena.StatusResult
		payload, errProto = catena.FromProto(reqValue)
		if errProto.Code != catena.StatusCodeOk {
			logger.Error("failed to convert proto value to native Go type", "error", errProto.Error)
			val, res := catena.ReplyError[catena.Value](catena.StatusCodeInvalidArgument, "invalid command payload")
			t.writeHTTPResult(w, res, val)
			return
		}
	}

	transportContext := t.retrieveMetadataFromRequest(r)
	cmdResult, res := t.runtime.InvokeExecuteCommandHandler(slot, commandFqoid, payload, transportContext)
	if res.Code != catena.StatusCodeOk {
		t.writeHTTPResult(w, res, catena.Value{})
		return
	}

	if !respond {
		cmdResult, _ = catena.CommandNoResponse()
	}

	_ = WriteProtoJSON(w, cmdResult.GetProtoResponse(), http.StatusOK)
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
