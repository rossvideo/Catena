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
 * @brief Shared GetAsset example logic for REST and gRPC.
 * @file getasset.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 */

package getasset

import (
	"embed"
	"fmt"
	"os"
	"sync"

	"github.com/rossvideo/catena/sdks/go/examples/exampleutil"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

//go:embed static/*
var staticFS embed.FS

var SlotList = []uint16{0}

// RegisterHandlers loads the embedded asset files and registers the GetAsset
// handler for slot 0.
func RegisterHandlers(srv catena.CatenaServer) {
	assets := &sync.Map{}

	payloads, err := catena.LoadPayloadsFromEmbed(staticFS, "static")
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to load embedded assets: %v\n", err)
		os.Exit(1)
	}
	for id, payload := range payloads {
		assets.Store(id, payload)
	}

	srv.RegisterGetAssetHandler(0, func(slot uint16, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)

		val, ok := assets.Load(fqoid)
		if !ok {
			logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found: "+fqoid)
		}

		payload := val.(catena.DataPayload)

		catenaAsset, res := catena.ToCatenaAsset(payload, true)
		if res.Code != catena.OK {
			logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", res.Error)
			return catena.ReplyError[catena.CatenaAsset](catena.INTERNAL, "failed to convert asset: "+res.Error)
		}

		logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
		return catena.Reply(catenaAsset)
	})
}

// RunExample encapsulates the full example lifecycle:
// SDK init, signal handling, server creation, handler registration, and graceful shutdown.
func RunExample(appName string, makeServer func(slots []uint16, cfg catena.Config) catena.CatenaServer) {
	assets := &sync.Map{}
	payloads, _ := catena.LoadPayloadsFromEmbed(staticFS, "static")
	for id, payload := range payloads {
		assets.Store(id, payload)
	}

	var firstAssetName string
	assets.Range(func(key, _ any) bool {
		if firstAssetName == "" {
			firstAssetName = key.(string)
		}
		return firstAssetName == ""
	})

	exampleutil.RunExample(exampleutil.RunConfig{
		AppName:          appName,
		Slots:            SlotList,
		MakeServer:       makeServer,
		RegisterHandlers: RegisterHandlers,
		OnReady: func(port int) {
			logger.Info("=======================================================")
			logger.Info("GetAsset Example", "transport", appName)
			logger.Info("=======================================================")
			logger.Info("Listening", "port", port)
			logger.Info("")
			logger.Info("Available endpoint:")
			logger.Info("  GET  /st2138-api/v1/0/asset/{oid}")
			logger.Info("  GET  /st2138-api/v1/0/asset/{oid}?compression=GZIP")
			logger.Info("  GET  /st2138-api/v1/0/asset/{oid}?compression=DEFLATE")
			logger.Info("")
			logger.Info("Available assets:")
			assets.Range(func(key, _ any) bool {
				logger.Info(fmt.Sprintf("  %s", key.(string)))
				return true
			})
			logger.Info("")
			if firstAssetName != "" {
				logger.Info("Example curl:")
				logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/asset/%s", port, firstAssetName))
				logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/asset/%s?compression=GZIP", port, firstAssetName))
			}
			logger.Info("=======================================================")
		},
	})
}
