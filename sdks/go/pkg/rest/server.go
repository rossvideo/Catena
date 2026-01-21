package rest

import (
	"encoding/json"
	"fmt"
	"math"
	"net/http"
	"strconv"
	"strings"
	"sync"

	"github.com/rossvideo/catena/sdks/go/internal"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Handlers now return (CatenaValue, StatusResult) so the server can respond consistently.
type DeviceHandler func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult)
type GetValueHandler func(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult)
type SetValueHandler func(value any, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult)
type GetAssetHandler func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult)
type ConnectHandler func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult)
type ExecuteCommandHandler func(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult)
type FallbackHandler func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult)

// Server provides REST API endpoints for Catena devices
type Server struct {
	mu                     sync.Mutex
	mux                    *http.ServeMux
	getDeviceHandlers      map[int]DeviceHandler
	getValueHandlers       map[int]GetValueHandler
	setValueHandlers       map[int]SetValueHandler
	getAssetHandlers       map[int]GetAssetHandler
	connectHandler         ConnectHandler
	executeCommandHandlers map[int]ExecuteCommandHandler
	fallbackHandler        FallbackHandler
}

// writeHTTPResult writes a CatenaValue and StatusResult to the HTTP response
func writeHTTPResult(w http.ResponseWriter, value catena.CatenaValue, result catena.StatusResult) {
	httpStatus := result.Code.ToHTTPStatus()

	// If there's an error message, write it as JSON
	if result.Error != "" {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(httpStatus)
		json.NewEncoder(w).Encode(map[string]string{"error": result.Error})
		return
	}

	// If Value is nil, just write the status code
	if value.Value == nil {
		w.WriteHeader(httpStatus)
		return
	}

	// Write the protobuf value as JSON
	if err := internal.WriteResponseJSON(w, value.Value, httpStatus); err != nil {
		logger.Error("failed to write response", "error", err)
		w.WriteHeader(http.StatusInternalServerError)
	}
}

// NewServer creates a new REST server for the given device slots
func NewServer(slots []int) *Server {
	s := &Server{
		mux:                    http.NewServeMux(),
		getDeviceHandlers:      make(map[int]DeviceHandler),
		getValueHandlers:       make(map[int]GetValueHandler),
		setValueHandlers:       make(map[int]SetValueHandler),
		getAssetHandlers:       make(map[int]GetAssetHandler),
		executeCommandHandlers: make(map[int]ExecuteCommandHandler),
	}

	// Register default handlers for each slot
	for _, slot := range slots {
		s.RegisterGetDeviceHandler(slot, DefaultDeviceHandler)
		s.RegisterGetValueHandler(slot, DefaultGetValueHandler)
		s.RegisterSetValueHandler(slot, DefaultSetValueHandler)
		s.RegisterGetAssetHandler(slot, DefaultGetAssetHandler)
		s.RegisterExecuteCommandHandler(slot, DefaultExecuteCommandHandler)
	}

	// Register default connect handler
	s.RegisterConnectHandler(DefaultConnectHandler)

	// Register routes
	s.RegisterRoutes()

	return s
}

// Start starts the HTTP server on the specified port using this server's mux
func (s *Server) Start(port int) error {
	addr := fmt.Sprintf(":%d", port)
	logger.Info("Starting HTTP server", "address", addr)
	return http.ListenAndServe(addr, s.mux)
}

// Default handlers that return "not implemented"

func DefaultDeviceHandler(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("GetDevice not implemented")
}

func DefaultGetValueHandler(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("GetValue not implemented")
}

func DefaultSetValueHandler(value any, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("SetValue not implemented")
}

func DefaultGetAssetHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("GetAsset not implemented")
}

func DefaultConnectHandler(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("Connect not implemented")
}

func DefaultExecuteCommandHandler(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("ExecuteCommand not implemented")
}

// Handler registration methods

func (s *Server) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getDeviceHandlers[slot] = handler
}

func (s *Server) RegisterGetValueHandler(slot int, handler GetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getValueHandlers[slot] = handler
}

func (s *Server) RegisterSetValueHandler(slot int, handler SetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.setValueHandlers[slot] = handler
}

func (s *Server) RegisterGetAssetHandler(slot int, handler GetAssetHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getAssetHandlers[slot] = handler
}

func (s *Server) RegisterConnectHandler(handler ConnectHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.connectHandler = handler
}

func (s *Server) RegisterExecuteCommandHandler(slot int, handler ExecuteCommandHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.executeCommandHandlers[slot] = handler
}

func (s *Server) RegisterFallbackHandler(handler FallbackHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.fallbackHandler = handler
}

// Lookup helper functions

func (s *Server) lookupGetValue(slot int) GetValueHandler {
	if handler, ok := s.getValueHandlers[slot]; ok {
		return handler
	}
	return DefaultGetValueHandler
}

func (s *Server) lookupSetValue(slot int) SetValueHandler {
	if handler, ok := s.setValueHandlers[slot]; ok {
		return handler
	}
	return DefaultSetValueHandler
}

func (s *Server) lookupGetAsset(slot int) GetAssetHandler {
	if handler, ok := s.getAssetHandlers[slot]; ok {
		return handler
	}
	return DefaultGetAssetHandler
}

func (s *Server) lookupConnect() ConnectHandler {
	if s.connectHandler != nil {
		return s.connectHandler
	}
	return DefaultConnectHandler
}

func (s *Server) lookupExecuteCommand(slot int) ExecuteCommandHandler {
	if handler, ok := s.executeCommandHandlers[slot]; ok {
		return handler
	}
	return DefaultExecuteCommandHandler
}

// RegisterRoutes sets up all HTTP routes
func (s *Server) RegisterRoutes() {
	// Device endpoint: GET /st2138-api/v1/{slot}
	s.mux.HandleFunc("/st2138-api/v1/", func(w http.ResponseWriter, r *http.Request) {
		parts := strings.Split(strings.Trim(r.URL.Path, "/"), "/")
		if len(parts) < 3 {
			val, res := catena.ReplyBadRequest("invalid path format")
			writeHTTPResult(w, val, res)
			return
		}

		slotStr := parts[2]
		slot, err := strconv.Atoi(slotStr)
		if err != nil || slot < 0 || slot > math.MaxInt16 {
			val, res := catena.ReplyBadRequest("invalid slot number")
			writeHTTPResult(w, val, res)
			return
		}

		// Route based on path structure
		if len(parts) == 3 && r.Method == http.MethodGet {
			// GET /st2138-api/v1/{slot} - Get device info
			handler, ok := s.getDeviceHandlers[slot]
			if !ok {
				val, res := catena.ReplyNotFound("device not found")
				writeHTTPResult(w, val, res)
				return
			}
			val, res := handler(w, r)
			writeHTTPResult(w, val, res)
			return
		}

		if len(parts) >= 4 {
			endpoint := parts[3]
			switch endpoint {
			case "value":
				s.handleValueEndpoint(w, r, slot, parts[4:])
			case "asset":
				s.handleAssetEndpoint(w, r, slot, parts[4:])
			case "command":
				s.handleCommandEndpoint(w, r, slot, parts[4:])
			default:
				val, res := catena.ReplyNotFound("unknown endpoint")
				writeHTTPResult(w, val, res)
			}
			return
		}

		val, res := catena.ReplyNotFound("endpoint not found")
		writeHTTPResult(w, val, res)
	})

	// Connect endpoint: GET /st2138-api/v1/connect
	s.mux.HandleFunc("/st2138-api/v1/connect", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			val, res := catena.ReplyMethodNotAllowed("only GET allowed")
			writeHTTPResult(w, val, res)
			return
		}
		handler := s.lookupConnect()
		val, res := handler(w, r)
		writeHTTPResult(w, val, res)
	})

	// Catch-all for 404 - must be registered last
	s.mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if s.fallbackHandler != nil {
			val, res := s.fallbackHandler(w, r)
			writeHTTPResult(w, val, res)
			return
		}
		val, res := catena.ReplyNotFound("endpoint not found")
		writeHTTPResult(w, val, res)
	})
}

func (s *Server) handleValueEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	fqoid := strings.Join(pathParts, "/")

	switch r.Method {
	case http.MethodGet:
		handler := s.lookupGetValue(slot)
		val, res := handler(slot, fqoid)
		writeHTTPResult(w, val, res)

	case http.MethodPut:
		// Read request body
		reqValue, err := internal.ReadRequestJSON(r)
		if err != nil {
			logger.Error("failed to read request", "error", err)
			val, res := catena.ReplyBadRequest("invalid request body")
			writeHTTPResult(w, val, res)
			return
		}

		// Convert proto value to native Go type
		nativeValue := catena.FromProto(reqValue)

		handler := s.lookupSetValue(slot)
		val, res := handler(nativeValue, slot, fqoid)
		writeHTTPResult(w, val, res)

	default:
		val, res := catena.ReplyMethodNotAllowed("only GET, PUT, PATCH allowed")
		writeHTTPResult(w, val, res)
	}
}

func (s *Server) handleAssetEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	if r.Method != http.MethodGet {
		val, res := catena.ReplyMethodNotAllowed("only GET allowed")
		writeHTTPResult(w, val, res)
		return
	}

	fqoid := strings.Join(pathParts, "/")
	handler := s.lookupGetAsset(slot)
	val, res := handler(w, r, slot, fqoid)
	writeHTTPResult(w, val, res)
}

func (s *Server) handleCommandEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	if r.Method != http.MethodPost {
		val, res := catena.ReplyMethodNotAllowed("only POST allowed")
		writeHTTPResult(w, val, res)
		return
	}

	commandFqoid := strings.Join(pathParts, "/")

	// Read command payload
	var payload any
	if r.ContentLength > 0 {
		reqValue, err := internal.ReadRequestJSON(r)
		if err != nil {
			logger.Error("failed to read command payload", "error", err)
			val, res := catena.ReplyBadRequest("invalid command payload")
			writeHTTPResult(w, val, res)
			return
		}
		payload = catena.FromProto(reqValue)
	}

	handler := s.lookupExecuteCommand(slot)
	val, res := handler(w, r, slot, commandFqoid, payload)
	writeHTTPResult(w, val, res)
}
