package rest

import (
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"github.com/rossvideo/catena/sdks/go/internal"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Server provides REST API endpoints for Catena devices
type Server struct {
	mux                    *http.ServeMux
	getDeviceHandlers      map[int]func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)
	getValueHandlers       map[int]func(int, string) (catena.CatenaValue, catena.StatusResult)
	setValueHandlers       map[int]func(any, int, string) (catena.CatenaValue, catena.StatusResult)
	getAssetHandlers       map[int]func(int, string) (catena.CatenaValue, catena.StatusResult)
	connectHandler         func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)
	executeCommandHandlers map[int]func(http.ResponseWriter, *http.Request, int, string, any) (catena.CatenaValue, catena.StatusResult)
	notFoundHandler        func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)
}

// writeHTTPResult writes a CatenaValue and StatusResult to the HTTP response
func writeHTTPResult(w http.ResponseWriter, value catena.CatenaValue, result catena.StatusResult) {
	httpStatus := result.Code.ToHTTPStatus()

	// If there's an error message, write it as JSON
	if result.Error != "" {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(httpStatus)
		fmt.Fprintf(w, `{"error": "%s"}`, result.Error)
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
		getDeviceHandlers:      make(map[int]func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)),
		getValueHandlers:       make(map[int]func(int, string) (catena.CatenaValue, catena.StatusResult)),
		setValueHandlers:       make(map[int]func(any, int, string) (catena.CatenaValue, catena.StatusResult)),
		getAssetHandlers:       make(map[int]func(int, string) (catena.CatenaValue, catena.StatusResult)),
		executeCommandHandlers: make(map[int]func(http.ResponseWriter, *http.Request, int, string, any) (catena.CatenaValue, catena.StatusResult)),
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

// StartHTTPServer starts the HTTP server on the specified port
func StartHTTPServer(port int) error {
	addr := fmt.Sprintf(":%d", port)
	logger.Info("Starting HTTP server", "address", addr)
	return http.ListenAndServe(addr, nil)
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

func DefaultGetAssetHandler(slot int, fqoid string) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("GetAsset not implemented")
}

func DefaultConnectHandler(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("Connect not implemented")
}

func DefaultExecuteCommandHandler(w http.ResponseWriter, r *http.Request, slot int, commandFqoid string, payload any) (catena.CatenaValue, catena.StatusResult) {
	return catena.ReplyNotImplemented("ExecuteCommand not implemented")
}

// Handler registration methods

func (s *Server) RegisterGetDeviceHandler(slot int, handler func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)) {
	s.getDeviceHandlers[slot] = handler
}

func (s *Server) RegisterGetValueHandler(slot int, handler func(int, string) (catena.CatenaValue, catena.StatusResult)) {
	s.getValueHandlers[slot] = handler
}

func (s *Server) RegisterSetValueHandler(slot int, handler func(any, int, string) (catena.CatenaValue, catena.StatusResult)) {
	s.setValueHandlers[slot] = handler
}

func (s *Server) RegisterGetAssetHandler(slot int, handler func(int, string) (catena.CatenaValue, catena.StatusResult)) {
	s.getAssetHandlers[slot] = handler
}

func (s *Server) RegisterNotFoundHandler(handler func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)) {
	s.notFoundHandler = handler
}

func (s *Server) RegisterConnectHandler(handler func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult)) {
	s.connectHandler = handler
}

func (s *Server) RegisterExecuteCommandHandler(slot int, handler func(http.ResponseWriter, *http.Request, int, string, any) (catena.CatenaValue, catena.StatusResult)) {
	s.executeCommandHandlers[slot] = handler
}

// Lookup helper functions

func (s *Server) lookupGetValue(slot int) func(int, string) (catena.CatenaValue, catena.StatusResult) {
	if handler, ok := s.getValueHandlers[slot]; ok {
		return handler
	}
	return DefaultGetValueHandler
}

func (s *Server) lookupSetValue(slot int) func(any, int, string) (catena.CatenaValue, catena.StatusResult) {
	if handler, ok := s.setValueHandlers[slot]; ok {
		return handler
	}
	return DefaultSetValueHandler
}

func (s *Server) lookupGetAsset(slot int) func(int, string) (catena.CatenaValue, catena.StatusResult) {
	if handler, ok := s.getAssetHandlers[slot]; ok {
		return handler
	}
	return DefaultGetAssetHandler
}

func (s *Server) lookupConnect() func(http.ResponseWriter, *http.Request) (catena.CatenaValue, catena.StatusResult) {
	if s.connectHandler != nil {
		return s.connectHandler
	}
	return DefaultConnectHandler
}

func (s *Server) lookupExecuteCommand(slot int) func(http.ResponseWriter, *http.Request, int, string, any) (catena.CatenaValue, catena.StatusResult) {
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
		if err != nil {
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
		if s.notFoundHandler != nil {
			val, res := s.notFoundHandler(w, r)
			writeHTTPResult(w, val, res)
			return
		}
		val, res := catena.ReplyNotFound("endpoint not found")
		writeHTTPResult(w, val, res)
	})

	http.Handle("/", s.mux)
}

func (s *Server) handleValueEndpoint(w http.ResponseWriter, r *http.Request, slot int, pathParts []string) {
	fqoid := strings.Join(pathParts, "/")

	switch r.Method {
	case http.MethodGet:
		handler := s.lookupGetValue(slot)
		val, res := handler(slot, fqoid)
		writeHTTPResult(w, val, res)

	case http.MethodPut, http.MethodPatch:
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
	val, res := handler(slot, fqoid)
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
