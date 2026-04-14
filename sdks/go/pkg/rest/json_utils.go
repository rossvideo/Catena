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

// injectJSONField sets key:value in a JSON object, preserving original key order.
func injectJSONField(data []byte, key string, value any) ([]byte, error) {
	obj, err := parseOrderedJSON(data)
	if err != nil {
		return nil, err
	}
	obj.set(key, value)
	return obj.MarshalJSON()
}

// PostProcessDeviceJSON ensures all SMPTE-schema-required fields are present
// in the Device JSON even when proto3 omits them at their default values.
// Preserves the original field ordering from protojson.
func PostProcessDeviceJSON(data []byte, slot uint32) ([]byte, error) {
	obj, err := parseOrderedJSON(data)
	if err != nil {
		return nil, err
	}

	obj.setFirst("slot", slot)

	if v, ok := obj.get("commands"); ok {
		if cmds, ok := v.(*orderedObj); ok {
			for _, p := range cmds.pairs {
				if cmd, ok := p.val.(*orderedObj); ok {
					cmd.setDefault("response", false)
				}
			}
		}
	}

	if v, ok := obj.get("menu_groups"); ok {
		if mgs, ok := v.(*orderedObj); ok {
			for _, p := range mgs.pairs {
				if mg, ok := p.val.(*orderedObj); ok {
					mg.setDefault("order", int64(0))
				}
			}
		}
	}

	if v, ok := obj.get("constraints"); ok {
		if cs, ok := v.(*orderedObj); ok {
			for _, p := range cs.pairs {
				if c, ok := p.val.(*orderedObj); ok {
					if r, ok := c.get("int32_range"); ok {
						if rObj, ok := r.(*orderedObj); ok {
							rObj.setDefault("min_value", int64(0))
							rObj.setDefault("max_value", int64(0))
						}
					}
					if r, ok := c.get("float32_range"); ok {
						if rObj, ok := r.(*orderedObj); ok {
							rObj.setDefault("min_value", 0.0)
							rObj.setDefault("max_value", 0.0)
						}
					}
				}
			}
		}
	}

	return obj.MarshalJSON()
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
