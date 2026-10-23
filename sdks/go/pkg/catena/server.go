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
 * @brief Server interface and handler types for the Catena SDK.
 * @file server.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-09-23
 */

package catena

import (
	"context"
	"errors"
	"fmt"
	"log/slog"
	"maps"
	"math"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

type EndpointType int

const (
	EndpointGetSlots EndpointType = iota
	EndpointGetDevice
	EndpointGetValue
	EndpointGetParam
	EndpointSetValue
	EndpointReadAsset
	EndpointCreateAsset
	EndpointUpdateAsset
	EndpointDeleteAsset
	EndpointExecuteCommand
	EndpointParamInfo
	EndpointReadLanguagePack
	EndpointCreateLanguagePack
	EndpointUpdateLanguagePack
	EndpointDeleteLanguagePack
	EndpointConnect
	EndpointListLanguages

	// endpointTypeMax is a sentinel that must always be last. Add new endpoint
	// types above it. It is not a real endpoint; it bounds the enumeration so
	// tests can range over every EndpointType without a hand-maintained list.
	endpointTypeMax
)

// String returns the human-readable name of the endpoint type. The switch must
// stay exhaustive: adding a new EndpointType without a case here returns
// "Unknown", which the enumeration test flags.
func (e EndpointType) String() string {
	switch e {
	case EndpointGetSlots:
		return "GetSlots"
	case EndpointGetDevice:
		return "GetDevice"
	case EndpointGetValue:
		return "GetValue"
	case EndpointSetValue:
		return "SetValue"
	case EndpointGetParam:
		return "GetParam"
	case EndpointReadAsset:
		return "ReadAsset"
	case EndpointCreateAsset:
		return "CreateAsset"
	case EndpointUpdateAsset:
		return "UpdateAsset"
	case EndpointDeleteAsset:
		return "DeleteAsset"
	case EndpointExecuteCommand:
		return "ExecuteCommand"
	case EndpointParamInfo:
		return "ParamInfo"
	case EndpointReadLanguagePack:
		return "ReadLanguagePack"
	case EndpointCreateLanguagePack:
		return "CreateLanguagePack"
	case EndpointUpdateLanguagePack:
		return "UpdateLanguagePack"
	case EndpointDeleteLanguagePack:
		return "DeleteLanguagePack"
	case EndpointConnect:
		return "Connect"
	case EndpointListLanguages:
		return "ListLanguages"
	default:
		return "Unknown"
	}
}

var allowAllAccessHandler AccessHandler = func(endpointType EndpointType, ctx HandlerContext) bool { return true }

var ErrServerStopped = errors.New("server is stopped")

// defaultServerMaxShutdownWait is a fallback safety cap for shutdown paths when
// callers pass an unbounded context. Caller-provided earlier deadlines still win.
const defaultServerMaxShutdownWait = 10 * time.Second

func ValidateSlot(slot uint32) (uint16, StatusResult) {
	if slot > uint32(math.MaxUint16) {
		return 0, StatusWithCode(StatusCodeInvalidArgument, fmt.Sprintf("invalid slot number: %d", slot))

	}
	return uint16(slot), StatusWithCode(StatusCodeOk, "")
}

func ValidateSlotString(slot string) (uint16, StatusResult) {
	slotInt, err := strconv.ParseUint(slot, 10, 32)
	if err != nil {
		return 0, StatusWithCode(StatusCodeInvalidArgument, fmt.Sprintf("invalid slot string: %s", slot))
	}
	return ValidateSlot(uint32(slotInt))
}

type Transport interface {
	// Start begins transport operation with the given context.
	// The context signals when the transport should gracefully exit.
	// Start should return quickly and may spawn background goroutines.
	// Transports should monitor ctx.Done() and exit cleanly when signaled.
	// ServerRuntime is passed to allow transports to invoke handlers and register connections.
	Start(ctx context.Context, runtime ServerRuntime) error

	// Shutdown closes the transport and waits for cleanup.
	// The context provides a deadline/cancellation boundary; Shutdown must
	// respect it and return by that deadline.
	Shutdown(ctx context.Context) error
}

// Server is the public API for application code.
// The concrete implementation is intentionally hidden.
type Server interface {
	RegisterTransport(transport Transport) error
	DeregisterTransport(ctx context.Context, transport Transport) error
	Wait()
	Shutdown(ctx context.Context)

	// The Register*Handler methods below store the handler for a slot,
	// replacing any handler previously registered for that slot and endpoint.
	// Registering the first handler for a new slot makes the slot visible via
	// GetSlots and pushes a SlotsAdded update to connected clients. Each doc
	// restates the handler signature so it can be read at the call site; the
	// handler type's doc carries the full implementation contract. Signatures
	// qualify the SDK's own types with the "catena." package prefix so they can be
	// pasted directly into application code in another package.

	// RegisterGetDeviceHandler registers the GetDevice handler for a slot. The
	// handler streams device components through stream.Send, stops on a Send
	// error, and returns a terminal status; see [DeviceHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.GetDeviceRequest, stream catena.Stream[st2138.DeviceComponent]) catena.StatusResult
	RegisterGetDeviceHandler(slot uint16, handler DeviceHandler)

	// RegisterGetValueHandler registers the GetValue handler for a slot.
	// See [GetValueHandler].
	//
	// Registering this handler is optional: if no GetValue handler is registered
	// for the slot but a GetParam handler is, the SDK derives GetValue responses
	// from GetParam by returning just the param's value. If neither is
	// registered, GetValue returns NotFound.
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.GetValueRequest) (st2138.Value, catena.StatusResult)
	RegisterGetValueHandler(slot uint16, handler GetValueHandler)

	// RegisterGetParamHandler registers the GetParam handler for a slot.
	// See [GetParamHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.GetParamRequest) (st2138.Param, catena.StatusResult)
	RegisterGetParamHandler(slot uint16, handler GetParamHandler)

	// RegisterSetValueHandler registers the SetValue handler for a slot. The
	// handler must apply its batch atomically (all-or-nothing); see
	// [SetValueHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.SetValueRequest) catena.StatusResult
	RegisterSetValueHandler(slot uint16, handler SetValueHandler)

	// RegisterReadAssetHandler registers the ReadAsset handler for a slot. The
	// handler streams asset chunks through stream.Send, stops on a Send error,
	// and returns a terminal status; see [ReadAssetHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.ReadAssetRequest, stream catena.Stream[st2138.Asset]) catena.StatusResult
	RegisterReadAssetHandler(slot uint16, handler ReadAssetHandler)

	// RegisterCreateAssetHandler registers the CreateAsset handler for a slot.
	// See [CreateAssetHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.CreateAssetRequest) catena.StatusResult
	RegisterCreateAssetHandler(slot uint16, handler CreateAssetHandler)

	// RegisterUpdateAssetHandler registers the UpdateAsset handler for a slot.
	// See [UpdateAssetHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.UpdateAssetRequest) catena.StatusResult
	RegisterUpdateAssetHandler(slot uint16, handler UpdateAssetHandler)

	// RegisterDeleteAssetHandler registers the DeleteAsset handler for a slot.
	// See [DeleteAssetHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.DeleteAssetRequest) catena.StatusResult
	RegisterDeleteAssetHandler(slot uint16, handler DeleteAssetHandler)

	// RegisterExecuteCommandHandler registers the ExecuteCommand handler for a
	// slot. The handler streams responses through stream.Send, stops on a Send
	// error, and returns a terminal status; see [ExecuteCommandHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.ExecuteCommandRequest, stream catena.Stream[st2138.CommandResponse]) catena.StatusResult
	RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler)

	// RegisterParamInfoHandler registers the ParamInfo handler for a slot. The
	// handler streams descriptors through stream.Send, stops on a Send error,
	// and returns a terminal status (Ok / NotFound / Internal); see
	// [ParamInfoHandler].
	//
	// Registering this handler is optional: if no ParamInfo handler is registered
	// for the slot but a GetParam handler is, the SDK derives ParamInfo responses
	// for a specific oidPrefix from GetParam (flattening the param's sub-param
	// subtree when recursive). The whole-tree request (oidPrefix == "") is not
	// derivable from the single-OID GetParam and returns NotFound; serve it with a
	// real ParamInfo handler. If neither handler is registered, ParamInfo returns
	// NotFound.
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.ParamInfoRequest, stream catena.Stream[st2138.ParamInfo]) catena.StatusResult
	RegisterParamInfoHandler(slot uint16, handler ParamInfoHandler)

	// RegisterListLanguagesHandler registers the ListLanguages handler for a
	// slot. See [ListLanguagesHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.ListLanguagesRequest) ([]string, catena.StatusResult)
	RegisterListLanguagesHandler(slot uint16, handler ListLanguagesHandler)

	// RegisterReadLanguagePackHandler registers the ReadLanguagePack handler
	// for a slot. See [ReadLanguagePackHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.ReadLanguagePackRequest) (catena.LanguagePack, catena.StatusResult)
	RegisterReadLanguagePackHandler(slot uint16, handler ReadLanguagePackHandler)

	// RegisterCreateLanguagePackHandler registers the CreateLanguagePack
	// handler for a slot. See [CreateLanguagePackHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.CreateLanguagePackRequest) catena.StatusResult
	RegisterCreateLanguagePackHandler(slot uint16, handler CreateLanguagePackHandler)

	// RegisterUpdateLanguagePackHandler registers the UpdateLanguagePack
	// handler for a slot. See [UpdateLanguagePackHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.UpdateLanguagePackRequest) catena.StatusResult
	RegisterUpdateLanguagePackHandler(slot uint16, handler UpdateLanguagePackHandler)

	// RegisterDeleteLanguagePackHandler registers the DeleteLanguagePack
	// handler for a slot. See [DeleteLanguagePackHandler].
	//
	// Handler signature:
	//
	//	func(ctx catena.HandlerContext, req catena.DeleteLanguagePackRequest) catena.StatusResult
	RegisterDeleteLanguagePackHandler(slot uint16, handler DeleteLanguagePackHandler)

	// RegisterHeartbeatHandler registers the heartbeat handler for a slot. The
	// handler fires only while StartHeartbeat is running; see [HeartbeatHandler].
	//
	// Handler signature:
	//
	//	func(slot uint16)
	RegisterHeartbeatHandler(slot uint16, handler HeartbeatHandler)

	// RegisterAccessHandler registers the global access handler. Unlike the
	// per-slot methods above, it applies to every slot, has no slot-visibility
	// side effect, and passing nil resets to allow-all. See [AccessHandler].
	//
	// Handler signature:
	//
	//	func(endpointType catena.EndpointType, ctx catena.HandlerContext) bool
	RegisterAccessHandler(handler AccessHandler)

	// RegisterProductStruct hands the mandatory product struct for a slot to the
	// SDK. Once registered, the SDK injects the product param into the device on
	// GetDevice, answers GetParam/GetValue/ParamInfo for product/*, and rejects
	// SetValue writes to product/* with StatusCodePermissionDenied — business
	// logic no longer needs to handle the product struct. GetParam/GetValue/
	// ParamInfo serve product/* even when no handler is registered for the slot,
	// so a product-only slot (e.g. a command-only device with no params) still
	// exposes its mandatory product param.
	// The product param carries the st2138:mon access scope: when authorization
	// is enabled and a product struct is registered for the slot, GetDevice omits
	// the product param for callers without the monitor read scope, and
	// GetValue/GetParam/ParamInfo reject product/* requests from such callers with
	// StatusCodePermissionDenied.
	// Note: If you do not register a product struct for a slot, product/* requests
	// fall through to the registered handler for that slot.
	RegisterProductStruct(slot uint16, product ProductStruct)

	SetMaxConnections(max int)
	ConnectionCount() int
	BroadcastUpdate(slot uint16, oid string, value any, scope string)

	StartHeartbeat(interval time.Duration)
	StopHeartbeat()
}

// interface of funcs that Transports use to interact with the server without circular imports
type ServerRuntime interface {
	IsDev() bool
	GetSlots(transportContext TransportContext) ([]uint16, StatusResult)
	InvokeGetDeviceHandler(transportContext TransportContext, req GetDeviceRequest, stream Stream[st2138.DeviceComponent]) StatusResult
	InvokeGetValueHandler(transportContext TransportContext, req GetValueRequest) (st2138.Value, StatusResult)
	InvokeGetParamHandler(transportContext TransportContext, req GetParamRequest) (st2138.Param, StatusResult)
	InvokeSetValueHandler(transportContext TransportContext, req SetValueRequest) StatusResult
	InvokeReadAssetHandler(transportContext TransportContext, req ReadAssetRequest, stream Stream[st2138.Asset]) StatusResult
	InvokeCreateAssetHandler(transportContext TransportContext, req CreateAssetRequest) StatusResult
	InvokeUpdateAssetHandler(transportContext TransportContext, req UpdateAssetRequest) StatusResult
	InvokeDeleteAssetHandler(transportContext TransportContext, req DeleteAssetRequest) StatusResult
	InvokeExecuteCommandHandler(transportContext TransportContext, req ExecuteCommandRequest, stream Stream[st2138.CommandResponse]) StatusResult
	InvokeParamInfoHandler(transportContext TransportContext, req ParamInfoRequest, stream Stream[st2138.ParamInfo]) StatusResult
	InvokeListLanguagesHandler(transportContext TransportContext, req ListLanguagesRequest) ([]string, StatusResult)
	InvokeReadLanguagePackHandler(transportContext TransportContext, req ReadLanguagePackRequest) (LanguagePack, StatusResult)
	InvokeCreateLanguagePackHandler(transportContext TransportContext, req CreateLanguagePackRequest) StatusResult
	InvokeUpdateLanguagePackHandler(transportContext TransportContext, req UpdateLanguagePackRequest) StatusResult
	InvokeDeleteLanguagePackHandler(transportContext TransportContext, req DeleteLanguagePackRequest) StatusResult
	RegisterTransportConnection(transport Transport, transportContext TransportContext) (*Connection, StatusResult)
	ShutdownTransportConnections(ctx context.Context, transport Transport)
	DeregisterConnection(connID int)
	Logger() *slog.Logger
}

var _ Server = (*server)(nil)
var _ ServerRuntime = (*server)(nil)

type server struct {
	options                    ServerOptions
	log                        *slog.Logger // To be distributed to other components
	serverLogger               *slog.Logger // To be used for server-specific logging
	mu                         sync.Mutex
	ctx                        context.Context
	ctxCancel                  context.CancelFunc
	authzEnabled               bool
	jwtValidator               jwtValidatorInterface
	maxShutdownWait            time.Duration
	shutdown                   bool
	stopped                    chan struct{}
	slots                      map[uint16]struct{}
	getDeviceHandlers          map[uint16]DeviceHandler
	getValueHandlers           map[uint16]GetValueHandler
	getParamHandlers           map[uint16]GetParamHandler
	setValueHandlers           map[uint16]SetValueHandler
	readAssetHandlers          map[uint16]ReadAssetHandler
	createAssetHandlers        map[uint16]CreateAssetHandler
	updateAssetHandlers        map[uint16]UpdateAssetHandler
	deleteAssetHandlers        map[uint16]DeleteAssetHandler
	executeCommandHandlers     map[uint16]ExecuteCommandHandler
	paramInfoHandlers          map[uint16]ParamInfoHandler
	listLanguagesHandlers      map[uint16]ListLanguagesHandler
	readLanguagePackHandlers   map[uint16]ReadLanguagePackHandler
	createLanguagePackHandlers map[uint16]CreateLanguagePackHandler
	updateLanguagePackHandlers map[uint16]UpdateLanguagePackHandler
	deleteLanguagePackHandlers map[uint16]DeleteLanguagePackHandler
	heartbeatHandlers          map[uint16]HeartbeatHandler
	productStructs             map[uint16]ProductStruct // SDK-managed product per slot
	accessHandler              AccessHandler            // optional fallback for slots without specific handlers
	connectionQueue            connectionQueueInterface
	heartbeat                  *Heartbeat
	transports                 []Transport

	// Redirection seams for the generic registerHandler/invokeHandler helpers.
	// Generic functions cannot be stored in fields, so the helpers stay generic
	// but route their non-generic work through these fields, which unit tests may
	// overwrite to mock registration bookkeeping or the authorization gate.
	registerHandlerFn func(slot uint16, store func())
	invokeGateFn      func(transportContext TransportContext, endpoint EndpointType, writeAccess bool) (HandlerContext, StatusResult)
}

func NewServer(opts config.ServerOptions) (Server, error) {
	ctx, cancel := context.WithCancel(context.Background())

	var log *slog.Logger
	if opts.Logger == nil {
		log = slog.New(slog.DiscardHandler)
	} else {
		log = opts.Logger
	}

	var validator jwtValidatorInterface
	if opts.AuthzEnabled {
		var err error
		validator, err = newJwtValidator(ctx, log, opts.JwtOptions)
		if err != nil {
			cancel()
			return nil, fmt.Errorf("create jwt validator: %w", err)
		}
	}

	s := &server{
		options:                    opts,
		log:                        log,
		serverLogger:               log.With("component", "server"),
		ctx:                        ctx,
		ctxCancel:                  cancel,
		authzEnabled:               opts.AuthzEnabled,
		jwtValidator:               validator,
		maxShutdownWait:            defaultServerMaxShutdownWait, // override in unittests if needed
		shutdown:                   false,
		stopped:                    make(chan struct{}),
		slots:                      make(map[uint16]struct{}),
		getDeviceHandlers:          make(map[uint16]DeviceHandler),
		getValueHandlers:           make(map[uint16]GetValueHandler),
		getParamHandlers:           make(map[uint16]GetParamHandler),
		setValueHandlers:           make(map[uint16]SetValueHandler),
		readAssetHandlers:          make(map[uint16]ReadAssetHandler),
		createAssetHandlers:        make(map[uint16]CreateAssetHandler),
		updateAssetHandlers:        make(map[uint16]UpdateAssetHandler),
		deleteAssetHandlers:        make(map[uint16]DeleteAssetHandler),
		executeCommandHandlers:     make(map[uint16]ExecuteCommandHandler),
		paramInfoHandlers:          make(map[uint16]ParamInfoHandler),
		listLanguagesHandlers:      make(map[uint16]ListLanguagesHandler),
		readLanguagePackHandlers:   make(map[uint16]ReadLanguagePackHandler),
		createLanguagePackHandlers: make(map[uint16]CreateLanguagePackHandler),
		updateLanguagePackHandlers: make(map[uint16]UpdateLanguagePackHandler),
		deleteLanguagePackHandlers: make(map[uint16]DeleteLanguagePackHandler),
		heartbeatHandlers:          make(map[uint16]HeartbeatHandler),
		productStructs:             make(map[uint16]ProductStruct),
		accessHandler:              allowAllAccessHandler,
		connectionQueue:            newConnectionQueue(opts.MaxConnections, log),
		transports:                 []Transport{},
	}

	// Default the redirection seams to the real implementations. Tests may
	// overwrite these fields to mock registration or the authorization gate.
	s.registerHandlerFn = s.realRegisterHandler
	s.invokeGateFn = s.realInvokeGate

	return s, nil
}

func (s *server) boundedShutdownContext(parent context.Context) (context.Context, context.CancelFunc) {
	// also defend against nil parent contexts
	if parent == nil {
		parent = context.Background()
	}
	if s.maxShutdownWait <= 0 {
		return parent, func() {}
	}
	return context.WithTimeout(parent, s.maxShutdownWait)
}

func (s *server) RegisterTransport(transport Transport) error {
	if transport == nil {
		return fmt.Errorf("cannot register nil transport")
	}

	// Lock for the entire function. transport.Start is expected to return quickly
	// and do its work in background goroutines, so holding the lock is safe.
	// This eliminates the race window between Start and appending to transports.
	s.mu.Lock()
	defer s.mu.Unlock()

	if s.shutdown {
		return ErrServerStopped
	}

	// Pass server context so transport can derive its own child contexts if needed.
	err := transport.Start(s.ctx, s)
	if err != nil {
		return err
	}

	// after transport has started, add it to the list
	s.transports = append(s.transports, transport)

	return nil
}

func (s *server) DeregisterTransport(ctx context.Context, transport Transport) error {
	s.mu.Lock()
	idx := -1
	for i, t := range s.transports {
		if t == transport {
			idx = i
			break
		}
	}

	if idx == -1 {
		s.mu.Unlock()
		return nil // Transport not found; could also return an error if desired
	}

	s.transports = append(s.transports[:idx], s.transports[idx+1:]...)
	s.mu.Unlock()

	shutdownCtx, cancel := s.boundedShutdownContext(ctx)
	defer cancel()

	// Shutdown may block while draining work; call it outside the server lock.
	err := transport.Shutdown(shutdownCtx)
	if err != nil {
		s.serverLogger.Error("Error shutting down transport", "error", err)
	}

	// drain any remaining connections owned by this transport
	// shutdown also used the same internal cq shutdownConnection method,
	// this will catch any connections that arrived after the transport's Shutdown was called but before it returned
	s.connectionQueue.shutdownOwner(shutdownCtx, transport)

	return err
}

func (s *server) Wait() {
	<-s.stopped
}

func (s *server) Shutdown(ctx context.Context) {
	s.mu.Lock()
	if s.shutdown {
		s.mu.Unlock()
		return
	}
	s.shutdown = true
	transports := s.transports
	s.transports = nil
	s.mu.Unlock()

	// cancel the server context to signal transports to stop accepting new work
	s.ctxCancel()

	// stop the heartbeat if its running
	s.StopHeartbeat()

	shutdownCtx, cancel := s.boundedShutdownContext(ctx)
	defer cancel()

	// Shutdown all transports outside the lock.
	for _, t := range transports {
		err := t.Shutdown(shutdownCtx)
		if err != nil {
			s.serverLogger.Error("Error shutting down transport", "error", err)
		}
	}

	// tell the connection queue to drain any remaining connections
	s.connectionQueue.shutdown(shutdownCtx)

	close(s.stopped)
}

func (s *server) IsDev() bool {
	return s.options.IsDev
}

func (s *server) Logger() *slog.Logger {
	return s.log
}

// parseTransportContext parses the transport context and returns a HandlerContext.
// It validates the access token and extracts the read and write scopes from the token.
func (s *server) parseTransportContext(transportContext TransportContext) (HandlerContext, StatusResult) {
	accessToken := strings.TrimSpace(transportContext.AccessToken)
	if strings.HasPrefix(strings.ToLower(accessToken), "bearer ") {
		accessToken = strings.TrimSpace(accessToken[len("Bearer "):])
	}
	if accessToken == "" {
		return HandlerContext{}, StatusWithCode(StatusCodeUnauthenticated, "missing access token")
	}
	if s.jwtValidator == nil {
		return HandlerContext{}, StatusWithCode(StatusCodeUnauthenticated, "jwt validator is not configured")
	}

	token, err := s.jwtValidator.validateJwt(accessToken)
	if err != nil {
		s.serverLogger.Warn("Failed to validate access token", "error", err)
		return HandlerContext{}, StatusWithCode(StatusCodeUnauthenticated, "invalid access token")
	}

	readScopes, writeScopes := extractTokenScopes(token)

	handlerContext := HandlerContext{
		Token:        token,
		readScopes:   readScopes,
		writeScopes:  writeScopes,
		Metadata:     maps.Clone(transportContext.Metadata),
		authzEnabled: true,
	}
	return handlerContext, StatusWithCode(StatusCodeOk, "")
}

// resolveHandlerContext builds a HandlerContext for an incoming request.
// When authorization is disabled, JWT validation is skipped so clients are not
// required to send an access token.
func (s *server) resolveHandlerContext(transportContext TransportContext) (HandlerContext, StatusResult) {
	if !s.authzEnabled {
		return HandlerContext{
			Metadata:     maps.Clone(transportContext.Metadata),
			authzEnabled: false,
		}, StatusWithCode(StatusCodeOk, "")
	}
	return s.parseTransportContext(transportContext)
}

func (s *server) isAccessAllowed(endpointType EndpointType, handlerContext HandlerContext) bool {
	if !s.authzEnabled {
		return true
	}

	s.mu.Lock()
	accessHandler := s.accessHandler
	s.mu.Unlock()

	return accessHandler(endpointType, handlerContext)
}

func (s *server) hasReadAccess(handlerContext HandlerContext) bool {
	return handlerContext.HasAnyReadScope()
}

func (s *server) hasWriteAccess(handlerContext HandlerContext) bool {
	return handlerContext.HasAnyWriteScope()
}

func (s *server) GetSlots(transportContext TransportContext) ([]uint16, StatusResult) {
	// enforce access checks
	handlerContext, res := s.invokeGateFn(transportContext, EndpointGetSlots, false)
	if res.IsError() {
		return nil, res
	}
	defer handlerContext.release()

	s.mu.Lock()
	slots := s.getSlotsLocked()
	s.mu.Unlock()

	return slots, StatusWithCode(StatusCodeOk, "")
}

// call this within a locked s.mu context
func (s *server) getSlotsLocked() []uint16 {
	slots := make([]uint16, 0, len(s.slots))
	for slot := range s.slots {
		slots = append(slots, slot)
	}
	return slots
}

// must be called from a locked context
func (s *server) registerSlotLocked(slot uint16) bool {
	_, found := s.slots[slot]
	s.slots[slot] = struct{}{}
	return !found
}

func (s *server) notifySlotsAdded(slot uint16) {
	s.connectionQueue.notifyUpdate(&protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: []uint32{uint32(slot)},
			},
		},
	}, "")
}

// realRegisterHandler is the default implementation behind s.registerHandlerFn.
// store performs the typed map write; it is invoked under the server lock so the
// write, slot registration, and new-slot detection are atomic.
// By using a store func, we can avoid generics which is better for testing
// because generics cannot be stored in fields so can't be mocked.
func (s *server) realRegisterHandler(slot uint16, store func()) {
	s.mu.Lock()
	store()
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

// requestContext derives the context handed to a handler for a single unary
// request. It is canceled when EITHER the server starts shutting down (s.ctx)
// or the transport's per-request context (reqCtx) is canceled or times out.
// reqCtx is the value/deadline parent so request-scoped values and deadlines
// reach the handler; server shutdown contributes cancellation only.
//
// The caller MUST call the returned stop func once the handler returns.
// Otherwise the AfterFunc registration lingers on s.ctx until server shutdown,
// leaking one registration per request.
func (s *server) requestContext(reqCtx context.Context) (context.Context, context.CancelFunc) {
	if reqCtx == nil {
		reqCtx = context.Background()
	}
	ctx, cancel := context.WithCancel(reqCtx)
	// When the server context is done, cancel this request context too. stop
	// unregisters the callback when the request finishes first (the common case).
	stop := context.AfterFunc(s.ctx, cancel)
	return ctx, func() {
		stop()
		cancel()
	}
}

// invokeHandler runs the shared gate-keeping for an endpoint (context
// resolution, scope + endpoint access checks and handler lookup) and then
// defers to call for the endpoint-specific handler invocation. The
// authorization gate is delegated to s.invokeGateFn so tests can redirect it;
// notFound is returned with StatusCodeNotFound when no handler is registered for
// slot.
func invokeHandler[H, T any](
	s *server,
	transportContext TransportContext,
	endpoint EndpointType,
	writeAccess bool,
	handlers map[uint16]H,
	slot uint16,
	notFound string,
	call func(handler H, ctx HandlerContext) (T, StatusResult),
) (T, StatusResult) {
	return withGate(s, transportContext, endpoint, writeAccess,
		func(ctx HandlerContext) (T, StatusResult) {
			var zero T

			s.mu.Lock()
			handler, ok := handlers[slot]
			s.mu.Unlock()

			//TODO: add default handler lookup when custom default handlers are supported
			if !ok {
				s.serverLogger.Warn("no handler registered for slot", "endpoint", endpoint, "slot", slot)
				return zero, StatusWithCode(StatusCodeNotFound, notFound)
			}

			return call(handler, ctx)
		})
}

// withGate runs the access gate for endpoint (building the request
// HandlerContext), invokes fn with that context, and releases the context when
// fn returns. It centralizes the gate + context lifecycle so callers that need
// custom handler resolution (e.g. deriving one endpoint's response from
// another's handler) can share the same access semantics as invokeHandler. The
// gate is always evaluated against the endpoint actually being served, never
// the handler that ends up producing the response.
func withGate[T any](
	s *server,
	transportContext TransportContext,
	endpoint EndpointType,
	writeAccess bool,
	fn func(ctx HandlerContext) (T, StatusResult),
) (T, StatusResult) {
	var zero T

	handlerContext, res := s.invokeGateFn(transportContext, endpoint, writeAccess)
	if res.IsError() {
		return zero, res
	}
	// The gate built the request context (so the access handler could observe it);
	// this unary request ends when fn returns, so release it here.
	defer handlerContext.release()

	return fn(handlerContext)
}

// realInvokeGate is the default implementation behind s.invokeGateFn. It
// resolves the caller context and enforces scope + endpoint access. The
// StatusResult reports any error, the HandlerContext only contains valid
// data when the StatusResult is OK. invokeHandler should only proceed
// to call the endpoint handler when the StatusResult is OK.
func (s *server) realInvokeGate(transportContext TransportContext, endpoint EndpointType, writeAccess bool) (HandlerContext, StatusResult) {
	handlerContext, res := s.resolveHandlerContext(transportContext)
	if res.IsError() {
		return HandlerContext{}, res
	}

	// Build the request context up front so the access handler (which runs below)
	// can observe cancellation/deadline via ctx.Context(). Ownership of the cancel
	// passes to the caller through handlerContext.release(); on the rejection path
	// below no caller sees it, so we release here.
	ctx, cancel := s.requestContext(transportContext.Ctx)
	handlerContext.ctx = ctx
	handlerContext.ctxCancel = cancel
	handlerContext.setLogger(s.log, endpoint)

	granted := false
	if writeAccess {
		granted = s.hasWriteAccess(handlerContext)
	} else {
		granted = s.hasReadAccess(handlerContext)
	}
	if !granted || !s.isAccessAllowed(endpoint, handlerContext) {
		handlerContext.release()
		return HandlerContext{}, StatusWithCode(StatusCodePermissionDenied, "Permission denied")
	}

	return handlerContext, StatusWithCode(StatusCodeOk, "")
}

// Handler registration methods
func (s *server) RegisterGetDeviceHandler(slot uint16, handler DeviceHandler) {
	s.registerHandlerFn(slot, func() { s.getDeviceHandlers[slot] = handler })
}

func (s *server) RegisterGetValueHandler(slot uint16, handler GetValueHandler) {
	s.registerHandlerFn(slot, func() { s.getValueHandlers[slot] = handler })
}

func (s *server) RegisterGetParamHandler(slot uint16, handler GetParamHandler) {
	s.registerHandlerFn(slot, func() { s.getParamHandlers[slot] = handler })
}

func (s *server) RegisterSetValueHandler(slot uint16, handler SetValueHandler) {
	s.registerHandlerFn(slot, func() { s.setValueHandlers[slot] = handler })
}

func (s *server) RegisterReadAssetHandler(slot uint16, handler ReadAssetHandler) {
	s.registerHandlerFn(slot, func() { s.readAssetHandlers[slot] = handler })
}

func (s *server) RegisterCreateAssetHandler(slot uint16, handler CreateAssetHandler) {
	s.registerHandlerFn(slot, func() { s.createAssetHandlers[slot] = handler })
}

func (s *server) RegisterUpdateAssetHandler(slot uint16, handler UpdateAssetHandler) {
	s.registerHandlerFn(slot, func() { s.updateAssetHandlers[slot] = handler })
}

func (s *server) RegisterDeleteAssetHandler(slot uint16, handler DeleteAssetHandler) {
	s.registerHandlerFn(slot, func() { s.deleteAssetHandlers[slot] = handler })
}

func (s *server) RegisterListLanguagesHandler(slot uint16, handler ListLanguagesHandler) {
	s.registerHandlerFn(slot, func() { s.listLanguagesHandlers[slot] = handler })
}

func (s *server) RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler) {
	s.registerHandlerFn(slot, func() { s.executeCommandHandlers[slot] = handler })
}

func (s *server) RegisterParamInfoHandler(slot uint16, handler ParamInfoHandler) {
	s.registerHandlerFn(slot, func() { s.paramInfoHandlers[slot] = handler })
}

func (s *server) RegisterReadLanguagePackHandler(slot uint16, handler ReadLanguagePackHandler) {
	s.registerHandlerFn(slot, func() { s.readLanguagePackHandlers[slot] = handler })
}

func (s *server) RegisterCreateLanguagePackHandler(slot uint16, handler CreateLanguagePackHandler) {
	s.registerHandlerFn(slot, func() { s.createLanguagePackHandlers[slot] = handler })
}

func (s *server) RegisterUpdateLanguagePackHandler(slot uint16, handler UpdateLanguagePackHandler) {
	s.registerHandlerFn(slot, func() { s.updateLanguagePackHandlers[slot] = handler })
}

func (s *server) RegisterDeleteLanguagePackHandler(slot uint16, handler DeleteLanguagePackHandler) {
	s.registerHandlerFn(slot, func() { s.deleteLanguagePackHandlers[slot] = handler })
}

func (s *server) RegisterHeartbeatHandler(slot uint16, handler HeartbeatHandler) {
	s.registerHandlerFn(slot, func() { s.heartbeatHandlers[slot] = handler })
}

func (s *server) RegisterAccessHandler(handler AccessHandler) {
	s.mu.Lock()
	if handler == nil {
		handler = allowAllAccessHandler
	}
	s.accessHandler = handler
	s.mu.Unlock()
}

func (s *server) RegisterProductStruct(slot uint16, product ProductStruct) {
	s.registerHandlerFn(slot, func() { s.productStructs[slot] = product })
}

// productForSlot returns the SDK-managed product struct registered for a slot,
// if any.
func (s *server) productForSlot(slot uint16) (ProductStruct, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	product, ok := s.productStructs[slot]
	return product, ok
}

func (s *server) InvokeGetDeviceHandler(transportContext TransportContext, req GetDeviceRequest, stream Stream[st2138.DeviceComponent]) StatusResult {
	// Wrap the transport's stream so Send also fails on server shutdown, not just
	// client disconnect. Cancellation is then transparent to the handler: it just
	// gets an error from the next Send once the server is shutting down.
	stream = shutdownStream[st2138.DeviceComponent]{inner: stream, shutdown: s.ctx}
	_, res := invokeHandler(s, transportContext, EndpointGetDevice, false, s.getDeviceHandlers, req.Slot,
		"No device defined at slot",
		func(handler DeviceHandler, ctx HandlerContext) (struct{}, StatusResult) {
			out := stream
			// Only touch the handler's chunks when the SDK manages the product for
			// this slot; otherwise deliver whatever the business logic produces
			// untouched. The wrapper enforces the product's st2138:mon read scope:
			// callers with mon get the SDK-managed product injected into Device
			// chunks (overwriting whatever the business logic sent); callers
			// without mon get no product param at all, even one the business logic
			// added, since the SDK-managed product struct is read-only and
			// mon-scoped regardless of who created it.
			if product, has := s.productForSlot(req.Slot); has {
				out = productDeviceStream{
					inner:   stream,
					product: product,
					hasMon:  ctx.HasReadScope(st2138.ScopeMon),
				}
			}
			return struct{}{}, handler(ctx, req, out)
		})
	return res
}

func (s *server) InvokeGetValueHandler(transportContext TransportContext, req GetValueRequest) (st2138.Value, StatusResult) {
	return withGate(s, transportContext, EndpointGetValue, false,
		func(ctx HandlerContext) (st2138.Value, StatusResult) {
			// The SDK owns product/* only when a product struct is registered for the
			// slot; answer it directly instead of passing to business logic. The
			// product param is mon-scoped, so gate the SDK-managed read on mon. When
			// no product struct is registered, product/* falls through to the
			// business-logic handler, which does its own scoping.
			if isProductOid(req.Fqoid) {
				if product, has := s.productForSlot(req.Slot); has {
					if scopeRes := ctx.RequireReadScope(st2138.ScopeMon); scopeRes.IsError() {
						return ReplyError[st2138.Value](scopeRes.Code, scopeRes.Error)
					}
					return productValueForOid(product, req.Fqoid)
				}
			}

			s.mu.Lock()
			handler, ok := s.getValueHandlers[req.Slot]
			paramHandler, hasParam := s.getParamHandlers[req.Slot]
			s.mu.Unlock()

			if ok {
				return handler(ctx, req)
			}

			// No GetValue handler: derive the value from GetParam when one is
			// registered, since a value is a strict subset of a param. The gate
			// above already authorized this as a GetValue request, so dispatch to
			// the GetParam handler directly (not InvokeGetParamHandler) to avoid
			// re-running the GetParam access gate. Logged at Debug: for slots
			// without a GetValue handler this is the normal path on every request,
			// so it must not spam Info.
			if hasParam {
				s.serverLogger.Debug("GetValue not registered; deriving from GetParam handler", "slot", req.Slot, "fqoid", req.Fqoid)
				param, res := paramHandler(ctx, GetParamRequest{Slot: req.Slot, Fqoid: req.Fqoid})
				if res.IsError() {
					return st2138.Value{}, res
				}
				if param.Proto == nil {
					return st2138.Value{}, StatusWithCode(StatusCodeInternal, "GetParam handler returned nil param")
				}
				return Reply(st2138.Value{Proto: param.Proto.GetValue()})
			}

			s.serverLogger.Warn("no handler registered for slot", "endpoint", EndpointGetValue, "slot", req.Slot)
			return st2138.Value{}, StatusWithCode(StatusCodeNotFound, "fqoid "+req.Fqoid+" not found at slot "+strconv.Itoa(int(req.Slot)))
		})
}

func (s *server) InvokeGetParamHandler(transportContext TransportContext, req GetParamRequest) (st2138.Param, StatusResult) {
	return withGate(s, transportContext, EndpointGetParam, false,
		func(ctx HandlerContext) (st2138.Param, StatusResult) {
			// The SDK owns product/* only when a product struct is registered for the
			// slot; answer it directly instead of passing to business logic. The
			// product param is mon-scoped, so gate the SDK-managed read on mon. This
			// runs before the handler lookup so a product-only slot (e.g. a
			// command-only device with no params, hence no GetParam handler) still
			// serves product/*. When no product struct is registered, product/*
			// falls through to the business-logic handler, which does its own scoping.
			if isProductOid(req.Fqoid) {
				if product, has := s.productForSlot(req.Slot); has {
					if scopeRes := ctx.RequireReadScope(st2138.ScopeMon); scopeRes.IsError() {
						return st2138.Param{}, scopeRes
					}
					return productParamForOid(product, req.Fqoid)
				}
			}

			s.mu.Lock()
			handler, ok := s.getParamHandlers[req.Slot]
			s.mu.Unlock()

			if !ok {
				s.serverLogger.Warn("no handler registered for slot", "endpoint", EndpointGetParam, "slot", req.Slot)
				return st2138.Param{}, StatusWithCode(StatusCodeNotFound, "fqoid "+req.Fqoid+" not found at slot "+strconv.Itoa(int(req.Slot)))
			}
			return handler(ctx, req)
		})
}

// InvokeSetValueHandler applies one or more parameter values for a slot. The
// registered SetValueHandler receives the full slice of entries so it can apply
// them atomically; single-value endpoints pass a one-element slice. Access
// checks run once for the whole batch.
func (s *server) InvokeSetValueHandler(transportContext TransportContext, req SetValueRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointSetValue, true, s.setValueHandlers, req.Slot,
		"no SetValue handler registered for slot "+strconv.Itoa(int(req.Slot)),
		func(handler SetValueHandler, ctx HandlerContext) (struct{}, StatusResult) {
			// The product struct is always read-only; reject any write targeting
			// product/* regardless of whether the SDK or business logic manages it.
			for _, entry := range req.Entries {
				if isProductOid(entry.Fqoid) {
					return struct{}{}, StatusWithCode(StatusCodePermissionDenied, "product params are read-only")
				}
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) InvokeReadAssetHandler(transportContext TransportContext, req ReadAssetRequest, stream Stream[st2138.Asset]) StatusResult {
	// Wrap the transport's stream so Send also fails on server shutdown, not just
	// client disconnect. Cancellation is then transparent to the handler: it just
	// gets an error from the next Send once the server is shutting down.
	stream = shutdownStream[st2138.Asset]{inner: stream, shutdown: s.ctx}
	_, res := invokeHandler(s, transportContext, EndpointReadAsset, false, s.readAssetHandlers, req.Slot,
		"fqoid "+req.Fqoid+" not found at slot "+strconv.Itoa(int(req.Slot)),
		func(handler ReadAssetHandler, ctx HandlerContext) (struct{}, StatusResult) {
			return struct{}{}, handler(ctx, req, stream)
		})
	return res
}

// enforceAssetWriteScope authorizes an asset mutation. Per ST 2138 the asset
// write endpoints (CreateAsset/UpdateAsset/DeleteAsset) require adm, op, or cfg
// write scope; a caller holding only the monitor scope is rejected even though
// it technically carries a write scope. The generic gate's coarse write check
// (HasAnyWriteScope) admits mon:w, so this narrower check runs per endpoint.
// HasWriteScope returns true for every scope when authorization is disabled, so
// this passes in that mode.
func enforceAssetWriteScope(ctx HandlerContext) StatusResult {
	if ctx.HasWriteScope(st2138.ScopeOp) || ctx.HasWriteScope(st2138.ScopeCfg) || ctx.HasWriteScope(st2138.ScopeAdm) {
		return StatusWithCode(StatusCodeOk, "")
	}
	return StatusWithCode(StatusCodePermissionDenied, "asset writes require adm, op, or cfg write scope")
}

func (s *server) InvokeCreateAssetHandler(transportContext TransportContext, req CreateAssetRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointCreateAsset, true, s.createAssetHandlers, req.Slot,
		"no CreateAsset handler registered for slot "+strconv.Itoa(int(req.Slot)),
		func(handler CreateAssetHandler, ctx HandlerContext) (struct{}, StatusResult) {
			if scopeRes := enforceAssetWriteScope(ctx); scopeRes.IsError() {
				return struct{}{}, scopeRes
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) InvokeUpdateAssetHandler(transportContext TransportContext, req UpdateAssetRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointUpdateAsset, true, s.updateAssetHandlers, req.Slot,
		"no UpdateAsset handler registered for slot "+strconv.Itoa(int(req.Slot)),
		func(handler UpdateAssetHandler, ctx HandlerContext) (struct{}, StatusResult) {
			if scopeRes := enforceAssetWriteScope(ctx); scopeRes.IsError() {
				return struct{}{}, scopeRes
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) InvokeDeleteAssetHandler(transportContext TransportContext, req DeleteAssetRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointDeleteAsset, true, s.deleteAssetHandlers, req.Slot,
		"no DeleteAsset handler registered for slot "+strconv.Itoa(int(req.Slot)),
		func(handler DeleteAssetHandler, ctx HandlerContext) (struct{}, StatusResult) {
			if scopeRes := enforceAssetWriteScope(ctx); scopeRes.IsError() {
				return struct{}{}, scopeRes
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) InvokeExecuteCommandHandler(transportContext TransportContext, req ExecuteCommandRequest, stream Stream[st2138.CommandResponse]) StatusResult {
	// When the caller opts out of responses, swap in a nullStream so any chunks
	// the handler sends are discarded here rather than each transport having to
	// gobble them itself. The handler still receives req.Respond and may skip sending.
	if !req.Respond {
		stream = nullStream[st2138.CommandResponse]{}
	}
	// wrap the transport's stream so Send also fails on server shutdown, not just
	// client disconnect. Cancellation is then transparent to the handler: it just
	// gets an error from the next Send once the server is shutting down.
	stream = shutdownStream[st2138.CommandResponse]{inner: stream, shutdown: s.ctx}
	_, res := invokeHandler(s, transportContext, EndpointExecuteCommand, true, s.executeCommandHandlers, req.Slot,
		"ExecuteCommand "+req.CommandFqoid+" not found at slot "+strconv.Itoa(int(req.Slot)),
		func(handler ExecuteCommandHandler, ctx HandlerContext) (struct{}, StatusResult) {
			return struct{}{}, handler(ctx, req, stream)
		})
	return res
}

func (s *server) InvokeParamInfoHandler(transportContext TransportContext, req ParamInfoRequest, stream Stream[st2138.ParamInfo]) StatusResult {
	// Wrap the transport's stream so Send also fails on server shutdown, not just
	// client disconnect. Cancellation is then transparent to the handler: it just
	// gets an error from the next Send once the server is shutting down.
	stream = shutdownStream[st2138.ParamInfo]{inner: stream, shutdown: s.ctx}
	_, res := withGate(s, transportContext, EndpointParamInfo, false,
		func(ctx HandlerContext) (struct{}, StatusResult) {
			// The SDK owns product/* ParamInfo only when a product struct is registered
			// for the slot; answer it directly instead of passing to business logic. The
			// product param is mon-scoped, so gate the SDK-managed request on mon. When
			// no product struct is registered, product/* falls through to the
			// business-logic handler, which does its own scoping.
			if isProductOid(req.OidPrefix) {
				if product, has := s.productForSlot(req.Slot); has {
					if scopeRes := ctx.RequireReadScope(st2138.ScopeMon); scopeRes.IsError() {
						return struct{}{}, scopeRes
					}
					return struct{}{}, productParamInfosForOid(product, req.OidPrefix, req.Recursive, stream)
				}
			}

			s.mu.Lock()
			handler, ok := s.paramInfoHandlers[req.Slot]
			paramHandler, hasParam := s.getParamHandlers[req.Slot]
			s.mu.Unlock()

			if ok {
				return struct{}{}, handler(ctx, req, stream)
			}

			// No ParamInfo handler: derive from GetParam when one is registered,
			// since a param carries every field a ParamInfo needs (recursive
			// requests flatten the returned param's own sub-param subtree). GetParam
			// is single-OID, so the whole-tree request (oidPrefix == "") cannot be
			// served this way and stays NotFound — it needs a real ParamInfo handler.
			// The gate above already authorized this as a ParamInfo request, so
			// dispatch to the GetParam handler directly (not InvokeGetParamHandler)
			// to avoid re-running the GetParam access gate. Logged at Debug: for
			// slots without a ParamInfo handler this is the normal path on every
			// request, so it must not spam Info.
			if hasParam && req.OidPrefix != "" {
				s.serverLogger.Debug("ParamInfo not registered; deriving from GetParam handler", "slot", req.Slot, "oidPrefix", req.OidPrefix)
				param, res := paramHandler(ctx, GetParamRequest{Slot: req.Slot, Fqoid: req.OidPrefix})
				if res.IsError() {
					return struct{}{}, res
				}
				return struct{}{}, ParamInfosFromParam(req.OidPrefix, &param, req.Recursive, stream)
			}

			s.serverLogger.Warn("no handler registered for slot", "endpoint", EndpointParamInfo, "slot", req.Slot)
			return struct{}{}, StatusWithCode(StatusCodeNotFound, "ParamInfo "+req.OidPrefix+" not found at slot "+strconv.Itoa(int(req.Slot)))
		})
	return res
}

func (s *server) InvokeListLanguagesHandler(transportContext TransportContext, req ListLanguagesRequest) ([]string, StatusResult) {
	return invokeHandler(s, transportContext, EndpointListLanguages, false, s.listLanguagesHandlers, req.Slot,
		"no ListLanguages handler registered for slot "+strconv.Itoa(int(req.Slot)),
		func(handler ListLanguagesHandler, ctx HandlerContext) ([]string, StatusResult) {
			return handler(ctx, req)
		})
}

func (s *server) InvokeReadLanguagePackHandler(transportContext TransportContext, req ReadLanguagePackRequest) (LanguagePack, StatusResult) {
	return invokeHandler(s, transportContext, EndpointReadLanguagePack, false, s.readLanguagePackHandlers, req.Slot,
		"LanguagePack "+req.Language+" not found at slot "+strconv.Itoa(int(req.Slot)),
		func(handler ReadLanguagePackHandler, ctx HandlerContext) (LanguagePack, StatusResult) {
			// Argument validation runs after the auth gate so an unauthorized
			// caller only ever sees a security error, never a validation hint.
			if req.Language == "" {
				return LanguagePack{}, StatusWithCode(StatusCodeInvalidArgument, "language is required")
			}
			return handler(ctx, req)
		})
}

func (s *server) InvokeCreateLanguagePackHandler(transportContext TransportContext, req CreateLanguagePackRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointCreateLanguagePack, true, s.createLanguagePackHandlers, req.Slot,
		"CreateLanguagePack handler not found at slot "+strconv.Itoa(int(req.Slot)),
		func(handler CreateLanguagePackHandler, ctx HandlerContext) (struct{}, StatusResult) {
			// Mutations require the adm scope specifically, not merely any write
			// scope, so narrow the coarse write gate now that we have the context.
			if res := ctx.RequireWriteScope(st2138.ScopeAdm); res.IsError() {
				return struct{}{}, res
			}
			// Argument validation runs after the scope check so an unauthorized
			// caller only ever sees a security error, never a validation hint.
			if req.Language == "" {
				return struct{}{}, StatusWithCode(StatusCodeInvalidArgument, "language is required")
			}
			if req.LanguagePack.Proto == nil {
				return struct{}{}, StatusWithCode(StatusCodeInvalidArgument, "language_pack is required")
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) InvokeUpdateLanguagePackHandler(transportContext TransportContext, req UpdateLanguagePackRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointUpdateLanguagePack, true, s.updateLanguagePackHandlers, req.Slot,
		"UpdateLanguagePack handler not found at slot "+strconv.Itoa(int(req.Slot)),
		func(handler UpdateLanguagePackHandler, ctx HandlerContext) (struct{}, StatusResult) {
			// Mutations require the adm scope specifically, not merely any write scope.
			if res := ctx.RequireWriteScope(st2138.ScopeAdm); res.IsError() {
				return struct{}{}, res
			}
			if req.Language == "" {
				return struct{}{}, StatusWithCode(StatusCodeInvalidArgument, "language is required")
			}
			if req.LanguagePack.Proto == nil {
				return struct{}{}, StatusWithCode(StatusCodeInvalidArgument, "language_pack is required")
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) InvokeDeleteLanguagePackHandler(transportContext TransportContext, req DeleteLanguagePackRequest) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointDeleteLanguagePack, true, s.deleteLanguagePackHandlers, req.Slot,
		"DeleteLanguagePack handler not found at slot "+strconv.Itoa(int(req.Slot)),
		func(handler DeleteLanguagePackHandler, ctx HandlerContext) (struct{}, StatusResult) {
			// Mutations require the adm scope specifically, not merely any write scope.
			if res := ctx.RequireWriteScope(st2138.ScopeAdm); res.IsError() {
				return struct{}{}, res
			}
			if req.Language == "" {
				return struct{}{}, StatusWithCode(StatusCodeInvalidArgument, "language is required")
			}
			return struct{}{}, handler(ctx, req)
		})
	return res
}

func (s *server) RegisterTransportConnection(transport Transport, transportContext TransportContext) (*Connection, StatusResult) {
	// enforce access checks
	handlerContext, res := s.invokeGateFn(transportContext, EndpointConnect, false)
	if res.IsError() {
		return nil, res
	}

	s.mu.Lock()
	defer s.mu.Unlock()

	// build the initial update with the current slots
	slots := s.getSlotsLocked()
	uint32Slots := make([]uint32, len(slots))
	for i, slot := range slots {
		uint32Slots[i] = uint32(slot)
	}
	initialUpdate := &protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: uint32Slots,
			},
		},
	}

	// register and it will send the initial update to the new connection before returning it
	// this ensures the transport receives the initial update before it starts processing the connection
	conn, res := s.connectionQueue.registerOwnedConnection(transport, handlerContext, initialUpdate)
	if res.IsError() {
		// the connection was not stored, so no teardown will release it
		handlerContext.release()
		return nil, res
	}
	return conn, res
}

// ShutdownTransportConnections allows transports to signal
// Useful if their graceful shutdown requires closing connections before the transport
// itself is shutdown
func (s *server) ShutdownTransportConnections(ctx context.Context, owner Transport) {
	shutdownCtx, cancel := s.boundedShutdownContext(ctx)
	defer cancel()
	s.connectionQueue.shutdownOwner(shutdownCtx, owner)
}

// DeregisterConnection removes a streaming connection
func (s *server) DeregisterConnection(connID int) {
	s.connectionQueue.deregisterConnection(connID)
}

// SetMaxConnections sets the maximum number of streaming connections
func (s *server) SetMaxConnections(max int) {
	s.connectionQueue.setMaxConnections(max)
}

// ConnectionCount returns the number of active streaming connections
func (s *server) ConnectionCount() int {
	return s.connectionQueue.connectionCount()
}

// BroadcastUpdate converts a native Go value into a proto PushUpdates message
// and sends it to all connected streaming clients. Business logic calls this with
// plain Go types; the proto serialization is handled internally.
// The value is deep-copied internally, so callers may release locks and freely
// mutate or discard the original input after BroadcastUpdate returns.
// Callers must still prevent concurrent mutation of value while this function is running.
func (s *server) BroadcastUpdate(slot uint16, oid string, value any, scope string) {
	protoValue, err := st2138.ToProto(value)
	if err != nil {
		s.serverLogger.Error("BroadcastUpdate: failed to convert value to proto", "error", err)
		return
	}
	update := &protos.PushUpdates{
		Slot: uint32(slot),
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid:   oid,
				Value: protoValue,
			},
		},
	}
	if !s.authzEnabled {
		scope = ""
	}
	s.connectionQueue.notifyUpdate(update, scope)
}

// StartHeartbeat begins periodic invocation of all registered heartbeat handlers.
// If a heartbeat is already running, it is stopped before starting the new one.
// If the interval is invalid (zero or negative), the existing heartbeat is preserved.
func (s *server) StartHeartbeat(interval time.Duration) {
	if interval <= 0 {
		s.serverLogger.Error("StartHeartbeat: invalid interval, heartbeat not changed", "interval", interval)
		return
	}

	hb := NewHeartbeat()
	hb.OnTick(func() {
		s.mu.Lock()
		handlers := make(map[uint16]HeartbeatHandler, len(s.heartbeatHandlers))
		maps.Copy(handlers, s.heartbeatHandlers)
		s.mu.Unlock()
		for slot, handler := range handlers {
			func() {
				defer func() {
					if r := recover(); r != nil {
						s.serverLogger.Error("panic in heartbeat handler", "slot", slot, "error", r)
					}
				}()
				handler(slot)
			}()
		}
	})

	// Grab and clear the old heartbeat under the lock.
	s.mu.Lock()
	old := s.heartbeat
	s.heartbeat = nil
	s.mu.Unlock()

	// Stop the old heartbeat outside the lock (blocks until its goroutine exits).
	if old != nil {
		old.Stop()
	}

	// Atomically store and start the new heartbeat so a concurrent StopHeartbeat
	// cannot miss the new instance.
	s.mu.Lock()
	s.heartbeat = hb
	err := hb.Start(interval)
	s.mu.Unlock()

	if err != nil {
		s.serverLogger.Error("Heartbeat failed to start", "interval", interval, "error", err)
	} else {
		s.serverLogger.Info("Heartbeat started", "interval", interval)
	}
}

// StopHeartbeat stops the heartbeat if one is running.
func (s *server) StopHeartbeat() {
	s.mu.Lock()
	hb := s.heartbeat
	s.heartbeat = nil
	s.mu.Unlock()

	if hb != nil {
		hb.Stop()
		s.serverLogger.Info("Heartbeat stopped")
	}
}
