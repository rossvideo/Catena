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
 * @brief JSON utilities for the Catena REST API.
 * @file json_utils.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-04
 */

package rest

import (
	"encoding/json"
	"fmt"
	"io"
	"mime"
	"net/http"
	"reflect"
	"strconv"

	"github.com/valyala/fastjson"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
)

// protoMarshalOpts is the shared marshaller for all proto-to-JSON conversions
// in the REST layer. Uses proto field names and omits unpopulated fields.
var protoMarshalOpts = protojson.MarshalOptions{
	UseProtoNames:   true,
	EmitUnpopulated: false,
}

// deviceMarshalOpts uses EmitUnpopulated so proto3 default values (e.g. slot:0)
// are visible in the output. A cleanup pass then strips the undesirable empties.
var deviceMarshalOpts = protojson.MarshalOptions{
	UseProtoNames:   true,
	EmitUnpopulated: true,
}

// MarshalProtoJSON marshals a proto message to JSON using proto field names
// and without emitting unpopulated fields.
func MarshalProtoJSON(msg proto.Message) ([]byte, error) {
	return protoMarshalOpts.Marshal(msg)
}

// MarshalDeviceJSON marshals a protos.Device to JSON with SMPTE-compliant cleanup.
// Uses EmitUnpopulated so proto3 default values (e.g. slot:0) are visible,
// then strips fields with empty values (null, {}, [], "") at all nesting
// levels since these represent unset proto fields, not meaningful data.
// Also strips schema-forbidden fields that have default zero values and
// the "response" field from params (only valid on commands).
func MarshalDeviceJSON(device *protos.Device) ([]byte, error) {
	if device == nil {
		return nil, nil
	}
	data, err := deviceMarshalOpts.Marshal(device)
	if err != nil {
		return nil, err
	}
	return cleanDeviceJSON(data)
}

// MarshalComponentParamJSON marshals a GetParam response (component param) to
// JSON with the same SMPTE-compliant treatment as MarshalDeviceJSON. It uses
// EmitUnpopulated so proto3 defaults that are meaningful for a param survive
// (e.g. a constraint's min_value:0, or a current value of 0/0.0/""), then
// strips the schema-forbidden extras. Without this the shared marshaller would
// drop those zero-valued fields, since it is a bare Param outside a Device.
func MarshalComponentParamJSON(component *protos.DeviceComponent_ComponentParam) ([]byte, error) {
	if component == nil {
		return nil, nil
	}
	data, err := deviceMarshalOpts.Marshal(component)
	if err != nil {
		return nil, err
	}
	return cleanComponentParamJSON(data)
}

// MarshalAssetJSON marshals a protos.ExternalObjectPayload to JSON.
func MarshalAssetJSON(asset *protos.ExternalObjectPayload) ([]byte, error) {
	if asset == nil {
		return nil, nil
	}
	return protoMarshalOpts.Marshal(asset)
}

// MarshalDeviceComponentJSON marshals one streamed DeviceComponent chunk to
// JSON with the same SMPTE-compliant treatment as MarshalDeviceJSON: it uses
// EmitUnpopulated so meaningful proto3 defaults survive (e.g. a constraint's
// min_value:0, or a current value of 0/0.0/""), then strips the
// schema-forbidden extras from whichever oneof shape the chunk carries.
func MarshalDeviceComponentJSON(component *protos.DeviceComponent) ([]byte, error) {
	if component == nil {
		return nil, nil
	}
	data, err := deviceMarshalOpts.Marshal(component)
	if err != nil {
		return nil, err
	}
	return cleanDeviceComponentJSON(data)
}

// marshalDeviceComponentWire adapts MarshalDeviceComponentJSON to the
// restStream marshal signature, which hands over the chunk's wire proto.
func marshalDeviceComponentWire(msg proto.Message) ([]byte, error) {
	component, ok := msg.(*protos.DeviceComponent)
	if !ok {
		return nil, fmt.Errorf("expected *protos.DeviceComponent, got %T", msg)
	}
	return MarshalDeviceComponentJSON(component)
}

type jsonPrimitive interface {
	~int | ~int8 | ~int16 | ~int32 | ~int64 |
		~uint | ~uint8 | ~uint16 | ~uint32 | ~uint64 |
		~float32 | ~float64 | ~string | ~bool
}

// injectJSONField sets key:value in a JSON object using fastjson.
// The type constraint ensures only JSON-compatible primitives are accepted.
// Dispatch is based on the underlying reflect.Kind so derived types
// (e.g. `type MyStr string`) are handled correctly rather than falling
// through to a numeric-string default that would emit invalid JSON.
func injectJSONField[T jsonPrimitive](data []byte, key string, value T) []byte {
	var p fastjson.Parser
	v, err := p.ParseBytes(data)
	if err != nil {
		return data
	}
	var a fastjson.Arena
	rv := reflect.ValueOf(value)
	switch rv.Kind() {
	case reflect.String:
		v.Set(key, a.NewString(rv.String()))
	case reflect.Bool:
		if rv.Bool() {
			v.Set(key, a.NewTrue())
		} else {
			v.Set(key, a.NewFalse())
		}
	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		v.Set(key, a.NewNumberString(strconv.FormatInt(rv.Int(), 10)))
	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
		v.Set(key, a.NewNumberString(strconv.FormatUint(rv.Uint(), 10)))
	case reflect.Float32, reflect.Float64:
		bits := 64
		if rv.Kind() == reflect.Float32 {
			bits = 32
		}
		v.Set(key, a.NewNumberString(strconv.FormatFloat(rv.Float(), 'g', -1, bits)))
	default:
		v.Set(key, a.NewNumberString(fmt.Sprint(value)))
	}
	return v.MarshalTo(nil)
}

// WriteProtoJSON marshals any proto.Message to JSON and writes it to the HTTP
// response with the specified status code. If msg is nil (interface nil or nil
// concrete pointer), only the status code is written.
func WriteProtoJSON(w http.ResponseWriter, msg proto.Message, statusCode int) error {
	w.Header().Set("Content-Type", "application/json")

	if msg == nil || reflect.ValueOf(msg).IsNil() {
		w.WriteHeader(statusCode)
		return nil
	}

	b, err := MarshalProtoJSON(msg)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		_ = json.NewEncoder(w).Encode(map[string]string{
			"error": "failed to marshal response",
		})
		return fmt.Errorf("failed to marshal response: %w", err)
	}

	w.WriteHeader(statusCode)
	_, writeErr := w.Write(b)
	return writeErr
}

// ReadAssetRequestJSON reads and unmarshals a JSON request body into a
// protos.ExternalObjectPayload (the inverse of MarshalAssetJSON). It validates
// the Content-Type header and returns an error if it's not application/json.
func ReadAssetRequestJSON(r *http.Request) (*protos.ExternalObjectPayload, error) {
	defer r.Body.Close()

	contentType := r.Header.Get("Content-Type")
	if contentType == "" {
		return nil, fmt.Errorf("missing Content-Type header")
	}
	mediaType, _, err := mime.ParseMediaType(contentType)
	if err != nil {
		return nil, fmt.Errorf("invalid content type: %s", contentType)
	}
	if mediaType != "application/json" {
		return nil, fmt.Errorf("unsupported content type: %s, expected application/json", mediaType)
	}

	data, err := io.ReadAll(r.Body)
	if err != nil {
		return nil, fmt.Errorf("failed to read request body: %w", err)
	}

	payload := &protos.ExternalObjectPayload{}
	if err := (protojson.UnmarshalOptions{
		DiscardUnknown: true,
	}).Unmarshal(data, payload); err != nil {
		return nil, fmt.Errorf("failed to unmarshal request body: %w", err)
	}
	return payload, nil
}

// ReadRequestJSON reads and unmarshals a JSON request body into a protos.Value.
// It validates the Content-Type header and returns an error if it's not application/json.
func ReadRequestJSON(r *http.Request) (*protos.Value, catena.StatusResult) {
	defer r.Body.Close()

	data, err := io.ReadAll(r.Body)
	if err != nil {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("failed to read request body: %v", err)}
	}
	return parseValueJSON(r.Header.Get("Content-Type"), data)
}

// parseValueJSON validates a Content-Type and unmarshals already-read JSON bytes
// into a protos.Value. It is split out of ReadRequestJSON so callers that must
// treat an empty body specially (the command endpoint accepts an empty body as
// "no value" per ST 2138) can inspect the body before deciding to parse it.
func parseValueJSON(contentType string, data []byte) (*protos.Value, catena.StatusResult) {
	if contentType == "" {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: "missing Content-Type header"}
	}
	mediaType, _, err := mime.ParseMediaType(contentType)
	if err != nil {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("invalid content type: %s", contentType)}
	}
	if mediaType != "application/json" {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("unsupported content type: %s, expected application/json", mediaType)}
	}

	v := &protos.Value{}
	if err := (protojson.UnmarshalOptions{
		DiscardUnknown: true,
	}).Unmarshal(data, v); err != nil {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("failed to unmarshal request body: %v", err)}
	}
	return v, catena.StatusResult{Code: catena.StatusCodeOk}
}

// ReadMultiSetValuesRequestJSON reads and unmarshals a SetValues request body of
// the form {"values":[{"oid":"...","value":{<Value oneof>}}, ...]} into a slice
// of catena.SetValueEntry. The body is parsed directly into a
// protos.MultiSetValuePayload; the body's slot field (if present) is ignored
// since the slot is taken from the request path.
func ReadMultiSetValuesRequestJSON(r *http.Request) ([]catena.SetValueEntry, catena.StatusResult) {
	defer r.Body.Close()

	contentType := r.Header.Get("Content-Type")
	if contentType == "" {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: "missing Content-Type header"}
	}
	mediaType, _, err := mime.ParseMediaType(contentType)
	if err != nil {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("invalid content type: %s", contentType)}
	}
	if mediaType != "application/json" {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("unsupported content type: %s, expected application/json", mediaType)}
	}

	data, err := io.ReadAll(r.Body)
	if err != nil {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("failed to read request body: %v", err)}
	}

	payload := &protos.MultiSetValuePayload{}
	if err := (protojson.UnmarshalOptions{
		DiscardUnknown: true,
	}).Unmarshal(data, payload); err != nil {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("failed to unmarshal request body: %v", err)}
	}

	entries := make([]catena.SetValueEntry, 0, len(payload.GetValues()))
	for i, sv := range payload.GetValues() {
		nativeValue, convErr := st2138.FromProto(sv.GetValue())
		if convErr != nil {
			return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("invalid value at index %d: %v", i, convErr)}
		}

		entries = append(entries, catena.SetValueEntry{Fqoid: sv.GetOid(), Value: nativeValue})
	}

	return entries, catena.StatusResult{Code: catena.StatusCodeOk}
}

// --- Device JSON cleanup via fastjson AST ---

// zeroFields lists numeric fields that the SMPTE schema forbids when at their
// proto3 default of 0 (meaning "unset/unlimited").
var zeroFields = []string{"precision", "max_length", "total_length"}

// preserveEmptyStringFields lists the JSON keys whose empty-string ("") value
// is a legitimate, meaningful value and must NOT be stripped during cleanup.
// A param's "string_value" of "" is a real value; every other empty string in
// the device tree (widget, access_scope, template_oid, default_scope, and the
// values of metadata maps such as display_strings and client_hints) is an unset
// default that the SMPTE schema forbids and is removed by deleteEmptyFields.
// A key-based strip allow-list cannot cover map values, whose keys are arbitrary
// (language codes, hint names), so cleanup instead strips every empty string
// except the keys listed here.
var preserveEmptyStringFields = map[string]bool{"string_value": true}

// cleanDeviceJSON parses protojson output, strips unwanted fields, and
// re-serializes. Uses fastjson for efficient in-place manipulation.
func cleanDeviceJSON(data []byte) ([]byte, error) {
	var p fastjson.Parser
	v, err := p.ParseBytes(data)
	if err != nil {
		return nil, fmt.Errorf("cleanDeviceJSON parse: %w", err)
	}

	deleteDefaultFields(v)
	deleteResponseFromParams(v)
	deleteEmptyFields(v)

	return v.MarshalTo(nil), nil
}

// cleanDeviceComponentJSON strips the same schema-forbidden extras as
// cleanDeviceJSON but for one streamed DeviceComponent chunk, whose oneof
// determines where the device-shaped subtree lives: a device chunk nests the
// full device under "device", a param chunk carries {"oid","param"} under
// "param", and a command chunk keeps its "response" field since response is
// valid on commands.
func cleanDeviceComponentJSON(data []byte) ([]byte, error) {
	var p fastjson.Parser
	v, err := p.ParseBytes(data)
	if err != nil {
		return nil, fmt.Errorf("cleanDeviceComponentJSON parse: %w", err)
	}

	deleteDefaultFields(v)

	if deviceVal := v.Get("device"); deviceVal != nil {
		deleteResponseFromParams(deviceVal)
	}
	// "response" is only valid on commands; strip it from a param chunk's subtree.
	if paramVal := v.Get("param"); paramVal != nil {
		deleteResponseFalse(paramVal)
	}

	deleteEmptyFields(v)

	return v.MarshalTo(nil), nil
}

// cleanComponentParamJSON strips the same schema-forbidden extras as
// cleanDeviceJSON but for a component param's {"oid":...,"param":{...}} shape.
// deleteDefaultFields only removes named zero-valued numeric metadata and
// deleteEmptyFields preserves "string_value", so a current value of 0/0.0/""
// survives without special handling.
func cleanComponentParamJSON(data []byte) ([]byte, error) {
	var p fastjson.Parser
	v, err := p.ParseBytes(data)
	if err != nil {
		return nil, fmt.Errorf("cleanComponentParamJSON parse: %w", err)
	}

	deleteDefaultFields(v)

	// "response" is only valid on commands; strip it from the param subtree.
	if paramVal := v.Get("param"); paramVal != nil {
		deleteResponseFalse(paramVal)
	}

	deleteEmptyFields(v)

	return v.MarshalTo(nil), nil
}

// deleteDefaultFields recursively walks the JSON tree and removes numeric fields
// named in zeroFields whose value is 0, a schema-forbidden default. These are
// targeted by field name so that meaningful zero values elsewhere (e.g. a
// constraint's min_value:0) are preserved. Empty-string defaults are handled
// separately by deleteEmptyFields.
func deleteDefaultFields(v *fastjson.Value) {
	if v.Type() != fastjson.TypeObject {
		return
	}
	obj, err := v.Object()
	if err != nil {
		return
	}

	var toDelete []string
	obj.Visit(func(key []byte, val *fastjson.Value) {
		switch val.Type() {
		case fastjson.TypeNumber:
			k := string(key)
			for _, zf := range zeroFields {
				if k == zf && val.GetInt() == 0 {
					toDelete = append(toDelete, k)
					break
				}
			}
		case fastjson.TypeObject:
			deleteDefaultFields(val)
		case fastjson.TypeArray:
			for _, elem := range val.GetArray() {
				deleteDefaultFields(elem)
			}
		}
	})
	for _, k := range toDelete {
		v.Del(k)
	}
}

// deleteResponseFromParams removes "response" keys with value false inside
// the "params" subtree, but preserves them inside "commands".
// Proto field ordering guarantees params appears before commands.
func deleteResponseFromParams(v *fastjson.Value) {
	if v.Type() != fastjson.TypeObject {
		return
	}
	paramsVal := v.Get("params")
	if paramsVal == nil || paramsVal.Type() != fastjson.TypeObject {
		return
	}
	deleteResponseFalse(paramsVal)
}

// deleteResponseFalse recursively deletes "response":false from an object tree.
func deleteResponseFalse(v *fastjson.Value) {
	if v.Type() != fastjson.TypeObject {
		return
	}
	obj, err := v.Object()
	if err != nil {
		return
	}

	var needsDelete bool
	obj.Visit(func(key []byte, val *fastjson.Value) {
		k := string(key)
		if k == "response" && val.Type() == fastjson.TypeFalse {
			needsDelete = true
			return
		}
		if val.Type() == fastjson.TypeObject {
			deleteResponseFalse(val)
		}
	})
	if needsDelete {
		v.Del("response")
	}
}

// deleteEmptyFields recursively removes object fields whose values are null,
// {}, [], or an empty string "". ("Fields" refers to JSON key/value pairs.)
// Empty strings are stripped everywhere except for keys in
// preserveEmptyStringFields (e.g. a param's "string_value" of ""), so both
// scalar metadata strings and the values of metadata maps (display_strings,
// client_hints, ...) are cleaned. Returns true if the value itself is
// considered empty after deletions (object with no remaining keys, or empty
// array), enabling cascading removal by the caller. Object elements within
// arrays are cleaned in place; array elements are not removed even if they
// become empty, to avoid shifting indices (so a legitimate "" inside a string
// array value is preserved).
func deleteEmptyFields(v *fastjson.Value) bool {
	switch v.Type() {
	case fastjson.TypeObject:
		obj, err := v.Object()
		if err != nil {
			return false
		}

		var toDelete []string
		obj.Visit(func(key []byte, val *fastjson.Value) {
			if shouldDeleteField(string(key), val) {
				toDelete = append(toDelete, string(key))
			}
		})
		for _, k := range toDelete {
			v.Del(k)
		}

		empty := true
		obj2, _ := v.Object()
		obj2.Visit(func(_ []byte, _ *fastjson.Value) {
			empty = false
		})
		return empty

	case fastjson.TypeArray:
		for _, elem := range v.GetArray() {
			deleteEmptyFields(elem)
		}
		return len(v.GetArray()) == 0

	default:
		return false
	}
}

// shouldDeleteField reports whether the key/value pair should be removed: null
// values, empty objects/arrays (recursively cleaned first), and empty strings
// whose key is not in preserveEmptyStringFields.
func shouldDeleteField(key string, val *fastjson.Value) bool {
	switch val.Type() {
	case fastjson.TypeNull:
		return true
	case fastjson.TypeString:
		return len(val.GetStringBytes()) == 0 && !preserveEmptyStringFields[key]
	case fastjson.TypeObject, fastjson.TypeArray:
		return deleteEmptyFields(val)
	default:
		return false
	}
}
