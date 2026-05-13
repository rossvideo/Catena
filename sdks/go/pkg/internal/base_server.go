package internal

import (
	"fmt"
	"math"
	"strconv"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Handler type aliases — canonical definitions live in the catena package.
type DeviceHandler = catena.DeviceHandler
type GetValueHandler = catena.GetValueHandler
type SetValueHandler = catena.SetValueHandler
type GetAssetHandler = catena.GetAssetHandler
type ExecuteCommandHandler = catena.ExecuteCommandHandler
type GetParamInfoHandler = catena.GetParamInfoHandler

type BaseServer struct {
	Mu                     sync.Mutex
	Slots                  []uint32
	getDeviceHandlers      map[uint16]DeviceHandler
	getValueHandlers       map[uint16]GetValueHandler
	setValueHandlers       map[uint16]SetValueHandler
	getAssetHandlers       map[uint16]GetAssetHandler
	executeCommandHandlers map[uint16]ExecuteCommandHandler
	getParamInfoHandlers   map[uint16]GetParamInfoHandler
	connectionQueue        *ConnectionQueue
}

// Default handlers that return "not implemented"
func DefaultDeviceHandler() (catena.CatenaDevice, catena.StatusResult) {
	logger.Warning("DefaultDeviceHandler called - no handler registered for this slot")
	return catena.ReplyError[catena.CatenaDevice](catena.NOT_FOUND, "No device defined at slot")
}

func DefaultGetValueHandler(slot uint16, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	logger.Warning("DefaultGetValueHandler called - no handler registered for this slot", "slot", slot, "fqoid", fqoid)
	return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func DefaultSetValueHandler(value any, slot uint16, fqoid string) catena.StatusResult {
	logger.Warning("DefaultSetValueHandler called - no handler registered for this slot", "slot", slot, "fqoid", fqoid)
	return catena.StatusWithCode(catena.NOT_FOUND, "fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func DefaultGetAssetHandler(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
	logger.Warning("DefaultGetAssetHandler called - no handler registered for this slot", "slot", slot, "fqoid", fqoid)
	return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "fqoid "+fqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func DefaultExecuteCommandHandler(slot uint16, commandFqoid string, payload any) (catena.CommandResult, catena.StatusResult) {
	logger.Warning("DefaultExecuteCommandHandler called - no handler registered for this slot", "slot", slot, "commandFqoid", commandFqoid)
	return catena.CommandError(catena.NOT_FOUND, "ExecuteCommand "+commandFqoid+" not found at slot "+strconv.Itoa(int(slot)))
}

func DefaultGetParamInfoHandler(slot uint16, oidPrefix string, recursive bool) ([]catena.CatenaParamInfo, catena.StatusResult) {
	logger.Warning("DefaultGetParamInfoHandler called - no handler registered for this slot", "slot", slot, "oid_prefix", oidPrefix, "recursive", recursive)
	return nil, catena.StatusWithCode(catena.NOT_FOUND, "no param info handler registered for slot "+strconv.Itoa(int(slot)))
}

// Handler registration methods
func (bs *BaseServer) RegisterGetDeviceHandler(slot uint16, handler DeviceHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.getDeviceHandlers[slot] = handler
}

func (bs *BaseServer) RegisterGetValueHandler(slot uint16, handler GetValueHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.getValueHandlers[slot] = handler
}

func (bs *BaseServer) RegisterSetValueHandler(slot uint16, handler SetValueHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.setValueHandlers[slot] = handler
}

func (bs *BaseServer) RegisterGetAssetHandler(slot uint16, handler GetAssetHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.getAssetHandlers[slot] = handler
}

func (bs *BaseServer) RegisterExecuteCommandHandler(slot uint16, handler ExecuteCommandHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.executeCommandHandlers[slot] = handler
}

func (bs *BaseServer) RegisterGetParamInfoHandler(slot uint16, handler GetParamInfoHandler) {
	bs.Mu.Lock()
	defer bs.Mu.Unlock()
	bs.getParamInfoHandlers[slot] = handler
}

// Lookup helper functions
func (bs *BaseServer) LookupGetDeviceHandler(slot uint16) DeviceHandler {
	if handler, ok := bs.getDeviceHandlers[slot]; ok {
		return handler
	}
	return DefaultDeviceHandler
}

func (bs *BaseServer) LookupGetValueHandler(slot uint16) GetValueHandler {
	if handler, ok := bs.getValueHandlers[slot]; ok {
		return handler
	}
	return DefaultGetValueHandler
}

func (bs *BaseServer) LookupSetValueHandler(slot uint16) SetValueHandler {
	if handler, ok := bs.setValueHandlers[slot]; ok {
		return handler
	}
	return DefaultSetValueHandler
}

func (bs *BaseServer) LookupGetAssetHandler(slot uint16) GetAssetHandler {
	if handler, ok := bs.getAssetHandlers[slot]; ok {
		return handler
	}
	return DefaultGetAssetHandler
}

func (bs *BaseServer) LookupExecuteCommandHandler(slot uint16) ExecuteCommandHandler {
	if handler, ok := bs.executeCommandHandlers[slot]; ok {
		return handler
	}
	return DefaultExecuteCommandHandler
}

func (bs *BaseServer) LookupGetParamInfoHandler(slot uint16) GetParamInfoHandler {
	if handler, ok := bs.getParamInfoHandlers[slot]; ok {
		return handler
	}
	return DefaultGetParamInfoHandler
}

func NewBaseServer(slots []uint16, maxConnections int) *BaseServer {
	bs := &BaseServer{
		Slots:                  make([]uint32, len(slots)),
		getDeviceHandlers:      make(map[uint16]DeviceHandler),
		getValueHandlers:       make(map[uint16]GetValueHandler),
		setValueHandlers:       make(map[uint16]SetValueHandler),
		getAssetHandlers:       make(map[uint16]GetAssetHandler),
		executeCommandHandlers: make(map[uint16]ExecuteCommandHandler),
		getParamInfoHandlers:   make(map[uint16]GetParamInfoHandler),
		connectionQueue:        newConnectionQueue(maxConnections),
	}

	for i, slot := range slots {
		bs.Slots[i] = uint32(slot)
	}

	for _, slot := range slots {
		bs.RegisterGetDeviceHandler(slot, DefaultDeviceHandler)
		bs.RegisterGetValueHandler(slot, DefaultGetValueHandler)
		bs.RegisterSetValueHandler(slot, DefaultSetValueHandler)
		bs.RegisterGetAssetHandler(slot, DefaultGetAssetHandler)
		bs.RegisterExecuteCommandHandler(slot, DefaultExecuteCommandHandler)
		bs.RegisterGetParamInfoHandler(slot, DefaultGetParamInfoHandler)
	}

	return bs
}

func (bs *BaseServer) ValidateSlot(slot uint32) (uint16, catena.StatusResult) {
	if slot > uint32(math.MaxUint16) {
		return 0, catena.StatusWithCode(catena.INVALID_ARGUMENT, fmt.Sprintf("invalid slot number: %d", slot))

	}
	return uint16(slot), catena.StatusWithCode(catena.OK, "")
}

func (bs *BaseServer) ValidateSlotString(slot string) (uint16, catena.StatusResult) {
	slotInt, err := strconv.ParseUint(slot, 10, 32)
	if err != nil {
		return 0, catena.StatusWithCode(catena.INVALID_ARGUMENT, fmt.Sprintf("invalid slot string: %s", slot))
	}
	return bs.ValidateSlot(uint32(slotInt))
}

// RegisterConnection registers a new streaming connection
func (bs *BaseServer) RegisterConnection() (int, *Connection) {
	return bs.connectionQueue.registerConnection()
}

// DeregisterConnection removes a streaming connection
func (bs *BaseServer) DeregisterConnection(connID int) {
	bs.connectionQueue.deregisterConnection(connID)
}

// notifyUpdate broadcasts an update to all connected clients
func (bs *BaseServer) NotifyUpdate(update *protos.PushUpdates) {
	bs.connectionQueue.NotifyUpdate(update)
}

// SetMaxConnections sets the maximum number of streaming connections
func (bs *BaseServer) SetMaxConnections(max int) {
	bs.connectionQueue.SetMaxConnections(max)
}

// shutdown gracefully shuts down all streaming connections
func (bs *BaseServer) ShutdownConnections() {
	bs.connectionQueue.shutdown()
}

// ConnectionCount returns the number of active streaming connections
func (bs *BaseServer) ConnectionCount() int {
	return bs.connectionQueue.ConnectionCount()
}

// BroadcastUpdate converts a native Go value into a proto PushUpdates message
// and sends it to all connected streaming clients. Business logic calls this with
// plain Go types; the proto serialization is handled internally.
func (bs *BaseServer) BroadcastUpdate(slot uint16, oid string, value any) {
	protoValue, res := catena.ToProto(value)
	if res.Code != catena.OK {
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
	bs.NotifyUpdate(update)
}
