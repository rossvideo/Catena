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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package rest

import (
	//"io"
	//"log"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// DeviceServer adapts BaseServer to catena.Device semantics.
type DeviceServer struct {
    *Server
}

// NewDeviceServer wires a DeviceManager into BaseServer routing with defaults.
func NewDeviceServer(mgr *catena.DeviceManager) *DeviceServer {
    /*bs := NewServer(
        mgr.ListSlots,
        func(slot int) (Device, bool) {
            dev, ok := mgr.Get(slot)
            if !ok {
                return nil, false
            }
            return dev, true
        },
    )*/

    ds := &DeviceServer{}

    // Provide sane defaults backed by catena.Device.
    /*ds.SetDeviceHandler(func(w http.ResponseWriter, r *http.Request, slot int, dev catena.Device) {
        if r.Method != http.MethodGet {
            http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
            return
        }
        desc, err := dev.Describe(r.Context())
        if err != nil {
            http.Error(w, err.Error(), http.StatusInternalServerError)
            return
        }
        writeJSON(w, desc)
    })

    ds.SetParamHandler(func(w http.ResponseWriter, r *http.Request, slot int, dev catena.Device, path string) {
        switch r.Method {
        case http.MethodGet:
            val, err := dev.GetParam(r.Context(), path)
            if err != nil {
                http.Error(w, err.Error(), http.StatusNotFound)
                return
            }
            writeJSON(w, val)
        case http.MethodPut, http.MethodPatch:
            var v catena.Value
            if err := json.NewDecoder(r.Body).Decode(&v); err != nil {
                http.Error(w, "invalid JSON", http.StatusBadRequest)
                return
            }
            if err := dev.SetParam(r.Context(), path, v); err != nil {
                http.Error(w, err.Error(), http.StatusBadRequest)
                return
            }
            w.WriteHeader(http.StatusNoContent)
        default:
            http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
        }
    })

    ds.SetAssetHandler(func(w http.ResponseWriter, r *http.Request, slot int, dev catena.Device, id string) {
        if r.Method != http.MethodGet {
            http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
            return
        }
        rc, err := dev.GetAsset(r.Context(), id)
        if err != nil {
            http.Error(w, err.Error(), http.StatusNotFound)
            return
        }
        defer rc.Close()
        w.Header().Set("Content-Type", "application/octet-stream")
        if _, err := io.Copy(w, rc); err != nil {
            log.Printf("error streaming asset %s: %v", id, err)
        }
    })*/

    return ds
}

// Device-aware handler setters adapt to BaseServer’s entity handlers.
/*func (s *DeviceServer) SetDeviceHandler(h func(http.ResponseWriter, *http.Request, int, catena.Device)) {
    s.Server.SetEntityHandler(func(w http.ResponseWriter, r *http.Request, slot int, ent Entity) {
        if dev, ok := ent.(catena.Device); ok {
            h(w, r, slot, dev)
            return
        }
        http.Error(w, "invalid entity type", http.StatusInternalServerError)
    })
}

func (s *DeviceServer) SetParamHandler(h func(http.ResponseWriter, *http.Request, int, catena.Device, string)) {
    s.BaseServer.SetParamHandler(func(w http.ResponseWriter, r *http.Request, slot int, ent Entity, path string) {
        if dev, ok := ent.(catena.Device); ok {
            h(w, r, slot, dev, path)
            return
        }
        http.Error(w, "invalid entity type", http.StatusInternalServerError)
    })
}

func (s *DeviceServer) SetAssetHandler(h func(http.ResponseWriter, *http.Request, int, catena.Device, string)) {
    s.BaseServer.SetAssetHandler(func(w http.ResponseWriter, r *http.Request, slot int, ent Entity, id string) {
        if dev, ok := ent.(catena.Device); ok {
            h(w, r, slot, dev, id)
            return
        }
        http.Error(w, "invalid entity type", http.StatusInternalServerError)
    })
}*/
