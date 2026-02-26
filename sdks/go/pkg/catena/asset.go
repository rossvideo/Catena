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
	"bytes"
	"compress/gzip"
	"compress/zlib"
	"crypto/sha256"
	"fmt"
	"io"
	"io/fs"
	"mime"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/build/go/protos"
)

const (
	EncodingUncompressed int32 = 0
	EncodingGzip         int32 = 1
	EncodingDeflate      int32 = 2
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
// Encoding is auto-detected from the file extension: .gz → GZIP, .zz → DEFLATE.
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

	dp := ToPayload(data, contentType, name)
	dp.PayloadEncoding = PayloadEncodingFromExt(name)
	return dp, nil
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

// CompressGzip compresses data using gzip format.
func CompressGzip(data []byte) ([]byte, error) {
	var buf bytes.Buffer
	w := gzip.NewWriter(&buf)
	if _, err := w.Write(data); err != nil {
		return nil, fmt.Errorf("gzip write: %w", err)
	}
	if err := w.Close(); err != nil {
		return nil, fmt.Errorf("gzip close: %w", err)
	}
	return buf.Bytes(), nil
}

// CompressDeflate compresses data using zlib (deflate) format.
func CompressDeflate(data []byte) ([]byte, error) {
	var buf bytes.Buffer
	w := zlib.NewWriter(&buf)
	if _, err := w.Write(data); err != nil {
		return nil, fmt.Errorf("zlib write: %w", err)
	}
	if err := w.Close(); err != nil {
		return nil, fmt.Errorf("zlib close: %w", err)
	}
	return buf.Bytes(), nil
}

// DecompressGzip decompresses gzip-encoded data.
func DecompressGzip(data []byte) ([]byte, error) {
	r, err := gzip.NewReader(bytes.NewReader(data))
	if err != nil {
		return nil, fmt.Errorf("gzip reader: %w", err)
	}
	defer r.Close()
	return io.ReadAll(r)
}

// DecompressDeflate decompresses zlib (deflate)-encoded data.
func DecompressDeflate(data []byte) ([]byte, error) {
	r, err := zlib.NewReader(bytes.NewReader(data))
	if err != nil {
		return nil, fmt.Errorf("zlib reader: %w", err)
	}
	defer r.Close()
	return io.ReadAll(r)
}

// DecodePayload decompresses payload bytes according to the given encoding.
func DecodePayload(data []byte, encoding int32) ([]byte, error) {
	switch encoding {
	case EncodingGzip:
		return DecompressGzip(data)
	case EncodingDeflate:
		return DecompressDeflate(data)
	case EncodingUncompressed:
		return data, nil
	default:
		return nil, fmt.Errorf("unsupported encoding: %d", encoding)
	}
}

// EncodePayload compresses raw data according to the given encoding.
func EncodePayload(data []byte, encoding int32) ([]byte, error) {
	switch encoding {
	case EncodingGzip:
		return CompressGzip(data)
	case EncodingDeflate:
		return CompressDeflate(data)
	case EncodingUncompressed:
		return data, nil
	default:
		return nil, fmt.Errorf("unsupported encoding: %d", encoding)
	}
}

// ParsePayloadEncoding converts a string (e.g. "GZIP", "DEFLATE", "UNCOMPRESSED")
// to the corresponding encoding constant.
func ParsePayloadEncoding(s string) (int32, error) {
	switch strings.ToUpper(strings.TrimSpace(s)) {
	case "UNCOMPRESSED", "":
		return EncodingUncompressed, nil
	case "GZIP":
		return EncodingGzip, nil
	case "DEFLATE":
		return EncodingDeflate, nil
	default:
		return 0, fmt.Errorf("invalid payload encoding: %s", s)
	}
}

// PayloadEncodingFromExt returns the payload encoding implied by a file extension.
// ".gz" → GZIP, ".zz" → DEFLATE, anything else → UNCOMPRESSED.
func PayloadEncodingFromExt(filename string) int32 {
	switch filepath.Ext(filename) {
	case ".gz":
		return EncodingGzip
	case ".zz":
		return EncodingDeflate
	default:
		return EncodingUncompressed
	}
}

// TranscodeAssetPayload decodes the asset's current payload encoding and
// re-encodes it to targetEncoding, modifying the asset in place.
// If the asset is already in the target encoding, this is a no-op.
func TranscodeAssetPayload(asset *CatenaAsset, targetEncoding int32) error {
	original := asset.GetProtoAsset()
	if original == nil || original.GetPayload() == nil {
		return fmt.Errorf("asset has no payload")
	}

	dp := original.GetPayload()
	currentEncoding := int32(dp.GetPayloadEncoding())

	if currentEncoding == targetEncoding {
		return nil
	}

	rawData, err := DecodePayload(dp.GetPayload(), currentEncoding)
	if err != nil {
		return fmt.Errorf("decode: %w", err)
	}

	encodedData, err := EncodePayload(rawData, targetEncoding)
	if err != nil {
		return fmt.Errorf("encode: %w", err)
	}

	cloned := proto.Clone(original).(*protos.ExternalObjectPayload)
	cloned.Payload.Kind = &protos.DataPayload_Payload{Payload: encodedData}
	cloned.Payload.PayloadEncoding = protos.DataPayload_PayloadEncoding(targetEncoding)
	newDigest := sha256.Sum256(encodedData)
	cloned.Payload.Digest = newDigest[:]
	asset.asset = cloned

	return nil
}
