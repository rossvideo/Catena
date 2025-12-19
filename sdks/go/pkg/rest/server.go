package rest

import (
	"net/http"
	"strconv"
	"strings"
	"sync"

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

// Server is decoupled from catena.Device and just wires HTTP routes.
type Server struct {
	getDevice     DeviceHandler
	getValue      map[int]GetValueHandler
	setValue      map[int]SetValueHandler
	getAsset      map[int]AssetHandler

	slotList []int
	mux      http.ServeMux
	mu       sync.RWMutex
}

func NewServer(slotList []int) *Server {
	logger.Info("Creating new REST server with slots: %v", slotList)

	server := Server{
		getDevice:     DeviceHandler(nil),
		getValue:      make(map[int]GetValueHandler),
		setValue:      make(map[int]SetValueHandler),
		getAsset:      make(map[int]AssetHandler),
		slotList:      slotList,
		mux:           *http.NewServeMux(),
	}

	// Register HTTP routes.
	server.RegisterRoutes()

	return &server
}

func (s *Server) StartHTTPServer(port int) {
	addr := ":" + strconv.Itoa(port)
	logger.Info("Starting HTTP server on %s", addr)
	go func() {
		if err := http.ListenAndServe(addr, &s.mux); err != nil {
			logger.Error("HTTP server failed: %v", err)
			panic("HTTP server failed: " + err.Error())
		}
	}()
}

// Default handlers return HTTPResult for uniform handling.
func (s *Server) DefaultDeviceHandler(w http.ResponseWriter, r *http.Request) catena.StatusResult {
	logger.Warning("GET /device: no handler registered")
	return catena.NotImplemented("no device handler registered")
}

func (s *Server) DefaultGetValueHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult {
	logger.Warning("GET /value/%s: no handler registered for slot %d", fqoid, slot)
	return catena.NotImplemented("no getParamValue handler registered for slot " + strconv.Itoa(slot))
}

func (s *Server) DefaultSetValueHandler(value any, slot int, fqoid string) catena.StatusResult {
	logger.Warning("PUT /value/%s: no handler registered for slot %d", fqoid, slot)
	return catena.NotImplemented("no setParamValue handler registered for slot " + strconv.Itoa(slot))
}

func (s *Server) DefaultGetAssetHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult {
	logger.Warning("GET /asset/%s: no handler registered for slot %d", fqoid, slot)
	return catena.NotImplemented("no getAsset handler registered for slot " + strconv.Itoa(slot))
}

// Registration APIs: set one generic handler per slot.
func (s *Server) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	// getDevice is global; last registration wins.
	s.getDevice = handler
	logger.Debug("Registered device handler (global), requested by slot %d", slot)
}

func (s *Server) RegisterGetValueHandler(slot int, handler GetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getValue[slot] = handler
	logger.Debug("Registered GET value handler for slot %d", slot)
}

func (s *Server) RegisterSetValueHandler(slot int, handler SetValueHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.setValue[slot] = handler
	logger.Debug("Registered SET value handler for slot %d", slot)
}

func (s *Server) RegisterGetAssetHandler(slot int, handler AssetHandler) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.getAsset[slot] = handler
	logger.Debug("Registered asset handler for slot %d", slot)
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
	logger.Debug("Registering routes for %d slots", len(s.slotList))
	for _, slot := range s.slotList {
		prefix := "/st2138-api/v1/" + strconv.Itoa(slot)

		// Entity/device-like endpoint: GET
		s.mux.HandleFunc(prefix+"/device", func(w http.ResponseWriter, r *http.Request) {
			logger.Info("%s /device started", r.Method)
			if r.Method != http.MethodGet {
				logger.Warning("/device: method %s not allowed, expected GET", r.Method)
				writeHTTPResult(w, catena.MethodNotAllowed("method not allowed"))
				return
			}
			if s.getDevice != nil {
				res := s.getDevice(w, r)
				writeHTTPResult(w, res)
				logger.Info("GET /device finished")
				return
			}
			writeHTTPResult(w, s.DefaultDeviceHandler(w, r))
		})

		// Param endpoint: fqoid is the path after /value/
		s.mux.HandleFunc(prefix+"/value/", func(w http.ResponseWriter, r *http.Request) {
			fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/value/")
			if fqoid == "" {
				logger.Warning("%s /value: missing param fqoid", r.Method)
				writeHTTPResult(w, catena.BadRequest("missing param fqoid"))
				return
			}
			switch r.Method {
			case http.MethodGet:
				logger.Info("GET /value/%s started", fqoid)
				if handler, ok := s.lookupGetValue(slot); ok {
					res := handler(slot, fqoid)
					writeHTTPResult(w, res)
					logger.Info("GET /value/%s finished", fqoid)
					return
				}
				writeHTTPResult(w, s.DefaultGetValueHandler(w, r, slot, fqoid))
			case http.MethodPut:
				value, err := internal.ReadValueFromRequest(r)
				if err != nil {
					logger.Error("Slot %d: SetParam %s - invalid JSON: %v", slot, fqoid, err)
					writeHTTPResult(w, catena.BadRequest("invalid JSON"))
					return
				}
				logger.Info("%s /value/%s started", r.Method, fqoid)
				if handler, ok := s.lookupSetValue(slot); ok {
					res := handler(value, slot, fqoid)
					writeHTTPResult(w, res)
					logger.Info("%s /value/%s finished", r.Method, fqoid)
					return
				}
				writeHTTPResult(w, s.DefaultSetValueHandler(value, slot, fqoid))
			default:
				logger.Warning("%s /value/%s: method not allowed", r.Method, fqoid)
				writeHTTPResult(w, catena.MethodNotAllowed("method not allowed"))
			}
		})

		// Asset endpoint: fqoid is the path after /asset/
		s.mux.HandleFunc(prefix+"/asset/", func(w http.ResponseWriter, r *http.Request) {
			fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/asset/")
			if fqoid == "" {
				logger.Warning("GET /asset: missing asset fqoid")
				writeHTTPResult(w, catena.BadRequest("missing asset fqoid"))
				return
			}
			if r.Method != http.MethodGet {
				logger.Warning("%s /asset/%s: method not allowed", r.Method, fqoid)
				writeHTTPResult(w, catena.MethodNotAllowed("method not allowed"))
				return
			}
			logger.Info("GET /asset/%s started", fqoid)
			if handler, ok := s.lookupGetAsset(slot); ok {
				res := handler(w, r, slot, fqoid)
				writeHTTPResult(w, res)
				logger.Info("GET /asset/%s finished", fqoid)
				return
			}
			writeHTTPResult(w, s.DefaultGetAssetHandler(w, r, slot, fqoid))
		})
	}
}
