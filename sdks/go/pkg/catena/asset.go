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
 * @brief Asset handling for the Catena SDK.
 * @file asset.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"crypto/sha256"
	"fmt"
	"io/fs"
	"mime"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/build/go/protos"
)

// CatenaAsset wraps protos.ExternalObjectPayload for asset handling
type CatenaAsset struct {
	asset *protos.ExternalObjectPayload
}

// DataPayload is a helper type for business logic to create assets
// It mirrors protos.DataPayload structure for convenient construction
type DataPayload struct {
	Metadata        map[string]string // Information about the payload (e.g. content-type, filename)
	Digest          []byte            // SHA-256 digest of the payload
	PayloadEncoding int32             // 0=UNCOMPRESSED, 1=GZIP, 2=DEFLATE
	Payload         []byte            // Embedded binary data (use this OR Url, not both)
	Url             string            // URL to external resource (use this OR Payload, not both)
}

// ToPayload creates a DataPayload from raw bytes with metadata and auto-computed SHA-256 digest.
// This is the primary helper for creating asset payloads from in-memory data.
func ToPayload(data []byte, contentType string, filename string) DataPayload {
	hash := sha256.Sum256(data)
	return DataPayload{
		Payload:         data,
		PayloadEncoding: 0, // UNCOMPRESSED
		Metadata: map[string]string{
			"content-type": contentType,
			"file-name":    filename,
			"size":         strconv.Itoa(len(data)),
		},
		Digest: hash[:],
	}
}

// ToPayloadFromPath creates a DataPayload by reading a file from disk.
// Implemented by treating the file's directory as an fs.FS and delegating to ToPayloadFromFS.
func ToPayloadFromPath(path string) (DataPayload, error) {
	dir := filepath.Dir(path)
	name := filepath.Base(path)
	return ToPayloadFromFS(os.DirFS(dir), name)
}

// ToPayloadFromFS creates a DataPayload by reading a file from an fs.FS (e.g. embed.FS).
func ToPayloadFromFS(fsys fs.FS, path string) (DataPayload, error) {
	data, err := fs.ReadFile(fsys, path)
	if err != nil {
		return DataPayload{}, fmt.Errorf("failed to read file from FS: %w", err)
	}

	// Use forward slashes for base name in case path is from embed (always /)
	name := path
	if i := strings.LastIndex(name, "/"); i >= 0 {
		name = name[i+1:]
	}
	contentType := mime.TypeByExtension(filepath.Ext(name))
	if contentType == "" {
		contentType = "application/octet-stream"
	}

	return ToPayload(data, contentType, name), nil
}

// LoadPayloadsFromEmbed walks an embedded filesystem from root and returns a map of
func LoadPayloadsFromEmbed(embedFS fs.FS, root string) (map[string]DataPayload, error) {
	out := make(map[string]DataPayload)
	err := fs.WalkDir(embedFS, root, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() {
			return nil
		}
		payload, err := ToPayloadFromFS(embedFS, path)
		if err != nil {
			return fmt.Errorf("%s: %w", path, err)
		}
		relPath, err := filepath.Rel(root, path)
		if err != nil {
			relPath = filepath.Base(path)
		}
		assetID := strings.ReplaceAll(relPath, string(filepath.Separator), "/")
		out[assetID] = payload
		return nil
	})
	if err != nil {
		return nil, err
	}
	return out, nil
}

// ToPayloadFromURL creates a DataPayload with a URL reference (no embedded data).
// Use this when the asset is hosted externally and shouldn't be embedded.
func ToPayloadFromURL(url string) DataPayload {
	return DataPayload{
		Url: url,
	}
}

// ToCatenaAsset converts DataPayload to CatenaAsset by building the proto directly
func ToCatenaAsset(dp DataPayload, cachable bool) (CatenaAsset, error) {
	// Build the proto DataPayload directly
	protoPayload := &protos.DataPayload{
		Metadata:        dp.Metadata,
		Digest:          dp.Digest,
		PayloadEncoding: protos.DataPayload_PayloadEncoding(dp.PayloadEncoding),
	}

	// Handle oneof: either url or payload (not both)
	if dp.Url != "" && len(dp.Payload) == 0 {
		protoPayload.Kind = &protos.DataPayload_Url{Url: dp.Url}
	} else if len(dp.Payload) > 0 && dp.Url == "" {
		protoPayload.Kind = &protos.DataPayload_Payload{Payload: dp.Payload}
	} else {
		return CatenaAsset{asset: nil}, fmt.Errorf("either payload or url must be provided in DataPayload, but not both")
	}

	// Build the ExternalObjectPayload
	asset := &protos.ExternalObjectPayload{
		Cachable: cachable,
		Payload:  protoPayload,
	}

	return CatenaAsset{asset: asset}, nil
}

// GetProtoAsset returns the underlying protos.ExternalObjectPayload
func (ca CatenaAsset) GetProtoAsset() *protos.ExternalObjectPayload {
	return ca.asset
}

// ToJSON converts CatenaAsset to JSON bytes using protojson
func (ca CatenaAsset) ToJSON() ([]byte, error) {
	if ca.asset == nil {
		return nil, nil
	}

	return (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(ca.asset)
}
