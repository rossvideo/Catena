package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// registerGetParamHandlers wires GetParam for slot 0. GetParam returns the full
// parameter (metadata + current value) for a single OID, unlike GetValue (value
// only) or ParamInfo (descriptor only).
func registerGetParamHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	srv.RegisterGetParamHandler(0, func(slot uint16, fqoid string, ctx catena.HandlerContext) (*catena.Param, catena.StatusResult) {
		logger.Info("GetParam", "slot", slot, "fqoid", fqoid)
		if !ctx.HasReadScope(catena.ScopeCfg) {
			return nil, catena.StatusWithCode(catena.StatusCodePermissionDenied, "configuration scope required")
		}

		switch fqoid {
		case "counter", "/counter":
			param := catena.NewParamInt32(counter.GetValue()).
				WithName(catena.NewPolyglotText("en", "Counter").
					With("es", "Contador").
					With("fr", "Compteur").
					With("ja", "カウンター")).
				WithMinimalSet(true)
			return param, catena.StatusWithCode(catena.StatusCodeOk, "")
		case "running", "/running":
			param := catena.NewParamInt32(counter.RunningInt32()).
				WithName(catena.NewPolyglotText("en", "Counter Running Status")).
				WithReadOnly(true).
				WithMinimalSet(true)
			return param, catena.StatusWithCode(catena.StatusCodeOk, "")
		default:
			return nil, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
		}
	})
}
