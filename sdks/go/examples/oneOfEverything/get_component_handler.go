package main

import (
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// registerGetComponentHandlers wires GetComponent for every slot. GetComponent
// returns one device component (param, constraint, menu, command, or
// language pack) for a single oid. The on-the-wire GetParam route is served by
// this same handler with kind PARAM.
//
// Rather than maintaining a separate switch per slot, each handler rebuilds the
// slot's device definition via buildDeviceDefinition and looks the requested
// oid up inside the kind's namespace. This keeps GetComponent in lockstep with
// GetDevice: a component can never drift between the two endpoints because both
// come from one source. Language packs live in the shared language store, which
// also backs the language-pack endpoints.
func registerGetComponentHandlers(srv catena.Server, counter *CounterState, state *ExampleState, languages *LanguageStore) {
	for _, slot := range slotList {
		srv.RegisterGetComponentHandler(slot, func(ctx catena.HandlerContext, req catena.GetComponentRequest) (st2138.ComponentResponse, catena.StatusResult) {
			ctx.Logger().Info("GetComponent", "slot", req.Slot, "oid", req.Oid, "kind", req.Kind)

			switch req.Kind {
			case st2138.ComponentKindLanguagePack:
				return languagePackComponent(languages, req.Slot, req.Oid)
			case st2138.ComponentKindParam, st2138.ComponentKindConstraint, st2138.ComponentKindMenu, st2138.ComponentKindCommand:
				return deviceComponent(req, counter, state)
			default:
				return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeInvalidArgument, "invalid component kind")
			}
		})
	}
}

func deviceComponent(req catena.GetComponentRequest, counter *CounterState, state *ExampleState) (st2138.ComponentResponse, catena.StatusResult) {
	device, status := buildDeviceDefinition(req.Slot, counter, state)
	if status.IsError() {
		return st2138.ComponentResponse{}, status
	}

	switch req.Kind {
	case st2138.ComponentKindParam:
		param, found := lookupNested(device.Proto.GetParams(), req.Oid)
		if !found {
			return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeNotFound, "param not found: "+req.Oid)
		}
		return catena.Reply(st2138.ComponentResponseParam(req.Oid, &st2138.Param{Proto: param}))
	case st2138.ComponentKindCommand:
		command, found := lookupNested(device.Proto.GetCommands(), req.Oid)
		if !found {
			return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeNotFound, "command not found: "+req.Oid)
		}
		return catena.Reply(st2138.ComponentResponseCommand(req.Oid, &st2138.Param{Proto: command}))
	case st2138.ComponentKindConstraint:
		constraint, found := device.Proto.GetConstraints()[req.Oid]
		if !found {
			return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeNotFound, "constraint not found: "+req.Oid)
		}
		return catena.Reply(st2138.ComponentResponseConstraint(req.Oid, &st2138.Constraint{Proto: constraint}))
	case st2138.ComponentKindMenu:
		menu, found := lookupMenu(device.Proto.GetMenuGroups(), req.Oid)
		if !found {
			return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeNotFound, "menu not found: "+req.Oid)
		}
		return catena.Reply(st2138.ComponentResponseMenu(req.Oid, &st2138.Menu{Proto: menu}))
	default:
		return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeInvalidArgument, "invalid component kind")
	}
}

func languagePackComponent(languages *LanguageStore, slot uint16, language string) (st2138.ComponentResponse, catena.StatusResult) {
	pack, ok := languages.Get(slot, language)
	if !ok {
		return catena.ReplyError[st2138.ComponentResponse](catena.StatusCodeNotFound, "language not found: "+language)
	}
	return catena.Reply(st2138.ComponentResponseLanguagePack(language, pack.GetName(), pack.GetWords()))
}

// lookupNested resolves a (possibly nested, slash-delimited) oid against a
// param map, descending into sub-params for each path segment. The oid is not
// normalized: a leading, trailing, or doubled slash yields an empty segment
// that matches no param, so malformed oids are rejected here exactly as the
// transports reject them. It returns the matching proto param, or false if the
// oid is empty or any segment is missing.
func lookupNested(params map[string]*protos.Param, oid string) (*protos.Param, bool) {
	if oid == "" {
		return nil, false
	}

	var current *protos.Param
	for _, segment := range strings.Split(oid, "/") {
		p, ok := params[segment]
		if !ok {
			return nil, false
		}
		current = p
		params = p.GetParams()
	}
	return current, true
}

// lookupMenu resolves "menu-group/menu-name" against the device's menu groups.
func lookupMenu(menuGroup map[string]*protos.MenuGroup, oid string) (*protos.Menu, bool) {
	groupName, menuName, ok := strings.Cut(oid, "/")
	if !ok || groupName == "" || menuName == "" {
		return nil, false
	}
	group := menuGroup[groupName]
	if group == nil {
		return nil, false
	}
	menu, found := group.GetMenus()[menuName]
	return menu, found
}
