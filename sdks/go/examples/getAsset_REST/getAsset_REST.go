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
	"encoding/base64"
	"fmt"
	"io"
	"net/http"
	"os"
	"strconv"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

// Asset represents a binary asset with metadata
type Asset struct {
	ContentType string
	Data        []byte
	Filename    string
}

func main() {
	cfg := logger.ParseConfigWithVerbosity("CATENA")
	cfg.AppName = "asset_example"

	if err := logger.Init(cfg); err != nil {
		fmt.Fprintf(os.Stderr, "failed to initialize logger: %v\n", err)
		os.Exit(1)
	}
	defer logger.Close()

	portStr := envOr("CATENA_PORT", "6254")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		logger.Error("invalid CATENA_PORT", "error", err)
		os.Exit(1)
	}

	// Create asset stores for each slot
	// In a real implementation, these might read from disk or a database

	// ==========================================================================
	// Slot 0: Image Assets
	// ==========================================================================
	imageAssets := &sync.Map{}
	// Simple 1x1 red PNG (base64 encoded for portability)
	redPixelPNG, _ := base64.StdEncoding.DecodeString(
		"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg==")
	// Simple 1x1 blue PNG
	bluePixelPNG, _ := base64.StdEncoding.DecodeString(
		"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPj/HwADBwIAMCbHYQAAAABJRU5ErkJggg==")
	imageAssets.Store("thumbnail", Asset{ContentType: "image/png", Data: redPixelPNG, Filename: "thumbnail.png"})
	imageAssets.Store("icon", Asset{ContentType: "image/png", Data: bluePixelPNG, Filename: "icon.png"})

	// ==========================================================================
	// Slot 1: Configuration/Text Assets
	// ==========================================================================
	configAssets := &sync.Map{}
	configAssets.Store("device_config", Asset{
		ContentType: "application/json",
		Data:        []byte(`{"device":"example","version":"1.0","settings":{"mode":"auto"}}`),
		Filename:    "device_config.json",
	})
	configAssets.Store("preset_1", Asset{
		ContentType: "application/json",
		Data:        []byte(`{"name":"Preset 1","video":{"resolution":"1080p60","colorspace":"BT.709"}}`),
		Filename:    "preset_1.json",
	})
	configAssets.Store("readme", Asset{
		ContentType: "text/plain",
		Data:        []byte("This is a sample README file.\nIt contains device documentation."),
		Filename:    "README.txt",
	})

	// ==========================================================================
	// Slot 2: Binary/Firmware Assets
	// ==========================================================================
	binaryAssets := &sync.Map{}
	binaryAssets.Store("firmware_v1", Asset{
		ContentType: "application/octet-stream",
		Data:        []byte{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}, // Fake firmware header
		Filename:    "firmware_v1.0.0.bin",
	})
	binaryAssets.Store("calibration_data", Asset{
		ContentType: "application/octet-stream",
		Data:        []byte{0xCA, 0x11, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x01},
		Filename:    "calibration.dat",
	})

	slotList := []int{0, 1, 2}
	srv := rest.NewServer(slotList)

	assetStores := map[int]*sync.Map{
		0: imageAssets,
		1: configAssets,
		2: binaryAssets,
	}

	slotDescriptions := map[int]string{
		0: "Image Assets (PNG thumbnails, icons)",
		1: "Configuration Assets (JSON configs, text)",
		2: "Binary Assets (firmware, calibration data)",
	}

	for slot := range assetStores {
		slot := slot
		store := assetStores[slot]
		desc := slotDescriptions[slot]

		srv.RegisterGetAssetHandler(slot, func(w http.ResponseWriter, r *http.Request, slotNum int, fqoid string) catena.StatusResult {
			logger.Info("GetAsset", "slot", slotNum, "fqoid", fqoid, "type", desc)

			val, ok := store.Load(fqoid)
			if !ok {
				logger.Warning("GetAsset not found", "slot", slotNum, "fqoid", fqoid)
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
				logger.Error("GetAsset streaming error", "slot", slotNum, "fqoid", fqoid, "error", err)
				return catena.InternalServerError("failed to stream asset")
			}

			// Return OK with nil payload since we've already written to the response
			return catena.StatusResult{Status: http.StatusOK}
		})
	}

	// Not found handler
	srv.RegisterNotFoundHandler(func(w http.ResponseWriter, r *http.Request) catena.StatusResult {
		return catena.NotFound("endpoint not found: " + r.URL.Path)
	})

	logger.Info("Asset Example listening", "port", port)
	logger.Info("Available slots and assets:")
	for slot, desc := range slotDescriptions {
		logger.Info("  ", "slot", slot, "description", desc)
	}
	logger.Info("Example URLs:")
	logger.Info("  GET /st2138-api/v1/0/asset/thumbnail")
	logger.Info("  GET /st2138-api/v1/1/asset/device_config")
	logger.Info("  GET /st2138-api/v1/2/asset/firmware_v1")

	srv.StartHTTPServer(port)
	select {}
}

func envOr(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}

