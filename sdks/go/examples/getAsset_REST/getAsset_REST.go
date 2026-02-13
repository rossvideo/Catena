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
 * @brief GetAsset REST example.
 * @file getAsset_REST.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-12
 */

package main

import (
	"embed"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

//go:embed static/*
var staticFS embed.FS

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	srv          *rest.Server
)

func main() {
	// Initialize SDK with prefix and app name
	cfg, err := catena.InitOptions(catena.Options{AppName: "asset_request_REST"})
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize SDK: %v\n", err)
		os.Exit(1)
	}
	defer catena.Close()

	// Handle signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	// Port comes from the unified config (parsed from CATENA_PORT)
	port := cfg.Port

	// ==========================================================================
	// Asset Storage
	// ==========================================================================
	assets := &sync.Map{}

	// Load assets from embedded static directory using catena helper
	payloads, err := catena.LoadPayloadsFromEmbed(staticFS, "static")
	if err != nil {
		fmt.Fprintf(os.Stderr, "failed to load embedded assets: %v\n", err)
		os.Exit(1)
	}
	for id, payload := range payloads {
		assets.Store(id, payload)
	}

	// Collect asset names for example curl command
	var firstAssetName string
	assets.Range(func(key, _ any) bool {
		if firstAssetName == "" {
			firstAssetName = key.(string)
		}
		return firstAssetName == "" // Stop after finding first asset
	})

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []int{0}
	srv = rest.NewServer(slotList)

	// Register GetAsset handler
	srv.RegisterGetAssetHandler(0, func(slot int, fqoid string) (catena.CatenaAsset, catena.StatusResult) {
		logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)

		val, ok := assets.Load(fqoid)
		if !ok {
			logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
			return catena.ReplyError[catena.CatenaAsset](catena.NOT_FOUND, "asset not found: "+fqoid)
		}

		payload := val.(catena.DataPayload)

		// Convert DataPayload to CatenaAsset when returning
		catenaAsset, err := catena.ToCatenaAsset(payload, true)
		if err != nil {
			logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", err)
			return catena.ReplyError[catena.CatenaAsset](catena.INTERNAL, "failed to convert asset: "+err.Error())
		}

		logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
		return catena.Reply(catenaAsset)
	})

	// Not found handler
	srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
		return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found")
	})

	// ==========================================================================
	// Start Server
	// ==========================================================================
	logger.Info("=======================================================")
	logger.Info("Asset Request REST Example")
	logger.Info("=======================================================")
	logger.Info("REST server starting", "port", port)
	logger.Info("")
	logger.Info("Available endpoint:")
	logger.Info("  GET  /st2138-api/v1/0/asset/{oid}  - GetAsset")
	logger.Info("")
	if firstAssetName != "" {
		logger.Info("Example curl:")
		logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/asset/%s", port, firstAssetName))
	}
	logger.Info("=======================================================")

	// Start server in a goroutine so shutdown handling works
	go func() {
		if err := srv.Start(port); err != nil {
			logger.Error("server failed", "error", err)
			os.Exit(1)
		}
	}()

	// Wait for shutdown signal
	<-shutdownChan
	logger.Info("Server shutdown complete")
}
