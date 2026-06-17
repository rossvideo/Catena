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

package catena

import (
	"context"
	"io"
	"net"
	"net/http"
	"strings"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

func TestConnectionPropsDefaults(t *testing.T) {
	c := NewConnectionProps(config.DashboardOptions{})
	if c.Endpoint() != "/connect/connection-props.xml" {
		t.Errorf("default endpoint = %q", c.Endpoint())
	}
	xml := fetchPropsXML(t, config.DashboardOptions{})

	wants := []string{
		"<properties version=\"1.0\">",
		"<comment>DashBoard Device Connection Settings</comment>",
		"<entry key=\"base-url\">http://localhost/</entry>",
		"<entry key=\"serviceUrl\">service:catena-device</entry>",
		"<entry key=\"equipmentType\">st2138-rest</entry>",
		"<entry key=\"address\">localhost</entry>",
		"<entry key=\"port\">6254</entry>",
		"<entry key=\"connectionType\">TCP</entry>",
		"<entry key=\"index-url\">connect/connection-props.xml</entry>",
		"<entry key=\"refresh-interval\">30000</entry>",
		"</properties>",
	}
	for _, w := range wants {
		if !strings.Contains(xml, w) {
			t.Errorf("XML missing %q\nGot:\n%s", w, xml)
		}
	}

	// node-id / node-name omitted when unset.
	if strings.Contains(xml, "node-id") || strings.Contains(xml, "node-name") {
		t.Errorf("expected node-id/node-name to be omitted, got:\n%s", xml)
	}
}

func TestConnectionPropsCustomValues(t *testing.T) {
	xml := fetchPropsXML(t, config.DashboardOptions{
		Protocol:        ProtocolST2138Grpc,
		ServiceHostname: "10.62.251.47",
		ServicePort:     5254,
		RefreshInterval: 15000,
		NodeName:        "Enterprise Management SSG",
		NodeID:          "Enterprise Management SSG-a4:bb:6d:6a:6f:a3",
		ServiceName:     "service:broadcast-equipment",
	})

	wants := []string{
		"<entry key=\"base-url\">http://10.62.251.47/</entry>",
		"<entry key=\"serviceUrl\">service:broadcast-equipment</entry>",
		"<entry key=\"equipmentType\">st2138-grpc</entry>",
		"<entry key=\"address\">10.62.251.47</entry>",
		"<entry key=\"port\">5254</entry>",
		"<entry key=\"node-id\">Enterprise Management SSG-a4:bb:6d:6a:6f:a3</entry>",
		"<entry key=\"node-name\">Enterprise Management SSG</entry>",
		"<entry key=\"refresh-interval\">15000</entry>",
	}
	for _, w := range wants {
		if !strings.Contains(xml, w) {
			t.Errorf("XML missing %q\nGot:\n%s", w, xml)
		}
	}
}

func TestConnectionPropsTLSScheme(t *testing.T) {
	xml := fetchPropsXML(t, config.DashboardOptions{
		ServiceHostname: "host.example",
		ServiceTLS:      true,
	})
	// DashBoard does not recognize "https"; base-url stays http even with TLS.
	if !strings.Contains(xml, "<entry key=\"base-url\">http://host.example/</entry>") {
		t.Errorf("expected http base-url, got:\n%s", xml)
	}
	if !strings.Contains(xml, "<entry key=\"connectionType\">SSL</entry>") {
		t.Errorf("expected SSL connectionType, got:\n%s", xml)
	}
}

func TestConnectionPropsEndpointNormalized(t *testing.T) {
	c := NewConnectionProps(config.DashboardOptions{Endpoint: "connect/foo.xml"})
	if c.Endpoint() != "/connect/foo.xml" {
		t.Errorf("endpoint not normalized: %q", c.Endpoint())
	}
}

func TestConnectionPropsXMLEscaping(t *testing.T) {
	xml := fetchPropsXML(t, config.DashboardOptions{NodeName: `a&b<c>"d'`})
	// encoding/xml escapes quotes as numeric character references.
	if !strings.Contains(xml, "a&amp;b&lt;c&gt;&#34;d&#39;") {
		t.Errorf("value not escaped, got:\n%s", xml)
	}
}

// fetchPropsXML starts a ConnectionProps server on a free port, fetches the
// served XML over HTTP, and returns the response body. This mirrors how the
// C++ tests verify content, since there is no in-process content getter.
func fetchPropsXML(t *testing.T, opts config.DashboardOptions) string {
	t.Helper()
	port := freePort(t)
	opts.Port = port
	c := NewConnectionProps(opts)
	if err := c.Start(); err != nil {
		t.Fatalf("Start() error: %v", err)
	}
	t.Cleanup(func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		_ = c.Stop(ctx)
	})

	base := "http://127.0.0.1:" + itoa(port)
	waitForServer(t, base+"/health")

	resp, err := http.Get(base + c.Endpoint())
	if err != nil {
		t.Fatalf("GET endpoint: %v", err)
	}
	body, _ := io.ReadAll(resp.Body)
	resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		t.Fatalf("endpoint status = %d, want 200", resp.StatusCode)
	}
	return string(body)
}

// freePort returns an available TCP port for tests.
func freePort(t *testing.T) int {
	t.Helper()
	l, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("failed to find free port: %v", err)
	}
	defer l.Close()
	return l.Addr().(*net.TCPAddr).Port
}

func TestConnectionPropsServeAndStop(t *testing.T) {
	port := freePort(t)
	c := NewConnectionProps(config.DashboardOptions{
		Port:            port,
		ServiceHostname: "127.0.0.1",
		NodeName:        "test-node",
		NodeID:          "test-node-id",
	})

	if err := c.Start(); err != nil {
		t.Fatalf("Start() error: %v", err)
	}
	t.Cleanup(func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		_ = c.Stop(ctx)
	})

	if !c.IsRunning() {
		t.Fatalf("expected server to be running")
	}

	base := "http://127.0.0.1:" + itoa(port)

	// Wait until the listener is accepting connections.
	waitForServer(t, base+"/health")

	// Endpoint returns the XML body.
	resp, err := http.Get(base + "/connect/connection-props.xml")
	if err != nil {
		t.Fatalf("GET endpoint: %v", err)
	}
	body, _ := io.ReadAll(resp.Body)
	resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		t.Fatalf("endpoint status = %d, want 200", resp.StatusCode)
	}
	if ct := resp.Header.Get("Content-Type"); ct != "application/xml" {
		t.Errorf("Content-Type = %q, want application/xml", ct)
	}
	if !strings.Contains(string(body), "<entry key=\"node-name\">test-node</entry>") {
		t.Errorf("body missing node-name:\n%s", body)
	}

	// Health probe.
	hResp, err := http.Get(base + "/health")
	if err != nil {
		t.Fatalf("GET health: %v", err)
	}
	hResp.Body.Close()
	if hResp.StatusCode != http.StatusOK {
		t.Errorf("health status = %d, want 200", hResp.StatusCode)
	}

	// Unknown path 404.
	nfResp, err := http.Get(base + "/nope")
	if err != nil {
		t.Fatalf("GET unknown: %v", err)
	}
	nfResp.Body.Close()
	if nfResp.StatusCode != http.StatusNotFound {
		t.Errorf("unknown path status = %d, want 404", nfResp.StatusCode)
	}

	// Non-GET method on endpoint -> 405.
	req, _ := http.NewRequest(http.MethodPost, base+"/connect/connection-props.xml", nil)
	mResp, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatalf("POST endpoint: %v", err)
	}
	mResp.Body.Close()
	if mResp.StatusCode != http.StatusMethodNotAllowed {
		t.Errorf("POST status = %d, want 405", mResp.StatusCode)
	}

	// Stop and verify it is no longer running.
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	if err := c.Stop(ctx); err != nil {
		t.Errorf("Stop() error: %v", err)
	}
	if c.IsRunning() {
		t.Errorf("expected server to be stopped")
	}
}

func TestConnectionPropsDoubleStart(t *testing.T) {
	port := freePort(t)
	c := NewConnectionProps(config.DashboardOptions{Port: port})
	if err := c.Start(); err != nil {
		t.Fatalf("first Start() error: %v", err)
	}
	t.Cleanup(func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		_ = c.Stop(ctx)
	})

	if err := c.Start(); err == nil {
		t.Errorf("expected error on second Start()")
	}
}

func TestConnectionPropsStopWhenNotRunning(t *testing.T) {
	c := NewConnectionProps(config.DashboardOptions{})
	if err := c.Stop(context.Background()); err != nil {
		t.Errorf("Stop() on idle server error: %v", err)
	}
}

// waitForServer polls url until it responds or the timeout elapses.
func waitForServer(t *testing.T, url string) {
	t.Helper()
	deadline := time.Now().Add(2 * time.Second)
	for time.Now().Before(deadline) {
		resp, err := http.Get(url)
		if err == nil {
			resp.Body.Close()
			return
		}
		time.Sleep(10 * time.Millisecond)
	}
	t.Fatalf("server did not become ready: %s", url)
}

// itoa is a tiny helper to avoid importing strconv in this test file.
func itoa(n int) string {
	if n == 0 {
		return "0"
	}
	var buf [20]byte
	i := len(buf)
	for n > 0 {
		i--
		buf[i] = byte('0' + n%10)
		n /= 10
	}
	return string(buf[i:])
}
