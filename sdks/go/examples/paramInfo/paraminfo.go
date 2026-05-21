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
 * @brief Shared ParamInfo example logic for REST and gRPC.
 * @file paraminfo.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package paraminfo

import (
	"fmt"
	"strings"

	"github.com/rossvideo/catena/sdks/go/examples/exampleutil"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

var SlotList = []uint16{0}

// paramEntry holds the metadata for a single parameter in our toy device model.
type paramEntry struct {
	OID         string
	Name        catena.PolyglotText
	Type        catena.ParamType
	TemplateOID string
	ArrayLen    uint32
	Children    []paramEntry
}

// sampleParams defines a small device model used by this example.
// It contains a mix of scalars, structs, and arrays so the different
// modes of the param-info endpoint (unary, stream, recursive) can all
// be demonstrated.
var sampleParams = []paramEntry{
	{
		OID:  "brightness",
		Name: catena.NewPolyglotText("en", "Brightness"),
		Type: catena.ParamTypeInt32,
	},
	{
		OID:  "label",
		Name: catena.NewPolyglotText("en", "Label"),
		Type: catena.ParamTypeString,
	},
	{
		OID:  "video",
		Name: catena.NewPolyglotText("en", "Video Settings"),
		Type: catena.ParamTypeStruct,
		Children: []paramEntry{
			{
				OID:  "video/resolution",
				Name: catena.NewPolyglotText("en", "Resolution"),
				Type: catena.ParamTypeString,
			},
			{
				OID:  "video/frame_rate",
				Name: catena.NewPolyglotText("en", "Frame Rate"),
				Type: catena.ParamTypeFloat32,
			},
		},
	},
	{
		OID:      "presets",
		Name:     catena.NewPolyglotText("en", "Presets"),
		Type:     catena.ParamTypeStringArray,
		ArrayLen: 5,
	},
}

// flattenParams converts the tree of paramEntries into a flat list of
// CatenaParamInfo. When recursive is true children are included; when
// false only the entries at the matched level are returned.
func flattenParams(entries []paramEntry, recursive bool) []catena.ParamInfo {
	var out []catena.ParamInfo
	for _, e := range entries {
		out = append(out, catena.NewParamInfo(e.OID, e.Name, e.Type, e.TemplateOID, e.ArrayLen))
		if recursive && len(e.Children) > 0 {
			out = append(out, flattenParams(e.Children, true)...)
		}
	}
	return out
}

// findByOID walks the tree and returns the entry whose OID matches, or nil.
func findByOID(entries []paramEntry, oid string) *paramEntry {
	for i := range entries {
		if entries[i].OID == oid {
			return &entries[i]
		}
		if child := findByOID(entries[i].Children, oid); child != nil {
			return child
		}
	}
	return nil
}

// RegisterHandlers registers the GetParamInfo handler for slot 0 with
// the static sample device model.
func RegisterHandlers(srv catena.Server) {
	srv.RegisterParamInfoHandler(0, func(slot uint16, oidPrefix string, recursive bool) ([]catena.ParamInfo, catena.StatusResult) {
		logger.Info("GetParamInfo", "slot", slot, "oid_prefix", oidPrefix, "recursive", recursive)

		// Normalise the oid: the REST layer sends a leading "/", the gRPC
		// layer sends the raw oid_prefix from the proto. Strip the leading
		// slash so lookups against our model work uniformly.
		oid := strings.TrimPrefix(oidPrefix, "/")

		// Mode 1 & 2: empty oid → return top-level params.
		if oid == "" {
			infos := flattenParams(sampleParams, recursive)
			return infos, catena.StatusWithCode(catena.OK, "")
		}

		// Mode 3: specific param by OID.
		entry := findByOID(sampleParams, oid)
		if entry == nil {
			return nil, catena.StatusWithCode(catena.NOT_FOUND, "parameter not found: "+oid)
		}

		result := []catena.ParamInfo{
			catena.NewParamInfo(entry.OID, entry.Name, entry.Type, entry.TemplateOID, entry.ArrayLen),
		}
		if recursive && len(entry.Children) > 0 {
			result = append(result, flattenParams(entry.Children, true)...)
		}
		return result, catena.StatusWithCode(catena.OK, "")
	})
}

// RunExample encapsulates the full example lifecycle.
func RunExample(appName string) {
	exampleutil.RunExample(exampleutil.RunConfig{
		AppName:          appName,
		Slots:            SlotList,
		RegisterHandlers: RegisterHandlers,
		OnReady: func(cfg catena.Config) {
			port := cfg.Port
			logger.Info("=======================================================")
			logger.Info("ParamInfo Example")
			logger.Info("=======================================================")
			logger.Info("Listening", "port", port)
			logger.Info("")
			logger.Info("Available parameters:")
			for _, p := range flattenParams(sampleParams, true) {
				logger.Info(fmt.Sprintf("  %-25s  type=%s  array_len=%d",
					p.GetOid(), p.GetParamType(), p.GetArrayLength()))
			}
			logger.Info("")

			if cfg.UseGrpc {
				logger.Info("Test with grpcurl:")
				logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid_prefix\": \"\", \"recursive\": false}' localhost:%d st2138.CatenaService/ParamInfoRequest", port))
				logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid_prefix\": \"video\", \"recursive\": true}' localhost:%d st2138.CatenaService/ParamInfoRequest", port))
				logger.Info(fmt.Sprintf("  grpcurl -plaintext -d '{\"slot\": 0, \"oid_prefix\": \"brightness\"}' localhost:%d st2138.CatenaService/ParamInfoRequest", port))
			}
			if cfg.UseRest {
				logger.Info("REST endpoints:")
				logger.Info(fmt.Sprintf("  Unary:          GET http://localhost:%d/st2138-api/v1/0/param-info/brightness", port))
				logger.Info(fmt.Sprintf("  Stream all:     GET http://localhost:%d/st2138-api/v1/0/param-info/stream", port))
				logger.Info(fmt.Sprintf("  Stream subtree: GET http://localhost:%d/st2138-api/v1/0/param-info/video/stream?recursive", port))
				logger.Info("")
				logger.Info("Example curl commands:")
				logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/param-info/brightness", port))
				logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/param-info/stream", port))
				logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/param-info/video/stream?recursive", port))
			}
			logger.Info("=======================================================")
		},
	})
}
