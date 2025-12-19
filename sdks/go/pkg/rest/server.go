package rest

import (
	"log/slog"
	"net/http"
	"strconv"
	"strings"
	"sync"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/internal"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Device is opaque for this base server.
type Device any

// Renderer
func writeHTTPResult(w http.ResponseWriter, res catena.StatusResult) {
	switch {
	case res.Status >= 200 && res.Status < 300:
		if res.Status == http.StatusNoContent || res.Payload == nil {
			w.WriteHeader(res.Status)
			return
		}
		internal.WriteResponseJSON(w, res)
	default:
		http.Error(w, nonEmpty(res.Message, http.StatusText(res.Status)), res.Status)
	}
}

func nonEmpty(s, fallback string) string {
	if s != "" {
		return s
	}
	return fallback
}

// Handlers now return HTTPResult so the server can respond consistently.
type DeviceHandler func(w http.ResponseWriter, r *http.Request) catena.StatusResult
type GetValueHandler func(slot int, fqoid string) catena.StatusResult
type SetValueHandler func(value any, slot int, fqoid string) catena.StatusResult

// AssetHandler keeps w/r so implementations can stream content directly; they still return an HTTPResult for status.
type AssetHandler func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult

// NotFoundHandler is called when an endpoint is requested that does not exist.
type NotFoundHandler func(w http.ResponseWriter, r *http.Request) catena.StatusResult

// Server is decoupled from catena.Device and just wires HTTP routes.
type Server struct {
	getDevice DeviceHandler
	getValue  map[int]GetValueHandler
	setValue  map[int]SetValueHandler
	getAsset  map[int]AssetHandler
	notFound  NotFoundHandler

	slotList []int
	mux      http.ServeMux
	mu       sync.RWMutex
	log      *slog.Logger
}

func NewServer(slotList []int) *Server {
	log := logger.GetNamed("rest-server")
	log.Info("Creating new REST server", "slots", slotList)

	server := Server{
		getDevice: DeviceHandler(nil),
		getValue:  make(map[int]GetValueHandler),
		setValue:  make(map[int]SetValueHandler),
		getAsset:  make(map[int]AssetHandler),
		slotList:  slotList,
		mux:       *http.NewServeMux(),
		log:       log,
	}

	// Register HTTP routes.
	server.RegisterRoutes()

	return &server
}

func (s *Server) StartHTTPServer(port int) {
	addr := ":" + strconv.Itoa(port)
	s.log.Info("Starting HTTP server", "address", addr)
	go func() {
		if err := http.ListenAndServe(addr, &s.mux); err != nil {
			s.log.Error("HTTP server failed", "error", err)
			panic("HTTP server failed: " + err.Error())
		}
	}()
}

// Default handlers return HTTPResult for uniform handling.
func (s *Server) DefaultDeviceHandler(w http.ResponseWriter, r *http.Request) catena.StatusResult {
	s.log.Warn("No handler registered", "method", "GET", "endpoint", "/device")
	return catena.NotImplemented("no device handler registered")
}

func (s *Server) DefaultGetValueHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult {
	s.log.Warn("No handler registered", "method", "GET", "endpoint", "/value", "fqoid", fqoid, "slot", slot)
	return catena.NotImplemented("no getParamValue handler registered for slot " + strconv.Itoa(slot))
}

func (s *Server) DefaultSetValueHandler(value any, slot int, fqoid string) catena.StatusResult {
	s.log.Warn("No handler registered", "method", "PUT", "endpoint", "/value", "fqoid", fqoid, "slot", slot)
	return catena.NotImplemented("no setParamValue handler registered for slot " + strconv.Itoa(slot))
}

func (s *Server) DefaultGetAssetHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult {
	s.log.Warn("No handler registered", "method", "GET", "endpoint", "/asset", "fqoid", fqoid, "slot", slot)
	return catena.NotImplemented("no getAsset handler registered for slot " + strconv.Itoa(slot))
}

// Registration APIs: set one generic handler per slot.
func (s *Server) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	// getDevice is global; last registration wins.
	s.getDevice = handler
	s.log.Debug("Registered device handler", "slot", slot)
}

func (s *Server) RegisterGetValueHandler(slot int, handler GetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getValue[slot] = handler
	s.log.Debug("Registered GET value handler", "slot", slot)
}

func (s *Server) RegisterSetValueHandler(slot int, handler SetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.setValue[slot] = handler
	s.log.Debug("Registered SET value handler", "slot", slot)
}

func (s *Server) RegisterGetAssetHandler(slot int, handler AssetHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getAsset[slot] = handler
	s.log.Debug("Registered asset handler", "slot", slot)
}

// RegisterNotFoundHandler registers a handler for requests to non-existent endpoints.
// This registers a catch-all route at "/" that matches any unhandled paths.
func (s *Server) RegisterNotFoundHandler(handler NotFoundHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.notFound = handler
	s.mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		res := handler(w, r)
		writeHTTPResult(w, res)
	})
	s.log.Debug("Registered not-found handler")
}

// Internal lookups (read-locked).
func (s *Server) lookupGetValue(slot int) (GetValueHandler, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	h, ok := s.getValue[slot]
	return h, ok
}

func (s *Server) lookupSetValue(slot int) (SetValueHandler, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	h, ok := s.setValue[slot]
	return h, ok
}

func (s *Server) lookupGetAsset(slot int) (AssetHandler, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	h, ok := s.getAsset[slot]
	return h, ok
}

func (s *Server) RegisterRoutes() {
	s.log.Debug("Registering routes", "slot_count", len(s.slotList))
	for _, slot := range s.slotList {
		// capture loop variable
		slot := slot

		prefix := "/st2138-api/v1/" + strconv.Itoa(slot)

		// Entity/device-like endpoint: GET
		s.mux.HandleFunc(prefix+"/device", func(w http.ResponseWriter, r *http.Request) {
			s.log.Info("Request started", "method", r.Method, "endpoint", "/device")
			if r.Method != http.MethodGet {
				s.log.Warn("Method not allowed", "method", r.Method, "endpoint", "/device", "expected", "GET")
				writeHTTPResult(w, catena.MethodNotAllowed("method not allowed"))
				return
			}
			if s.getDevice != nil {
				res := s.getDevice(w, r)
				writeHTTPResult(w, res)
				s.log.Info("Request finished", "method", "GET", "endpoint", "/device")
				return
			}
			writeHTTPResult(w, s.DefaultDeviceHandler(w, r))
		})

		// Param endpoint: fqoid is the path after /value/
		s.mux.HandleFunc(prefix+"/value/", func(w http.ResponseWriter, r *http.Request) {
			fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/value/")
			if fqoid == "" {
				s.log.Warn("Missing param fqoid", "method", r.Method, "endpoint", "/value")
				writeHTTPResult(w, catena.BadRequest("missing param fqoid"))
				return
			}
			switch r.Method {
			case http.MethodGet:
				s.log.Info("Request started", "method", "GET", "endpoint", "/value", "fqoid", fqoid)
				if handler, ok := s.lookupGetValue(slot); ok {
					res := handler(slot, fqoid)
					writeHTTPResult(w, res)
					s.log.Info("Request finished", "method", "GET", "endpoint", "/value", "fqoid", fqoid)
					return
				}
				writeHTTPResult(w, s.DefaultGetValueHandler(w, r, slot, fqoid))
			case http.MethodPut:
				value, err := internal.ReadValueFromRequest(r)
				if err != nil {
					s.log.Error("Invalid JSON in request", "slot", slot, "fqoid", fqoid, "error", err)
					writeHTTPResult(w, catena.BadRequest("invalid JSON"))
					return
				}
				s.log.Info("Request started", "method", r.Method, "endpoint", "/value", "fqoid", fqoid)
				if handler, ok := s.lookupSetValue(slot); ok {
					res := handler(value, slot, fqoid)
					writeHTTPResult(w, res)
					s.log.Info("Request finished", "method", r.Method, "endpoint", "/value", "fqoid", fqoid)
					return
				}
				writeHTTPResult(w, s.DefaultSetValueHandler(value, slot, fqoid))
			default:
				s.log.Warn("Method not allowed", "method", r.Method, "endpoint", "/value", "fqoid", fqoid)
				writeHTTPResult(w, catena.MethodNotAllowed("method not allowed"))
			}
		})

		// Asset endpoint: fqoid is the path after /asset/
		s.mux.HandleFunc(prefix+"/asset/", func(w http.ResponseWriter, r *http.Request) {
			fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/asset/")
			if fqoid == "" {
				s.log.Warn("Missing asset fqoid", "method", "GET", "endpoint", "/asset")
				writeHTTPResult(w, catena.BadRequest("missing asset fqoid"))
				return
			}
			if r.Method != http.MethodGet {
				s.log.Warn("Method not allowed", "method", r.Method, "endpoint", "/asset", "fqoid", fqoid)
				writeHTTPResult(w, catena.MethodNotAllowed("method not allowed"))
				return
			}
			s.log.Info("Request started", "method", "GET", "endpoint", "/asset", "fqoid", fqoid)
			if handler, ok := s.lookupGetAsset(slot); ok {
				res := handler(w, r, slot, fqoid)
				writeHTTPResult(w, res)
				s.log.Info("Request finished", "method", "GET", "endpoint", "/asset", "fqoid", fqoid)
				return
			}
			writeHTTPResult(w, s.DefaultGetAssetHandler(w, r, slot, fqoid))
		})
	}
}
