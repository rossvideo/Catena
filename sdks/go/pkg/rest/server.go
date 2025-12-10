package rest

import (
    "net/http"
    "strconv"
    "strings"
    "sync"
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

    server := Server{
        getDevice: 		DeviceHandler(nil),
        getParamValue:  make(map[int]ParamValueHandler),
        setParamValue:  make(map[int]ParamValueHandler),
        getAsset:  		make(map[int]AssetHandler),
		slotList:    	slotList,
		mux: 			*http.NewServeMux(),
    }

	// Start HTTP server.
    server.RegisterRoutes()

	return &server
}

func (s *Server) StartHTTPServer(port int) {
    addr := ":" + strconv.Itoa(port)
    go func() {
        if err := http.ListenAndServe(addr, &s.mux); err != nil {
            panic("HTTP server failed: " + err.Error())
        }
    }()
}

func (s *Server) DefaultDeviceHandler(w http.ResponseWriter, r *http.Request) {
	http.Error(w, "no device handler registered", http.StatusNotImplemented)
}

func (s *Server) DefaultGetParamValueHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) {
	http.Error(w, "no getParamValue handler registered for slot "+strconv.Itoa(slot)+": fqoid /"+fqoid, http.StatusNotImplemented)
}

func (s *Server) DefaultSetParamValueHandler(w http.ResponseWriter, r *http.Request, slot int, fqoid string) {
	http.Error(w, "no setParamValue handler registered for slot "+strconv.Itoa(slot)+": fqoid /"+fqoid, http.StatusNotImplemented)
}

func (s *Server) DefaultGetAssetHandler(w http.ResponseWriter, r *http.Request, slot int, path string) {
	http.Error(w, "no getAsset handler registered", http.StatusNotImplemented)
}

// Registration APIs: populate per-slot maps keyed by fqoid.
func (s *Server) RegisterGetDeviceHandler(slot int, handler DeviceHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if s.getDevice == nil {
        s.getDevice = handler
    }
}

func (s *Server) RegisterGetParamHandler(slot int, handler ParamValueHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if _, ok := s.getParamValue[slot]; !ok {
        s.getParamValue[slot] = handler
    }
}

func (s *Server) RegisterSetParamHandler(slot int, handler ParamValueHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if _, ok := s.setParamValue[slot]; !ok {
        s.setParamValue[slot] = handler
    }
}

func (s *Server) RegisterGetAssetHandler(slot int, handler AssetHandler) {
    s.mu.Lock()
    defer s.mu.Unlock()
    if _, ok := s.getAsset[slot]; !ok {
        s.getAsset[slot] = handler
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
    for _, slot := range s.slotList {
        // capture loop variable
        slot := slot

        prefix := "/st2138-api/v1/" + strconv.Itoa(slot)

        // Entity/device-like endpoint: GET
        // fqoid can be provided via query "?id=..." or empty "" for default.
        s.mux.HandleFunc(prefix+"/device", func(w http.ResponseWriter, r *http.Request) {
            if r.Method != http.MethodGet {
                http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
                return
            }
            if s.getDevice != nil {
                s.getDevice(w, r)
            } else {
				s.DefaultDeviceHandler(w, r)
			}
        })

        // Param endpoint: fqoid is the path after /value/
        s.mux.HandleFunc(prefix+"/value/", func(w http.ResponseWriter, r *http.Request) {
            fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/value/")
            if fqoid == "" {
                http.Error(w, "missing param fqoid", http.StatusBadRequest)
                return
            }
            switch r.Method {
            case http.MethodGet:
                if handler, ok := s.lookupGetParamValue(slot); ok {
                    handler(w, r, slot, fqoid)
               	} else {
				s.DefaultGetParamValueHandler(w, r, slot, fqoid)
				return
			}
            case http.MethodPut, http.MethodPatch:
                if handler, ok := s.lookupSetParamValue(slot); ok {
                    handler(w, r, slot, fqoid)
                } else {
				s.DefaultSetParamValueHandler(w, r, slot, fqoid)
				return
			}
            default:
                http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
            }
        })

        // Asset endpoint: fqoid is the path after /asset/
        s.mux.HandleFunc(prefix+"/asset/", func(w http.ResponseWriter, r *http.Request) {
            fqoid := strings.TrimPrefix(r.URL.Path, prefix+"/asset/")
            if fqoid == "" {
                http.Error(w, "missing asset fqoid", http.StatusBadRequest)
                return
            }
            if r.Method != http.MethodGet {
                http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
                return
            }
            if handler, ok := s.lookupGetAsset(slot); ok {
                handler(w, r, slot, fqoid)
            } else {
				s.DefaultGetAssetHandler(w, r, slot, fqoid)
			}
        })
    }
}
