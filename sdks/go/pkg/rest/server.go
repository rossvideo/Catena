package rest

import (
	"encoding/json"
	"io"
	"log"
	"net/http"
	"strings"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

type Server struct {
	dev catena.Device
}

func NewServer(dev catena.Device) *Server {
	return &Server{dev: dev}
}

func (s *Server) RegisterRoutes(mux *http.ServeMux) {
	mux.HandleFunc("/st2138-api/v1/0/device", s.handleDevice) // /st2138-api/v1/device
	mux.HandleFunc("/st2138-api/v1/0/param/", s.handleParam)  // /st2138-api/v1/param/{path}
	mux.HandleFunc("/st2138-api/v1/0/asset/", s.handleAsset)  // /st2138-api/v1/asset/{id}
}

func (s *Server) handleDevice(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	desc, err := s.dev.Describe(r.Context())
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	writeJSON(w, desc)
}

func (s *Server) handleParam(w http.ResponseWriter, r *http.Request) {
	// URL: /st2138-api/v1/0/param/{path}
	path := strings.TrimPrefix(r.URL.Path, "/st2138-api/v1/0/param/")
	if path == "" {
		http.Error(w, "missing param path", http.StatusBadRequest)
		return
	}

	switch r.Method {
	case http.MethodGet:
		val, err := s.dev.GetParam(r.Context(), path)
		if err != nil {
			http.Error(w, err.Error(), http.StatusNotFound)
			return
		}
		writeJSON(w, val)

	case http.MethodPut:
		var val catena.Value
		if err := json.NewDecoder(r.Body).Decode(&val); err != nil {
			http.Error(w, "invalid JSON", http.StatusBadRequest)
			return
		}
		if err := s.dev.SetParam(r.Context(), path, val); err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		w.WriteHeader(http.StatusNoContent)

	default:
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
	}
}

func (s *Server) handleAsset(w http.ResponseWriter, r *http.Request) {
	// URL: /st2138-api/v1/0/asset/{id}

	id := strings.TrimPrefix(r.URL.Path, "/st2138-api/v1/0/asset/")
	if id == "" {
		http.Error(w, "missing asset id", http.StatusBadRequest)
		return
	}

	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	rc, err := s.dev.GetAsset(r.Context(), id)
	if err != nil {
		http.Error(w, err.Error(), http.StatusNotFound)
		return
	}
	defer rc.Close()

	// For demo: assume generic binary; in real Catena you'd set correct type
	w.Header().Set("Content-Type", "application/octet-stream")
	if _, err := io.Copy(w, rc); err != nil {
		log.Printf("error streaming asset %s: %v", id, err)
	}
}

func writeJSON(w http.ResponseWriter, v any) {
	w.Header().Set("Content-Type", "application/json")
	if err := json.NewEncoder(w).Encode(v); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}
