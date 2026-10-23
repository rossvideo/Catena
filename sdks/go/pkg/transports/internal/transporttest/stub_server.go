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
	"log/slog"
	"sync"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

type StubServerRuntime struct {
	mu                       sync.Mutex
	tb                       testing.TB
	Dev                      bool
	Slots                    []uint16
	GetSlotsFn               func(ctx catena.TransportContext) ([]uint16, catena.StatusResult)
	GetDeviceFn              func(ctx catena.TransportContext, req catena.GetDeviceRequest) ([]st2138.DeviceComponent, catena.StatusResult)
	GetValueFn               func(ctx catena.TransportContext, req catena.GetValueRequest) (st2138.Value, catena.StatusResult)
	GetParamFn               func(ctx catena.TransportContext, req catena.GetParamRequest) (st2138.Param, catena.StatusResult)
	SetValueFn               func(ctx catena.TransportContext, req catena.SetValueRequest) catena.StatusResult
	ReadAssetFn              func(ctx catena.TransportContext, req catena.ReadAssetRequest) ([]st2138.Asset, catena.StatusResult)
	CreateAssetFn            func(ctx catena.TransportContext, req catena.CreateAssetRequest) catena.StatusResult
	UpdateAssetFn            func(ctx catena.TransportContext, req catena.UpdateAssetRequest) catena.StatusResult
	DeleteAssetFn            func(ctx catena.TransportContext, req catena.DeleteAssetRequest) catena.StatusResult
	CommandFn                func(ctx catena.TransportContext, req catena.ExecuteCommandRequest) ([]st2138.CommandResponse, catena.StatusResult)
	ParamInfoFn              func(ctx catena.TransportContext, req catena.ParamInfoRequest) ([]st2138.ParamInfo, catena.StatusResult)
	ListLanguagesFn          func(ctx catena.TransportContext, req catena.ListLanguagesRequest) ([]string, catena.StatusResult)
	LanguagePackFn           func(ctx catena.TransportContext, req catena.ReadLanguagePackRequest) (catena.LanguagePack, catena.StatusResult)
	AddLanguageFn            func(ctx catena.TransportContext, req catena.CreateLanguagePackRequest) catena.StatusResult
	UpdateLanguageFn         func(ctx catena.TransportContext, req catena.UpdateLanguagePackRequest) catena.StatusResult
	DeleteLanguageFn         func(ctx catena.TransportContext, req catena.DeleteLanguagePackRequest) catena.StatusResult
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
	Log                      *slog.Logger
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

func (s *StubServerRuntime) Logger() *slog.Logger {
	if s != nil && s.Log != nil {
		return s.Log
	}
	return slog.New(slog.DiscardHandler)
}

func (s *StubServerRuntime) GetSlots(ctx catena.TransportContext) ([]uint16, catena.StatusResult) {
	if s.GetSlotsFn != nil {
		return s.GetSlotsFn(ctx)
	}
	return s.Slots, catena.StatusResult{Code: catena.StatusCodeOk}
}

func (s *StubServerRuntime) InvokeGetDeviceHandler(ctx catena.TransportContext, req catena.GetDeviceRequest, stream catena.Stream[st2138.DeviceComponent]) catena.StatusResult {
	if s.GetDeviceFn != nil {
		components, status := s.GetDeviceFn(ctx, req)
		for _, component := range components {
			if err := stream.Send(component); err != nil {
				return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
			}
		}
		return status
	}
	s.panicf("GetDevice handler not implemented in stubServerRuntime for slot %d", req.Slot)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "GetDevice handler not implemented"}
}

func (s *StubServerRuntime) InvokeGetValueHandler(ctx catena.TransportContext, req catena.GetValueRequest) (st2138.Value, catena.StatusResult) {
	if s.GetValueFn != nil {
		return s.GetValueFn(ctx, req)
	}
	s.panicf("GetValue handler not implemented in stubServerRuntime for slot %d, fqoid %s", req.Slot, req.Fqoid)
	return catena.ReplyError[st2138.Value](catena.StatusCodeInternal, "GetValue handler not implemented")
}

func (s *StubServerRuntime) InvokeGetParamHandler(ctx catena.TransportContext, req catena.GetParamRequest) (st2138.Param, catena.StatusResult) {
	if s.GetParamFn != nil {
		return s.GetParamFn(ctx, req)
	}
	s.panicf("GetParam handler not implemented in stubServerRuntime for slot %d, fqoid %s", req.Slot, req.Fqoid)
	return st2138.Param{}, catena.StatusResult{Code: catena.StatusCodeInternal, Error: "GetParam handler not implemented"}
}

func (s *StubServerRuntime) InvokeSetValueHandler(ctx catena.TransportContext, req catena.SetValueRequest) catena.StatusResult {
	if s.SetValueFn != nil {
		return s.SetValueFn(ctx, req)
	}
	s.panicf("SetValue handler not implemented in stubServerRuntime for slot %d, count %d", req.Slot, len(req.Entries))
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *StubServerRuntime) InvokeReadAssetHandler(ctx catena.TransportContext, req catena.ReadAssetRequest, stream catena.Stream[st2138.Asset]) catena.StatusResult {
	if s.ReadAssetFn != nil {
		chunks, status := s.ReadAssetFn(ctx, req)
		for _, chunk := range chunks {
			if err := stream.Send(chunk); err != nil {
				return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
			}
		}
		return status
	}
	panic(fmt.Sprintf("ReadAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", req.Slot, req.Fqoid))
}

func (s *StubServerRuntime) InvokeCreateAssetHandler(ctx catena.TransportContext, req catena.CreateAssetRequest) catena.StatusResult {
	if s.CreateAssetFn != nil {
		return s.CreateAssetFn(ctx, req)
	}
	s.panicf("CreateAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", req.Slot, req.Fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "CreateAsset handler not implemented"}
}

func (s *StubServerRuntime) InvokeUpdateAssetHandler(ctx catena.TransportContext, req catena.UpdateAssetRequest) catena.StatusResult {
	if s.UpdateAssetFn != nil {
		return s.UpdateAssetFn(ctx, req)
	}
	s.panicf("UpdateAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", req.Slot, req.Fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "UpdateAsset handler not implemented"}
}

func (s *StubServerRuntime) InvokeDeleteAssetHandler(ctx catena.TransportContext, req catena.DeleteAssetRequest) catena.StatusResult {
	if s.DeleteAssetFn != nil {
		return s.DeleteAssetFn(ctx, req)
	}
	s.panicf("DeleteAsset handler not implemented in stubServerRuntime for slot %d, fqoid %s", req.Slot, req.Fqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal, Error: "DeleteAsset handler not implemented"}
}

func (s *StubServerRuntime) InvokeExecuteCommandHandler(ctx catena.TransportContext, req catena.ExecuteCommandRequest, stream catena.Stream[st2138.CommandResponse]) catena.StatusResult {
	if s.CommandFn != nil {
		results, status := s.CommandFn(ctx, req)
		for _, result := range results {
			if err := stream.Send(result); err != nil {
				return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
			}
		}
		return status
	}
	s.panicf("ExecuteCommand handler not implemented in stubServerRuntime for slot %d, commandFqoid %s", req.Slot, req.CommandFqoid)
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *StubServerRuntime) InvokeParamInfoHandler(ctx catena.TransportContext, req catena.ParamInfoRequest, stream catena.Stream[st2138.ParamInfo]) catena.StatusResult {
	if s.ParamInfoFn != nil {
		infos, res := s.ParamInfoFn(ctx, req)
		for _, info := range infos {
			if err := stream.Send(info); err != nil {
				return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
			}
		}
		return res
	}
	s.panicf("ParamInfo handler not implemented in stubServerRuntime for slot %d, oidPrefix %s", req.Slot, req.OidPrefix)
	return catena.StatusResult{Code: catena.StatusCodeInternal}
}

func (s *StubServerRuntime) InvokeListLanguagesHandler(ctx catena.TransportContext, req catena.ListLanguagesRequest) ([]string, catena.StatusResult) {
	if s.ListLanguagesFn != nil {
		return s.ListLanguagesFn(ctx, req)
	}
	s.panicf("ListLanguages handler not implemented in stubServerRuntime for slot %d", req.Slot)
	return nil, catena.StatusResult{Code: catena.StatusCodeInternal, Error: "ListLanguages handler not implemented"}
}

func (s *StubServerRuntime) InvokeReadLanguagePackHandler(ctx catena.TransportContext, req catena.ReadLanguagePackRequest) (catena.LanguagePack, catena.StatusResult) {
	if s.LanguagePackFn != nil {
		return s.LanguagePackFn(ctx, req)
	}
	s.panicf("LanguagePack handler not implemented in stubServerRuntime for slot %d, language %s", req.Slot, req.Language)
	return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeInternal, "LanguagePack handler not implemented")
}

func (s *StubServerRuntime) InvokeCreateLanguagePackHandler(ctx catena.TransportContext, req catena.CreateLanguagePackRequest) catena.StatusResult {
	if s.AddLanguageFn != nil {
		return s.AddLanguageFn(ctx, req)
	}
	s.panicf("AddLanguage handler not implemented in stubServerRuntime for slot %d, language %s", req.Slot, req.Language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "AddLanguage handler not implemented")
}

func (s *StubServerRuntime) InvokeUpdateLanguagePackHandler(ctx catena.TransportContext, req catena.UpdateLanguagePackRequest) catena.StatusResult {
	if s.UpdateLanguageFn != nil {
		return s.UpdateLanguageFn(ctx, req)
	}
	s.panicf("UpdateLanguage handler not implemented in stubServerRuntime for slot %d, language %s", req.Slot, req.Language)
	return catena.StatusWithCode(catena.StatusCodeInternal, "UpdateLanguage handler not implemented")
}

func (s *StubServerRuntime) InvokeDeleteLanguagePackHandler(ctx catena.TransportContext, req catena.DeleteLanguagePackRequest) catena.StatusResult {
	if s.DeleteLanguageFn != nil {
		return s.DeleteLanguageFn(ctx, req)
	}
	s.panicf("DeleteLanguage handler not implemented in stubServerRuntime for slot %d, language %s", req.Slot, req.Language)
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
