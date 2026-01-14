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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
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
	"bytes"
	"embed"
	"fmt"
	"io"
	"io/fs"
	"mime"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"syscall"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

//go:embed static/*
var staticFS embed.FS

// Asset represents a binary asset with metadata
type Asset struct {
	ContentType string
	Data        []byte
	Filename    string
}

// Global state for graceful shutdown
var (
	shutdownChan = make(chan struct{})
	srv          *rest.Server
)

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "asset_request_REST"

	if err := logger.Init(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
	defer logger.Close()

	// Handle signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	portStr := envOr("CATENA_PORT", "6254")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		logger.Error("invalid CATENA_PORT", "error", err)
		os.Exit(1)
	}

	// ==========================================================================
	// Asset Storage
	// ==========================================================================
	assets := &sync.Map{}
	assetsList := &sync.Map{}

	// Load assets from embedded static directory
	loadAssetsFromEmbedded(staticFS, "static", assets, assetsList)

	// Build assets list for logging
	var assetNames []string
	assetsList.Range(func(key, value any) bool {
		assetNames = append(assetNames, key.(string))
		return true
	})

	// ==========================================================================
	// Server Setup
	// ==========================================================================
	slotList := []int{0}
	srv = rest.NewServer(slotList)

	// Register GetAsset handler
	srv.RegisterGetAssetHandler(0, func(w http.ResponseWriter, r *http.Request, slot int, fqoid string) catena.StatusResult {
		logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)

		val, ok := assets.Load(fqoid)
		if !ok {
			logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
			return catena.NotFound("asset not found: " + fqoid)
		}

		asset := val.(Asset)

		// Set appropriate headers for binary content
		w.Header().Set("Content-Type", asset.ContentType)
		w.Header().Set("Content-Length", strconv.Itoa(len(asset.Data)))
		if asset.Filename != "" {
			w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=%q", asset.Filename))
		}

		// Stream the content
		reader := bytes.NewReader(asset.Data)
		if _, err := io.Copy(w, reader); err != nil {
			logger.Error("Asset streaming error", "slot", slot, "fqoid", fqoid, "error", err)
			return catena.InternalServerError("failed to stream asset")
		}

		logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid)
		return catena.StatusResult{Status: http.StatusOK}
	})

	// Not found handler
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		return catena.NotFound("endpoint not found: " + r.URL.Path)
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
	logger.Info("Loaded assets:")
	assetsList.Range(func(key, value any) bool {
		logger.Info("  ", "asset", key)
		return true
	})
	logger.Info("")
	logger.Info("Example curl:")
	if len(assetNames) > 0 {
		logger.Info(fmt.Sprintf("  curl http://localhost:%d/st2138-api/v1/0/asset/%s", port, assetNames[0]))
	}
	logger.Info("=======================================================")

	srv.StartHTTPServer(port)

	// Wait for shutdown signal
	<-shutdownChan
	logger.Info("Server shutdown complete")
}

// loadAssetsFromEmbedded loads all files from the embedded filesystem into the asset store
func loadAssetsFromEmbedded(embedFS embed.FS, root string, assets *sync.Map, assetsList *sync.Map) {
	err := fs.WalkDir(embedFS, root, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			logger.Warning("Error accessing path", "path", path, "error", err)
			return nil // Continue walking
		}

		if d.IsDir() {
			return nil // Skip directories
		}

		// Read file contents
		data, err := embedFS.ReadFile(path)
		if err != nil {
			logger.Warning("Failed to read file", "path", path, "error", err)
			return nil
		}

		// Determine content type from extension
		ext := filepath.Ext(path)
		contentType := mime.TypeByExtension(ext)
		if contentType == "" {
			contentType = "application/octet-stream"
		}

		// Use relative path from root as the asset ID
		relPath, err := filepath.Rel(root, path)
		if err != nil {
			relPath = filepath.Base(path)
		}
		// Normalize path separators for URL use
		assetID := strings.ReplaceAll(relPath, string(filepath.Separator), "/")

		asset := Asset{
			ContentType: contentType,
			Data:        data,
			Filename:    filepath.Base(path),
		}

		assets.Store(assetID, asset)
		assetsList.Store(assetID, true)
		logger.Info("Loaded asset", "id", assetID, "size", len(data), "type", contentType)

		return nil
	})

	if err != nil {
		logger.Error("Error walking embedded directory", "root", root, "error", err)
	}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
