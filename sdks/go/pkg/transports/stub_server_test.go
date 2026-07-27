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
 * @brief ServerRuntime stub for testing transports without a full Catena server implementation.
 * @file stub_server_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-08
 */

package transports

import (
	"context"
	"fmt"
	"sync"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

type stubServerRuntime struct {
	mu                       sync.Mutex
	tb                       testing.TB
	isDev                    bool
	slots                    []uint16
	getSlotsFn               func(ctx catena.TransportContext) ([]uint16, catena.StatusResult)
	getDeviceFn              func(slot uint16, ctx catena.TransportContext) (st2138.Device, catena.StatusResult)
	getValueFn               func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Value, catena.StatusResult)
	getParamFn               func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult)
	setValueFn               func(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult
	readAssetFn              func(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult)
	createAssetFn            func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult
	updateAssetFn            func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult
	deleteAssetFn            func(slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult
	commandFn                func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.TransportContext) ([]st2138.CommandResponse, catena.StatusResult)
	paramInfoFn              func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]st2138.ParamInfo, catena.StatusResult)
	listLanguagesFn          func(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult)
	languagePackFn           func(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult)
	addLanguageFn            func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult
	updateLanguageFn         func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult
	deleteLanguageFn         func(slot uint16, language string, ctx catena.TransportContext) catena.StatusResult
	registerTransportConnFn  func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult)
	deregisterConnFn         func(connID int)
	shutdownTransportConnsFn func(ctx context.Context, transport catena.Transport)
	registerCalls            int
	deregisterCalls          int
	shutdownCalls            int
	lastRegisterID           int
	lastDeregisterID         int
	lastRegisterOwner        any
	lastShutdownOwner        any
}

func makeStubServerRuntime(tb testing.TB) *stubServerRuntime {
	return &stubServerRuntime{
		tb: tb,
	}
}

func (s *stubServerRuntime) panicf(format string, args ...any) {
	panic(fmt.Sprintf(format, args...))
}

var _ catena.ServerRuntime = (*stubServerRuntime)(nil)

func (s *stubServerRuntime) IsDev() bool {
	return s.isDev
}

func (s *stubServerRuntime) GetSlots(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
	if s.getSlotsFn != nil {
		return s.getSlotsFn(ctx)
	}
	return s.slots, catena.StatusResult{Code: catena.StatusCodeOk}
}

func (s *stubServerRuntime) InvokeGetDeviceHandler(slot uint16, ctx catena.TransportContext) (st2138.Device, catena.StatusResult) {
	if s.getDeviceFn != nil {
		return s.getDeviceFn(slot, ctx)
	}
	s.panicf("GetDevice handler not implemented in stubServerRuntime for slot %d", slot)
	return catena.ReplyError[st2138.Device](catena.StatusCodeInternal, "GetDevice handler not implemented")
}

func (s *stubServerRuntime) InvokeGetValueHandler(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Value, catena.StatusResult) {
	if s.getValueFn != nil {
		return s.getValueFn(slot, fqoid, ctx)
	}
	s.panicf("GetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.ReplyError[st2138.Value](catena.StatusCodeInternal, "GetValue handler not implemented")
}

func (s *stubServerRuntime) InvokeGetParamHandler(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Param, catena.StatusResult) {
	if s.getParamFn != nil {
		return s.getParamFn(slot, fqoid, ctx)
	}
	s.panicf("GetParam handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return st2138.Param{}, catena.StatusResult{Code: catena.StatusCodeInternal, Error: "GetParam handler not implemented"}
}

func (s *stubServerRuntime) InvokeSetValueHandler(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
	if s.setValueFn != nil {
		return s.setValueFn(slot, entries, ctx)
	}
	s.panicf("SetValue handler not implemented in stubServerRuntime for slot %d, count %d", slot, len(entries))
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) InvokeReadAssetHandler(slot uint16, fqoid string, ctx catena.TransportContext) (st2138.Asset, catena.StatusResult) {
	if s.readAssetFn != nil {
		return s.readAssetFn(slot, fqoid, ctx)
	}
	s.panicf("ReadAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.ReplyError[st2138.Asset](catena.StatusCodeInternal, "ReadAsset handler not implemented")
}

func (s *stubServerRuntime) InvokeCreateAssetHandler(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult {
	if s.createAssetFn != nil {
		return s.createAssetFn(slot, fqoid, asset, ctx)
	}
	s.panicf("CreateAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "CreateAsset handler not implemented"}
}

func (s *stubServerRuntime) InvokeUpdateAssetHandler(slot uint16, fqoid string, asset st2138.Asset, ctx catena.TransportContext) catena.StatusResult {
	if s.updateAssetFn != nil {
		return s.updateAssetFn(slot, fqoid, asset, ctx)
	}
	s.panicf("UpdateAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "UpdateAsset handler not implemented"}
}

func (s *stubServerRuntime) InvokeDeleteAssetHandler(slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
	if s.deleteAssetFn != nil {
		return s.deleteAssetFn(slot, fqoid, ctx)
	}
	s.panicf("DeleteAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "DeleteAsset handler not implemented"}
}

func (s *stubServerRuntime) InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any, respond bool, stream catena.Stream[st2138.CommandResponse], ctx catena.TransportContext) catena.StatusResult {
	if s.commandFn != nil {
		results, status := s.commandFn(slot, commandFqoid, payload, respond, ctx)
		for _, result := range results {
			if err := stream.Send(result); err != nil {
				return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
			}
		}
		return status
	}
	s.panicf("ExecuteCommand handler not implemented in stubServerRuntime for slot %d, commandFqoid %s", slot, commandFqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool, stream catena.Stream[st2138.ParamInfo], ctx catena.TransportContext) catena.StatusResult {
	if s.paramInfoFn != nil {
		infos, res := s.paramInfoFn(slot, oidPrefix, recursive, ctx)
		for _, info := range infos {
			if err := stream.Send(info); err != nil {
				return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
			}
		}
		return res
	}
	s.panicf("ParamInfo handler not implemented in stubServerRuntime for slot %d, oidPrefix %s", slot, oidPrefix)
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) InvokeListLanguagesHandler(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult) {
	if s.listLanguagesFn != nil {
		return s.listLanguagesFn(slot, ctx)
	}
	s.panicf("ListLanguages handler not implemented in stubServerRuntime for slot %d", slot)
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal, Error: "ListLanguages handler not implemented"}
}

func (s *stubServerRuntime) InvokeReadLanguagePackHandler(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult) {
	if s.languagePackFn != nil {
		return s.languagePackFn(slot, language, ctx)
	}
	s.panicf("LanguagePack handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeInternal, "LanguagePack handler not implemented")
}

func (s *stubServerRuntime) InvokeCreateLanguagePackHandler(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
	if s.addLanguageFn != nil {
		return s.addLanguageFn(slot, language, languagePack, ctx)
	}
	s.panicf("AddLanguage handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "AddLanguage handler not implemented")
}

func (s *stubServerRuntime) InvokeUpdateLanguagePackHandler(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
	if s.updateLanguageFn != nil {
		return s.updateLanguageFn(slot, language, languagePack, ctx)
	}
	s.panicf("UpdateLanguage handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "UpdateLanguage handler not implemented")
}

func (s *stubServerRuntime) InvokeDeleteLanguagePackHandler(slot uint16, language string, ctx catena.TransportContext) catena.StatusResult {
	if s.deleteLanguageFn != nil {
		return s.deleteLanguageFn(slot, language, ctx)
	}
	s.panicf("DeleteLanguage handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "DeleteLanguage handler not implemented")
}

func (s *stubServerRuntime) RegisterTransportConnection(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.registerTransportConnFn != nil {
		conn, res := s.registerTransportConnFn(transport, ctx)
		s.registerCalls++
		if conn != nil {
			s.lastRegisterID = conn.ID
		}
		s.lastRegisterOwner = transport
		return conn, res
	}
	s.panicf("RegisterTransportConnection not implemented in stubServerRuntime")
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *stubServerRuntime) DeregisterConnection(connID int) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.deregisterConnFn != nil {
		s.deregisterCalls++
		s.lastDeregisterID = connID
		s.deregisterConnFn(connID)
		return
	}
	s.panicf("DeregisterConnection not implemented in stubServerRuntime")
}

func (s *stubServerRuntime) ShutdownTransportConnections(ctx context.Context, transport catena.Transport) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.shutdownTransportConnsFn != nil {
		s.lastShutdownOwner = transport
		s.shutdownCalls++
		s.shutdownTransportConnsFn(ctx, transport)
		return
	}
	s.panicf("ShutdownTransportConnections not implemented in stubServerRuntime")
}

// WithConnection wires register/deregister behavior for a single fixed connection.
func (s *stubServerRuntime) WithConnection(
	connection *catena.Connection,
) {
	s.tb.Helper()

	s.registerTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
		return connection, catena.StatusResult{Code: catena.StatusCodeOk}
	}

	s.deregisterConnFn = func(connID int) {
		if connID != connection.ID {
			s.tb.Errorf("expected to deregister connection with id %d, got %d", connection.ID, connID)
		}
	}

	s.shutdownTransportConnsFn = func(ctx context.Context, transport catena.Transport) {
		if transport != s.lastRegisterOwner {
			s.tb.Errorf("expected to shutdown connections for transport %v, got %v", s.lastRegisterOwner, transport)
		}
		// notify any waiters that the connection has been "closed"
		connection.Done <- struct{}{}
	}
}
