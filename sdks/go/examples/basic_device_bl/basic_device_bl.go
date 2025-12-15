package main

import (
    "fmt"
    "os"
    "reflect"
    "strconv"
    "sync"

    "github.com/rossvideo/catena/sdks/go/pkg/logger"
    "github.com/rossvideo/catena/sdks/go/pkg/rest"
    "github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// Implementation for registering parameter handlers for every fqoid on a given slot
func registerBasicParamHandlers(srv *rest.Server, params *sync.Map, slot int) {
    srv.RegisterSetParamHandler(slot, func(value any, slot int, fqoid string) catena.StatusResult {
        logger.Info("Slot %d: SetParam %s", slot, fqoid)
        val, ok := params.Load(fqoid)
        if !ok {
            logger.Warning("Slot %d: SetParam %s - param not found", slot, fqoid)
            return catena.NotFound("param not found")
        }
        if reflect.TypeOf(val) != reflect.TypeOf(value) {
            logger.Error("Slot %d: SetParam %s - type mismatch", slot, fqoid)
            return catena.BadRequest("type mismatch")
        }
        params.Store(fqoid, value)
        return catena.NoContent()
    })

    srv.RegisterGetParamHandler(slot, func(slot int, fqoid string) catena.StatusResult {
        logger.Info("Slot %d: GetParam %s", slot, fqoid)
        v, ok := params.Load(fqoid)
        if !ok {
            logger.Warning("Slot %d: GetParam %s - param not found", slot, fqoid)
            return catena.NotFound("param not found")
        }
        return catena.OK(v)
    })
}

// For a given slot implement param handlers for a specific param oid only
func registerSpecificParamHandlers(srv *rest.Server, params *sync.Map, fqoid string, slot int) {
    srv.RegisterSetParamHandler(slot, func(value any, slot int, fqoid_ string) catena.StatusResult {
        if fqoid_ != fqoid {
            return catena.NotImplemented("no handler for fqoid " + fqoid_)
        }
        logger.Info("Slot %d: SetSpecificParam %s", slot, fqoid_)
        val, ok := params.Load(fqoid_)
        if !ok {
            logger.Warning("Slot %d: SetSpecificParam %s - param not found", slot, fqoid_)
            return catena.NotFound("param not found")
        }
        if reflect.TypeOf(val) != reflect.TypeOf(value) {
            logger.Error("Slot %d: SetSpecificParam %s - type mismatch", slot, fqoid_)
            return catena.BadRequest("type mismatch")
        }
        params.Store(fqoid_, value)
        return catena.NoContent()
    })

    srv.RegisterGetParamHandler(slot, func(slot int, fqoid_ string) catena.StatusResult {
        if fqoid_ != fqoid {
            return catena.NotImplemented("no handler for fqoid " + fqoid_)
        }
        logger.Info("Slot %d: GetSpecificParam %s", slot, fqoid_)
        v, ok := params.Load(fqoid_)
        if !ok {
            logger.Warning("Slot %d: GetSpecificParam %s - param not found", slot, fqoid_)
            return catena.NotFound("param not found")
        }
        return catena.OK(v)
    })
}

func main() {
    // Initialize the logger
    logDir := envOr("CATENA_LOG_DIR", "./logs")
    silent := os.Getenv("CATENA_SILENT") == "true"

    err := logger.Init(logger.Config{
        AppName:        "basic_device_bl",
        LogDir:         logDir,
        Silent:         silent,
        MinLevel:       logger.LevelInfo,
        WriteToFile:    true,
        WriteToConsole: true,
    })
    if err != nil {
        fmt.Fprintf(os.Stderr, "failed to initialize logger: %v\n", err)
        os.Exit(1)
    }
    defer logger.Close()

    portStr := envOr("CATENA_PORT", "6254")
    port, err := strconv.Atoi(portStr)
    if err != nil {
        logger.Error("invalid CATENA_PORT: %v", err)
        os.Exit(1)
    }
    logger.Info("Starting Dummy BaseServer on port %d", port)

    var params0 = &sync.Map{}
    params0.Store("alpha", "alpha")
    params0.Store("beta", 42)
    params0.Store("counter", 0)

    var params1 = &sync.Map{}
    params1.Store("alpha", "default")
    params1.Store("beta", float32(3.14))
    params1.Store("status", "idle")

    var params2 = &sync.Map{}
    params2.Store("struct", map[string]interface{}{"field1": 1, "field2": "two"})
    params2.Store("int_list", []int{1, 2, 3, 4, 5})
    params2.Store("float_list", []float32{1.1, 2.2, 3.3})

    slotList := []int{0, 1, 2}

    // Build a BaseServer (decoupled from catena).
    srv := rest.NewServer(slotList)

    // Register param handlers for each device.
    // Device 0: basic param handlers for all params.
    registerBasicParamHandlers(srv, params0, 0)
    // Device 1: specific param handler for "alpha" only.
    registerSpecificParamHandlers(srv, params1, "alpha", 1)
    // Device 2: basic param handlers for all params.
    registerBasicParamHandlers(srv, params2, 2)

    addr := ":" + strconv.Itoa(port)
    logger.Info("Dummy BaseServer listening on %s", addr)
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