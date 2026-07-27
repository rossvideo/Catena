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
 * @file stub_server.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-08
 */

package transporttest

import (
	"context"
	"fmt"
	"sync"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

type StubServerRuntime struct {
	mu                       sync.Mutex
	tb                       testing.TB
	Dev                      bool
	Slots                    []uint16
	GetSlotsFn               func(ctx catena.TransportContext) ([]uint16, catena.StatusResult)
	GetDeviceFn              func(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult)
	GetValueFn               func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult)
	GetParamFn               func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Param, catena.StatusResult)
	SetValueFn               func(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult
	ReadAssetFn              func(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult)
	CreateAssetFn            func(slot uint16, fqoid string, asset catena.Asset, ctx catena.TransportContext) catena.StatusResult
	UpdateAssetFn            func(slot uint16, fqoid string, asset catena.Asset, ctx catena.TransportContext) catena.StatusResult
	DeleteAssetFn            func(slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult
	CommandFn                func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.TransportContext) ([]catena.CommandResult, catena.StatusResult)
	ParamInfoFn              func(slot uint16, oidPrefix string, recursive bool, ctx catena.TransportContext) ([]catena.ParamInfo, catena.StatusResult)
	ListLanguagesFn          func(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult)
	LanguagePackFn           func(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult)
	AddLanguageFn            func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult
	UpdateLanguageFn         func(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult
	DeleteLanguageFn         func(slot uint16, language string, ctx catena.TransportContext) catena.StatusResult
	RegisterTransportConnFn  func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult)
	DeregisterConnFn         func(connID int)
	ShutdownTransportConnsFn func(ctx context.Context, transport catena.Transport)
	RegisterCalls            int
	DeregisterCalls          int
	ShutdownCalls            int
	LastRegisterID           int
	LastDeregisterID         int
	LastRegisterOwner        any
	LastShutdownOwner        any
}

func MakeStubServerRuntime(tb testing.TB) *StubServerRuntime {
	return &StubServerRuntime{
		tb: tb,
	}
}

func (s *StubServerRuntime) panicf(format string, args ...any) {
	panic(fmt.Sprintf(format, args...))
}

var _ catena.ServerRuntime = (*StubServerRuntime)(nil)

func (s *StubServerRuntime) IsDev() bool {
	return s.Dev
}

func (s *StubServerRuntime) GetSlots(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
	if s.GetSlotsFn != nil {
		return s.GetSlotsFn(ctx)
	}
	return s.Slots, catena.StatusResult{Code: catena.StatusCodeOk}
}

func (s *StubServerRuntime) InvokeGetDeviceHandler(slot uint16, ctx catena.TransportContext) (catena.Device, catena.StatusResult) {
	if s.GetDeviceFn != nil {
		return s.GetDeviceFn(slot, ctx)
	}
	s.panicf("GetDevice handler not implemented in stubServerRuntime for slot %d", slot)
	return catena.ReplyError[catena.Device](catena.StatusCodeInternal, "GetDevice handler not implemented")
}

func (s *StubServerRuntime) InvokeGetValueHandler(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Value, catena.StatusResult) {
	if s.GetValueFn != nil {
		return s.GetValueFn(slot, fqoid, ctx)
	}
	s.panicf("GetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.ReplyError[catena.Value](catena.StatusCodeInternal, "GetValue handler not implemented")
}

func (s *StubServerRuntime) InvokeGetParamHandler(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Param, catena.StatusResult) {
	if s.GetParamFn != nil {
		return s.GetParamFn(slot, fqoid, ctx)
	}
	s.panicf("GetParam handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.Param{}, catena.StatusResult{Code: catena.StatusCodeInternal, Error: "GetParam handler not implemented"}
}

func (s *StubServerRuntime) InvokeSetValueHandler(slot uint16, entries []catena.SetValueEntry, ctx catena.TransportContext) catena.StatusResult {
	if s.SetValueFn != nil {
		return s.SetValueFn(slot, entries, ctx)
	}
	s.panicf("SetValue handler not implemented in stubServerRuntime for slot %d, count %d", slot, len(entries))
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *StubServerRuntime) InvokeReadAssetHandler(slot uint16, fqoid string, ctx catena.TransportContext) (catena.Asset, catena.StatusResult) {
	if s.ReadAssetFn != nil {
		return s.ReadAssetFn(slot, fqoid, ctx)
	}
	s.panicf("ReadAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.ReplyError[catena.Asset](catena.StatusCodeInternal, "ReadAsset handler not implemented")
}

func (s *StubServerRuntime) InvokeCreateAssetHandler(slot uint16, fqoid string, asset catena.Asset, ctx catena.TransportContext) catena.StatusResult {
	if s.CreateAssetFn != nil {
		return s.CreateAssetFn(slot, fqoid, asset, ctx)
	}
	s.panicf("CreateAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "CreateAsset handler not implemented"}
}

func (s *StubServerRuntime) InvokeUpdateAssetHandler(slot uint16, fqoid string, asset catena.Asset, ctx catena.TransportContext) catena.StatusResult {
	if s.UpdateAssetFn != nil {
		return s.UpdateAssetFn(slot, fqoid, asset, ctx)
	}
	s.panicf("UpdateAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "UpdateAsset handler not implemented"}
}

func (s *StubServerRuntime) InvokeDeleteAssetHandler(slot uint16, fqoid string, ctx catena.TransportContext) catena.StatusResult {
	if s.DeleteAssetFn != nil {
		return s.DeleteAssetFn(slot, fqoid, ctx)
	}
	s.panicf("DeleteAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", slot, fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "DeleteAsset handler not implemented"}
}

func (s *StubServerRuntime) InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any, respond bool, stream catena.Stream[catena.CommandResult], ctx catena.TransportContext) catena.StatusResult {
	if s.CommandFn != nil {
		results, status := s.CommandFn(slot, commandFqoid, payload, respond, ctx)
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

func (s *StubServerRuntime) InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool, stream catena.Stream[catena.ParamInfo], ctx catena.TransportContext) catena.StatusResult {
	if s.ParamInfoFn != nil {
		infos, res := s.ParamInfoFn(slot, oidPrefix, recursive, ctx)
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

func (s *StubServerRuntime) InvokeListLanguagesHandler(slot uint16, ctx catena.TransportContext) ([]string, catena.StatusResult) {
	if s.ListLanguagesFn != nil {
		return s.ListLanguagesFn(slot, ctx)
	}
	s.panicf("ListLanguages handler not implemented in stubServerRuntime for slot %d", slot)
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal, Error: "ListLanguages handler not implemented"}
}

func (s *StubServerRuntime) InvokeReadLanguagePackHandler(slot uint16, language string, ctx catena.TransportContext) (catena.LanguagePack, catena.StatusResult) {
	if s.LanguagePackFn != nil {
		return s.LanguagePackFn(slot, language, ctx)
	}
	s.panicf("LanguagePack handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeInternal, "LanguagePack handler not implemented")
}

func (s *StubServerRuntime) InvokeCreateLanguagePackHandler(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
	if s.AddLanguageFn != nil {
		return s.AddLanguageFn(slot, language, languagePack, ctx)
	}
	s.panicf("AddLanguage handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "AddLanguage handler not implemented")
}

func (s *StubServerRuntime) InvokeUpdateLanguagePackHandler(slot uint16, language string, languagePack catena.LanguagePack, ctx catena.TransportContext) catena.StatusResult {
	if s.UpdateLanguageFn != nil {
		return s.UpdateLanguageFn(slot, language, languagePack, ctx)
	}
	s.panicf("UpdateLanguage handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "UpdateLanguage handler not implemented")
}

func (s *StubServerRuntime) InvokeDeleteLanguagePackHandler(slot uint16, language string, ctx catena.TransportContext) catena.StatusResult {
	if s.DeleteLanguageFn != nil {
		return s.DeleteLanguageFn(slot, language, ctx)
	}
	s.panicf("DeleteLanguage handler not implemented in stubServerRuntime for slot %d, language %s", slot, language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "DeleteLanguage handler not implemented")
}

func (s *StubServerRuntime) RegisterTransportConnection(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.RegisterTransportConnFn != nil {
		conn, res := s.RegisterTransportConnFn(transport, ctx)
		s.RegisterCalls++
		if conn != nil {
			s.LastRegisterID = conn.ID
		}
		s.LastRegisterOwner = transport
		return conn, res
	}
	s.panicf("RegisterTransportConnection not implemented in stubServerRuntime")
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *StubServerRuntime) DeregisterConnection(connID int) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.DeregisterConnFn != nil {
		s.DeregisterCalls++
		s.LastDeregisterID = connID
		s.DeregisterConnFn(connID)
		return
	}
	s.panicf("DeregisterConnection not implemented in stubServerRuntime")
}

func (s *StubServerRuntime) ShutdownTransportConnections(ctx context.Context, transport catena.Transport) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.ShutdownTransportConnsFn != nil {
		s.LastShutdownOwner = transport
		s.ShutdownCalls++
		s.ShutdownTransportConnsFn(ctx, transport)
		return
	}
	s.panicf("ShutdownTransportConnections not implemented in stubServerRuntime")
}

// WithConnection wires register/deregister behavior for a single fixed connection.
func (s *StubServerRuntime) WithConnection(
	connection *catena.Connection,
) {
	s.tb.Helper()

	s.RegisterTransportConnFn = func(transport catena.Transport, ctx catena.TransportContext) (*catena.Connection, catena.StatusResult) {
		return connection, catena.StatusResult{Code: catena.StatusCodeOk}
	}

	s.DeregisterConnFn = func(connID int) {
		if connID != connection.ID {
			s.tb.Errorf("expected to deregister connection with id %d, got %d", connection.ID, connID)
		}
	}

	s.ShutdownTransportConnsFn = func(ctx context.Context, transport catena.Transport) {
		if transport != s.LastRegisterOwner {
			s.tb.Errorf("expected to shutdown connections for transport %v, got %v", s.LastRegisterOwner, transport)
		}
		// notify any waiters that the connection has been "closed"
		connection.Done <- struct{}{}
	}
}
