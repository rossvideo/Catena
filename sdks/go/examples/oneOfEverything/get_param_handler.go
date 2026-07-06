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

			deviceInfo, ok := buildDeviceDefinition(slot, counter, state)
			if !ok {
				return catena.Param{}, catena.StatusWithCode(catena.StatusCodeNotFound, "device not found")
			}

			device, err := catena.ToDevice(deviceInfo)
			if err != nil {
				return catena.Param{}, catena.StatusWithCode(catena.StatusCodeInternal, "failed to build device: "+err.Error())
			}

			param, found := lookupParam(device.GetProtoDevice(), fqoid)
			if !found {
				return catena.Param{}, catena.StatusWithCode(catena.StatusCodeNotFound, "param not found: "+fqoid)
			}

			return catena.Param{Proto: param}, catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}

// lookupParam resolves a (possibly nested, slash-delimited) fqoid against a
// device's top-level params, descending into sub-params for each path segment.
// A leading slash is tolerated. It returns the matching proto param, or false
// if any segment is missing.
func lookupParam(device *protos.Device, fqoid string) (*protos.Param, bool) {
	trimmed := strings.Trim(fqoid, "/")
	if trimmed == "" {
		return nil, false
	}

	params := device.GetParams()
	var current *protos.Param
	for _, segment := range strings.Split(trimmed, "/") {
		p, ok := params[segment]
		if !ok {
			return nil, false
		}
		current = p
		params = p.GetParams()
	}
	return current, true
}
