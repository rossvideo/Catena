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
	"fmt"
	"net"
	"net/http"
	"strings"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// ConnectionProtocol identifies the Catena transport advertised to DashBoard.
type ConnectionProtocol int

const (
	// ProtocolST2138Rest advertises a ST 2138 REST device.
	ProtocolST2138Rest ConnectionProtocol = iota
	// ProtocolST2138Grpc advertises a ST 2138 gRPC device.
	ProtocolST2138Grpc
)

// String returns the canonical protocol identifier used in logs.
func (p ConnectionProtocol) String() string {
	switch p {
	case ProtocolST2138Rest:
		return "st2138-rest"
	case ProtocolST2138Grpc:
		return "st2138-grpc"
	default:
		return ""
	}
}

// Default values
const (
	defaultConnectionPropsEndpoint    = "/connect/connection-props.xml"
	defaultConnectionPropsService     = "service:catena-device"
	defaultConnectionPropsRefreshMs   = 30000
	defaultConnectionPropsHostname    = "localhost"
	defaultConnectionPropsPort        = 8080
	defaultConnectionPropsServicePort = 6254
)

// ConnectionPropsOptions configures a ConnectionProps server
type ConnectionPropsOptions struct {
	// Protocol is the transport being advertised (used for logging and the
	// advertised equipmentType).
	Protocol ConnectionProtocol
	// Dashboard carries the shared connection settings advertised to DashBoard
	// (listen port, service host/port, TLS). These typically come from the
	// loaded runtime configuration.
	Dashboard config.DashboardOptions
	// RefreshInterval is the DashBoard refresh interval in milliseconds (default 30000).
	RefreshInterval uint32
	// NodeName is the optional human-readable node name.
	NodeName string
	// NodeID is the optional unique node identifier.
	NodeID string
	// ServiceName is the advertised service URL (default "service:catena-device").
	ServiceName string
	// Endpoint is the path served (default "/connect/connection-props.xml").
	Endpoint string
}

// ConnectionProps is a lightweight HTTP server that serves a single endpoint
// (/connect/connection-props.xml) with the connection properties consumed by
// the DashBoard "Detect Frame Information" workflow. It is transport-agnostic
// and can front either a REST or gRPC Catena device.
type ConnectionProps struct {
	opts ConnectionPropsOptions

	mu      sync.Mutex
	server  *http.Server
	content string
	running bool
}

// NewConnectionProps builds a ConnectionProps server, applying defaults for any
// unset option. The XML payload is generated once at construction time.
func NewConnectionProps(opts ConnectionPropsOptions) *ConnectionProps {
	if opts.Dashboard.Port == 0 {
		opts.Dashboard.Port = defaultConnectionPropsPort
	}
	if opts.Dashboard.ServicePort == 0 {
		opts.Dashboard.ServicePort = defaultConnectionPropsServicePort
	}
	if opts.Dashboard.ServiceHostname == "" {
		opts.Dashboard.ServiceHostname = defaultConnectionPropsHostname
	}
	if opts.RefreshInterval == 0 {
		opts.RefreshInterval = defaultConnectionPropsRefreshMs
	}
	if opts.ServiceName == "" {
		opts.ServiceName = defaultConnectionPropsService
	}
	if opts.Endpoint == "" {
		opts.Endpoint = defaultConnectionPropsEndpoint
	}
	if !strings.HasPrefix(opts.Endpoint, "/") {
		opts.Endpoint = "/" + opts.Endpoint
	}

	c := &ConnectionProps{opts: opts}
	c.content = c.generateXML()
	logger.Info("Connection props server constructed",
		"endpoint", opts.Endpoint, "port", opts.Dashboard.Port, "protocol", opts.Protocol.String())
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
		logger.Warning("Connection props server already running", "port", c.opts.Dashboard.Port)
		return fmt.Errorf("connection props server already running")
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/", c.handle)

	addr := fmt.Sprintf(":%d", c.opts.Dashboard.Port)
	// Bind synchronously so that errors (privileged port, address already in
	// use, etc.) are returned to the caller instead of only being logged
	// asynchronously after Start has already reported success.
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

	logger.Info("Stopping connection props server", "port", c.opts.Dashboard.Port)
	err := srv.Shutdown(ctx)
	if err != nil {
		// Best-effort hard close so we never leak the listener.
		_ = srv.Close()
	}

	c.mu.Lock()
	c.running = false
	c.mu.Unlock()

	logger.Info("Connection props server stopped")
	return err
}

// handle serves the connection-props endpoint and a /health probe; everything
// else returns 404. Only GET is permitted on the props endpoint.
func (c *ConnectionProps) handle(w http.ResponseWriter, r *http.Request) {
	logger.Debug("Connection props request", "method", r.Method, "path", r.URL.Path)

	switch r.URL.Path {
	case c.opts.Endpoint:
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
	case "/health":
		w.Header().Set("Content-Type", "text/plain")
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("OK"))
	default:
		http.Error(w, "Not Found", http.StatusNotFound)
	}
}

// generateXML renders the DashBoard connection-props XML payload.
func (c *ConnectionProps) generateXML() string {
	scheme := "http"
	connectionType := "TCP"
	if c.opts.Dashboard.ServiceTLS {
		scheme = "https"
		connectionType = "SSL"
	}

	var b strings.Builder
	b.WriteString("<properties version=\"1.0\">\n")
	b.WriteString("    <comment>DashBoard Device Connection Settings</comment>\n")
	writeEntry(&b, "base-url", fmt.Sprintf("%s://%s/", scheme, c.opts.Dashboard.ServiceHostname))
	writeEntry(&b, "serviceUrl", c.opts.ServiceName)
	writeEntry(&b, "equipmentType", c.opts.Protocol.String())
	writeEntry(&b, "address", c.opts.Dashboard.ServiceHostname)
	writeEntry(&b, "port", fmt.Sprintf("%d", c.opts.Dashboard.ServicePort))
	writeEntry(&b, "connectionType", connectionType)
	if c.opts.NodeID != "" {
		writeEntry(&b, "node-id", c.opts.NodeID)
	}
	if c.opts.NodeName != "" {
		writeEntry(&b, "node-name", c.opts.NodeName)
	}
	writeEntry(&b, "index-url", "connect/connection-props.xml")
	writeEntry(&b, "refresh-interval", fmt.Sprintf("%d", c.opts.RefreshInterval))
	b.WriteString("</properties>")
	return b.String()
}

// writeEntry appends a single <entry> line with an XML-escaped value.
func writeEntry(b *strings.Builder, key, value string) {
	b.WriteString("    <entry key=\"")
	b.WriteString(key)
	b.WriteString("\">")
	b.WriteString(xmlEscape(value))
	b.WriteString("</entry>\n")
}

// xmlEscape escapes the characters that are illegal in XML text content.
func xmlEscape(s string) string {
	replacer := strings.NewReplacer(
		"&", "&amp;",
		"<", "&lt;",
		">", "&gt;",
		"\"", "&quot;",
		"'", "&apos;",
	)
	return replacer.Replace(s)
}
