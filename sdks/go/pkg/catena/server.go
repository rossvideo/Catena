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
 * @date 2026-05-14
 */

package catena

import (
	"context"
	"errors"
	"fmt"
	"maps"
	"math"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

type EndpointType int

const (
	EndpointGetSlots EndpointType = iota
	EndpointGetDevice
	EndpointGetValue
	EndpointGetParam
	EndpointSetValue
	EndpointGetAsset
	EndpointExecuteCommand
	EndpointParamInfo
	EndpointLanguagePack
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
	case EndpointGetAsset:
		return "GetAsset"
	case EndpointExecuteCommand:
		return "ExecuteCommand"
	case EndpointParamInfo:
		return "ParamInfo"
	case EndpointLanguagePack:
		return "LanguagePack"
	case EndpointConnect:
		return "Connect"
	case EndpointListLanguages:
		return "ListLanguages"
	default:
		return "Unknown"
	}
}

// Handler function types used by both REST and gRPC servers.
type DeviceHandler func(slot uint16, ctx HandlerContext) (Device, StatusResult)
type GetValueHandler func(slot uint16, fqoid string, ctx HandlerContext) (Value, StatusResult)

// GetParamHandler returns the full parameter (metadata + value) for a slot/fqoid.
type GetParamHandler func(slot uint16, fqoid string, ctx HandlerContext) (Param, StatusResult)

// SetValueEntry is a single fqoid/value pair within a SetValue request.
type SetValueEntry struct {
	Fqoid string
	Value any
}

// SetValueHandler applies one or more parameter values for a slot. Single-value
// endpoints invoke it with a one-element slice; multi-value endpoints pass the
// full slice so the handler can apply them atomically (all-or-nothing).
type SetValueHandler func(slot uint16, entries []SetValueEntry, ctx HandlerContext) StatusResult
type GetAssetHandler func(slot uint16, fqoid string, ctx HandlerContext) (Asset, StatusResult)
type ExecuteCommandHandler func(slot uint16, commandFqoid string, payload any, respond bool, ctx HandlerContext, stream Stream[CommandResult]) StatusResult

// ParamInfoHandler streams parameter information for a slot. The handler emits
// each ParamInfo chunk through stream.Send and returns a terminal StatusResult.
// A Send error means the chunk could not be delivered - the client may have
// disconnected, or the transport hit an encoding or write failure - so the
// handler should stop and return.
type ParamInfoHandler func(slot uint16, oidPrefix string, recursive bool, ctx HandlerContext, stream Stream[ParamInfo]) StatusResult

// ListLanguagesHandler returns the language codes supported by the device model
// at a slot (e.g. ["en", "fr"]). An empty slice indicates no multi-lingual support.
type ListLanguagesHandler func(slot uint16, ctx HandlerContext) ([]string, StatusResult)

type LanguagePackHandler func(slot uint16, language string, ctx HandlerContext) (LanguagePack, StatusResult)
type AddLanguageHandler func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult
type UpdateLanguageHandler func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult
type DeleteLanguageHandler func(slot uint16, language string, ctx HandlerContext) StatusResult
type HeartbeatHandler func(slot uint16)
type AccessHandler func(endpointType EndpointType, ctx HandlerContext) bool

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

	RegisterGetDeviceHandler(slot uint16, handler DeviceHandler)
	RegisterGetValueHandler(slot uint16, handler GetValueHandler)
	RegisterGetParamHandler(slot uint16, handler GetParamHandler)
	RegisterSetValueHandler(slot uint16, handler SetValueHandler)
	RegisterGetAssetHandler(slot uint16, handler GetAssetHandler)
	RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler)
	RegisterParamInfoHandler(slot uint16, handler ParamInfoHandler)
	RegisterListLanguagesHandler(slot uint16, handler ListLanguagesHandler)
	RegisterLanguagePackHandler(slot uint16, handler LanguagePackHandler)
	RegisterAddLanguageHandler(slot uint16, handler AddLanguageHandler)
	RegisterUpdateLanguageHandler(slot uint16, handler UpdateLanguageHandler)
	RegisterDeleteLanguageHandler(slot uint16, handler DeleteLanguageHandler)
	RegisterHeartbeatHandler(slot uint16, handler HeartbeatHandler)
	RegisterAccessHandler(handler AccessHandler)

	// RegisterProductStruct hands the mandatory product struct for a slot to the
	// SDK. Once registered, the SDK injects the product param into the device on
	// GetDevice, answers GetValue and ParamInfo for product/*, and rejects
	// SetValue writes to product/* with StatusCodePermissionDenied — business
	// logic no longer needs to handle the product struct.
	// Note: If you do not register a product struct for a slot, requests for product/* values or info will fail with StatusCodeNotFound.
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
	InvokeGetDeviceHandler(slot uint16, transportContext TransportContext) (Device, StatusResult)
	InvokeGetValueHandler(slot uint16, fqoid string, transportContext TransportContext) (Value, StatusResult)
	InvokeGetParamHandler(slot uint16, fqoid string, transportContext TransportContext) (Param, StatusResult)
	InvokeSetValueHandler(slot uint16, entries []SetValueEntry, transportContext TransportContext) StatusResult
	InvokeGetAssetHandler(slot uint16, fqoid string, transportContext TransportContext) (Asset, StatusResult)
	InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any, respond bool, stream Stream[CommandResult], transportContext TransportContext) StatusResult
	InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool, stream Stream[ParamInfo], transportContext TransportContext) StatusResult
	InvokeListLanguagesHandler(slot uint16, transportContext TransportContext) ([]string, StatusResult)
	InvokeLanguagePackHandler(slot uint16, language string, transportContext TransportContext) (LanguagePack, StatusResult)
	InvokeAddLanguageHandler(slot uint16, language string, languagePack LanguagePack, transportContext TransportContext) StatusResult
	InvokeUpdateLanguageHandler(slot uint16, language string, languagePack LanguagePack, transportContext TransportContext) StatusResult
	InvokeDeleteLanguageHandler(slot uint16, language string, transportContext TransportContext) StatusResult
	RegisterTransportConnection(transport Transport, transportContext TransportContext) (*Connection, StatusResult)
	ShutdownTransportConnections(ctx context.Context, transport Transport)
	DeregisterConnection(connID int)
}

var _ Server = (*server)(nil)
var _ ServerRuntime = (*server)(nil)

type server struct {
	options                ServerOptions
	mu                     sync.Mutex
	ctx                    context.Context
	ctxCancel              context.CancelFunc
	authzEnabled           bool
	jwtValidator           jwtValidatorInterface
	maxShutdownWait        time.Duration
	shutdown               bool
	stopped                chan struct{}
	slots                  map[uint16]struct{}
	getDeviceHandlers      map[uint16]DeviceHandler
	getValueHandlers       map[uint16]GetValueHandler
	getParamHandlers       map[uint16]GetParamHandler
	setValueHandlers       map[uint16]SetValueHandler
	getAssetHandlers       map[uint16]GetAssetHandler
	executeCommandHandlers map[uint16]ExecuteCommandHandler
	paramInfoHandlers      map[uint16]ParamInfoHandler
	listLanguagesHandlers  map[uint16]ListLanguagesHandler
	languagePackHandlers   map[uint16]LanguagePackHandler
	addLanguageHandlers    map[uint16]AddLanguageHandler
	updateLanguageHandlers map[uint16]UpdateLanguageHandler
	deleteLanguageHandlers map[uint16]DeleteLanguageHandler
	heartbeatHandlers      map[uint16]HeartbeatHandler
	productStructs         map[uint16]ProductStruct // SDK-managed product per slot
	accessHandler          AccessHandler            // optional fallback for slots without specific handlers
	connectionQueue        connectionQueueInterface
	heartbeat              *Heartbeat
	transports             []Transport

	// Redirection seams for the generic registerHandler/invokeHandler helpers.
	// Generic functions cannot be stored in fields, so the helpers stay generic
	// but route their non-generic work through these fields, which unit tests may
	// overwrite to mock registration bookkeeping or the authorization gate.
	registerHandlerFn func(slot uint16, store func())
	invokeGateFn      func(transportContext TransportContext, endpoint EndpointType, writeAccess bool) (HandlerContext, StatusResult)
}

func NewServer(opts config.ServerOptions) (Server, error) {
	ctx, cancel := context.WithCancel(context.Background())

	var validator jwtValidatorInterface
	if opts.AuthzEnabled {
		var err error
		validator, err = newJwtValidator(ctx, opts.JwtOptions)
		if err != nil {
			cancel()
			return nil, fmt.Errorf("create jwt validator: %w", err)
		}
	}

	s := &server{
		options:                opts,
		ctx:                    ctx,
		ctxCancel:              cancel,
		authzEnabled:           opts.AuthzEnabled,
		jwtValidator:           validator,
		maxShutdownWait:        defaultServerMaxShutdownWait, // override in unittests if needed
		shutdown:               false,
		stopped:                make(chan struct{}),
		slots:                  make(map[uint16]struct{}),
		getDeviceHandlers:      make(map[uint16]DeviceHandler),
		getValueHandlers:       make(map[uint16]GetValueHandler),
		getParamHandlers:       make(map[uint16]GetParamHandler),
		setValueHandlers:       make(map[uint16]SetValueHandler),
		getAssetHandlers:       make(map[uint16]GetAssetHandler),
		executeCommandHandlers: make(map[uint16]ExecuteCommandHandler),
		paramInfoHandlers:      make(map[uint16]ParamInfoHandler),
		listLanguagesHandlers:  make(map[uint16]ListLanguagesHandler),
		languagePackHandlers:   make(map[uint16]LanguagePackHandler),
		addLanguageHandlers:    make(map[uint16]AddLanguageHandler),
		updateLanguageHandlers: make(map[uint16]UpdateLanguageHandler),
		deleteLanguageHandlers: make(map[uint16]DeleteLanguageHandler),
		heartbeatHandlers:      make(map[uint16]HeartbeatHandler),
		productStructs:         make(map[uint16]ProductStruct),
		accessHandler:          allowAllAccessHandler,
		connectionQueue:        newConnectionQueue(opts.MaxConnections),
		transports:             []Transport{},
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
		logger.Error("Error shutting down transport", "error", err)
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
			logger.Error("Error shutting down transport", "error", err)
		}
	}

	// tell the connection queue to drain any remaining connections
	s.connectionQueue.shutdown(shutdownCtx)

	close(s.stopped)
}

func (s *server) IsDev() bool {
	return s.options.IsDev
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
		logger.Warning("Failed to validate access token", "error", err)
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
	var zero T

	handlerContext, res := s.invokeGateFn(transportContext, endpoint, writeAccess)
	if res.IsError() {
		return zero, res
	}
	// The gate built the request context (so the access handler could observe it);
	// this unary request ends when call returns, so release it here.
	defer handlerContext.release()

	s.mu.Lock()
	handler, ok := handlers[slot]
	s.mu.Unlock()

	//TODO: add default handler lookup when custom default handlers are supported
	if !ok {
		logger.Warning("no handler registered for slot", "endpoint", endpoint, "slot", slot)
		return zero, StatusWithCode(StatusCodeNotFound, notFound)
	}

	return call(handler, handlerContext)
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

func (s *server) RegisterGetAssetHandler(slot uint16, handler GetAssetHandler) {
	s.registerHandlerFn(slot, func() { s.getAssetHandlers[slot] = handler })
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

func (s *server) RegisterLanguagePackHandler(slot uint16, handler LanguagePackHandler) {
	s.registerHandlerFn(slot, func() { s.languagePackHandlers[slot] = handler })
}

func (s *server) RegisterAddLanguageHandler(slot uint16, handler AddLanguageHandler) {
	s.registerHandlerFn(slot, func() { s.addLanguageHandlers[slot] = handler })
}

func (s *server) RegisterUpdateLanguageHandler(slot uint16, handler UpdateLanguageHandler) {
	s.registerHandlerFn(slot, func() { s.updateLanguageHandlers[slot] = handler })
}

func (s *server) RegisterDeleteLanguageHandler(slot uint16, handler DeleteLanguageHandler) {
	s.registerHandlerFn(slot, func() { s.deleteLanguageHandlers[slot] = handler })
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

func (s *server) InvokeGetDeviceHandler(slot uint16, transportContext TransportContext) (Device, StatusResult) {
	return invokeHandler(s, transportContext, EndpointGetDevice, false, s.getDeviceHandlers, slot,
		"No device defined at slot",
		func(handler DeviceHandler, ctx HandlerContext) (Device, StatusResult) {
			device, res := handler(slot, ctx)
			// Overwrite the product/* fields in whatever the business logic returned
			// with the SDK-managed product struct, if one is registered for the slot.
			if res.IsOk() {
				if product, has := s.productForSlot(slot); has && device.Proto != nil {
					device.WithParam(ProductOid, ProductParam(product))
				}
			}
			return device, res
		})
}

func (s *server) InvokeGetValueHandler(slot uint16, fqoid string, transportContext TransportContext) (Value, StatusResult) {
	return invokeHandler(s, transportContext, EndpointGetValue, false, s.getValueHandlers, slot,
		"fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)),
		func(handler GetValueHandler, ctx HandlerContext) (Value, StatusResult) {
			// The SDK owns product/* when a product struct is registered for the slot;
			// answer it directly instead of passing to business logic.
			if isProductOid(fqoid) {
				if product, has := s.productForSlot(slot); has {
					return productValueForOid(product, fqoid)
				}
			}
			// handle as normal
			return handler(slot, fqoid, ctx)
		})
}

func (s *server) InvokeGetParamHandler(slot uint16, fqoid string, transportContext TransportContext) (Param, StatusResult) {
	return invokeHandler(s, transportContext, EndpointGetParam, false, s.getParamHandlers, slot,
		"fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)),
		func(handler GetParamHandler, ctx HandlerContext) (Param, StatusResult) {
			// The SDK owns product/* when a product struct is registered for the slot;
			// answer it directly instead of passing to business logic.
			if isProductOid(fqoid) {
				if product, has := s.productForSlot(slot); has {
					return productParamForOid(product, fqoid)
				}
			}
			// handle as normal
			return handler(slot, fqoid, ctx)
		})
}

// InvokeSetValueHandler applies one or more parameter values for a slot. The
// registered SetValueHandler receives the full slice of entries so it can apply
// them atomically; single-value endpoints pass a one-element slice. Access
// checks run once for the whole batch.
func (s *server) InvokeSetValueHandler(slot uint16, entries []SetValueEntry, transportContext TransportContext) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointSetValue, true, s.setValueHandlers, slot,
		"no SetValue handler registered for slot "+strconv.Itoa(int(slot)),
		func(handler SetValueHandler, ctx HandlerContext) (struct{}, StatusResult) {
			// The product struct is always read-only; reject any write targeting
			// product/* regardless of whether the SDK or business logic manages it.
			for _, entry := range entries {
				if isProductOid(entry.Fqoid) {
					return struct{}{}, StatusWithCode(StatusCodePermissionDenied, "product params are read-only")
				}
			}
			return struct{}{}, handler(slot, entries, ctx)
		})
	return res
}

func (s *server) InvokeGetAssetHandler(slot uint16, fqoid string, transportContext TransportContext) (Asset, StatusResult) {
	return invokeHandler(s, transportContext, EndpointGetAsset, false, s.getAssetHandlers, slot,
		"fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)),
		func(handler GetAssetHandler, ctx HandlerContext) (Asset, StatusResult) {
			return handler(slot, fqoid, ctx)
		})
}

func (s *server) InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any, respond bool, stream Stream[CommandResult], transportContext TransportContext) StatusResult {
	// When the caller opts out of responses, swap in a nullStream so any chunks
	// the handler sends are discarded here rather than each transport having to
	// gobble them itself. The handler still receives respond and may skip sending.
	if !respond {
		stream = nullStream[CommandResult]{}
	}
	// wrap the transport's stream so Send also fails on server shutdown, not just
	// client disconnect. Cancellation is then transparent to the handler: it just
	// gets an error from the next Send once the server is shutting down.
	stream = shutdownStream[CommandResult]{inner: stream, shutdown: s.ctx}
	_, res := invokeHandler(s, transportContext, EndpointExecuteCommand, true, s.executeCommandHandlers, slot,
		"ExecuteCommand "+commandFqoid+" not found at slot "+strconv.Itoa(int(slot)),
		func(handler ExecuteCommandHandler, ctx HandlerContext) (struct{}, StatusResult) {
			return struct{}{}, handler(slot, commandFqoid, payload, respond, ctx, stream)
		})
	return res
}

func (s *server) InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool, stream Stream[ParamInfo], transportContext TransportContext) StatusResult {
	// Wrap the transport's stream so Send also fails on server shutdown, not just
	// client disconnect. Cancellation is then transparent to the handler: it just
	// gets an error from the next Send once the server is shutting down.
	stream = shutdownStream[ParamInfo]{inner: stream, shutdown: s.ctx}
	_, res := invokeHandler(s, transportContext, EndpointParamInfo, false, s.paramInfoHandlers, slot,
		"ParamInfo "+oidPrefix+" not found at slot "+strconv.Itoa(int(slot)),
		func(handler ParamInfoHandler, ctx HandlerContext) (struct{}, StatusResult) {
			// The SDK owns product/* ParamInfo when a product struct is registered for
			// the slot; answer it directly instead of passing to business logic.
			if isProductOid(oidPrefix) {
				if product, has := s.productForSlot(slot); has {
					return struct{}{}, productParamInfosForOid(product, oidPrefix, recursive, stream)
				}
			}
			// normal processing for non-product/* ParamInfo
			return struct{}{}, handler(slot, oidPrefix, recursive, ctx, stream)
		})
	return res
}

func (s *server) InvokeListLanguagesHandler(slot uint16, transportContext TransportContext) ([]string, StatusResult) {
	return invokeHandler(s, transportContext, EndpointListLanguages, false, s.listLanguagesHandlers, slot,
		"no ListLanguages handler registered for slot "+strconv.Itoa(int(slot)),
		func(handler ListLanguagesHandler, ctx HandlerContext) ([]string, StatusResult) {
			return handler(slot, ctx)
		})
}

func (s *server) InvokeLanguagePackHandler(slot uint16, language string, transportContext TransportContext) (LanguagePack, StatusResult) {
	return invokeHandler(s, transportContext, EndpointLanguagePack, false, s.languagePackHandlers, slot,
		"LanguagePack "+language+" not found at slot "+strconv.Itoa(int(slot)),
		func(handler LanguagePackHandler, ctx HandlerContext) (LanguagePack, StatusResult) {
			return handler(slot, language, ctx)
		})
}

func (s *server) InvokeAddLanguageHandler(slot uint16, language string, languagePack LanguagePack, transportContext TransportContext) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointLanguagePack, true, s.addLanguageHandlers, slot,
		"AddLanguage handler not found at slot "+strconv.Itoa(int(slot)),
		func(handler AddLanguageHandler, ctx HandlerContext) (struct{}, StatusResult) {
			return struct{}{}, handler(slot, language, languagePack, ctx)
		})
	return res
}

func (s *server) InvokeUpdateLanguageHandler(slot uint16, language string, languagePack LanguagePack, transportContext TransportContext) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointLanguagePack, true, s.updateLanguageHandlers, slot,
		"UpdateLanguage handler not found at slot "+strconv.Itoa(int(slot)),
		func(handler UpdateLanguageHandler, ctx HandlerContext) (struct{}, StatusResult) {
			return struct{}{}, handler(slot, language, languagePack, ctx)
		})
	return res
}

func (s *server) InvokeDeleteLanguageHandler(slot uint16, language string, transportContext TransportContext) StatusResult {
	_, res := invokeHandler(s, transportContext, EndpointLanguagePack, true, s.deleteLanguageHandlers, slot,
		"DeleteLanguage handler not found at slot "+strconv.Itoa(int(slot)),
		func(handler DeleteLanguageHandler, ctx HandlerContext) (struct{}, StatusResult) {
			return struct{}{}, handler(slot, language, ctx)
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
	protoValue, res := ToProto(value)
	if res.Code != StatusCodeOk {
		logger.Error("BroadcastUpdate: failed to convert value to proto", "error", res.Error)
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
		logger.Error("StartHeartbeat: invalid interval, heartbeat not changed", "interval", interval)
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
						logger.Error("panic in heartbeat handler", "slot", slot, "error", r)
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
		logger.Error("Heartbeat failed to start", "interval", interval, "error", err)
	} else {
		logger.Info("Heartbeat started", "interval", interval)
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
		logger.Info("Heartbeat stopped")
	}
}
