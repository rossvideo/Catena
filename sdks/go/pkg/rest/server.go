package rest

import (
	"net/http"
	"strconv"
	"strings"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Entity is an opaque type the BaseServer routes to per slot.
type Device any

// Base handlers are entity-agnostic.
type DeviceHandler func(w http.ResponseWriter, r *http.Request)
type ParamValueHandler func(w http.ResponseWriter, r *http.Request, slot int, fqoid string)
type AssetHandler func(w http.ResponseWriter, r *http.Request, slot int, fqoid string)

// Server is decoupled from catena.Device and just wires HTTP routes.
type Server struct {
    getDevice  		DeviceHandler
    getParamValue   map[int]ParamValueHandler
    setParamValue   map[int]ParamValueHandler
    getAsset   		map[int]AssetHandler

	slotList		[]int
	mux 			http.ServeMux
    mu             	sync.RWMutex
}

func NewServer(slotList []int) *Server {
	logger.Info("Creating new REST server with slots: %v", slotList)

    server := Server{
        getDevice: 		DeviceHandler(nil),
        getParamValue:  make(map[int]ParamValueHandler),
        setParamValue:  make(map[int]ParamValueHandler),
        getAsset:  		make(map[int]AssetHandler),
		slotList:    	slotList,
		mux: 			*http.NewServeMux(),
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

func (s *Server) DefaultDeviceHandler(w http.ResponseWriter, r *http.Request) {
	logger.Warning("GET /device: no handler registered")
	http.Error(w, "no device handler registered", http.StatusNotImplemented)
}

func (s *Server) DefaultGetParamValueHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) {
	logger.Warning("GET /value/%s: no handler registered for slot %d", fqoid, slot)
	http.Error(w, "no getParamValue handler registered for slot "+strconv.Itoa(slot)+": fqoid /"+fqoid, http.StatusNotImplemented)
}

func (s *Server) DefaultSetParamValueHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) {
	logger.Warning("PUT /value/%s: no handler registered for slot %d", fqoid, slot)
	http.Error(w, "no setParamValue handler registered for slot "+strconv.Itoa(slot)+": fqoid /"+fqoid, http.StatusNotImplemented)
}

func (s *Server) DefaultGetAssetHandler(w http.ResponseWriter, r *http.Request, slot int, path string) {
	logger.Warning("GET /asset/%s: no handler registered for slot %d", path, slot)
	http.Error(w, "no getAsset handler registered", http.StatusNotImplemented)
}

// Registration APIs: populate per-slot maps keyed by fqoid.
func (s *Server) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if s.getDevice == nil {
        s.getDevice = handler
		logger.Debug("Registered device handler for slot %d", slot)
    }
}

func (s *Server) RegisterGetParamHandler(slot int, handler ParamValueHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if _, ok := s.getParamValue[slot]; !ok {
        s.getParamValue[slot] = handler
		logger.Debug("Registered GET param handler for slot %d", slot)
    }
}

func (s *Server) RegisterSetParamHandler(slot int, handler ParamValueHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if _, ok := s.setParamValue[slot]; !ok {
        s.setParamValue[slot] = handler
		logger.Debug("Registered SET param handler for slot %d", slot)
    }
}

func (s *Server) RegisterGetAssetHandler(slot int, handler AssetHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if _, ok := s.getAsset[slot]; !ok {
        s.getAsset[slot] = handler
		logger.Debug("Registered asset handler for slot %d", slot)
    }
}

// Internal lookups (read-locked).
func (s *Server) lookupGetParamValue(slot int) (ParamValueHandler, bool) {
    s.mu.RLock()
    defer s.mu.RUnlock()
    if m, ok := s.getParamValue[slot]; ok {
        return m, ok
    }
    return nil, false
}

func (s *Server) lookupSetParamValue(slot int) (ParamValueHandler, bool) {
    s.mu.RLock()
    defer s.mu.RUnlock()
    if m, ok := s.setParamValue[slot]; ok {
        return m, ok
    }
    return nil, false
}

func (s *Server) lookupGetAsset(slot int) (AssetHandler, bool) {
    s.mu.RLock()
    defer s.mu.RUnlock()
    if m, ok := s.getAsset[slot]; ok {
        return m, ok
    }
    return nil, false
}

func (s *Server) RegisterRoutes() {
	logger.Debug("Registering routes for %d slots", len(s.slotList))
    for _, slot := range s.slotList {
        // capture loop variable
        slot := slot

        prefix := "/st2138-api/v1/" + strconv.Itoa(slot)

        // Entity/device-like endpoint: GET
        // fqoid can be provided via query "?id=..." or empty "" for default.
        s.mux.HandleFunc(prefix+"/device", func(w http.ResponseWriter, r *http.Request) {
			logger.Info("%s /device started", r.Method)
            if r.Method != http.MethodGet {
				logger.Warning("/device: method %s not allowed, expected GET", r.Method)
                http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
                return
            }
            if s.getDevice != nil {
                s.getDevice(w, r)
				logger.Info("GET /device finished")
            } else {
				s.DefaultDeviceHandler(w, r)
			}
        })

        // Param endpoint: fqoid is the path after /value/
        s.mux.HandleFunc(prefix+"/value/", func(w http.ResponseWriter, r *http.Request) {
            fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/value/")
            if fqoid == "" {
				logger.Warning("%s /value: missing param fqoid", r.Method)
                http.Error(w, "missing param fqoid", http.StatusBadRequest)
                return
            }
            switch r.Method {
            case http.MethodGet:
				logger.Info("GET /value/%s started", fqoid)
                if handler, ok := s.lookupGetParamValue(slot); ok {
                    handler(w, r, slot, fqoid)
					logger.Info("GET /value/%s finished", fqoid)
               	} else {
				s.DefaultGetParamValueHandler(w, r, slot, fqoid)
				return
			}
            case http.MethodPut, http.MethodPatch:
				logger.Info("%s /value/%s started", r.Method, fqoid)
                if handler, ok := s.lookupSetParamValue(slot); ok {
                    handler(w, r, slot, fqoid)
					logger.Info("%s /value/%s finished", r.Method, fqoid)
                } else {
				s.DefaultSetParamValueHandler(w, r, slot, fqoid)
				return
			}
            default:
				logger.Warning("%s /value/%s: method not allowed", r.Method, fqoid)
                http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
            }
        })

        // Asset endpoint: fqoid is the path after /asset/
        s.mux.HandleFunc(prefix+"/asset/", func(w http.ResponseWriter, r *http.Request) {
            fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/asset/")
            if fqoid == "" {
				logger.Warning("GET /asset: missing asset fqoid")
                http.Error(w, "missing asset fqoid", http.StatusBadRequest)
                return
            }
            if r.Method != http.MethodGet {
				logger.Warning("%s /asset/%s: method not allowed", r.Method, fqoid)
                http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
                return
            }
			logger.Info("GET /asset/%s started", fqoid)
            if handler, ok := s.lookupGetAsset(slot); ok {
                handler(w, r, slot, fqoid)
				logger.Info("GET /asset/%s finished", fqoid)
            } else {
				s.DefaultGetAssetHandler(w, r, slot, fqoid)
			}
        })
    }
}
