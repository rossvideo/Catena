package main

import (
    "encoding/json"
    "io"
    "log"
    "net/http"
    "os"
    "strconv"

    "github.com/rossvideo/catena/sdks/go/pkg/catena"
    "github.com/rossvideo/catena/sdks/go/pkg/rest"
)

//different handlers for each method vs switch case in one handler
//instead of callbacks per endpoint, could have default handlers in server that could be overridden?
//instead of callbacks, declare interface and callback is abstract function on interface, tgive interface to device

//business logic optional, real device manager
func initBusinessLogicV2(srv *rest.Server, mgr *catena.DeviceManager) {
    // Device callback
    srv.SetDeviceHandler(func(w http.ResponseWriter, r *http.Request, slot int, dev catena.Device) {
        desc, err := dev.Describe(r.Context())
        if err != nil {
            http.Error(w, err.Error(), http.StatusInternalServerError)
            return
        }
        writeJSON(w, desc)
    })

    // Param callback
    srv.SetParamHandler(func(w http.ResponseWriter, r *http.Request, slot int, dev catena.Device, path string) {
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

    // Asset callback
    srv.SetAssetHandler(func(w http.ResponseWriter, r *http.Request, slot int, dev catena.Device, id string) {
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
    })
}

func main() {
    assetRoot := envOr("CATENA_ASSET_ROOT", "./static")
    portStr := envOr("CATENA_PORT", "6254")
    port, err := strconv.Atoi(portStr)
    if err != nil {
        log.Fatalf("invalid CATENA_PORT: %v", err)
    }

    // Build multiple devices.
    devA := catena.NewSimpleDevice(assetRoot + "/devA")
    devB := catena.NewSimpleDevice(assetRoot + "/devB")

    mgr := catena.NewDeviceManager()
    mgr.Register(0, devA)
    mgr.Register(1, devB)

    srv := rest.NewServer(mgr)

    // Register business-logic callbacks per endpoint.
    initBusinessLogic(srv, mgr)

    mux := http.NewServeMux()
    srv.RegisterRoutes(mux)

    addr := ":" + strconv.Itoa(port)
    log.Printf("Catena Go REST listening on %s", addr)
    if err := http.ListenAndServe(addr, mux); err != nil {
        log.Fatal(err)
    }
}

func envOr(key, def string) string {
    if v := os.Getenv(key); v != "" {
        return v
    }
    return def
}

func writeJSON(w http.ResponseWriter, v any) {
    w.Header().Set("Content-Type", "application/json")
    _ = json.NewEncoder(w).Encode(v)
}