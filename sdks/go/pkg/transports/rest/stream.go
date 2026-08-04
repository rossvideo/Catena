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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Transport-side adapters for catena.Stream.
 * @file stream.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package rest

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strings"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// restStream adapts an HTTP response to catena.Stream, emitting each chunk as a
// Server-Sent Events frame. The SSE headers are written lazily on the first
// successful Send so that, if the handler produces nothing, the transport can
// still choose a different HTTP status. marshal is supplied per endpoint so the
// REST transport keeps ownership of its JSON rules.
type restStream[T catena.Message] struct {
	w              http.ResponseWriter
	flusher        http.Flusher
	marshal        func(proto.Message) ([]byte, error)
	ctx            context.Context
	devMode        bool
	headersWritten bool
	sent           int
}

// Send marshals the chunk and writes it as one SSE frame, flushing so the
// client receives it immediately. A non-nil error means the chunk was not
// delivered and the caller should stop streaming; it can come from the request
// context being cancelled (client disconnected), the marshal step failing, or
// the underlying write failing. The cause is not distinguished because every
// case is terminal for this stream.
func (s *restStream[T]) Send(chunk T) error {
	select {
	case <-s.ctx.Done():
		return s.ctx.Err()
	default:
	}

	data, err := s.marshal(chunk.Wire())
	if err != nil {
		return err
	}

	if !s.headersWritten {
		s.writeHeaders()
	}

	if _, err := fmt.Fprintf(s.w, "data: %s\n\n", data); err != nil {
		return err
	}
	s.flusher.Flush()
	s.sent++
	return nil
}

// writeHeaders commits the SSE response headers and a 200 status. It runs once,
// on the first chunk, so an error before any chunk can still set its own status.
func (s *restStream[T]) writeHeaders() {
	s.w.Header().Set("Content-Type", "text/event-stream")
	s.w.Header().Set("Cache-Control", "no-cache")
	s.w.Header().Set("Connection", "keep-alive")
	s.w.WriteHeader(http.StatusOK)
	s.flusher.Flush()
	s.headersWritten = true
}

// sendError reports a StatusResult as an SSE "error" event (per the SseError
// schema). It is used when an error occurs after the response headers have
// already been committed - once a 200 is on the wire the HTTP status can no
// longer change, so the failure is reported in-band as an error event instead.
// The StatusCode is converted to an HTTP status, and the message follows the
// same policy as unary errors: the detailed message is only exposed in dev mode,
// otherwise it is generalized to the standard HTTP status text (a blank message
// is omitted from the payload). It returns any write/flush error.
func (s *restStream[T]) sendError(result catena.StatusResult) error {
	httpStatus := ToHTTPStatus(result.Code)
	message := http.StatusText(httpStatus)
	if s.devMode {
		message = result.Error
	}

	payload := struct {
		Code    int    `json:"code"`
		Message string `json:"message,omitempty"`
	}{Code: httpStatus, Message: message}

	data, err := json.Marshal(payload)
	if err != nil {
		return err
	}

	if _, err := fmt.Fprintf(s.w, "event: error\ndata: %s\n\n", data); err != nil {
		return err
	}
	s.flusher.Flush()
	return nil
}

// firstStream retains only the first sent chunk, silently discarding the rest.
// The REST transport uses it for unary param-info requests, where the streaming
// handler still emits chunks but only the first is written back as a single JSON
// response, so retaining the rest would just waste memory. Discarding is not an
// error: Send always returns nil so the handler runs to completion and can still
// report StatusCodeOk.
type firstStream[T catena.Message] struct {
	item T
	has  bool
}

// Send retains the first chunk and discards any that follow. It always returns nil.
func (s *firstStream[T]) Send(chunk T) error {
	if !s.has {
		s.item = chunk
		s.has = true
	}
	return nil
}

// lastStream retains only the most recently sent chunk. The REST ExecuteCommand
// endpoint is unary: the handler may stream several CommandResponses, but the HTTP
// reply carries a single response, so only the final Send is kept (earlier ones
// are overwritten). Send always returns nil so the handler runs to completion.
type lastStream[T catena.Message] struct {
	item T
	has  bool
}

// Send overwrites the retained chunk with the latest one. It always returns nil.
func (s *lastStream[T]) Send(chunk T) error {
	s.item = chunk
	s.has = true
	return nil
}

// deviceAggregateStream assembles a streamed DeviceRequest response back into
// one complete device. The REST unary device route uses it: the handler may
// stream a Device chunk plus component chunks (params, constraints, menus,
// commands, language packs), but the HTTP reply carries a single device JSON
// body, so each component is merged into the accumulating device. Send always
// returns nil so the handler runs to completion.
type deviceAggregateStream struct {
	device *protos.Device
}

var _ catena.Stream[st2138.DeviceComponent] = (*deviceAggregateStream)(nil)

// Send merges one DeviceComponent chunk into the accumulating device. A Device
// chunk becomes the base (a second Device chunk is proto-merged into it);
// component chunks are placed at the location their oid names, creating the
// device skeleton and any intermediate params on demand so chunk order does
// not matter. Chunks with a nil body are ignored.
func (s *deviceAggregateStream) Send(chunk st2138.DeviceComponent) error {
	if chunk.Proto == nil {
		return nil
	}
	switch kind := chunk.Proto.Kind.(type) {
	case *protos.DeviceComponent_Device:
		if kind.Device == nil {
			return nil
		}
		if s.device == nil {
			s.device = kind.Device
		} else {
			proto.Merge(s.device, kind.Device)
		}
	case *protos.DeviceComponent_Param:
		if kind.Param.GetParam() == nil {
			return nil
		}
		s.ensureDevice()
		s.device.Params = setNestedParam(s.device.Params, kind.Param.GetOid(), kind.Param.GetParam())
	case *protos.DeviceComponent_SharedConstraint:
		if kind.SharedConstraint.GetConstraint() == nil {
			return nil
		}
		s.ensureDevice()
		if s.device.Constraints == nil {
			s.device.Constraints = map[string]*protos.Constraint{}
		}
		s.device.Constraints[kind.SharedConstraint.GetOid()] = kind.SharedConstraint.GetConstraint()
	case *protos.DeviceComponent_Menu:
		if kind.Menu.GetMenu() == nil {
			return nil
		}
		// The oid identifies the menu as "menu-group-name/menu-name".
		group, menu, found := strings.Cut(kind.Menu.GetOid(), "/")
		if !found {
			return nil
		}
		s.ensureDevice()
		if s.device.MenuGroups == nil {
			s.device.MenuGroups = map[string]*protos.MenuGroup{}
		}
		menuGroup, ok := s.device.MenuGroups[group]
		if !ok || menuGroup == nil {
			menuGroup = &protos.MenuGroup{}
			s.device.MenuGroups[group] = menuGroup
		}
		if menuGroup.Menus == nil {
			menuGroup.Menus = map[string]*protos.Menu{}
		}
		menuGroup.Menus[menu] = kind.Menu.GetMenu()
	case *protos.DeviceComponent_Command:
		if kind.Command.GetCommand() == nil {
			return nil
		}
		s.ensureDevice()
		s.device.Commands = setNestedParam(s.device.Commands, kind.Command.GetOid(), kind.Command.GetCommand())
	case *protos.DeviceComponent_LanguagePack:
		if kind.LanguagePack.GetLanguagePack() == nil {
			return nil
		}
		s.ensureDevice()
		if s.device.LanguagePacks == nil {
			s.device.LanguagePacks = &protos.LanguagePacks{}
		}
		if s.device.LanguagePacks.Packs == nil {
			s.device.LanguagePacks.Packs = map[string]*protos.LanguagePack{}
		}
		s.device.LanguagePacks.Packs[kind.LanguagePack.GetLanguage()] = kind.LanguagePack.GetLanguagePack()
	}
	return nil
}

// ensureDevice creates an empty base device when a component chunk arrives
// before (or without) a Device chunk.
func (s *deviceAggregateStream) ensureDevice() {
	if s.device == nil {
		s.device = &protos.Device{}
	}
}

// result returns the assembled device. ok is false when the handler produced
// no chunks at all.
func (s *deviceAggregateStream) result() (st2138.Device, bool) {
	if s.device == nil {
		return st2138.Device{}, false
	}
	return st2138.Device{Proto: s.device}, true
}

// setNestedParam places param at the location oid names inside a params map,
// walking the oid's JSON-pointer segments through sub-param maps and creating
// intermediate params on demand (so a "monitor/eq" chunk can arrive before, or
// without, its "monitor" parent). If an entry already exists at the final
// segment — for example a parent synthesized while placing an earlier child —
// the incoming param is proto-merged into it so nested children are preserved.
// It returns the (possibly newly created) map.
func setNestedParam(params map[string]*protos.Param, oid string, param *protos.Param) map[string]*protos.Param {
	if params == nil {
		params = map[string]*protos.Param{}
	}
	segments := strings.Split(oid, "/")
	current := params
	for i, segment := range segments {
		if i == len(segments)-1 {
			if existing, ok := current[segment]; ok && existing != nil {
				proto.Merge(existing, param)
			} else {
				current[segment] = param
			}
			break
		}
		parent, ok := current[segment]
		if !ok || parent == nil {
			parent = &protos.Param{}
			current[segment] = parent
		}
		if parent.Params == nil {
			parent.Params = map[string]*protos.Param{}
		}
		current = parent.Params
	}
	return params
}

// assetAggregateStream assembles a streamed ReadAsset response back into one
// complete asset. The REST unary asset route uses it: the handler may break a
// large asset into several chunks, but the HTTP reply carries a single
// external_object_payload JSON body, so the embedded payload bytes of every
// chunk are concatenated in send order while metadata, digest, cachable, and
// the payload encoding are taken from the first chunk. Send always returns nil
// so the handler runs to completion.
type assetAggregateStream struct {
	first   *protos.ExternalObjectPayload
	data    []byte
	hasData bool
}

var _ catena.Stream[st2138.Asset] = (*assetAggregateStream)(nil)

// Send accumulates one asset chunk. Chunks with a nil proto are ignored.
func (s *assetAggregateStream) Send(chunk st2138.Asset) error {
	if chunk.Proto == nil {
		return nil
	}
	if s.first == nil {
		s.first = chunk.Proto
	}
	if payload := chunk.Proto.GetPayload().GetPayload(); len(payload) > 0 {
		s.data = append(s.data, payload...)
		s.hasData = true
	}
	return nil
}

// result returns the assembled asset. ok is false when the handler produced no
// chunks at all. The returned asset clones the first chunk's proto so the
// concatenated payload does not alias any handler-owned buffer.
func (s *assetAggregateStream) result() (st2138.Asset, bool) {
	if s.first == nil {
		return st2138.Asset{}, false
	}
	out := proto.Clone(s.first).(*protos.ExternalObjectPayload)
	if s.hasData {
		if out.Payload == nil {
			out.Payload = &protos.DataPayload{}
		}
		out.Payload.Kind = &protos.DataPayload_Payload{Payload: s.data}
	}
	return st2138.Asset{Proto: out}, true
}
