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
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Handler function types used by both REST and gRPC servers.
type DeviceHandler func() (CatenaDevice, StatusResult)
type GetValueHandler func(slot uint16, fqoid string) (CatenaValue, StatusResult)
type SetValueHandler func(value any, slot uint16, fqoid string) StatusResult
type GetAssetHandler func(slot uint16, fqoid string) (CatenaAsset, StatusResult)
type ExecuteCommandHandler func(slot uint16, commandFqoid string, payload any) (CommandResult, StatusResult)
type ParamInfoHandler func(slot uint16, oidPrefix string, recursive bool) ([]CatenaParamInfo, StatusResult)
type HeartbeatHandler func(slot uint16)

var ErrServerStopped = errors.New("server is stopped")

// defaultServerMaxShutdownWait is a fallback safety cap for shutdown paths when
// callers pass an unbounded context. Caller-provided earlier deadlines still win.
const defaultServerMaxShutdownWait = 10 * time.Second

func ValidateSlot(slot uint32) (uint16, StatusResult) {
	if slot > uint32(math.MaxUint16) {
		return 0, StatusWithCode(INVALID_ARGUMENT, fmt.Sprintf("invalid slot number: %d", slot))

	}
	return uint16(slot), StatusWithCode(OK, "")
}

func ValidateSlotString(slot string) (uint16, StatusResult) {
	slotInt, err := strconv.ParseUint(slot, 10, 32)
	if err != nil {
		return 0, StatusWithCode(INVALID_ARGUMENT, fmt.Sprintf("invalid slot string: %s", slot))
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
	RegisterSetValueHandler(slot uint16, handler SetValueHandler)
	RegisterGetAssetHandler(slot uint16, handler GetAssetHandler)
	RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler)
	RegisterParamInfoHandler(slot uint16, handler ParamInfoHandler)
	RegisterHeartbeatHandler(slot uint16, handler HeartbeatHandler)

	SetMaxConnections(max int)
	ConnectionCount() int
	BroadcastUpdate(slot uint16, oid string, value any)

	StartHeartbeat(interval time.Duration)
	StopHeartbeat()
}

// interface of funcs that Transports use to interact with the server without circular imports
type ServerRuntime interface {
	GetSlots() []uint16
	InvokeGetDeviceHandler(slot uint16) (CatenaDevice, StatusResult)
	InvokeGetValueHandler(slot uint16, fqoid string) (CatenaValue, StatusResult)
	InvokeSetValueHandler(value any, slot uint16, fqoid string) StatusResult
	InvokeGetAssetHandler(slot uint16, fqoid string) (CatenaAsset, StatusResult)
	InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any) (CommandResult, StatusResult)
	InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool) ([]CatenaParamInfo, StatusResult)
	RegisterTransportConnection(owner any) (int, *Connection)
	DeregisterConnection(connID int)
}

var _ Server = (*server)(nil)
var _ ServerRuntime = (*server)(nil)

type server struct {
	mu                     sync.Mutex
	ctx                    context.Context
	ctxCancel              context.CancelFunc
	maxShutdownWait        time.Duration
	shutdown               bool
	stopped                chan struct{}
	slots                  map[uint16]struct{}
	getDeviceHandlers      map[uint16]DeviceHandler
	getValueHandlers       map[uint16]GetValueHandler
	setValueHandlers       map[uint16]SetValueHandler
	getAssetHandlers       map[uint16]GetAssetHandler
	executeCommandHandlers map[uint16]ExecuteCommandHandler
	paramInfoHandlers      map[uint16]ParamInfoHandler
	heartbeatHandlers      map[uint16]HeartbeatHandler
	connectionQueue        connectionQueueInterface
	heartbeat              *Heartbeat
	transports             []Transport
}

func NewServer(maxConnections int) Server {
	ctx, cancel := context.WithCancel(context.Background())
	return &server{
		ctx:                    ctx,
		ctxCancel:              cancel,
		maxShutdownWait:        defaultServerMaxShutdownWait, // override in unittests if needed
		shutdown:               false,
		stopped:                make(chan struct{}),
		slots:                  make(map[uint16]struct{}),
		getDeviceHandlers:      make(map[uint16]DeviceHandler),
		getValueHandlers:       make(map[uint16]GetValueHandler),
		setValueHandlers:       make(map[uint16]SetValueHandler),
		getAssetHandlers:       make(map[uint16]GetAssetHandler),
		executeCommandHandlers: make(map[uint16]ExecuteCommandHandler),
		paramInfoHandlers:      make(map[uint16]ParamInfoHandler),
		heartbeatHandlers:      make(map[uint16]HeartbeatHandler),
		connectionQueue:        newConnectionQueue(maxConnections),
		transports:             []Transport{},
	}
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

func (s *server) GetSlots() []uint16 {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.getSlotsLocked()
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
	})
}

// Handler registration methods
func (s *server) RegisterGetDeviceHandler(slot uint16, handler DeviceHandler) {
	s.mu.Lock()
	s.getDeviceHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) RegisterGetValueHandler(slot uint16, handler GetValueHandler) {
	s.mu.Lock()
	s.getValueHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) RegisterSetValueHandler(slot uint16, handler SetValueHandler) {
	s.mu.Lock()
	s.setValueHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) RegisterGetAssetHandler(slot uint16, handler GetAssetHandler) {
	s.mu.Lock()
	s.getAssetHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler) {
	s.mu.Lock()
	s.executeCommandHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) RegisterParamInfoHandler(slot uint16, handler ParamInfoHandler) {
	s.mu.Lock()
	s.paramInfoHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) RegisterHeartbeatHandler(slot uint16, handler HeartbeatHandler) {
	s.mu.Lock()
	s.heartbeatHandlers[slot] = handler
	newSlot := s.registerSlotLocked(slot)
	s.mu.Unlock()

	if newSlot {
		s.notifySlotsAdded(slot)
	}
}

func (s *server) InvokeGetDeviceHandler(slot uint16) (CatenaDevice, StatusResult) {
	s.mu.Lock()
	handler, ok := s.getDeviceHandlers[slot]
	s.mu.Unlock()

	if ok {
		return handler()
	}
	// TODO: lookup default handler for slot
	logger.Warning("GetDeviceHandler called - no handler registered for this slot")
	return ReplyError[CatenaDevice](NOT_FOUND, "No device defined at slot")
}

func (s *server) InvokeGetValueHandler(slot uint16, fqoid string) (CatenaValue, StatusResult) {
	s.mu.Lock()
	handler, ok := s.getValueHandlers[slot]
	s.mu.Unlock()

	if ok {
		return handler(slot, fqoid)
	}
	// TODO: lookup default handler for slot
	logger.Warning("GetValueHandler called - no handler registered for this slot", "slot", slot, "fqoid", fqoid)
	return ReplyError[CatenaValue](NOT_FOUND, "fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func (s *server) InvokeSetValueHandler(value any, slot uint16, fqoid string) StatusResult {
	s.mu.Lock()
	handler, ok := s.setValueHandlers[slot]
	s.mu.Unlock()

	if ok {
		return handler(value, slot, fqoid)
	}
	// TODO: lookup default handler for slot
	logger.Warning("SetValueHandler called - no handler registered for this slot", "slot", slot, "fqoid", fqoid)
	return StatusWithCode(NOT_FOUND, "fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func (s *server) InvokeGetAssetHandler(slot uint16, fqoid string) (CatenaAsset, StatusResult) {
	s.mu.Lock()
	handler, ok := s.getAssetHandlers[slot]
	s.mu.Unlock()

	if ok {
		return handler(slot, fqoid)
	}
	// TODO: lookup default handler for slot
	logger.Warning("GetAssetHandler called - no handler registered for this slot", "slot", slot, "fqoid", fqoid)
	return ReplyError[CatenaAsset](NOT_FOUND, "fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func (s *server) InvokeExecuteCommandHandler(slot uint16, commandFqoid string, payload any) (CommandResult, StatusResult) {
	s.mu.Lock()
	handler, ok := s.executeCommandHandlers[slot]
	s.mu.Unlock()

	if ok {
		return handler(slot, commandFqoid, payload)
	}
	// TODO: lookup default handler for slot
	logger.Warning("ExecuteCommandHandler called - no handler registered for this slot", "slot", slot, "commandFqoid", commandFqoid)
	return CommandError(NOT_FOUND, "ExecuteCommand "+commandFqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func (s *server) InvokeParamInfoHandler(slot uint16, oidPrefix string, recursive bool) ([]CatenaParamInfo, StatusResult) {
	s.mu.Lock()
	handler, ok := s.paramInfoHandlers[slot]
	s.mu.Unlock()

	if ok {
		return handler(slot, oidPrefix, recursive)
	}
	// TODO: lookup default handler for slot
	logger.Warning("ParamInfoHandler called - no handler registered for this slot", "slot", slot, "oidPrefix", oidPrefix)
	return nil, StatusWithCode(NOT_FOUND, "ParamInfo "+oidPrefix+" not found at slot "+strconv.Itoa(int(slot)))
}

func (s *server) RegisterTransportConnection(owner any) (int, *Connection) {
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
	return s.connectionQueue.registerOwnedConnection(owner, initialUpdate)
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
func (s *server) BroadcastUpdate(slot uint16, oid string, value any) {
	protoValue, res := ToProto(value)
	if res.Code != OK {
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
	s.connectionQueue.notifyUpdate(update)
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
