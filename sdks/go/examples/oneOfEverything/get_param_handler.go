package main

import (
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// registerGetParamHandlers wires GetParam for every slot. GetParam returns the
// full parameter (metadata + current value) for a single OID, unlike GetValue
// (value only) or ParamInfo (descriptor only).
//
// Rather than maintaining a separate switch per slot, each handler rebuilds the
// slot's device definition via buildDeviceDefinition and looks the requested
// OID up inside it. This keeps GetParam in lockstep with GetDevice: a param can
// never drift between the two endpoints because both come from one source.
func registerGetParamHandlers(srv catena.Server, counter *CounterState, state *ExampleState) {
	for _, slot := range slotList {
		srv.RegisterGetParamHandler(slot, func(slot uint16, fqoid string, ctx catena.HandlerContext) (catena.Param, catena.StatusResult) {
			logger.Info("GetParam", "slot", slot, "fqoid", fqoid)

			device, ok := buildDeviceDefinition(slot, counter, state)
			if !ok {
				return catena.Param{}, catena.StatusWithCode(catena.StatusCodeNotFound, "device not found")
			}

			param, found := lookupParam(device.ToProtoDevice(), fqoid)
			if !found {
				return catena.Param{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
			}

			return catena.Param{Proto: param}, catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}

// lookupParam resolves a (possibly nested, slash-delimited) fqoid against a
// device's top-level params, descending into sub-params for each path segment.
// The fqoid is not normalized: a leading, trailing, or doubled slash yields an
// empty segment that matches no param, so malformed fqoids are rejected here
// exactly as the transports and GetValue reject them. It returns the matching
// proto param, or false if the fqoid is empty or any segment is missing.
func lookupParam(device *protos.Device, fqoid string) (*protos.Param, bool) {
	if fqoid == "" {
		return nil, false
	}

	params := device.GetParams()
	var current *protos.Param
	for _, segment := range strings.Split(fqoid, "/") {
		p, ok := params[segment]
		if !ok {
			return nil, false
		}
		current = p
		params = p.GetParams()
	}
	return current, true
}
