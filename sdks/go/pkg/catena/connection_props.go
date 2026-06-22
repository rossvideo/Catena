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
 * @brief Lightweight HTTP server that serves DashBoard connection properties.
 * @file connection_props.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-06-08
 */

package catena

import (
	"context"
	"encoding/xml"
	"fmt"
	"net"
	"net/http"
	"strings"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// ConnectionProps is a lightweight HTTP server that serves a single endpoint
// (/connect/connection-props.xml) with the connection properties consumed by
// the DashBoard "Detect Frame Information" workflow. It is transport-agnostic
// and can front either a REST or gRPC Catena device.
type ConnectionProps struct {
	opts DashboardOptions

	mu      sync.Mutex
	server  *http.Server
	content string
	running bool
}

// NewConnectionProps builds a ConnectionProps server from the supplied options,
// which are expected to be initialized (see config.DefaultDashboardOptions). The
// XML payload is generated once at construction time.
func NewConnectionProps(opts DashboardOptions) *ConnectionProps {
	if !strings.HasPrefix(opts.Endpoint, "/") {
		opts.Endpoint = "/" + opts.Endpoint
	}

	c := &ConnectionProps{opts: opts}
	c.content = c.generateXML()
	logger.Info("Connection props server constructed",
		"endpoint", opts.Endpoint, "port", opts.Port, "protocol", string(opts.Protocol))
	return c
}

// Endpoint returns the path this server responds to.
func (c *ConnectionProps) Endpoint() string {
	return c.opts.Endpoint
}

// IsRunning reports whether the server is currently listening.
func (c *ConnectionProps) IsRunning() bool {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.running
}

// Start begins serving connection props in a background goroutine. It returns
// an error if the server is already running or the listener cannot be created.
func (c *ConnectionProps) Start() error {
	c.mu.Lock()
	if c.running {
		c.mu.Unlock()
		logger.Warning("Connection props server already running", "port", c.opts.Port)
		return fmt.Errorf("connection props server already running")
	}

	// Endpoint is known up front, so register explicit handlers for the
	// props endpoint and the health probe; ServeMux returns 404 for anything
	// else.
	mux := http.NewServeMux()
	mux.HandleFunc(c.opts.Endpoint, c.handleProps)
	mux.HandleFunc("/health", c.handleHealth)

	addr := fmt.Sprintf(":%d", c.opts.Port)
	// Bind synchronously so that errors (privileged port, address already in
	// use, etc.) are returned to the caller instead of only being logged
	// asynchronously after Start has already reported success.
	// TODO: replicate this synchronous-listener startup routine in RestTransport
	// (sdks/go/pkg/transports/rest_transport.go) so its errors surface to the caller too.
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		c.mu.Unlock()
		logger.Error("Connection props server failed to listen",
			"address", addr, "error", err)
		return fmt.Errorf("connection props server failed to listen on %s: %w", addr, err)
	}

	c.server = &http.Server{
		Addr:    addr,
		Handler: mux,
	}
	c.running = true
	srv := c.server
	c.mu.Unlock()

	go func() {
		logger.Info("Connection props server listening",
			"address", srv.Addr, "endpoint", c.opts.Endpoint)
		err := srv.Serve(listener)
		if err != nil && err != http.ErrServerClosed {
			logger.Error("Connection props server error", "error", err)
		}
		c.mu.Lock()
		c.running = false
		c.mu.Unlock()
	}()
	return nil
}

// Stop gracefully shuts down the server, honoring the provided context deadline.
// It is safe to call when the server is not running.
func (c *ConnectionProps) Stop(ctx context.Context) error {
	c.mu.Lock()
	srv := c.server
	running := c.running
	c.mu.Unlock()

	if !running || srv == nil {
		return nil
	}

	logger.Info("Stopping connection props server", "port", c.opts.Port)
	err := srv.Shutdown(ctx)
	if err != nil {
		// Best-effort hard close so we never leak the listener.
		logger.Warning("HTTP server shutdown timed out, forcing close")
		closeErr := srv.Close()
		if closeErr != nil {
			logger.Error("failed to force close HTTP server", "error", closeErr)
		}
	}

	c.mu.Lock()
	c.running = false
	c.mu.Unlock()

	logger.Info("Connection props server stopped")
	return err
}

// handleProps serves the connection-props endpoint. Only GET is permitted.
func (c *ConnectionProps) handleProps(w http.ResponseWriter, r *http.Request) {
	logger.Debug("Connection props request", "method", r.Method, "path", r.URL.Path)

	if r.Method != http.MethodGet {
		w.Header().Set("Allow", "GET")
		http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
		return
	}
	c.mu.Lock()
	body := c.content
	c.mu.Unlock()

	w.Header().Set("Content-Type", "application/xml")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
	w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte(body))
}

// handleHealth serves the /health probe.
func (c *ConnectionProps) handleHealth(w http.ResponseWriter, r *http.Request) {
	logger.Debug("Connection props request", "method", r.Method, "path", r.URL.Path)
	w.Header().Set("Content-Type", "text/plain")
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte("OK"))
}

// generateXML renders the DashBoard connection-props XML payload using the
// standard library marshaller.
func (c *ConnectionProps) generateXML() string {
	// DashBoard does not recognize the "https" scheme, so the advertised
	// base-url always uses "http"; TLS is signalled via connectionType=SSL.
	scheme := "http"
	connectionType := "TCP"
	if c.opts.ServiceTLS {
		connectionType = "SSL"
	}

	type entry struct {
		Key   string `xml:"key,attr"`
		Value string `xml:",chardata"`
	}

	entries := []entry{
		{Key: "base-url", Value: fmt.Sprintf("%s://%s/", scheme, c.opts.ServiceHostname)},
		{Key: "serviceUrl", Value: c.opts.ServiceName},
		{Key: "equipmentType", Value: string(c.opts.Protocol)},
		{Key: "address", Value: c.opts.ServiceHostname},
		{Key: "port", Value: fmt.Sprintf("%d", c.opts.ServicePort)},
		{Key: "connectionType", Value: connectionType},
	}
	if c.opts.NodeID != "" {
		entries = append(entries, entry{Key: "node-id", Value: c.opts.NodeID})
	}
	if c.opts.NodeName != "" {
		entries = append(entries, entry{Key: "node-name", Value: c.opts.NodeName})
	}
	entries = append(entries,
		entry{Key: "index-url", Value: c.opts.Endpoint},
		entry{Key: "refresh-interval", Value: fmt.Sprintf("%d", c.opts.RefreshInterval)},
	)

	doc := struct {
		XMLName xml.Name `xml:"properties"`
		Version string   `xml:"version,attr"`
		Comment string   `xml:"comment"`
		Entries []entry  `xml:"entry"`
	}{
		Version: "1.0",
		Comment: "DashBoard Device Connection Settings",
		Entries: entries,
	}

	out, err := xml.MarshalIndent(doc, "", "    ")
	if err != nil {
		logger.Error("Failed to marshal connection props XML", "error", err)
		return ""
	}
	return string(out)
}
