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
 * @brief handler types for the Catena SDK.
 * @file handlers.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-10-22
 */

package catena

import "github.com/rossvideo/catena/sdks/go/pkg/st2138"

// Handler function types used by both REST and gRPC servers.
//
// Every endpoint handler has the shape (ctx, req[, stream]): ctx is the
// caller's HandlerContext, req is the endpoint's XxxRequest struct carrying the
// request arguments, and stream is present only for streaming endpoints.
//
// A handler runs only after the SDK's authorization gate has enforced scope
// and endpoint access for the caller, so handlers do not re-check
// authorization. When a product struct is registered for a slot (see
// RegisterProductStruct), the SDK owns product/* requests and handlers never
// see them; otherwise product/* requests fall through to the handler.

// GetDeviceRequest carries the arguments of a GetDevice request.
type GetDeviceRequest struct {
	Slot uint16
}

// DeviceHandler streams the device model for a slot (GetDevice /
// DeviceRequest). A small device is typically emitted as a single
// st2138.ComponentDevice chunk through stream.Send; a large one may be broken
// into a ComponentDevice skeleton followed by ComponentParam /
// ComponentConstraint / ComponentMenu / ComponentCommand /
// ComponentLanguagePack chunks that fill in the pieces. The handler returns a
// terminal StatusResult: Ok once every chunk has been sent, or an error status
// (e.g. StatusCodeNotFound) when the slot has no device to report. A Send
// error means the chunk could not be delivered - the client may have
// disconnected, the transport hit an encoding or write failure, or the server
// is shutting down (the SDK wraps the stream so Send fails once shutdown
// begins) - so the handler should stop and return. A handler that can produce
// a whole st2138.Device can delegate the chunking to
// DeviceComponentsForRequest in device_components.go.
type DeviceHandler func(ctx HandlerContext, req GetDeviceRequest, stream Stream[st2138.DeviceComponent]) StatusResult

// GetValueRequest carries the arguments of a GetValue request.
type GetValueRequest struct {
	Slot  uint16
	Fqoid string
}

// GetValueHandler returns the current value of the parameter at fqoid for a
// slot (GetValue). It returns the value with an Ok status, or a zero Value
// with an error status (e.g. StatusCodeNotFound for an unknown fqoid).
//
// Registering a GetValueHandler is optional: when a slot has none, GetValue is
// served by deriving the value from that slot's GetParamHandler. See
// RegisterGetValueHandler.
type GetValueHandler func(ctx HandlerContext, req GetValueRequest) (st2138.Value, StatusResult)

// GetParamRequest carries the arguments of a GetParam request.
type GetParamRequest struct {
	Slot  uint16
	Fqoid string
}

// GetParamHandler returns the full parameter (metadata + value) at fqoid for a
// slot (GetParam). It returns the param with an Ok status, or a zero Param
// with an error status (e.g. StatusCodeNotFound for an unknown fqoid).
type GetParamHandler func(ctx HandlerContext, req GetParamRequest) (st2138.Param, StatusResult)

// SetValueEntry is a single fqoid/value pair within a SetValue request.
type SetValueEntry struct {
	Fqoid string
	Value any
}

// SetValueRequest carries the arguments of a SetValue request.
type SetValueRequest struct {
	Slot    uint16
	Entries []SetValueEntry
}

// SetValueHandler applies one or more parameter values for a slot. Single-value
// endpoints invoke it with a one-element slice; multi-value endpoints pass the
// full slice. The handler must apply the batch atomically (all-or-nothing): if
// any entry cannot be applied, it must leave every value unchanged and return
// an error status. On success it returns an Ok status.
type SetValueHandler func(ctx HandlerContext, req SetValueRequest) StatusResult

// ReadAssetRequest carries the arguments of a ReadAsset request.
type ReadAssetRequest struct {
	Slot  uint16
	Fqoid string
}

// ReadAssetHandler streams the asset stored at fqoid for a slot (ReadAsset /
// ExternalObjectRequest). A small asset is typically emitted as a single
// st2138.Asset chunk through stream.Send; a large one may be broken into
// several chunks whose embedded payload bytes the client concatenates in send
// order (metadata, digest, and cachable are taken from the first chunk). The
// handler returns a terminal StatusResult: Ok once every chunk has been sent,
// or an error status (e.g. StatusCodeNotFound for an unknown fqoid). A Send
// error means the chunk could not be delivered - the client may have
// disconnected, the transport hit an encoding or write failure, or the server
// is shutting down (the SDK wraps the stream so Send fails once shutdown
// begins) - so the handler should stop and return.
type ReadAssetHandler func(ctx HandlerContext, req ReadAssetRequest, stream Stream[st2138.Asset]) StatusResult

// CreateAssetRequest carries the arguments of a CreateAsset request.
type CreateAssetRequest struct {
	Slot  uint16
	Fqoid string
	Asset st2138.Asset
}

// CreateAssetHandler stores a new asset at fqoid (HTTP POST / CreateAsset).
// Business logic decides create-vs-conflict semantics and reports the outcome
// in the returned StatusResult.
type CreateAssetHandler func(ctx HandlerContext, req CreateAssetRequest) StatusResult

// UpdateAssetRequest carries the arguments of an UpdateAsset request.
type UpdateAssetRequest struct {
	Slot  uint16
	Fqoid string
	Asset st2138.Asset
}

// UpdateAssetHandler replaces an existing asset at fqoid (HTTP PUT /
// UpdateAsset). Business logic decides overwrite-vs-not-found semantics and
// reports the outcome in the returned StatusResult.
type UpdateAssetHandler func(ctx HandlerContext, req UpdateAssetRequest) StatusResult

// DeleteAssetRequest carries the arguments of a DeleteAsset request.
type DeleteAssetRequest struct {
	Slot  uint16
	Fqoid string
}

// DeleteAssetHandler removes the asset at fqoid (HTTP DELETE / DeleteAsset).
// Business logic decides delete-vs-not-found semantics and reports the outcome
// in the returned StatusResult.
type DeleteAssetHandler func(ctx HandlerContext, req DeleteAssetRequest) StatusResult

// ExecuteCommandRequest carries the arguments of an ExecuteCommand request.
// Respond=false means the caller opted out of responses.
type ExecuteCommandRequest struct {
	Slot         uint16
	CommandFqoid string
	Payload      any
	Respond      bool
}

// ExecuteCommandHandler executes the command at req.CommandFqoid for a slot with
// the payload req.Payload, emitting any responses through stream.Send and returning
// a terminal StatusResult. req.Respond=false means the caller opted out of
// responses; the SDK already substitutes a stream that discards chunks, so the
// handler may either skip sending or send as usual. A Send error means the
// chunk could not be delivered - the client may have disconnected, the
// transport hit an encoding or write failure, or the server is shutting down
// (the SDK wraps the stream so Send fails once shutdown begins) - so the
// handler should stop and return.
type ExecuteCommandHandler func(ctx HandlerContext, req ExecuteCommandRequest, stream Stream[st2138.CommandResponse]) StatusResult

// ParamInfoRequest carries the arguments of a ParamInfo request.
type ParamInfoRequest struct {
	Slot      uint16
	OidPrefix string
	Recursive bool
}

// ParamInfoHandler streams parameter information for a slot. The handler emits
// one ParamInfo chunk through stream.Send per parameter under oidPrefix
// (descending into children when recursive is true) and returns a terminal
// StatusResult: Ok once every descriptor has been sent, NotFound for an
// unknown oidPrefix, or Internal when a Send fails. A Send error means the
// chunk could not be delivered - the client may have disconnected, the
// transport hit an encoding or write failure, or the server is shutting down
// (the SDK wraps the stream so Send fails once shutdown begins) - so the
// handler should stop and return without polling ctx itself. See
// ParamInfosForRequest in param_info.go for a reference implementation.
//
// Registering a ParamInfoHandler is optional: when a slot has none, ParamInfo
// for a specific oidPrefix is served by deriving descriptors from that slot's
// GetParamHandler. The whole-tree request (oidPrefix == "") cannot be derived
// this way and requires a real handler. See RegisterParamInfoHandler.
type ParamInfoHandler func(ctx HandlerContext, req ParamInfoRequest, stream Stream[st2138.ParamInfo]) StatusResult

// ListLanguagesRequest carries the arguments of a ListLanguages request.
type ListLanguagesRequest struct {
	Slot uint16
}

// ListLanguagesHandler returns the language codes supported by the device model
// at a slot (e.g. ["en", "fr"]). An empty slice indicates no multi-lingual support.
type ListLanguagesHandler func(ctx HandlerContext, req ListLanguagesRequest) ([]string, StatusResult)

// ReadLanguagePackRequest carries the arguments of a ReadLanguagePack request.
type ReadLanguagePackRequest struct {
	Slot     uint16
	Language string
}

// ReadLanguagePackHandler returns the language pack for a language code at a
// slot (e.g. "en"). It returns the pack with an Ok status, or a zero
// LanguagePack with an error status (e.g. StatusCodeNotFound for an unknown
// language).
type ReadLanguagePackHandler func(ctx HandlerContext, req ReadLanguagePackRequest) (LanguagePack, StatusResult)

// CreateLanguagePackRequest carries the arguments of a CreateLanguagePack
// request.
type CreateLanguagePackRequest struct {
	Slot         uint16
	Language     string
	LanguagePack LanguagePack
}

// CreateLanguagePackHandler stores a new language pack for a language code at
// a slot. Business logic decides create-vs-conflict semantics and reports the
// outcome in the returned StatusResult.
type CreateLanguagePackHandler func(ctx HandlerContext, req CreateLanguagePackRequest) StatusResult

// UpdateLanguagePackRequest carries the arguments of an UpdateLanguagePack
// request.
type UpdateLanguagePackRequest struct {
	Slot         uint16
	Language     string
	LanguagePack LanguagePack
}

// UpdateLanguagePackHandler replaces the language pack for a language code at
// a slot. Business logic decides overwrite-vs-not-found semantics and reports
// the outcome in the returned StatusResult.
type UpdateLanguagePackHandler func(ctx HandlerContext, req UpdateLanguagePackRequest) StatusResult

// DeleteLanguagePackRequest carries the arguments of a DeleteLanguagePack
// request.
type DeleteLanguagePackRequest struct {
	Slot     uint16
	Language string
}

// DeleteLanguagePackHandler removes the language pack for a language code at a
// slot. Business logic decides delete-vs-not-found semantics and reports the
// outcome in the returned StatusResult.
type DeleteLanguagePackHandler func(ctx HandlerContext, req DeleteLanguagePackRequest) StatusResult

// HeartbeatHandler is invoked for its slot on every heartbeat tick while
// StartHeartbeat is running. It is called outside the server lock; a panic is
// recovered and logged, so one slot's failure does not stop the others.
type HeartbeatHandler func(slot uint16)

// AccessHandler decides whether the caller described by ctx may use an
// endpoint. It is global (not per-slot) and is consulted by the authorization
// gate - only when authorization is enabled - after the scope checks and
// before any endpoint handler runs. Returning false rejects the request with
// StatusCodePermissionDenied. Registering nil resets to allow-all.
type AccessHandler func(endpointType EndpointType, ctx HandlerContext) bool
