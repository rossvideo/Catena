package internal

import (
	"sync"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Handlers now return (CatenaValue, StatusResult) so the server can respond consistently.
type DeviceHandler func() (catena.CatenaDevice, catena.StatusResult)
type GetValueHandler func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult)
type SetValueHandler func(value any, slot int, fqoid string) catena.StatusResult
type GetAssetHandler func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult)
type ExecuteCommandHandler func(slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult)

type BaseServer struct {
	Mu                     sync.Mutex
	Slots                  []int
	GetDeviceHandlers      map[int]DeviceHandler
	GetValueHandlers       map[int]GetValueHandler
	SetValueHandlers       map[int]SetValueHandler
	GetAssetHandlers       map[int]GetAssetHandler
	ExecuteCommandHandlers map[int]ExecuteCommandHandler
	connectionQueue        *ConnectionQueue
}

// Default handlers that return "not implemented"
func DefaultDeviceHandler() (catena.CatenaDevice, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "GetDevice not implemented")
}

func DefaultGetValueHandler(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaValue](catena.UNIMPLEMENTED, "GetValue not implemented")
}

func DefaultSetValueHandler(value any, slot int, fqoid string) catena.StatusResult {
	return catena.StatusWithCode(catena.UNIMPLEMENTED, "SetValue not implemented")
}

func DefaultGetAssetHandler(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "GetAsset not implemented")
}

func DefaultExecuteCommandHandler(slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyError[catena.CatenaValue](catena.UNIMPLEMENTED, "ExecuteCommand not implemented")
}

// Handler registration methods
func (bs *BaseServer) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.GetDeviceHandlers[slot] = handler
}

func (bs *BaseServer) RegisterGetValueHandler(slot int, handler GetValueHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.GetValueHandlers[slot] = handler
}

func (bs *BaseServer) RegisterSetValueHandler(slot int, handler SetValueHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.SetValueHandlers[slot] = handler
}

func (bs *BaseServer) RegisterGetAssetHandler(slot int, handler GetAssetHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.GetAssetHandlers[slot] = handler
}

func (bs *BaseServer) RegisterExecuteCommandHandler(slot int, handler ExecuteCommandHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.ExecuteCommandHandlers[slot] = handler
}

// Lookup helper functions
func (bs *BaseServer) LookupGetValueHandler(slot int) GetValueHandler {
	if handler, ok := bs.GetValueHandlers[slot]; ok {
		return handler
	}
	return DefaultGetValueHandler
}

func (bs *BaseServer) LookupSetValueHandler(slot int) SetValueHandler {
	if handler, ok := bs.SetValueHandlers[slot]; ok {
		return handler
	}
	return DefaultSetValueHandler
}

func (bs *BaseServer) LookupGetAssetHandler(slot int) GetAssetHandler {
	if handler, ok := bs.GetAssetHandlers[slot]; ok {
		return handler
	}
	return DefaultGetAssetHandler
}

func (bs *BaseServer) LookupExecuteCommandHandler(slot int) ExecuteCommandHandler {
	if handler, ok := bs.ExecuteCommandHandlers[slot]; ok {
		return handler
	}
	return DefaultExecuteCommandHandler
}

func NewBaseServer(slots []int, maxConnections int) *BaseServer {
	bs := &BaseServer{
		Slots:                  slots,
		GetDeviceHandlers:      make(map[int]DeviceHandler),
		GetValueHandlers:       make(map[int]GetValueHandler),
		SetValueHandlers:       make(map[int]SetValueHandler),
		GetAssetHandlers:       make(map[int]GetAssetHandler),
		ExecuteCommandHandlers: make(map[int]ExecuteCommandHandler),
		connectionQueue:        NewConnectionQueue(maxConnections),
	}

	for _, slot := range slots {
		bs.RegisterGetDeviceHandler(slot, DefaultDeviceHandler)
		bs.RegisterGetValueHandler(slot, DefaultGetValueHandler)
		bs.RegisterSetValueHandler(slot, DefaultSetValueHandler)
		bs.RegisterGetAssetHandler(slot, DefaultGetAssetHandler)
		bs.RegisterExecuteCommandHandler(slot, DefaultExecuteCommandHandler)
	}

	return bs
}

// RegisterConnection registers a new streaming connection
func (bs *BaseServer) RegisterConnection() (int, *Connection) {
	return bs.connectionQueue.RegisterConnection()
}

// DeregisterConnection removes a streaming connection
func (bs *BaseServer) DeregisterConnection(connID int) {
	bs.connectionQueue.DeregisterConnection(connID)
}

// NotifyUpdate broadcasts an update to all connected clients
func (bs *BaseServer) NotifyUpdate(update *protos.PushUpdates) {
	bs.connectionQueue.NotifyUpdate(update)
}

// SetMaxConnections sets the maximum number of streaming connections
func (bs *BaseServer) SetMaxConnections(max int) {
	bs.connectionQueue.SetMaxConnections(max)
}

// ShutdownConnections gracefully shuts down all streaming connections
func (bs *BaseServer) ShutdownConnections() {
	bs.connectionQueue.Shutdown()
}

// ConnectionCount returns the number of active streaming connections
func (bs *BaseServer) ConnectionCount() int {
	return bs.connectionQueue.ConnectionCount()
}

// BroadcastUpdate converts a native Go value into a proto PushUpdates message
// and sends it to all connected streaming clients. Business logic calls this with
// plain Go types; the proto serialization is handled internally.
func (bs *BaseServer) BroadcastUpdate(slot int, oid string, value any) {
	protoValue, err := catena.ToProto(value)
	if err != nil {
		logger.Error("BroadcastUpdate: failed to convert value to proto", "error", err)
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
	bs.NotifyUpdate(update)
}
