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
 * @date 2026-02-04
 */

package rest

import (
	"encoding/json"
	"fmt"
	"io"
	"mime"
	"net/http"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
)

var protoMarshalOpts = protojson.MarshalOptions{
	UseProtoNames:   true,
	EmitUnpopulated: false,
}

// MarshalProtoJSON marshals a proto message to JSON using proto field names
// and without emitting unpopulated fields.
func MarshalProtoJSON(msg proto.Message) ([]byte, error) {
	return protoMarshalOpts.Marshal(msg)
}

// injectJSONField parses a JSON object, sets key to value, and re-serializes.
// Used to ensure fields like "slot":0 appear even though proto3 JSON omits
// default-valued scalars.
func injectJSONField(data []byte, key string, value any) ([]byte, error) {
	var m map[string]any
	if err := json.Unmarshal(data, &m); err != nil {
		return nil, err
	}
	m[key] = value
	return json.Marshal(m)
}

// PostProcessDeviceJSON ensures all SMPTE-schema-required fields are present
// in the Device JSON even when proto3 omits them at their default values.
func PostProcessDeviceJSON(data []byte, slot uint32) ([]byte, error) {
	var m map[string]any
	if err := json.Unmarshal(data, &m); err != nil {
		return nil, err
	}

	m["slot"] = slot

	injectCommandDefaults(m)
	injectMenuGroupDefaults(m)
	injectConstraintDefaults(m)

	return json.Marshal(m)
}

// injectCommandDefaults ensures "response" is present on every command.
// The schema requires it, but proto3 omits bool fields that are false.
func injectCommandDefaults(device map[string]any) {
	commands, ok := device["commands"].(map[string]any)
	if !ok {
		return
	}
	for _, cmd := range commands {
		cmdMap, ok := cmd.(map[string]any)
		if !ok {
			continue
		}
		setDefault(cmdMap, "response", false)
	}
}

// injectMenuGroupDefaults ensures "order" is present on menu groups and menus.
// The schema requires "order" on menu_group.
func injectMenuGroupDefaults(device map[string]any) {
	menuGroups, ok := device["menu_groups"].(map[string]any)
	if !ok {
		return
	}
	for _, mg := range menuGroups {
		mgMap, ok := mg.(map[string]any)
		if !ok {
			continue
		}
		setDefault(mgMap, "order", 0)
	}
}

// injectConstraintDefaults ensures required range fields are present.
// The schema requires min_value/max_value on int32_range and float32_range.
func injectConstraintDefaults(device map[string]any) {
	constraints, ok := device["constraints"].(map[string]any)
	if !ok {
		return
	}
	for _, c := range constraints {
		cMap, ok := c.(map[string]any)
		if !ok {
			continue
		}
		if r, ok := cMap["int32_range"].(map[string]any); ok {
			setDefault(r, "min_value", 0)
			setDefault(r, "max_value", 0)
		}
		if r, ok := cMap["float32_range"].(map[string]any); ok {
			setDefault(r, "min_value", 0)
			setDefault(r, "max_value", 0)
		}
	}
}

func setDefault(m map[string]any, key string, value any) {
	if _, exists := m[key]; !exists {
		m[key] = value
	}
}

// WriteCommandResponseJSON marshals a protos.CommandResponse to JSON and writes it
// to the HTTP response with the specified status code.
func WriteCommandResponseJSON(w http.ResponseWriter, cmdResp *protos.CommandResponse, statusCode int) error {
	w.Header().Set("Content-Type", "application/json")

	if cmdResp == nil {
		w.WriteHeader(statusCode)
		return nil
	}

	b, err := (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(cmdResp)
	if err != nil {
		w.WriteHeader(http.StatusInternalServerError)
		_ = json.NewEncoder(w).Encode(map[string]string{
			"error": "failed to marshal command response",
		})
		return fmt.Errorf("failed to marshal command response: %w", err)
	}

	w.WriteHeader(statusCode)
	_, writeErr := w.Write(b)
	return writeErr
}

// ReadRequestJSON reads and unmarshals a JSON request body into a protos.Value.
// It validates the Content-Type header and returns an error if it's not application/json.
func ReadRequestJSON(r *http.Request) (*protos.Value, catena.StatusResult) {
	defer r.Body.Close()

	// Check Content-Type header
	contentType := r.Header.Get("Content-Type")
	if contentType == "" {
		return nil, catena.StatusResult{Code: catena.BAD_REQUEST, Error: "missing Content-Type header"}
	} else {
		// Parse media type to handle parameters like charset
		mediaType, _, err := mime.ParseMediaType(contentType)
		if err != nil {
			return nil, catena.StatusResult{Code: catena.BAD_REQUEST, Error: fmt.Sprintf("invalid content type: %s", contentType)}
		}
		if mediaType != "application/json" {
			return nil, catena.StatusResult{Code: catena.INVALID_ARGUMENT, Error: fmt.Sprintf("unsupported content type: %s, expected application/json", mediaType)}
		}
	}

	data, err := io.ReadAll(r.Body)
	if err != nil || err == io.EOF {
		return nil, catena.StatusResult{Code: catena.BAD_REQUEST, Error: fmt.Sprintf("failed to read request body: %v", err)}
	}

	v := &protos.Value{}
	if err := (protojson.UnmarshalOptions{
		DiscardUnknown: true,
	}).Unmarshal(data, v); err != nil {
		return nil, catena.StatusResult{Code: catena.BAD_REQUEST, Error: fmt.Sprintf("failed to unmarshal request body: %v", err)}
	}
	return v, catena.StatusResult{Code: catena.OK}
}

// WriteResponseJSON marshals a protos.Value to JSON and writes it to the HTTP response
// with the specified status code. If the value is nil, only the status code is written.
func WriteResponseJSON(w http.ResponseWriter, value *protos.Value, statusCode int) error {
	w.Header().Set("Content-Type", "application/json")

	if value == nil {
		w.WriteHeader(statusCode)
		return nil
	}

	b, err := (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(value)
	if err != nil {
		// Marshaling failed (e.g., invalid UTF-8). Don't return a 2xx with an empty body.
		// Prefer a JSON error body so clients that always parse JSON don't explode.
		w.WriteHeader(http.StatusInternalServerError)
		_ = json.NewEncoder(w).Encode(map[string]string{
			"error": "failed to marshal response payload",
		})
		return fmt.Errorf("failed to marshal response: %w", err)
	}

	w.WriteHeader(statusCode)
	_, writeErr := w.Write(b)
	return writeErr
}
