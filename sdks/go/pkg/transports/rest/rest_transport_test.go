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
 * @brief Test for the REST transport for the Catena SDK.
 * @file rest_transport_test.go
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
	"errors"
	"fmt"
	"net"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
	"github.com/rossvideo/catena/sdks/go/pkg/transports/internal/transporttest"
	"google.golang.org/protobuf/proto"
)

func makeTestTransport(tb testing.TB) (*Transport, *transporttest.StubServerRuntime) {
	transport := NewTransport(config.RestOptions{Port: 8080})
	stubRuntime := transporttest.MakeStubServerRuntime(tb)
	stubRuntime.Dev = true
	transport.runtime = stubRuntime
	return transport, stubRuntime
}

func TestTransport_New(t *testing.T) {
	expected := 1234
	transport := NewTransport(config.RestOptions{Port: expected})
	if transport == nil {
		t.Fatal("NewTransport returned nil")
	}
	if transport.port != expected {
		t.Errorf("expected port %d, got %d", expected, transport.port)
	}
}

func TestTransport_PropagatesTransportContext(t *testing.T) {
	headers := map[string]string{
		"Authorization": "Bearer rest-token",
		"X-Test-Tenant": "tenant-a",
		"Content-Type":  "application/json",
	}
	assertContext := func(t *testing.T, ctx catena.TransportContext) {
		t.Helper()
		if ctx.AccessToken != headers["Authorization"] {
			t.Errorf("expected access token %q, got %q", headers["Authorization"], ctx.AccessToken)
		}
		if got := ctx.Metadata["X-Test-Tenant"]; len(got) != 1 || got[0] != headers["X-Test-Tenant"] {
			t.Errorf("expected X-Test-Tenant metadata %q, got %v", headers["X-Test-Tenant"], got)
		}
		if got := ctx.Metadata["Authorization"]; len(got) != 1 || got[0] != headers["Authorization"] {
			t.Errorf("expected Authorization metadata %q, got %v", headers["Authorization"], got)
		}
		if ctx.Ctx == nil {
			t.Error("expected the request context to be propagated, got nil")
		}
	}

	tests := []struct {
		name  string
		run   func(t *testing.T, transport *Transport)
		setup func(t *testing.T, runtime *transporttest.StubServerRuntime)
	}{
		{
			name: "get populated slots",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.GetSlotsFn = func(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
					assertContext(t, ctx)
					return []uint16{0}, catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/devices", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "get device",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.GetDeviceFn = func(slot uint16, ctx catena.TransportContext) (st2138.Device, catena.StatusResult) {
					assertContext(t, ctx)
					device := *st2138.NewDevice(slot)
					return catena.Reply(device)
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/0", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "get value",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.GetValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Value, catena.StatusResult) {
					assertContext(t, ctx)
					value, _ := st2138.ToValue(int32(42))
					return catena.Reply(value)
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/0/value/brightness", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "get param",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.GetParamFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
					assertContext(t, ctx)
					return *st2138.NewParamInt32(42), catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/0/param/brightness", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "set value",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.SetValueFn = func(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
					assertContext(t, ctx)
					return catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodPut, "/st2138-api/v1/0/value/brightness", `{"int32_value": 42}`, headers)
				assertStatus(t, rec, http.StatusNoContent)
			},
		},
		{
			name: "set values",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.SetValueFn = func(slot uint16, values []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
					assertContext(t, ctx)
					return catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodPut, "/st2138-api/v1/0/values",
					`{"values":[{"oid":"a","value":{"int32_value":1}}]}`, headers)
				assertStatus(t, rec, http.StatusNoContent)
			},
		},
		{
			name: "get asset",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
					assertContext(t, ctx)
					asset, _ := st2138.ToAsset(st2138.DataPayload{Payload: []byte("asset")}, false)
					return catena.Reply(asset)
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/0/asset/logo", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "execute command",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.CommandFn = func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
					assertContext(t, ctx)
					return []st2138.CommandResponse{st2138.CommandNoResponse()}, catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodPost, "/st2138-api/v1/0/command/reboot", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "param info",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
					assertContext(t, ctx)
					return []st2138.ParamInfo{
						st2138.NewParamInfo("text_box", nil, st2138.ParamTypeString, "", 0),
					}, catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/text_box", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "languages",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				runtime.ListLanguagesFn = func(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult) {
					assertContext(t, ctx)
					return []string{"en", "fr"}, catena.StatusWithCode(catena.StatusCodeOk, "")
				}
			},
			run: func(t *testing.T, transport *Transport) {
				rec := makeRequestWithHeaders(t, transport, http.MethodGet, "/st2138-api/v1/0/languages", "", headers)
				assertStatus(t, rec, http.StatusOK)
			},
		},
		{
			name: "connect",
			setup: func(t *testing.T, runtime *transporttest.StubServerRuntime) {
				connection := transporttest.MakeTestConnection(1)
				runtime.RegisterTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
					assertContext(t, ctx)
					return connection, catena.StatusWithCode(catena.StatusCodeOk, "")
				}
				runtime.DeregisterConnFn = func(connID int) {}
			},
			run: func(t *testing.T, transport *Transport) {
				ctx, cancel := context.WithCancel(context.Background())
				defer cancel()
				req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
				for key, value := range headers {
					req.Header.Set(key, value)
				}
				rec := httptest.NewRecorder()
				go transport.mux.ServeHTTP(rec, req)
				time.Sleep(50 * time.Millisecond)
				cancel()
				time.Sleep(50 * time.Millisecond)
				assertStatus(t, rec, http.StatusOK)
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			transport, runtime := makeTestTransport(t)
			tt.setup(t, runtime)
			tt.run(t, transport)
		})
	}
}

func TestTransport_GetDevice_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	device := *st2138.NewDevice(0).
		WithDetailLevel(st2138.DetailLevelFull)

	runtime.GetDeviceFn = func(slot uint16, ctx catena.TransportContext) (st2138.Device, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		return catena.Reply(device)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestTransport_GetDevice_NotFound(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.GetDeviceFn = func(slot uint16, ctx catena.TransportContext) (st2138.Device, catena.StatusResult) {
		handlerCalled = true
		if slot != 99 {
			t.Errorf("expected slot 99, got %d", slot)
		}
		return catena.ReplyError[st2138.Device](catena.StatusCodeNotFound, "device not found")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/99", "")
	if rec.Code != http.StatusNotFound && rec.Code != http.StatusOK {
		t.Errorf("expected status %d or %d, got %d", http.StatusNotFound, http.StatusOK, rec.Code)
	}
	if !handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestTransport_GetDevice_InvalidSlot(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.GetDeviceFn = func(slot uint16, ctx catena.TransportContext) (st2138.Device, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		return catena.ReplyError[st2138.Device](catena.StatusCodeInvalidArgument, "invalid slot")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/invalid", "")
	if rec.Code == http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestTransport_GetValue_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	value, _ := st2138.ToValue(int32(42))
	runtime.GetValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Value, catena.StatusResult) {
		if fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", fqoid)
		}
		return catena.Reply(value)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/value/brightness", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestTransport_GetParam_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.GetParamFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
		handlerCalled = true
		if fqoid != "text_box" {
			t.Errorf("expected fqoid 'text_box', got %s", fqoid)
		}
		param := st2138.NewParamString("Hello, World!").
			WithName(st2138.NewPolyglotText("en", "Text Box").With("es", "Caja de Texto"))
		return *param, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param/text_box", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	assertBodyContains(t, rec, `"oid":"text_box"`)
	assertBodyContains(t, rec, `"string_value":"Hello, World!"`)
	assertBodyContains(t, rec, `"type":"STRING"`)
}

func TestTransport_GetParam_NestedFqoid(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.GetParamFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
		if fqoid != "parent/child" {
			t.Errorf("expected fqoid 'parent/child', got %s", fqoid)
		}
		return *st2138.NewParamInt32(7), catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param/parent/child", "")
	assertStatus(t, rec, http.StatusOK)
	assertBodyContains(t, rec, `"oid":"parent/child"`)
}

func TestTransport_GetParam_MissingFqoid(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param", "")
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestTransport_GetParam_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/param/text_box", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
}

func TestTransport_GetParam_HandlerError(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.GetParamFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
		return st2138.Param{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param/missing", "")
	assertStatus(t, rec, http.StatusNotFound)
}

// TestTransport_GetParam_EmitsZeroValues verifies the param response keeps
// meaningful proto3 zero values that the default (omit-empty) marshaller drops:
// a constraint's min_value:0 and a current value of 0.
func TestTransport_GetParam_EmitsZeroValues(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.GetParamFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
		param := st2138.NewParamInt32(0).
			WithName(st2138.NewPolyglotText("en", "Zero")).
			WithConstraint(st2138.NewConstraintInt32Range(0, 100, 1))
		return *param, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param/zero", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	assertBodyContains(t, rec, `"oid":"zero"`)
	assertBodyContains(t, rec, `"min_value":0`)
	assertBodyContains(t, rec, `"int32_value":0`)
}

// TestTransport_GetParam_EmitsEmptyStringValue verifies that a current
// value of "" survives the empty-strip pass (it is detached and reattached).
func TestTransport_GetParam_EmitsEmptyStringValue(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.GetParamFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
		param := st2138.NewParamString("").
			WithName(st2138.NewPolyglotText("en", "Empty"))
		return *param, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param/empty", "")
	assertStatus(t, rec, http.StatusOK)
	assertBodyContains(t, rec, `"string_value":""`)
}

func TestTransport_SetValue_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.SetValueFn = func(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		handlerCalled = true
		if len(entries) != 1 {
			t.Fatalf("expected 1 entry, got %d", len(entries))
		}
		if v, ok := entries[0].Value.(int32); !ok || v != 42 {
			t.Errorf("expected value int32(42), got %v (%T)", entries[0].Value, entries[0].Value)
		}
		if entries[0].Fqoid != "brightness" {
			t.Errorf("expected fqoid 'brightness', got %s", entries[0].Fqoid)
		}
		return catena.StatusResult{Code: catena.StatusCodeOk}
	}

	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/value/brightness", `{"int32_value": 42}`)
	assertStatus(t, rec, http.StatusNoContent)
	if !handlerCalled {
		t.Error("registered handler was not called")
	}
}

func TestTransport_SetValue_InvalidContentType(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.SetValueFn = func(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		handlerCalled = true
		return catena.StatusResult{Code: catena.StatusCodeOk}
	}

	rec := makeRequestWithHeaders(t, transport, http.MethodPut, "/st2138-api/v1/0/value/brightness",
		`{"int32_value": 42}`, map[string]string{"Content-Type": "text/plain"})
	assertStatus(t, rec, http.StatusBadRequest)
	errMsg := assertHasError(t, rec)
	if errMsg != "invalid request body" {
		t.Errorf("expected error 'invalid request body', got %s", errMsg)
	}
	if handlerCalled {
		t.Error("registered handler should not have been called")
	}
}

func TestTransport_SetValues_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	var got []catena.SetValueEntry
	runtime.SetValueFn = func(slot uint16, values []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		got = values
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		return catena.StatusResult{Code: catena.StatusCodeOk}
	}

	body := `{"values":[{"oid":"ipv4","value":{"string_value":"192.168.1.1"}},{"oid":"brightness","value":{"int32_value":42}}]}`
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/values", body)
	assertStatus(t, rec, http.StatusNoContent)

	if len(got) != 2 {
		t.Fatalf("expected 2 entries, got %d", len(got))
	}
	if got[0].Fqoid != "ipv4" {
		t.Errorf("expected entry[0] fqoid 'ipv4', got %s", got[0].Fqoid)
	}
	if v, ok := got[0].Value.(string); !ok || v != "192.168.1.1" {
		t.Errorf("expected entry[0] value '192.168.1.1', got %v (%T)", got[0].Value, got[0].Value)
	}
	if got[1].Fqoid != "brightness" {
		t.Errorf("expected entry[1] fqoid 'brightness', got %s", got[1].Fqoid)
	}
	if v, ok := got[1].Value.(int32); !ok || v != 42 {
		t.Errorf("expected entry[1] value int32(42), got %v (%T)", got[1].Value, got[1].Value)
	}
}

func TestTransport_SetValues_DeliversAllEntries(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	callCount := 0
	var got []catena.SetValueEntry
	runtime.SetValueFn = func(slot uint16, values []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		callCount++
		got = values
		return catena.StatusResult{Code: catena.StatusCodeOk}
	}

	body := `{"values":[{"oid":"a","value":{"int32_value":1}},{"oid":"b","value":{"int32_value":2}}]}`
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/values", body)
	assertStatus(t, rec, http.StatusNoContent)
	if callCount != 1 {
		t.Errorf("expected handler called once, got %d", callCount)
	}
	if len(got) != 2 {
		t.Errorf("expected 2 entries delivered to handler, got %d", len(got))
	}
}

func TestTransport_SetValues_InvalidContentType(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.SetValueFn = func(slot uint16, values []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		handlerCalled = true
		return catena.StatusResult{Code: catena.StatusCodeOk}
	}

	rec := makeRequestWithHeaders(t, transport, http.MethodPut, "/st2138-api/v1/0/values",
		`{"values":[]}`, map[string]string{"Content-Type": "text/plain"})
	assertStatus(t, rec, http.StatusBadRequest)
	if errMsg := assertHasError(t, rec); errMsg != "invalid request body" {
		t.Errorf("expected error 'invalid request body', got %s", errMsg)
	}
	if handlerCalled {
		t.Error("handler should not have been called")
	}
}

func TestTransport_SetValues_MalformedBody(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/values", `{"values": not-json}`)
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestTransport_SetValues_FromProtoError(t *testing.T) {
	transport, _ := makeTestTransport(t)
	body := `{"values":[{"oid":"param","value":{"struct_variant_value":{"variant_name":"test"}}}]}`
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/values", body)
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestTransport_SetValues_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/values", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
}

func TestTransport_SetValues_HandlerError(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.SetValueFn = func(slot uint16, values []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "not found")
	}

	body := `{"values":[{"oid":"a","value":{"int32_value":1}}]}`
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/values", body)
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_ReadAsset_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	dp := st2138.DataPayload{
		Metadata: map[string]string{"content-type": "image/png"},
		Payload:  []byte("fake image"),
	}
	asset, _ := st2138.ToAsset(dp, true)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		if fqoid != "logo" {
			t.Errorf("expected fqoid 'logo', got %s", fqoid)
		}
		return catena.Reply(asset)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/asset/logo", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestTransport_Asset_MethodNotAllowed(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	getCalled := false
	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		getCalled = true
		return catena.Reply(st2138.Asset{})
	}

	// PATCH is not a supported asset method.
	rec := makeRequest(t, transport, http.MethodPatch, "/st2138-api/v1/0/asset/logo", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	if getCalled {
		t.Error("registered GET handler should not have been called for PATCH")
	}
}

const assetRequestBody = `{"cachable":true,"payload":{"payload_encoding":"UNCOMPRESSED","payload":"ZmFrZSBpbWFnZQ=="}}`

func TestTransport_CreateAsset_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	var gotFqoid string
	var gotPayload []byte
	runtime.CreateAssetFn = func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult {
		gotFqoid = fqoid
		dp, res := st2138.FromAsset(asset)
		if res != nil {
			t.Errorf("FromAsset error: %v", res)
		}
		gotPayload = dp.Payload
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/asset/logo", assetRequestBody)
	assertStatus(t, rec, http.StatusNoContent)
	if gotFqoid != "logo" {
		t.Errorf("expected fqoid 'logo', got %s", gotFqoid)
	}
	if string(gotPayload) != "fake image" {
		t.Errorf("expected payload 'fake image', got %q", string(gotPayload))
	}
}

func TestTransport_UpdateAsset_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	var gotFqoid string
	runtime.UpdateAssetFn = func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult {
		gotFqoid = fqoid
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/asset/logo", assetRequestBody)
	assertStatus(t, rec, http.StatusNoContent)
	if gotFqoid != "logo" {
		t.Errorf("expected fqoid 'logo', got %s", gotFqoid)
	}
}

func TestTransport_DeleteAsset_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	var gotFqoid string
	runtime.DeleteAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		gotFqoid = fqoid
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodDelete, "/st2138-api/v1/0/asset/logo", "")
	assertStatus(t, rec, http.StatusNoContent)
	if gotFqoid != "logo" {
		t.Errorf("expected fqoid 'logo', got %s", gotFqoid)
	}
}

func TestTransport_DeleteAsset_NotFound(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.DeleteAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found")
	}

	rec := makeRequest(t, transport, http.MethodDelete, "/st2138-api/v1/0/asset/missing", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_CreateAsset_InvalidBody(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.CreateAssetFn = func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult {
		handlerCalled = true
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/asset/logo", "{ not valid json")
	assertStatus(t, rec, http.StatusBadRequest)
	if handlerCalled {
		t.Error("handler should not be called with invalid body")
	}
}

func TestTransport_UpdateAsset_MissingContentType(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.UpdateAssetFn = func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult {
		handlerCalled = true
		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	// No Content-Type header set.
	rec := makeRequestWithHeaders(t, transport, http.MethodPut, "/st2138-api/v1/0/asset/logo", assetRequestBody, nil)
	assertStatus(t, rec, http.StatusBadRequest)
	if handlerCalled {
		t.Error("handler should not be called without Content-Type")
	}
}

func TestTransport_ExecuteCommand(t *testing.T) {
	t.Run("Route", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		handlerCalled := false
		runtime.CommandFn = func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			handlerCalled = true
			if commandFqoid != "reboot" {
				t.Errorf("expected commandFqoid 'reboot', got %s", commandFqoid)
			}
			return []st2138.CommandResponse{st2138.CommandNoResponse()}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/reboot", "")
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "application/json")
		response := parseJSONBody(t, rec)
		if _, ok := response["no_response"]; !ok {
			t.Errorf("expected no_response in CommandResponse, got %v", response)
		}
		if !handlerCalled {
			t.Error("registered handler was not called")
		}
	})

	t.Run("WithPayload", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		handlerCalled := false
		runtime.CommandFn = func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			handlerCalled = true
			if payload == nil {
				t.Error("expected payload to be non-nil")
			}
			val, _ := st2138.ToValue(payload)
			return []st2138.CommandResponse{st2138.CommandValue(val)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/process", `{"string_value": "test"}`)
		assertStatus(t, rec, http.StatusOK)
		response := parseJSONBody(t, rec)
		if _, ok := response["response"]; !ok {
			t.Errorf("expected response field in CommandResponse, got %v", response)
		}
		if !handlerCalled {
			t.Error("registered handler was not called")
		}
	})

	t.Run("MethodNotAllowed", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		handlerCalled := false
		runtime.CommandFn = func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			handlerCalled = true
			return []st2138.CommandResponse{st2138.CommandNoResponse()}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/command/reboot", "")
		assertStatus(t, rec, http.StatusMethodNotAllowed)
		if handlerCalled {
			t.Error("registered handler was not called")
		}
	})

	t.Run("NoCommand", func(t *testing.T) {
		transport, _ := makeTestTransport(t)
		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/", "")
		assertStatus(t, rec, http.StatusBadRequest)
		assertBodyContains(t, rec, "command FQOID is required")
	})

	// commands can't be nested in any way
	t.Run("NestedCommand", func(t *testing.T) {
		transport, _ := makeTestTransport(t)
		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/invalid/nested/command", "")
		assertStatus(t, rec, http.StatusNotFound)
		assertBodyContains(t, rec, "unknown command endpoint")
	})

	// the only valid string after /command/oid/ is 'stream'
	t.Run("NotStream", func(t *testing.T) {
		transport, _ := makeTestTransport(t)
		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/oid/not-stream", "")
		assertStatus(t, rec, http.StatusNotFound)
		assertBodyContains(t, rec, "unknown command endpoint")
	})

	t.Run("PayloadHandling", func(t *testing.T) {
		tests := []struct {
			name      string
			body      string
			expectNil bool
		}{
			{"no payload", "", true},
			{"with payload", `{"int32_value": 42}`, false},
		}

		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				transport, runtime := makeTestTransport(t)
				var receivedPayload any
				runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
					receivedPayload = payload
					return []st2138.CommandResponse{st2138.CommandNoResponse()}, catena.StatusWithCode(catena.StatusCodeOk, "")
				}

				makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/test", tt.body)

				if tt.expectNil && receivedPayload != nil {
					t.Errorf("expected nil payload, got %v", receivedPayload)
				} else if !tt.expectNil && receivedPayload == nil {
					t.Error("expected non-nil payload")
				}
			})
		}
	})

	t.Run("InvalidJSON", func(t *testing.T) {
		transport, _ := makeTestTransport(t)
		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/test", `{invalid json}`)
		assertStatus(t, rec, http.StatusBadRequest)
	})

	t.Run("FromProtoError", func(t *testing.T) {
		transport, _ := makeTestTransport(t)
		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/exec",
			`{"struct_variant_value": {"variant_name": "test"}}`)
		assertStatus(t, rec, http.StatusBadRequest)
	})

	t.Run("ResponseValue", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)
		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			val, _ := st2138.ToValue("command executed")
			return []st2138.CommandResponse{st2138.CommandValue(val)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run", "")
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "application/json")

		response := parseJSONBody(t, rec)
		resp, ok := response["response"]
		if !ok {
			t.Fatalf("expected 'response' field in CommandResponse, got %v", response)
		}
		respMap, ok := resp.(map[string]any)
		if !ok {
			t.Fatalf("expected response to be an object, got %T", resp)
		}
		if respMap["string_value"] != "command executed" {
			t.Errorf("expected string_value 'command executed', got %v", respMap["string_value"])
		}
	})

	t.Run("ExceptionResponse", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)
		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			return []st2138.CommandResponse{st2138.CommandException(
				"InvalidCommand",
				"Command not found: "+fqoid,
				st2138.NewPolyglotText("en", "Command not found"),
			)}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/bad_cmd", "")
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "application/json")

		response := parseJSONBody(t, rec)
		exc, ok := response["exception"]
		if !ok {
			t.Fatalf("expected 'exception' field in CommandResponse, got %v", response)
		}
		excMap, ok := exc.(map[string]any)
		if !ok {
			t.Fatalf("expected exception to be an object, got %T", exc)
		}
		if excMap["type"] != "InvalidCommand" {
			t.Errorf("expected exception type 'InvalidCommand', got %v", excMap["type"])
		}
		if excMap["details"] != "Command not found: bad_cmd" {
			t.Errorf("expected exception details, got %v", excMap["details"])
		}
		errMsg, ok := excMap["error_message"].(map[string]any)
		if !ok {
			t.Fatalf("expected error_message to be an object, got %T", excMap["error_message"])
		}
		displayStrings, ok := errMsg["display_strings"].(map[string]any)
		if !ok {
			t.Fatalf("expected display_strings in error_message, got %T", errMsg["display_strings"])
		}
		if displayStrings["en"] != "Command not found" {
			t.Errorf("expected en display string 'Command not found', got %v", displayStrings["en"])
		}
	})

	t.Run("HandlerError", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)
		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "command not found")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/missing", "")
		assertStatus(t, rec, http.StatusNotFound)
		if err := assertHasError(t, rec); !strings.Contains(err, "command not found") {
			t.Errorf("expected 'command not found' message, got %q", err)
		}
	})

	// The transport's only respond responsibility is parsing the flag from the
	// URL and passing it to the handler. Discarding responses when respond=false
	// is the server's job (see TestServer_InvokeExecuteCommandHandler), so these
	// cases only assert the value the handler received.
	t.Run("RespondFlagFromURL", func(t *testing.T) {
		cases := []struct {
			name  string
			query string
			want  bool
		}{
			{name: "absent defaults to false", query: "", want: false},
			{name: "respond=true", query: "?respond=true", want: true},
			{name: "respond=false", query: "?respond=false", want: false},
			{name: "other value stays false", query: "?respond=nope", want: false},
		}

		for _, tc := range cases {
			t.Run(tc.name, func(t *testing.T) {
				transport, runtime := makeTestTransport(t)
				handlerCalled := false
				runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
					handlerCalled = true
					if respond != tc.want {
						t.Errorf("handler received respond=%v, want %v", respond, tc.want)
					}
					return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
				}

				rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run"+tc.query, "")
				assertStatus(t, rec, http.StatusOK)
				if !handlerCalled {
					t.Error("registered handler was not called")
				}
			})
		}
	})

	t.Run("StreamRoute", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		gotRespond := false
		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			gotRespond = respond
			if fqoid != "run" {
				t.Errorf("expected commandFqoid 'run', got %s", fqoid)
			}
			v1, _ := st2138.ToValue(int32(1))
			v2, _ := st2138.ToValue(int32(2))
			return []st2138.CommandResponse{
				st2138.CommandValue(v1),
				st2138.CommandValue(v2),
			}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run/stream", "")
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "text/event-stream")

		if gotRespond {
			t.Error("expected respond=false by default for a stream request")
		}
		if dataCount := strings.Count(rec.Body.String(), "data:"); dataCount != 2 {
			t.Errorf("expected 2 SSE data events, got %d\nbody:\n%s", dataCount, rec.Body.String())
		}
	})

	// If the handler errors before sending anything, the stream has not committed
	// a status yet, so the transport can still report the error as an HTTP status.
	t.Run("StreamErrorNoneSent", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "command not found")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/missing/stream", "")
		assertStatus(t, rec, http.StatusNotFound)
		if err := assertHasError(t, rec); !strings.Contains(err, "command not found") {
			t.Errorf("expected 'command not found' message, got %q", err)
		}
	})

	// Exercise the case where the handler sends results but then returns error
	t.Run("StreamErrorAfterSomeSent", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			v1, _ := st2138.ToValue(int32(1))
			v2, _ := st2138.ToValue(int32(2))
			return []st2138.CommandResponse{
				st2138.CommandValue(v1),
				st2138.CommandValue(v2),
			}, catena.StatusWithCode(catena.StatusCodeInternal, "internal error")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run/stream", "")
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "text/event-stream")
		body := rec.Body.String()
		if dataCount := strings.Count(body, "data:"); dataCount != 3 {
			t.Errorf("expected 3 SSE data events, got %d\nbody:\n%s", dataCount, body)
		}
		if !strings.Contains(body, `"int32_value":1`) || !strings.Contains(body, `"int32_value":2`) {
			t.Errorf("expected SSE data events with int32 values, got %s", body)
		}
		if !strings.Contains(body, "event: error\n") {
			t.Errorf("expected an SSE error event, got body:\n%s", body)
		}
		if !strings.Contains(body, `"code":500`) {
			t.Errorf("expected the error event to carry status code 500, got body:\n%s", body)
		}
		if !strings.Contains(body, "internal error") {
			t.Errorf("expected the dev-mode error message in the error event, got body:\n%s", body)
		}
	})

	// A successful handler that streams nothing yields a well-formed empty event
	// stream (200), not an error.
	t.Run("StreamEmptyOk", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.CommandFn = func(slot uint16, fqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/command/run/stream", "")
		assertStatus(t, rec, http.StatusOK)
		if ct := rec.Header().Get("Content-Type"); ct != "text/event-stream" {
			t.Errorf("expected Content-Type 'text/event-stream', got %q", ct)
		}
		if body := rec.Body.String(); strings.Contains(body, "data:") {
			t.Errorf("expected no data events, got %q", body)
		}
	})
}

func TestTransport_Health_Route(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/health", "")
	assertStatus(t, rec, http.StatusOK)

	// assert empty body
	if rec.Body.Len() != 0 {
		t.Errorf("expected empty body, got %q", rec.Body.String())
	}
}

func TestTransport_Health_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/health", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
}

func TestTransport_Connect_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	connection := transporttest.MakeTestConnection(1)
	runtime.WithConnection(connection)

	// push an update to the connection
	connection.Updates <- &protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: []uint32{0, 1},
			},
		},
	}

	rec, cancel := setupSSEConnection(t, transport)
	cleanupSSE(cancel)

	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "text/event-stream")
	assertHeader(t, rec, "Cache-Control", "no-cache")

	body := rec.Body.String()
	if !strings.Contains(body, "data:") {
		t.Error("expected SSE data in response body")
	}
	if !strings.Contains(body, "slots_added") {
		t.Error("expected initial slots_added in response body")
	}
	if !strings.Contains(body, "\"slots\"") {
		t.Error("expected initial event in proto format with nested \"slots\" (SlotList)")
	}
	lines := strings.Split(body, "\n")
	for _, line := range lines {
		if strings.HasPrefix(line, "data: ") {
			var payload map[string]any
			if err := json.Unmarshal([]byte(strings.TrimPrefix(line, "data: ")), &payload); err != nil {
				t.Errorf("initial SSE data must be valid JSON: %v", err)
				break
			}
			slotsAdded, ok := payload["slots_added"].(map[string]any)
			if !ok {
				t.Error("expected slots_added object (SlotList) in initial event")
				break
			}
			slots, ok := slotsAdded["slots"].([]any)
			if !ok || len(slots) != 2 {
				t.Errorf("expected slots_added.slots to be array of 2 slots, got %T %v", slotsAdded["slots"], slotsAdded["slots"])
			}
			break
		}
	}
}

func TestTransport_Connect_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/connect", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	errMsg := assertHasError(t, rec)
	if !strings.Contains(errMsg, "GET") {
		t.Errorf("expected error to mention GET, got %s", errMsg)
	}
}

func TestTransport_GetPopulatedSlots_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.Slots = []uint16{0, 1, 5}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/devices", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	response := parseJSONBody(t, rec)
	slotsRaw, ok := response["slots"].([]any)
	if !ok {
		t.Fatalf("expected 'slots' field in JSON response, got %v", response)
	}

	// Verify the slots contain the expected values
	expectedSlots := []uint32{0, 1, 5}
	if len(slotsRaw) != len(expectedSlots) {
		t.Fatalf("expected %d slots, got %d", len(expectedSlots), len(slotsRaw))
	}

	for i, slotRaw := range slotsRaw {
		slot, ok := slotRaw.(float64) // JSON numbers are float64
		if !ok {
			t.Errorf("slot[%d]: expected number, got %T", i, slotRaw)
			continue
		}
		if uint32(slot) != expectedSlots[i] {
			t.Errorf("slot[%d]: expected %d, got %d", i, expectedSlots[i], uint32(slot))
		}
	}
}

func TestTransport_GetPopulatedSlots_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/devices", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
	errMsg := assertHasError(t, rec)
	if !strings.Contains(errMsg, "GET") {
		t.Errorf("expected error to mention GET, got %s", errMsg)
	}
}

func TestTransport_GetPopulatedSlots_EmptySlots(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/devices", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	response := parseJSONBody(t, rec)
	slotsRaw, ok := response["slots"].([]any)
	if !ok {
		t.Fatalf("expected 'slots' field in JSON response, got %v", response)
	}

	if len(slotsRaw) != 0 {
		t.Errorf("expected 0 slots, got %d", len(slotsRaw))
	}
}

func TestTransport_GetPopulatedSlots_RuntimeError(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.GetSlotsFn = func(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
		return nil, catena.StatusWithCode(catena.StatusCodePermissionDenied, "no slot access")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/devices", "")

	assertStatus(t, rec, http.StatusForbidden)
	errMsg := assertHasError(t, rec)
	if errMsg != "no slot access" && errMsg != "Forbidden" {
		t.Errorf("expected propagated access error, got %q", errMsg)
	}
}

func TestTransport_Fallback_Route(t *testing.T) {
	transport, _ := makeTestTransport(t)

	handlerCalled := false
	transport.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (st2138.Value, catena.StatusResult) {
		handlerCalled = true
		return catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "custom not found")
	})

	rec := makeRequest(t, transport, http.MethodGet, "/unknown/path", "")
	assertStatus(t, rec, http.StatusNotFound)
	if !handlerCalled {
		t.Error("registered handler should have been called, but was not")
	}
}

func TestTransport_NestedValuePath(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.GetValueFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Value, catena.StatusResult) {
		if fqoid != "nested/path/to/param" {
			t.Errorf("expected fqoid 'nested/path/to/param', got %s", fqoid)
		}
		value, _ := st2138.ToValue(int32(1))
		return catena.Reply(value)
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/value/nested/path/to/param", "")
	assertStatus(t, rec, http.StatusOK)
}

func TestTransport_UnknownEndpoint(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/unknown", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_InvalidPath(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/kjhgjnghf", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_InvalidPathNoSlash(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_NegativeSlot(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/-1", "")
	assertStatus(t, rec, http.StatusBadRequest)
}

func TestWriteHTTPResult_Error(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{
		Code:  catena.StatusCodeNotFound,
		Error: "test error message",
	}
	transport, _ := makeTestTransport(t)
	transport.writeHTTPResult(rec, result, st2138.Value{})

	errMsg := assertHasError(t, rec)
	if errMsg != "test error message" {
		t.Errorf("expected error message 'test error message', got %s", errMsg)
	}
}

func TestWriteHTTPStatusResult(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.StatusCodeOk}
	transport, _ := makeTestTransport(t)
	transport.writeHTTPStatusResult(rec, result)
	assertStatus(t, rec, http.StatusOK)
}

func TestWriteHTTPStatusResultNoBody_SuccessIs204(t *testing.T) {
	rec := httptest.NewRecorder()
	transport, _ := makeTestTransport(t)
	transport.writeHTTPStatusResultNoBody(rec, catena.StatusResult{Code: catena.StatusCodeOk})
	assertStatus(t, rec, http.StatusNoContent)
	if rec.Body.Len() != 0 {
		t.Errorf("expected empty body for 204, got %d bytes", rec.Body.Len())
	}
}

func TestWriteHTTPStatusResultNoBody_ErrorPreservesMapping(t *testing.T) {
	rec := httptest.NewRecorder()
	transport, _ := makeTestTransport(t)
	transport.writeHTTPStatusResultNoBody(rec, catena.StatusResult{
		Code:  catena.StatusCodeNotFound,
		Error: "missing",
	})
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_ErrorMessages_DevVsProd(t *testing.T) {
	tests := []struct {
		name     string
		isDev    bool
		expected string
	}{
		{"dev mode shows details", true, "detailed error"},
		{"prod mode hides details", false, "Not Found"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			transport, runtime := makeTestTransport(t)
			runtime.Dev = tt.isDev

			rec := httptest.NewRecorder()
			result := catena.StatusResult{Code: catena.StatusCodeNotFound, Error: "detailed error"}
			transport.writeHTTPResult(rec, result, st2138.Value{})

			assertBodyContains(t, rec, tt.expected)
		})
	}
}

func TestWriteFunctions_NilValues(t *testing.T) {
	tests := []struct {
		name           string
		fn             func(http.ResponseWriter)
		expectedStatus int
	}{
		{"nil value", func(w http.ResponseWriter) { writeValueResult(w, st2138.Value{}, http.StatusOK) }, http.StatusOK},
		{"nil device", func(w http.ResponseWriter) { writeDeviceResult(w, st2138.Device{}, http.StatusOK) }, http.StatusOK},
		{"nil asset", func(w http.ResponseWriter) { writeAssetResult(w, st2138.Asset{}, http.StatusOK) }, http.StatusInternalServerError},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			rec := httptest.NewRecorder()
			tt.fn(rec)
			assertStatus(t, rec, tt.expectedStatus)
		})
	}
}

func TestSetValue_FromProtoError(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/value/param",
		`{"struct_variant_value": {"variant_name": "test"}}`)
	if rec.Code == 0 {
		t.Error("expected a response code")
	}
}

func TestRouting_EdgeCases(t *testing.T) {
	transport, _ := makeTestTransport(t)

	tests := []struct {
		name string
		path string
	}{
		{"short path", "/st2138-api/"},
		{"invalid slot", "/st2138-api/v1/99999"},
		{"unknown endpoint", "/st2138-api/v1/0/unknown/test"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			rec := makeRequest(t, transport, http.MethodGet, tt.path, "")
			assertHasError(t, rec)
		})
	}
}

func TestValueEndpoint_Methods(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.SetValueFn = func(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
		return catena.StatusResult{Code: catena.StatusCodeOk}
	}

	tests := []struct {
		name   string
		method string
		body   string
	}{
		{"PUT with value", http.MethodPut, `{"int32_value": 100}`},
		{"invalid method", http.MethodPost, ""},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			makeRequest(t, transport, tt.method, "/st2138-api/v1/0/value/test", tt.body)
		})
	}
}

func TestTransport_Connect_TooManyConnections(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.RegisterTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
		return nil, catena.StatusResult{Code: catena.StatusCodeResourceExhausted, Error: "connection rejected"}
	}

	_, cleanup1 := setupSSEConnection(t, transport)

	rec2 := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/connect", "")
	assertStatus(t, rec2, http.StatusTooManyRequests)
	errMsg := assertHasError(t, rec2)
	if errMsg != "connection rejected" && errMsg != "Too Many Requests" {
		t.Errorf("expected error \"connection rejected\" (dev) or \"Too Many Requests\" (prod), got %q", errMsg)
	}

	cleanup1()
}

func TestWriteResults_ValidData(t *testing.T) {
	device := *st2138.NewDevice(0)
	rec := httptest.NewRecorder()
	writeDeviceResult(rec, device, http.StatusOK)
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	asset, _ := st2138.ToAsset(st2138.DataPayload{Payload: []byte("data")}, false)
	rec = httptest.NewRecorder()
	writeAssetResult(rec, asset, http.StatusOK)
	assertStatus(t, rec, http.StatusOK)
}

func TestDeviceEndpoint_WrongMethod(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0", "")
	assertHasError(t, rec)
}

func TestWriteHTTPStatusResult_WithError(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.StatusCodeInternal, Error: "error"}
	transport, _ := makeTestTransport(t)
	transport.writeHTTPStatusResult(rec, result)
	assertBodyContains(t, rec, "error")
}

func TestTransport_sendSSEEvent(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := httptest.NewRecorder()
	var w http.ResponseWriter = rec
	flusher := w.(http.Flusher)
	update := &protos.PushUpdates{
		Slot: 1,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}

	err := transport.sendSSEEvent(rec, flusher, update)
	if err != nil {
		t.Fatalf("sendSSEEvent: %v", err)
	}
	body := rec.Body.String()
	if !strings.HasPrefix(body, "data: ") {
		t.Errorf("expected body to start with 'data: ', got %q", body)
	}
	var decoded map[string]any
	if err := json.Unmarshal([]byte(strings.TrimPrefix(strings.Split(body, "\n")[0], "data: ")), &decoded); err != nil {
		t.Fatalf("SSE data not valid JSON: %v", err)
	}
	if decoded["slot"] != float64(1) {
		t.Errorf("expected slot=1, got %v", decoded["slot"])
	}
	valueObj, ok := decoded["value"].(map[string]any)
	if !ok {
		t.Fatal("expected nested 'value' object in SSE payload")
	}
	if valueObj["oid"] != "test/param" {
		t.Errorf("expected oid=test/param, got %v", valueObj["oid"])
	}
}

func TestTransport_Start(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.ShutdownTransportConnsFn = func(ctx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}
	listener, err := net.Listen("tcp", ":0")
	if err != nil {
		t.Fatalf("net.Listen: %v", err)
	}
	port := listener.Addr().(*net.TCPAddr).Port
	listener.Close()

	transport.port = port
	err = transport.Start(context.Background(), runtime)
	if err != nil {
		t.Errorf("Start: %v", err)
	}
	time.Sleep(200 * time.Millisecond)

	url := fmt.Sprintf("http://127.0.0.1:%d/st2138-api/v1", port)
	resp, err := http.Get(url)
	if err != nil {
		t.Fatalf("GET: %v", err)
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, resp.StatusCode)
	}
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	transport.Shutdown(ctx)
}

func TestTransport_Start_BindError(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	// Occupy a port so the transport's synchronous bind fails with
	// "address already in use".
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("net.Listen: %v", err)
	}
	defer listener.Close()
	port := listener.Addr().(*net.TCPAddr).Port

	transport.port = port
	err = transport.Start(context.Background(), runtime)
	if err == nil {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		transport.Shutdown(ctx)
		t.Fatal("expected Start to return a bind error, got nil")
	}
}

func TestTransport_Shutdown_NotStarted(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.ShutdownTransportConnsFn = func(ctx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	err := transport.Shutdown(ctx)
	if err != nil {
		t.Errorf("Shutdown not started: %v", err)
	}
}

func TestTransport_Shutdown_Deadline(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.ShutdownTransportConnsFn = func(ctx context.Context, gotTransport catena.Transport) {
		if gotTransport != transport {
			t.Errorf("expected transport %v, got %v", transport, gotTransport)
		}
	}

	// Use a dedicated server with a handler that blocks to ensure Shutdown has
	// active in-flight work and must wait until the context deadline.
	// The handler waits on request context cancellation so test cleanup relies on
	// server shutdown/close behavior, not a manual release channel.
	handlerStarted := make(chan struct{})
	transport.server = &http.Server{Handler: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		close(handlerStarted)
		<-r.Context().Done()
		w.WriteHeader(http.StatusOK)
	})}

	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("net.Listen: %v", err)
	}
	defer listener.Close()

	// start server in background
	serveDone := make(chan struct{})
	go func() {
		defer close(serveDone)
		_ = transport.server.Serve(listener)
	}()

	// Ensure one request is in-flight before shutdown.
	requestDone := make(chan struct{})
	go func() {
		defer close(requestDone)
		_, _ = http.Get("http://" + listener.Addr().String())
	}()

	select {
	case <-handlerStarted:
	case <-time.After(time.Second):
		t.Fatal("handler did not start in time")
	}

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Millisecond)
	defer cancel()

	err = transport.Shutdown(ctx)
	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("expected DeadlineExceeded error, got %v", err)
	}

	// request should be released by shutdown/force-close behavior
	// via request context cancellation.
	select {
	case <-requestDone:
	case <-time.After(time.Second):
		t.Error("expected request to be released after context deadline, but it was not")
	}

	// make sure server stops and goroutine exits
	select {
	case <-serveDone:
	case <-time.After(time.Second):
		t.Error("expected server to shut down after context deadline, but it did not")
	}
}

func TestWriteHTTPResult_DefaultType(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.StatusCodeOk}
	transport, _ := makeTestTransport(t)
	transport.writeHTTPResult(rec, result, nil)
	assertStatus(t, rec, http.StatusOK)
}

func TestTransport_ReadAsset_CompressionQueryParam_Gzip(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	dp := st2138.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test asset data for compression"),
	}
	asset, _ := st2138.ToAsset(dp, true)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		if fqoid != "test.txt" {
			t.Errorf("expected fqoid 'test.txt', got %s", fqoid)
		}
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=GZIP", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)

	var result map[string]any
	if err := json.Unmarshal(rec.Body.Bytes(), &result); err != nil {
		t.Fatalf("invalid JSON response: %v", err)
	}
	payload, ok := result["payload"].(map[string]any)
	if !ok {
		t.Fatal("expected payload field in response")
	}
	if payload["payload_encoding"] != "GZIP" {
		t.Errorf("expected payload_encoding GZIP, got %v", payload["payload_encoding"])
	}
}

func TestTransport_ReadAsset_CompressionQueryParam_Deflate(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	dp := st2138.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test asset data for compression"),
	}
	asset, _ := st2138.ToAsset(dp, true)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		if fqoid != "test.txt" {
			t.Errorf("expected fqoid 'test.txt', got %s", fqoid)
		}
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=DEFLATE", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)

	var result map[string]any
	if err := json.Unmarshal(rec.Body.Bytes(), &result); err != nil {
		t.Fatalf("invalid JSON response: %v", err)
	}
	payload, ok := result["payload"].(map[string]any)
	if !ok {
		t.Fatal("expected payload field in response")
	}
	if payload["payload_encoding"] != "DEFLATE" {
		t.Errorf("expected payload_encoding DEFLATE, got %v", payload["payload_encoding"])
	}
}

func TestTransport_ReadAsset_CompressionQueryParam_Uncompressed(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	original := []byte("test asset data")
	gzData, _ := st2138.CompressGzip(original)
	dp := st2138.DataPayload{
		Metadata:        map[string]string{"content-type": "text/plain"},
		Payload:         gzData,
		PayloadEncoding: st2138.EncodingGzip,
	}
	asset, _ := st2138.ToAsset(dp, true)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		if fqoid != "test.txt" {
			t.Errorf("expected fqoid 'test.txt', got %s", fqoid)
		}
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=UNCOMPRESSED", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)
	assertStatus(t, rec, http.StatusOK)
}

func TestTransport_Connect_StreamingNotSupported(t *testing.T) {
	transport, _ := makeTestTransport(t)
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)
	rec := httptest.NewRecorder()
	w := &noFlusher{ResponseWriter: rec}

	transport.handleConnect(w, req)

	assertStatus(t, rec, http.StatusInternalServerError)
	errMsg := assertHasError(t, rec)
	if !strings.Contains(errMsg, "streaming") && !strings.Contains(errMsg, "Internal") {
		t.Errorf("expected error to mention streaming or Internal, got %s", errMsg)
	}
}

func TestWriteHTTPStatusResult_ProdMode(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.StatusCodeNotFound, Error: "detailed internal error"}
	transport, runtime := makeTestTransport(t)
	runtime.Dev = false
	transport.writeHTTPStatusResult(rec, result)

	assertBodyContains(t, rec, "Not Found")
	assertBodyNotContains(t, rec, "detailed internal error")
}

func TestTransport_Connect_WithOrigin(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	runtime.WithConnection(transporttest.MakeTestConnection(1))

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil).WithContext(ctx)
	req.Header.Set("Origin", "https://example.com")
	rec := httptest.NewRecorder()

	go transport.mux.ServeHTTP(rec, req)
	time.Sleep(100 * time.Millisecond)
	cancel()
	time.Sleep(50 * time.Millisecond)

	assertHeader(t, rec, "Access-Control-Allow-Origin", "https://example.com")
}

func TestWriteValueResult_WriteError(t *testing.T) {
	value, _ := st2138.ToValue(int32(1))
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeValueResult(w, value, http.StatusOK)

	if rec.Code != http.StatusInternalServerError && rec.Code != http.StatusOK {
		t.Errorf("expected status 500 or 200 (if header already sent), got %d", rec.Code)
	}
}

func TestWriteDeviceResult_WriteError(t *testing.T) {
	device := *st2138.NewDevice(0)
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeDeviceResult(w, device, http.StatusOK)

	assertStatus(t, rec, http.StatusOK)
}

func TestWriteAssetResult_WriteError(t *testing.T) {
	asset, _ := st2138.ToAsset(st2138.DataPayload{Payload: []byte("x")}, false)
	rec := httptest.NewRecorder()
	w := &failWriter{ResponseWriter: rec, failOnWrite: true}

	writeAssetResult(w, asset, http.StatusOK)

	assertStatus(t, rec, http.StatusOK)
}

func TestWriteHTTPResult_WithError_NonDev(t *testing.T) {
	rec := httptest.NewRecorder()
	result := catena.StatusResult{Code: catena.StatusCodeNotFound, Error: "internal detail"}
	transport, runtime := makeTestTransport(t)
	runtime.Dev = false
	transport.writeHTTPResult(rec, result, st2138.Value{})

	assertBodyNotContains(t, rec, "internal detail")
}

func TestSendSSEEvent_MarshalError(t *testing.T) {
	origMarshal := marshalSSEFunc
	defer func() { marshalSSEFunc = origMarshal }()
	marshalSSEFunc = func(msg proto.Message) ([]byte, error) {
		return nil, fmt.Errorf("marshal failed")
	}

	transport, _ := makeTestTransport(t)
	rec := httptest.NewRecorder()
	var w http.ResponseWriter = rec
	flusher := w.(http.Flusher)
	update := &protos.PushUpdates{Slot: 0}

	err := transport.sendSSEEvent(rec, flusher, update)
	if err == nil || err.Error() != "marshal failed" {
		t.Errorf("expected 'marshal failed' error, got %v", err)
	}
}

func TestSendSSEEvent_WriteFailure(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := httptest.NewRecorder()
	w := &failFlusherWriter{ResponseRecorder: rec, failAfterN: 0}
	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 1}},
			},
		},
	}

	err := transport.sendSSEEvent(w, w, update)
	if err == nil {
		t.Error("expected error when writer fails")
	}
}

func TestTransport_Connect_PushUpdates(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	connection := transporttest.MakeTestConnection(1)
	runtime.WithConnection(connection)

	rec, cancel := setupSSEConnection(t, transport)

	connection.Updates <- &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "brightness",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}

	time.Sleep(100 * time.Millisecond)
	cleanupSSE(cancel)

	sseBody := rec.Body.String()
	if !strings.Contains(sseBody, "brightness") {
		t.Errorf("expected SSE body to contain 'brightness' from SetValue notification, got %s", sseBody)
	}
}

func TestTransport_Connect_ServerShutdown(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	connection := transporttest.MakeTestConnection(1)
	runtime.WithConnection(connection)

	_, cancel := setupSSEConnection(t, transport)

	connection.Done <- struct{}{}
	cancel()

	if runtime.RegisterCalls != runtime.DeregisterCalls {
		t.Errorf("expected deregister calls to match register calls after server shutdown, got %d deregister calls and %d register calls", runtime.DeregisterCalls, runtime.RegisterCalls)
	}
}

func TestHandleConnect_UpdateEventWriteFailure(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	rec := httptest.NewRecorder()
	w := &failFlusherWriter{ResponseRecorder: rec, failAfterN: 1}
	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/connect", nil)

	connection := transporttest.MakeTestConnection(1)
	runtime.WithConnection(connection)

	done := make(chan struct{})
	go func() {
		transport.handleConnect(w, req)
		close(done)
	}()
	time.Sleep(100 * time.Millisecond)

	// push an update multiple times to trigger the write failure in failFlusherWriter
	update := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   "test/param",
				Value: &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: 42}},
			},
		},
	}
	connection.Updates <- update
	connection.Updates <- update

	select {
	case <-done:
	case <-time.After(2 * time.Second):
		t.Fatal("handler did not exit after update write failure")
	}

	if runtime.RegisterCalls != runtime.DeregisterCalls {
		t.Errorf("expected deregister calls to match register calls after update event failure, got %d deregister calls and %d register calls", runtime.DeregisterCalls, runtime.RegisterCalls)
	}
}

func TestRouting_BasePathOnly(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/", "")
	assertStatus(t, rec, http.StatusBadRequest)
	assertHasError(t, rec)
}

func TestTransport_ReadAsset_CompressionQueryParam_Invalid(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	dp := st2138.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("test data"),
	}
	asset, _ := st2138.ToAsset(dp, true)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt?compression=INVALID", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusBadRequest {
		t.Errorf("expected status %d, got %d", http.StatusBadRequest, rec.Code)
	}
}

func TestTransport_ReadAsset_NoCompressionParam(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	dp := st2138.DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  []byte("uncompressed data"),
	}
	asset, _ := st2138.ToAsset(dp, true)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		return catena.Reply(asset)
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/test.txt", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusOK {
		t.Errorf("expected status %d, got %d", http.StatusOK, rec.Code)
	}
}

func TestTransport_ReadAsset_CompressionWithError(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.ReadAssetFn = func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
		return catena.ReplyError[st2138.Asset](catena.StatusCodeNotFound, "asset not found")
	}

	req := httptest.NewRequest(http.MethodGet, "/st2138-api/v1/0/asset/missing?compression=GZIP", nil)
	rec := httptest.NewRecorder()
	transport.mux.ServeHTTP(rec, req)

	if rec.Code != http.StatusNotFound {
		t.Errorf("expected status %d, got %d", http.StatusNotFound, rec.Code)
	}
}

// =============================================================================
// Test: ParamInfo endpoint
// =============================================================================
func TestTransport_ParamInfo(t *testing.T) {
	t.Run("unary route", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		handlerCalled := false
		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			handlerCalled = true
			// Mirroring the C++ controller, REST builds the fqoid by prepending "/"
			// to each path segment after the endpoint.
			if oidPrefix != "text_box" {
				t.Errorf("expected oidPrefix 'text_box', got %s", oidPrefix)
			}
			if recursive {
				t.Error("expected recursive=false for unary call")
			}
			return []st2138.ParamInfo{
				st2138.NewParamInfo("text_box", st2138.NewPolyglotText("en", "Text Box"), st2138.ParamTypeString, "", 0),
			}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/text_box", "")

		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "application/json")
		if !handlerCalled {
			t.Error("registered handler was not called")
		}

		response := parseJSONBody(t, rec)
		info, ok := response["info"].(map[string]any)
		if !ok {
			t.Fatalf("expected 'info' object in response, got %v", response)
		}
		if info["oid"] != "text_box" {
			t.Errorf("expected info.oid='text_box', got %v", info["oid"])
		}
		if info["type"] != "STRING" {
			t.Errorf("expected info.type='STRING', got %v", info["type"])
		}
	})

	t.Run("Nested fqoid", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		receivedOidPrefix := ""
		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			receivedOidPrefix = oidPrefix
			return []st2138.ParamInfo{
				st2138.NewParamInfo(oidPrefix, nil, st2138.ParamTypeInt32, "", 0),
			}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/parent/child", "")
		assertStatus(t, rec, http.StatusOK)
		if receivedOidPrefix != "parent/child" {
			t.Errorf("expected oidPrefix 'parent/child', got %s", receivedOidPrefix)
		}
	})

	t.Run("NotFound", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "Parameter not found: missing")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/missing", "")
		assertStatus(t, rec, http.StatusNotFound)
		if err := assertHasError(t, rec); !strings.Contains(err, "Parameter not found") {
			t.Errorf("expected 'Parameter not found' message, got %q", err)
		}
	})

	// TestTransport_ParamInfo_UnaryEmptyOk verifies that a unary handler which
	// reports success but produces no parameter is treated as an internal contract
	// violation, not a fabricated NotFound.
	t.Run("UnaryEmptyOk", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/missing", "")
		assertStatus(t, rec, http.StatusInternalServerError)
	})

	t.Run("HandlerError", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/text_box", "")
		assertStatus(t, rec, http.StatusNotFound)
	})

	t.Run("MethodNotAllowed", func(t *testing.T) {
		transport, _ := makeTestTransport(t)

		rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/param-info/text_box", "")
		assertStatus(t, rec, http.StatusMethodNotAllowed)
	})

	// TestTransport_ParamInfo_UnaryRecursiveRejected verifies the C++ rule that
	// recursive cannot be combined with a unary response.
	t.Run("UnaryRecursiveRejected", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		handlerCalled := false
		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			handlerCalled = true
			return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/text_box?recursive=true", "")
		assertStatus(t, rec, http.StatusBadRequest)
		if err := assertHasError(t, rec); !strings.Contains(err, "Recursive") {
			t.Errorf("expected 'Recursive ...' error message, got %q", err)
		}
		if handlerCalled {
			t.Error("handler should not be called when validation fails")
		}
	})

	// TestTransport_ParamInfo_UnaryMissingFqoidRejected verifies the C++ rule that
	// a unary request must include an fqoid.
	t.Run("UnaryMissingFqoidRejected", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)
		handlerCalled := false
		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			handlerCalled = true
			return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info", "")
		assertStatus(t, rec, http.StatusBadRequest)
		if err := assertHasError(t, rec); !strings.Contains(err, "Unary request must include fqoid") {
			t.Errorf("expected 'Unary request must include fqoid' error, got %q", err)
		}
		if handlerCalled {
			t.Error("handler should not be called when validation fails")
		}
	})

	// TestTransport_ParamInfo_RecursivePresenceOnly verifies the C++ semantics where
	// the presence of the `recursive` query parameter enables recursion regardless
	// of its value (so ?recursive=false STILL enables recursion).
	t.Run("RecursivePresenceOnly", func(t *testing.T) {
		cases := []struct {
			name  string
			query string
			want  bool
		}{
			{name: "no flag", query: "", want: false},
			{name: "recursive=true", query: "?recursive=true", want: true},
			{name: "recursive=false still enables", query: "?recursive=false", want: true},
			{name: "recursive with no value", query: "?recursive", want: true},
		}

		for _, tc := range cases {
			tc := tc
			t.Run(tc.name, func(t *testing.T) {
				transport, runtime := makeTestTransport(t)
				var gotRecursive bool
				runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
					gotRecursive = recursive
					return []st2138.ParamInfo{
						st2138.NewParamInfo("a", nil, st2138.ParamTypeInt32, "", 0),
					}, catena.StatusWithCode(catena.StatusCodeOk, "")
				}

				rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/stream"+tc.query, "")
				assertStatus(t, rec, http.StatusOK)
				if gotRecursive != tc.want {
					t.Errorf("recursive: got %v, want %v", gotRecursive, tc.want)
				}
			})
		}
	})
	t.Run("StreamRoute", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			if oidPrefix != "parent" {
				t.Errorf("expected oidPrefix 'parent', got %s", oidPrefix)
			}
			if !recursive {
				t.Error("expected recursive=true")
			}
			return []st2138.ParamInfo{
				st2138.NewParamInfo("parent", nil, st2138.ParamTypeStruct, "", 0),
				st2138.NewParamInfo("parent/child1", nil, st2138.ParamTypeInt32, "", 0),
				st2138.NewParamInfo("parent/child2", nil, st2138.ParamTypeStringArray, "", 3),
			}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/parent/stream?recursive=true", "")

		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "text/event-stream")

		body := rec.Body.String()
		dataCount := strings.Count(body, "data:")
		if dataCount != 3 {
			t.Errorf("expected 3 SSE data events, got %d\nbody:\n%s", dataCount, body)
		}
		if !strings.Contains(body, `"oid":"parent/child2"`) {
			t.Errorf("expected child2 entry in stream, got body:\n%s", body)
		}
	})

	t.Run("TopLevelStreamRoute", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			if oidPrefix != "" {
				t.Errorf("expected empty oidPrefix for top-level stream, got %q", oidPrefix)
			}
			return []st2138.ParamInfo{
				st2138.NewParamInfo("a", nil, st2138.ParamTypeInt32, "", 0),
				st2138.NewParamInfo("b", nil, st2138.ParamTypeFloat32, "", 0),
			}, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/stream", "")
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "text/event-stream")

		body := rec.Body.String()
		if strings.Count(body, "data:") != 2 {
			t.Errorf("expected 2 SSE data events for top-level stream, got body:\n%s", body)
		}
	})

	// TestTransport_ParamInfo_TopLevelStream_EmptyOk verifies that an empty
	// top-level result is a well-formed empty event stream (200), not a fabricated
	// NotFound. Handlers own NotFound for genuinely missing oids.
	t.Run("TopLevelStream_EmptyOk", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/stream", "")
		assertStatus(t, rec, http.StatusOK)
		if ct := rec.Header().Get("Content-Type"); ct != "text/event-stream" {
			t.Errorf("expected Content-Type 'text/event-stream', got %q", ct)
		}
		if body := rec.Body.String(); strings.Contains(body, "data:") {
			t.Errorf("expected no data events, got %q", body)
		}
	})

	// If BL just returns an error right away that can be returned as a http code
	t.Run("StreamErrorNoneSent", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/missing/stream", "")
		assertStatus(t, rec, http.StatusNotFound)
		if err := assertHasError(t, rec); !strings.Contains(err, "param not found") {
			t.Errorf("expected 'param not found' message, got %q", err)
		}
	})

	// hit the branch where BL streams some params, locking in the 200, then the handler returns an error.
	// The transport can no longer change the HTTP status, so it reports the failure in-band as an SSE
	// "error" event carrying the status code (and, in dev mode, the detailed message).
	t.Run("StreamErrorAfterSomeSent", func(t *testing.T) {
		transport, runtime := makeTestTransport(t)

		runtime.ParamInfoFn = func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult) {
			return []st2138.ParamInfo{
				st2138.NewParamInfo("parent/child1", nil, st2138.ParamTypeInt32, "", 0),
			}, catena.StatusWithCode(catena.StatusCodeInternal, "boom")
		}

		rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/param-info/parent/stream?recursive=true", "")

		// The first chunk committed the SSE 200, so the late error is reported as
		// an SSE "error" event rather than by rewriting the HTTP status.
		assertStatus(t, rec, http.StatusOK)
		assertContentType(t, rec, "text/event-stream")

		body := rec.Body.String()
		if dataCount := strings.Count(body, "data:"); dataCount != 2 {
			t.Errorf("expected the sent chunk plus the error event, got %d data events\nbody:\n%s", dataCount, body)
		}
		if !strings.Contains(body, `"oid":"parent/child1"`) {
			t.Errorf("expected the sent chunk in the stream, got body:\n%s", body)
		}
		if !strings.Contains(body, "event: error\n") {
			t.Errorf("expected an SSE error event, got body:\n%s", body)
		}
		if !strings.Contains(body, `"code":500`) {
			t.Errorf("expected the error event to carry status code 500, got body:\n%s", body)
		}
		if !strings.Contains(body, "boom") {
			t.Errorf("expected the dev-mode error message in the error event, got body:\n%s", body)
		}
	})
}

// =============================================================================
// Test: Languages endpoint
// =============================================================================

func TestTransport_Languages_Route(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	handlerCalled := false
	runtime.ListLanguagesFn = func(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult) {
		handlerCalled = true
		if slot != 0 {
			t.Errorf("expected slot 0, got %d", slot)
		}
		return []string{"en", "fr", "es", "de"}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/languages", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")
	if !handlerCalled {
		t.Error("registered handler was not called")
	}

	var response []string
	if err := json.Unmarshal(rec.Body.Bytes(), &response); err != nil {
		t.Fatalf("invalid JSON response: %v", err)
	}
	expected := []string{"en", "fr", "es", "de"}
	if len(response) != len(expected) {
		t.Fatalf("expected %d languages, got %d", len(expected), len(response))
	}
	for i, lang := range expected {
		if response[i] != lang {
			t.Errorf("language[%d]: expected %q, got %q", i, lang, response[i])
		}
	}
}

func TestTransport_Languages_EmptyList(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.ListLanguagesFn = func(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult) {
		return nil, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/languages", "")
	assertStatus(t, rec, http.StatusOK)
	assertContentType(t, rec, "application/json")

	var response []string
	if err := json.Unmarshal(rec.Body.Bytes(), &response); err != nil {
		t.Fatalf("invalid JSON response: %v", err)
	}
	if len(response) != 0 {
		t.Errorf("expected empty languages list, got %v", response)
	}
	if !strings.HasPrefix(strings.TrimSpace(rec.Body.String()), "[") {
		t.Errorf("expected bare JSON array, got %q", rec.Body.String())
	}
}

func TestTransport_Languages_HandlerError(t *testing.T) {
	transport, runtime := makeTestTransport(t)

	runtime.ListLanguagesFn = func(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult) {
		return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "no languages handler registered")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/languages", "")
	assertStatus(t, rec, http.StatusNotFound)
}

func TestTransport_Languages_MethodNotAllowed(t *testing.T) {
	transport, _ := makeTestTransport(t)
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/languages", "")
	assertStatus(t, rec, http.StatusMethodNotAllowed)
}
func TestTransport_LanguagePackGetSuccess(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.LanguagePackFn = func(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult) {
		if slot != 0 {
			t.Fatalf("expected slot 0, got %d", slot)
		}
		if language != "es" {
			t.Fatalf("expected language es, got %s", language)
		}

		return catena.NewLanguagePack().
			WithName("Spanish").
			WithWords(map[string]string{
				"greeting": "Hola",
				"parting":  "Adiós",
			}), catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/language-pack/es", "")

	if rec.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", rec.Code, rec.Body.String())
	}
	if !strings.Contains(rec.Body.String(), "Spanish") {
		t.Fatalf("expected response body to contain Spanish, got %s", rec.Body.String())
	}
}

func TestTransport_LanguagePackGetError(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.LanguagePackFn = func(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult) {
		return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeNotFound, "language pack not found")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/language-pack/de", "")

	if rec.Code != http.StatusNotFound {
		t.Fatalf("expected status 404, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackGetNilProto(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	// Handler reports success but returns an empty (nil-proto) pack. This is a
	// handler contract violation and must surface as 500, not overwrite the OK
	// result with a 404.
	runtime.LanguagePackFn = func(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult) {
		return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/language-pack/es", "")

	if rec.Code != http.StatusInternalServerError {
		t.Fatalf("expected status 500, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackPostSuccess(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.AddLanguageFn = func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
		if slot != 0 {
			t.Fatalf("expected slot 0, got %d", slot)
		}
		if language != "fr" {
			t.Fatalf("expected language fr, got %s", language)
		}

		if languagePack.Proto == nil || languagePack.GetName() != "French" {
			t.Fatalf("expected French language pack, got %#v", languagePack.Proto)
		}

		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	body := `{"name":"French","words":{"greeting":"Bonjour","parting":"Au revoir"}}`
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/language-pack/fr", body)

	if rec.Code != http.StatusNoContent {
		t.Fatalf("expected status 204, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackPostError(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.AddLanguageFn = func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
		return catena.StatusWithCode(catena.StatusCodePermissionDenied, "not allowed")
	}

	body := `{"name":"French"}`
	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/language-pack/fr", body)

	if rec.Code != http.StatusForbidden {
		t.Fatalf("expected status 403, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackPutSuccess(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.UpdateLanguageFn = func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
		if language != "fr" {
			t.Fatalf("expected language fr, got %s", language)
		}

		if languagePack.Proto == nil || languagePack.GetName() != "French Updated" {
			t.Fatalf("expected updated French language pack, got %#v", languagePack.Proto)
		}

		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	body := `{"name":"French Updated","words":{"greeting":"Salut"}}`
	rec := makeRequest(t, transport, http.MethodPut, "/st2138-api/v1/0/language-pack/fr", body)

	if rec.Code != http.StatusNoContent {
		t.Fatalf("expected status 204, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackDeleteSuccess(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.DeleteLanguageFn = func(slot uint16, language string, ctx catena.TransportContext) catena.StatusResult {
		if language != "fr" {
			t.Fatalf("expected language fr, got %s", language)
		}

		return catena.StatusWithCode(catena.StatusCodeOk, "")
	}

	rec := makeRequest(t, transport, http.MethodDelete, "/st2138-api/v1/0/language-pack/fr", "")

	if rec.Code != http.StatusNoContent {
		t.Fatalf("expected status 204, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackDeleteError(t *testing.T) {
	transport, runtime := makeTestTransport(t)
	runtime.DeleteLanguageFn = func(slot uint16, language string, ctx catena.TransportContext) catena.StatusResult {
		return catena.StatusWithCode(catena.StatusCodeNotFound, "language pack not found")
	}

	rec := makeRequest(t, transport, http.MethodDelete, "/st2138-api/v1/0/language-pack/fr", "")

	if rec.Code != http.StatusNotFound {
		t.Fatalf("expected status 404, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackInvalidJSON(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodPost, "/st2138-api/v1/0/language-pack/fr", `{invalid json}`)

	if rec.Code != http.StatusBadRequest {
		t.Fatalf("expected status 400, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackMissingLanguage(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodGet, "/st2138-api/v1/0/language-pack", "")

	if rec.Code != http.StatusBadRequest {
		t.Fatalf("expected status 400, got %d: %s", rec.Code, rec.Body.String())
	}
}

func TestTransport_LanguagePackUnsupportedMethod(t *testing.T) {
	transport, _ := makeTestTransport(t)

	rec := makeRequest(t, transport, http.MethodPatch, "/st2138-api/v1/0/language-pack/fr", "")

	if rec.Code != http.StatusMethodNotAllowed {
		t.Fatalf("expected status 405, got %d: %s", rec.Code, rec.Body.String())
	}
}
