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

package transports

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

// MarshalAssetJSON marshals a protos.ExternalObjectPayload to JSON.
func MarshalAssetJSON(asset *protos.ExternalObjectPayload) ([]byte, error) {
	if asset == nil {
		return nil, nil
	}
	return protoMarshalOpts.Marshal(asset)
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

// ReadRequestJSON reads and unmarshals a JSON request body into a protos.Value.
// It validates the Content-Type header and returns an error if it's not application/json.
func ReadRequestJSON(r *http.Request) (*protos.Value, catena.StatusResult) {
	defer r.Body.Close()

	contentType := r.Header.Get("Content-Type")
	if contentType == "" {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: "missing Content-Type header"}
	} else {
		mediaType, _, err := mime.ParseMediaType(contentType)
		if err != nil {
			return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("invalid content type: %s", contentType)}
		}
		if mediaType != "application/json" {
			return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("unsupported content type: %s, expected application/json", mediaType)}
		}
	}

	data, err := io.ReadAll(r.Body)
	if err != nil || err == io.EOF {
		return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("failed to read request body: %v", err)}
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
		nativeValue, convErr := catena.FromProto(sv.GetValue())
		if convErr.Code != catena.StatusCodeOk {
			return nil, catena.StatusResult{Code: catena.StatusCodeInvalidArgument, Error: fmt.Sprintf("invalid value at index %d: %v", i, convErr.Error)}
		}

		entries = append(entries, catena.SetValueEntry{Fqoid: sv.GetOid(), Value: nativeValue})
	}

	return entries, catena.StatusResult{Code: catena.StatusCodeOk}
}

// --- Device JSON cleanup via fastjson AST ---

// zeroFields lists device fields that the SMPTE schema forbids when at their
// proto3 default of 0 (meaning "unset/unlimited").
var zeroFields = []string{"precision", "max_length", "total_length"}

// cleanDeviceJSON parses protojson output, strips unwanted fields, and
// re-serializes. Uses fastjson for efficient in-place manipulation.
func cleanDeviceJSON(data []byte) ([]byte, error) {
	var p fastjson.Parser
	v, err := p.ParseBytes(data)
	if err != nil {
		return nil, fmt.Errorf("cleanDeviceJSON parse: %w", err)
	}

	deleteZeroFields(v)
	deleteResponseFromParams(v)
	deleteEmptyValues(v)

	return v.MarshalTo(nil), nil
}

// deleteZeroFields recursively walks the JSON tree and removes fields whose
// names are in zeroFields and whose values are the number 0.
func deleteZeroFields(v *fastjson.Value) {
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
			deleteZeroFields(val)
		case fastjson.TypeArray:
			for _, elem := range val.GetArray() {
				deleteZeroFields(elem)
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

// deleteEmptyValues recursively removes keys whose values are null, {}, [],
// or "". Returns true if the value itself is considered empty after
// deletions (object with no remaining keys, or empty array), enabling
// cascading removal by the caller. Object elements within arrays are
// cleaned in place; array elements are not removed even if they become
// empty, to avoid shifting indices.
func deleteEmptyValues(v *fastjson.Value) bool {
	switch v.Type() {
	case fastjson.TypeObject:
		obj, err := v.Object()
		if err != nil {
			return false
		}

		var toDelete []string
		obj.Visit(func(key []byte, val *fastjson.Value) {
			if shouldDeleteValue(val) {
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
			deleteEmptyValues(elem)
		}
		return len(v.GetArray()) == 0

	default:
		return false
	}
}

func shouldDeleteValue(val *fastjson.Value) bool {
	switch val.Type() {
	case fastjson.TypeNull:
		return true
	case fastjson.TypeString:
		return len(val.GetStringBytes()) == 0
	case fastjson.TypeObject, fastjson.TypeArray:
		return deleteEmptyValues(val)
	default:
		return false
	}
}
