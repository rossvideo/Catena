package main

import (
    "reflect"
    "log"
    "net/http"
    "os"
    "strconv"
    "sync"

    "github.com/rossvideo/catena/sdks/go/internal"
    "github.com/rossvideo/catena/sdks/go/pkg/rest"
)

//callback for connect endpoint that returns go channel to connection manager

//Implementation for registering parameter handlers for every fqoid on a given slot
func registerBasicParamHandlers(srv *rest.Server, params *sync.Map, slot int) {
    srv.RegisterSetParamHandler(slot, func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) {
        log.Printf("Slot %d: SetParam %s", slot, fqoid)
        v, err := internal.ReadValueFromRequest(r)
        if err != nil {
            http.Error(w, "invalid JSON", http.StatusBadRequest)
            return
        }
        val, ok := params.Load(fqoid)
        if !ok {
            http.Error(w, "param not found", http.StatusNotFound)
            return
        }
        if reflect.TypeOf(val) != reflect.TypeOf(v) {
            http.Error(w, "type mismatch", http.StatusBadRequest)
            return
        }
        params.Store(fqoid, v)
        w.WriteHeader(http.StatusNoContent)
    })

    srv.RegisterGetParamHandler(slot, func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) {
        log.Printf("Slot %d: GetParam %s", slot, fqoid)
        v, ok := params.Load(fqoid)
        if !ok {
            http.Error(w, "param not found", http.StatusNotFound)
            return
        }
        internal.WriteResponseJSON(w, v)
    })
}

//For a given slot implement param handlers for a specific param oid only
func registerSpecificParamHandlers(srv *rest.Server, params *sync.Map, fqoid string, slot int) {
    //Implementation for registering parameter handlers
    srv.RegisterSetParamHandler(slot, func(w http.ResponseWriter, r *http.Request, slot int, fqoid_ string) {
        if fqoid_ != fqoid {
            srv.DefaultSetParamValueHandler(w, r, slot, fqoid_)
            return
        }
        log.Printf("Slot %d: SetSpecificParam %s", slot, fqoid_)
        v, err := internal.ReadValueFromRequest(r)
        if err != nil {
            http.Error(w, "invalid JSON", http.StatusBadRequest)
            return
        }
        val, ok := params.Load(fqoid_)
        if !ok {
            http.Error(w, "param not found", http.StatusNotFound)
            return
        }
        if reflect.TypeOf(val) != reflect.TypeOf(v) {
            http.Error(w, "type mismatch", http.StatusBadRequest)
            return
        }
        params.Store(fqoid_, v)
        w.WriteHeader(http.StatusNoContent)
    })

    srv.RegisterGetParamHandler(slot, func(w http.ResponseWriter, r *http.Request, slot int, fqoid_ string) {
        if fqoid_ != fqoid {
            srv.DefaultGetParamValueHandler(w, r, slot, fqoid_)
            return
        }
        log.Printf("Slot %d: GetSpecificParam %s", slot, fqoid_)
        v, ok := params.Load(fqoid_)
        if !ok {
            http.Error(w, "param not found", http.StatusNotFound)
            return
        }
        internal.WriteResponseJSON(w, v)
    })
}

func main() {
    portStr := envOr("CATENA_PORT", "6254")
    port, err := strconv.Atoi(portStr)
    if err != nil {
        log.Fatalf("invalid CATENA_PORT: %v", err)
    }
    log.Printf("Starting Dummy BaseServer on port %d", port)

    var params0 = &sync.Map{}
    params0.Store("alpha", "alpha")
    params0.Store("beta", 42)
    params0.Store("counter", 0)
    var params1 = &sync.Map{}
    params1.Store("alpha", "default")
    params1.Store("beta", float32(3.14))
    params1.Store("status", "idle")

    slotList := []int{0, 1}

    // Build a BaseServer (decoupled from catena).
    srv := rest.NewServer(slotList)

    // Register param handlers for each device.
    // Device 0: basic param handlers for all params.
    registerBasicParamHandlers(srv, params0, 0)
    // Device 1: specific param handler for "alpha" only.
    registerSpecificParamHandlers(srv, params1, "alpha", 1)

    addr := ":" + strconv.Itoa(port)
    log.Printf("Dummy BaseServer listening on %s", addr)
    srv.StartHTTPServer(port)

    // Block forever.
    select {}
}

func envOr(key, def string) string {
    if v := os.Getenv(key); v != "" {
        return v
    }
    return def
}
