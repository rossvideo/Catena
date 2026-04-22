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
 * @file asset_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"bytes"
	"compress/gzip"
	"compress/zlib"
	"crypto/sha256"
	"encoding/json"
	"errors"
	"io/fs"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"testing/fstest"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestToCatenaAsset_WithPayload(t *testing.T) {
	dp := DataPayload{
		Metadata: map[string]string{
			"content-type": "image/png",
			"filename":     "test.png",
		},
		Digest:          []byte{0x01, 0x02, 0x03},
		PayloadEncoding: EncodingUncompressed,
		Payload:         []byte("fake image data"),
	}

	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	proto := asset.GetProtoAsset()
	if proto == nil {
		t.Fatal("expected non-nil proto asset")
	}
	if !proto.GetCachable() {
		t.Error("expected cachable to be true")
	}

	payload := proto.GetPayload()
	if payload == nil {
		t.Fatal("expected non-nil payload")
	}
	if payload.GetMetadata()["content-type"] != "image/png" {
		t.Errorf("expected content-type 'image/png', got %v", payload.GetMetadata()["content-type"])
	}
	if string(payload.GetPayload()) != "fake image data" {
		t.Errorf("expected payload 'fake image data', got %v", string(payload.GetPayload()))
	}
}

func TestToCatenaAsset_WithUrl(t *testing.T) {
	dp := DataPayload{
		Metadata: map[string]string{
			"content-type": "application/json",
		},
		Url: "https://example.com/resource.json",
	}

	asset, err := ToCatenaAsset(dp, false)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	proto := asset.GetProtoAsset()
	if proto == nil {
		t.Fatal("expected non-nil proto asset")
	}
	if proto.GetCachable() {
		t.Error("expected cachable to be false")
	}

	payload := proto.GetPayload()
	if payload == nil {
		t.Fatal("expected non-nil payload")
	}
	if payload.GetUrl() != "https://example.com/resource.json" {
		t.Errorf("expected URL 'https://example.com/resource.json', got %v", payload.GetUrl())
	}
}

func TestToCatenaAsset_BothPayloadAndUrl_Error(t *testing.T) {
	dp := DataPayload{
		Payload: []byte("some data"),
		Url:     "https://example.com/resource",
	}

	_, err := ToCatenaAsset(dp, true)
	if err.Code == OK {
		t.Error("expected error when both payload and url are provided")
	}
}

func TestToCatenaAsset_NeitherPayloadNorUrl_Error(t *testing.T) {
	dp := DataPayload{
		Metadata: map[string]string{
			"content-type": "text/plain",
		},
	}

	_, err := ToCatenaAsset(dp, true)
	if err.Code == OK {
		t.Error("expected error when neither payload nor url are provided")
	}
}

func TestToCatenaAsset_WithGzipEncoding(t *testing.T) {
	dp := DataPayload{
		Payload:         []byte("compressed data"),
		PayloadEncoding: EncodingGzip,
	}

	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	proto := asset.GetProtoAsset()
	if proto == nil {
		t.Fatal("expected non-nil proto asset")
	}

	payload := proto.GetPayload()
	if payload == nil {
		t.Fatal("expected non-nil payload")
	}
	if Encoding(payload.GetPayloadEncoding()) != EncodingGzip {
		t.Errorf("expected GZIP encoding, got %v", payload.GetPayloadEncoding())
	}
}

func TestToCatenaAsset_WithDeflateEncoding(t *testing.T) {
	dp := DataPayload{
		Payload:         []byte("compressed data"),
		PayloadEncoding: EncodingDeflate,
	}

	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	proto := asset.GetProtoAsset()
	payload := proto.GetPayload()
	if Encoding(payload.GetPayloadEncoding()) != EncodingDeflate {
		t.Errorf("expected DEFLATE encoding, got %v", payload.GetPayloadEncoding())
	}
}

func TestCatenaAsset_ToJSON(t *testing.T) {
	dp := DataPayload{
		Metadata: map[string]string{
			"content-type": "application/octet-stream",
		},
		Payload: []byte("binary data"),
	}

	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	jsonData, err := asset.ToJSON()
	if err.Code != OK {
		t.Fatalf("ToJSON error: %v", err.Error)
	}
	if jsonData == nil {
		t.Fatal("expected non-nil JSON data")
	}

	// Verify it's valid JSON
	var result map[string]any
	if err := json.Unmarshal(jsonData, &result); err != nil {
		t.Fatalf("invalid JSON output: %v", err)
	}

	// Check cachable field
	if result["cachable"] != true {
		t.Errorf("expected cachable true in JSON, got %v", result["cachable"])
	}
}

func TestCatenaAsset_ToJSON_Nil(t *testing.T) {
	asset := CatenaAsset{asset: nil}
	jsonData, err := asset.ToJSON()
	if err.Code != OK {
		t.Fatalf("ToJSON error: %v", err.Error)
	}
	if jsonData != nil {
		t.Error("expected nil JSON data for nil asset")
	}
}

func TestCatenaAsset_GetProtoAsset_Nil(t *testing.T) {
	asset := CatenaAsset{asset: nil}
	if asset.GetProtoAsset() != nil {
		t.Error("expected nil proto asset")
	}
}

func TestToCatenaAsset_WithDigest(t *testing.T) {
	digest := []byte{0xde, 0xad, 0xbe, 0xef, 0x00, 0x11, 0x22, 0x33}
	dp := DataPayload{
		Digest:  digest,
		Payload: []byte("data with digest"),
	}

	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	proto := asset.GetProtoAsset()
	payload := proto.GetPayload()

	if payload.GetDigest() == nil {
		t.Fatal("expected digest to be set")
	}
	if len(payload.GetDigest()) != len(digest) {
		t.Errorf("expected digest length %d, got %d", len(digest), len(payload.GetDigest()))
	}
	for i, b := range digest {
		if payload.GetDigest()[i] != b {
			t.Errorf("digest byte %d: expected %x, got %x", i, b, payload.GetDigest()[i])
		}
	}
}

func TestToCatenaAsset_EmptyPayload_WithUrl(t *testing.T) {
	// Test that empty payload slice with valid URL works
	dp := DataPayload{
		Payload: []byte{}, // explicitly empty
		Url:     "https://example.com/data",
	}

	asset, err := ToCatenaAsset(dp, false)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset error: %v", err.Error)
	}

	proto := asset.GetProtoAsset()
	payload := proto.GetPayload()
	if payload.GetUrl() != "https://example.com/data" {
		t.Errorf("expected URL, got %v", payload.GetUrl())
	}
}

func TestDataPayload_Fields(t *testing.T) {
	dp := DataPayload{
		Metadata: map[string]string{
			"key1": "value1",
			"key2": "value2",
		},
		Digest:          []byte{1, 2, 3},
		PayloadEncoding: EncodingGzip,
		Payload:         []byte("test"),
		Url:             "",
	}

	// Verify all fields are accessible
	if len(dp.Metadata) != 2 {
		t.Errorf("expected 2 metadata entries, got %d", len(dp.Metadata))
	}
	if len(dp.Digest) != 3 {
		t.Errorf("expected digest length 3, got %d", len(dp.Digest))
	}
	if dp.PayloadEncoding != EncodingGzip {
		t.Errorf("expected PayloadEncoding GZIP, got %v", dp.PayloadEncoding)
	}
	if string(dp.Payload) != "test" {
		t.Errorf("expected Payload 'test', got %s", dp.Payload)
	}
}

func TestToPayload(t *testing.T) {
	data := []byte("hello world")
	contentType := "text/plain"
	filename := "hello.txt"

	dp := ToPayload(data, contentType, filename)

	if dp.PayloadEncoding != EncodingUncompressed {
		t.Errorf("expected PayloadEncoding UNCOMPRESSED, got %v", dp.PayloadEncoding)
	}
	if string(dp.Payload) != string(data) {
		t.Errorf("expected Payload %q, got %q", data, dp.Payload)
	}
	if dp.Metadata["content-type"] != contentType {
		t.Errorf("expected content-type %q, got %q", contentType, dp.Metadata["content-type"])
	}
	if dp.Metadata["file-name"] != filename {
		t.Errorf("expected file-name %q, got %q", filename, dp.Metadata["file-name"])
	}
	if dp.Metadata["size"] != "11" {
		t.Errorf("expected size \"11\", got %q", dp.Metadata["size"])
	}
	expectedHash := sha256.Sum256(data)
	if len(dp.Digest) != len(expectedHash) {
		t.Fatalf("expected digest length %d, got %d", len(expectedHash), len(dp.Digest))
	}
	for i := range expectedHash {
		if dp.Digest[i] != expectedHash[i] {
			t.Errorf("digest byte %d: expected %x, got %x", i, expectedHash[i], dp.Digest[i])
		}
	}
}

func TestToPayloadFromPath(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "asset.bin")
	data := []byte("file contents")
	if err := os.WriteFile(path, data, 0644); err != nil {
		t.Fatalf("write temp file: %v", err)
	}

	dp, err := ToPayloadFromPath(path)
	if err != nil {
		t.Fatalf("ToPayloadFromPath: %v", err)
	}
	if string(dp.Payload) != string(data) {
		t.Errorf("expected Payload %q, got %q", data, dp.Payload)
	}
	if dp.Metadata["file-name"] != "asset.bin" {
		t.Errorf("expected file-name asset.bin, got %q", dp.Metadata["file-name"])
	}
}

func TestToPayloadFromPath_NotFound(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "nonexistent.txt")

	_, err := ToPayloadFromPath(path)
	if err == nil {
		t.Error("expected error for missing file")
	}
}

func TestToPayloadFromFS_Success(t *testing.T) {
	fsys := fstest.MapFS{
		"dir/file.txt": {Data: []byte("hello")},
	}
	dp, err := ToPayloadFromFS(fsys, "dir/file.txt")
	if err != nil {
		t.Fatalf("ToPayloadFromFS: %v", err)
	}
	if string(dp.Payload) != "hello" {
		t.Errorf("expected Payload \"hello\", got %q", dp.Payload)
	}
	if dp.Metadata["content-type"] != "text/plain; charset=utf-8" {
		t.Errorf("expected content-type for .txt, got %q", dp.Metadata["content-type"])
	}
	if dp.Metadata["file-name"] != "file.txt" {
		t.Errorf("expected file-name file.txt, got %q", dp.Metadata["file-name"])
	}
}

func TestToPayloadFromFS_UnknownExtension(t *testing.T) {
	fsys := fstest.MapFS{
		"data.unknown": {Data: []byte("x")},
	}
	dp, err := ToPayloadFromFS(fsys, "data.unknown")
	if err != nil {
		t.Fatalf("ToPayloadFromFS: %v", err)
	}
	if dp.Metadata["content-type"] != "application/octet-stream" {
		t.Errorf("expected application/octet-stream for unknown ext, got %q", dp.Metadata["content-type"])
	}
}

func TestToPayloadFromFS_ReadError(t *testing.T) {
	fsys := fstest.MapFS{}
	_, err := ToPayloadFromFS(fsys, "missing.txt")
	if err == nil {
		t.Error("expected error when file does not exist")
	}
}

func TestLoadPayloadsFromEmbed(t *testing.T) {
	fsys := fstest.MapFS{
		"root/a.txt":   {Data: []byte("a")},
		"root/b/c.txt": {Data: []byte("c")},
	}
	out, err := LoadPayloadsFromEmbed(fsys, "root")
	if err != nil {
		t.Fatalf("LoadPayloadsFromEmbed: %v", err)
	}
	if len(out) != 2 {
		t.Fatalf("expected 2 entries, got %d", len(out))
	}
	if p, ok := out["a.txt"]; !ok || string(p.Payload) != "a" {
		t.Errorf("expected out[\"a.txt\"] with payload \"a\", got ok=%v payload=%q", ok, out["a.txt"].Payload)
	}
	if p, ok := out["b/c.txt"]; !ok || string(p.Payload) != "c" {
		t.Errorf("expected out[\"b/c.txt\"] with payload \"c\", got ok=%v payload=%q", ok, out["b/c.txt"].Payload)
	}
}

func TestLoadPayloadsFromEmbed_EmptyRoot(t *testing.T) {
	fsys := fstest.MapFS{
		"f1.txt": {Data: []byte("1")},
	}
	out, err := LoadPayloadsFromEmbed(fsys, ".")
	if err != nil {
		t.Fatalf("LoadPayloadsFromEmbed: %v", err)
	}
	if len(out) != 1 {
		t.Fatalf("expected 1 entry, got %d", len(out))
	}
}

// openFailFS wraps an fs.FS and returns an error from Open for a specific path.
type openFailFS struct {
	fs.FS
	failPath string
}

func (f openFailFS) Open(name string) (fs.File, error) {
	if name == f.failPath {
		return nil, errors.New("read forbidden")
	}
	return f.FS.Open(name)
}

func TestLoadPayloadsFromEmbed_WalkDirError(t *testing.T) {
	// Non-existent root: WalkDir calls the callback with err != nil (covers lines 120 and 138).
	fsys := fstest.MapFS{"root/a.txt": {Data: []byte("a")}}
	_, err := LoadPayloadsFromEmbed(fsys, "nonexistent")
	if err == nil {
		t.Fatal("expected error when root does not exist")
	}
}

func TestLoadPayloadsFromEmbed_ToPayloadFromFSError(t *testing.T) {
	// FS that lists a file but Open fails for it, so ToPayloadFromFS fails (covers line 127).
	fsys := openFailFS{
		FS: fstest.MapFS{
			"root/ok.txt":  {Data: []byte("ok")},
			"root/bad.txt": {Data: []byte("bad")},
		},
		failPath: "root/bad.txt",
	}
	_, err := LoadPayloadsFromEmbed(fsys, "root")
	if err == nil {
		t.Fatal("expected error when ToPayloadFromFS fails for a file")
	}
	if err.Error() == "" || !strings.Contains(err.Error(), "root/bad.txt") {
		t.Errorf("expected error to mention path root/bad.txt: %v", err)
	}
}

func TestToPayloadFromURL(t *testing.T) {
	url := "https://example.com/asset.json"
	dp := ToPayloadFromURL(url)
	if dp.Url != url {
		t.Errorf("expected Url %q, got %q", url, dp.Url)
	}
	if len(dp.Payload) != 0 {
		t.Errorf("expected empty Payload, got %d bytes", len(dp.Payload))
	}
}

func TestCompressDecompressGzip_Roundtrip(t *testing.T) {
	original := []byte("hello world, this is test data for gzip compression")
	compressed, err := CompressGzip(original)
	if err != nil {
		t.Fatalf("CompressGzip: %v", err)
	}
	if len(compressed) == 0 {
		t.Fatal("expected non-empty compressed data")
	}

	decompressed, err := DecompressGzip(compressed)
	if err != nil {
		t.Fatalf("DecompressGzip: %v", err)
	}
	if string(decompressed) != string(original) {
		t.Errorf("roundtrip mismatch: got %q, want %q", decompressed, original)
	}
}

func TestCompressDecompressDeflate_Roundtrip(t *testing.T) {
	original := []byte("hello world, this is test data for deflate compression")
	compressed, err := CompressDeflate(original)
	if err != nil {
		t.Fatalf("CompressDeflate: %v", err)
	}
	if len(compressed) == 0 {
		t.Fatal("expected non-empty compressed data")
	}

	decompressed, err := DecompressDeflate(compressed)
	if err != nil {
		t.Fatalf("DecompressDeflate: %v", err)
	}
	if string(decompressed) != string(original) {
		t.Errorf("roundtrip mismatch: got %q, want %q", decompressed, original)
	}
}

func TestDecompressGzip_InvalidData(t *testing.T) {
	_, err := DecompressGzip([]byte("not gzip data"))
	if err == nil {
		t.Error("expected error for invalid gzip data")
	}
}

func TestDecompressDeflate_InvalidData(t *testing.T) {
	_, err := DecompressDeflate([]byte("not deflate data"))
	if err == nil {
		t.Error("expected error for invalid deflate data")
	}
}

func TestDecodePayload(t *testing.T) {
	original := []byte("test payload data")

	gzData, _ := CompressGzip(original)
	deflateData, _ := CompressDeflate(original)

	tests := []struct {
		name     string
		data     []byte
		encoding Encoding
		want     string
		wantErr  bool
	}{
		{"uncompressed", original, EncodingUncompressed, "test payload data", false},
		{"gzip", gzData, EncodingGzip, "test payload data", false},
		{"deflate", deflateData, EncodingDeflate, "test payload data", false},
		{"unsupported encoding", original, Encoding(99), "", true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := DecodePayload(tt.data, tt.encoding)
			if (err != nil) != tt.wantErr {
				t.Fatalf("DecodePayload error = %v, wantErr %v", err, tt.wantErr)
			}
			if !tt.wantErr && string(got) != tt.want {
				t.Errorf("DecodePayload = %q, want %q", got, tt.want)
			}
		})
	}
}

func TestEncodePayload(t *testing.T) {
	original := []byte("test payload data")

	tests := []struct {
		name     string
		encoding Encoding
		wantErr  bool
	}{
		{"uncompressed", EncodingUncompressed, false},
		{"gzip", EncodingGzip, false},
		{"deflate", EncodingDeflate, false},
		{"unsupported", Encoding(99), true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			encoded, err := EncodePayload(original, tt.encoding)
			if (err != nil) != tt.wantErr {
				t.Fatalf("EncodePayload error = %v, wantErr %v", err, tt.wantErr)
			}
			if tt.wantErr {
				return
			}

			decoded, err := DecodePayload(encoded, tt.encoding)
			if err != nil {
				t.Fatalf("DecodePayload after encode: %v", err)
			}
			if string(decoded) != string(original) {
				t.Errorf("encode→decode roundtrip: got %q, want %q", decoded, original)
			}
		})
	}
}

func TestParsePayloadEncoding(t *testing.T) {
	tests := []struct {
		input   string
		want    Encoding
		wantErr bool
	}{
		{"UNCOMPRESSED", EncodingUncompressed, false},
		{"uncompressed", EncodingUncompressed, false},
		{"GZIP", EncodingGzip, false},
		{"gzip", EncodingGzip, false},
		{"DEFLATE", EncodingDeflate, false},
		{"deflate", EncodingDeflate, false},
		{"", EncodingUncompressed, false},
		{"  GZIP  ", EncodingGzip, false},
		{"INVALID", EncodingUncompressed, true},
	}

	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			got, err := ParsePayloadEncoding(tt.input)
			if (err.Code != OK) != tt.wantErr {
				t.Fatalf("ParsePayloadEncoding(%q) error = %v, wantErr %v", tt.input, err.Error, tt.wantErr)
			}
			if got != tt.want {
				t.Errorf("ParsePayloadEncoding(%q) = %d, want %d", tt.input, got, tt.want)
			}
		})
	}
}

func TestPayloadEncodingFromExt(t *testing.T) {
	tests := []struct {
		filename string
		want     Encoding
	}{
		{"file.gz", EncodingGzip},
		{"file.zz", EncodingDeflate},
		{"file.txt", EncodingUncompressed},
		{"file.png", EncodingUncompressed},
		{"file", EncodingUncompressed},
	}

	for _, tt := range tests {
		t.Run(tt.filename, func(t *testing.T) {
			got := PayloadEncodingFromExt(tt.filename)
			if got != tt.want {
				t.Errorf("PayloadEncodingFromExt(%q) = %d, want %d", tt.filename, got, tt.want)
			}
		})
	}
}

func TestTranscodeAssetPayload(t *testing.T) {
	original := []byte("transcode test data")

	// Create an uncompressed asset
	dp := DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  original,
	}
	asset, res := ToCatenaAsset(dp, true)
	if res.Code != OK {
		t.Fatalf("ToCatenaAsset: %v", res.Error)
	}

	// Transcode to GZIP
	if res := TranscodeAssetPayload(&asset, EncodingGzip); res.Code != OK {
		t.Fatalf("TranscodeAssetPayload to GZIP: %v", res.Error)
	}
	proto := asset.GetProtoAsset().GetPayload()
	if Encoding(proto.GetPayloadEncoding()) != EncodingGzip {
		t.Errorf("expected GZIP encoding, got %v", proto.GetPayloadEncoding())
	}

	// Verify the payload is actually gzip compressed (can decompress)
	decompressed, err := DecompressGzip(proto.GetPayload())
	if err != nil {
		t.Fatalf("DecompressGzip: %v", err)
	}
	if string(decompressed) != string(original) {
		t.Errorf("decompressed = %q, want %q", decompressed, original)
	}

	// Transcode from GZIP to DEFLATE
	if res := TranscodeAssetPayload(&asset, EncodingDeflate); res.Code != OK {
		t.Fatalf("TranscodeAssetPayload GZIP→DEFLATE: %v", res.Error)
	}
	proto = asset.GetProtoAsset().GetPayload()
	if Encoding(proto.GetPayloadEncoding()) != EncodingDeflate {
		t.Errorf("expected DEFLATE encoding, got %v", proto.GetPayloadEncoding())
	}
	decompressed, err = DecompressDeflate(proto.GetPayload())
	if err != nil {
		t.Fatalf("DecompressDeflate: %v", err)
	}
	if string(decompressed) != string(original) {
		t.Errorf("decompressed = %q, want %q", decompressed, original)
	}

	// Transcode from DEFLATE to UNCOMPRESSED
	if res := TranscodeAssetPayload(&asset, EncodingUncompressed); res.Code != OK {
		t.Fatalf("TranscodeAssetPayload DEFLATE→UNCOMPRESSED: %v", res.Error)
	}
	proto = asset.GetProtoAsset().GetPayload()
	if Encoding(proto.GetPayloadEncoding()) != EncodingUncompressed {
		t.Errorf("expected UNCOMPRESSED encoding, got %v", proto.GetPayloadEncoding())
	}
	if string(proto.GetPayload()) != string(original) {
		t.Errorf("payload = %q, want %q", proto.GetPayload(), original)
	}
}

func TestTranscodeAssetPayload_RecomputesDigest(t *testing.T) {
	original := []byte("digest recompute test data")
	dp := ToPayload(original, "text/plain", "test.txt")

	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset: %v", err.Error)
	}

	originalDigest := make([]byte, len(asset.GetProtoAsset().GetPayload().GetDigest()))
	copy(originalDigest, asset.GetProtoAsset().GetPayload().GetDigest())
	expectedOriginal := sha256.Sum256(original)
	if len(originalDigest) != len(expectedOriginal) {
		t.Fatalf("original digest length %d, want %d", len(originalDigest), len(expectedOriginal[:]))
	}
	for i := range expectedOriginal {
		if originalDigest[i] != expectedOriginal[i] {
			t.Fatalf("original digest mismatch at byte %d", i)
		}
	}

	if res := TranscodeAssetPayload(&asset, EncodingGzip); res.Code != OK {
		t.Fatalf("TranscodeAssetPayload to GZIP: %v", res.Error)
	}

	newPayloadBytes := asset.GetProtoAsset().GetPayload().GetPayload()
	expectedDigest := sha256.Sum256(newPayloadBytes)
	actualDigest := asset.GetProtoAsset().GetPayload().GetDigest()

	if len(actualDigest) != sha256.Size {
		t.Fatalf("digest length after transcode = %d, want %d", len(actualDigest), sha256.Size)
	}
	for i := range expectedDigest {
		if actualDigest[i] != expectedDigest[i] {
			t.Fatalf("digest mismatch after transcode at byte %d: got %x, want %x", i, actualDigest[i], expectedDigest[i])
		}
	}

	for i := range originalDigest {
		if originalDigest[i] != actualDigest[i] {
			return
		}
	}
	t.Error("digest should differ after transcoding to a different encoding")
}

func TestTranscodeAssetPayload_SameEncoding_Noop(t *testing.T) {
	dp := DataPayload{
		Payload: []byte("data"),
	}
	asset, _ := ToCatenaAsset(dp, true)
	originalBytes := asset.GetProtoAsset().GetPayload().GetPayload()

	if res := TranscodeAssetPayload(&asset, EncodingUncompressed); res.Code != OK {
		t.Fatalf("TranscodeAssetPayload same encoding: %v", res.Error)
	}
	if string(asset.GetProtoAsset().GetPayload().GetPayload()) != string(originalBytes) {
		t.Error("expected no-op when transcoding to same encoding")
	}
}

func TestTranscodeAssetPayload_DoesNotMutateOriginalProto(t *testing.T) {
	original := []byte("shared payload data")
	dp := DataPayload{
		Metadata: map[string]string{"content-type": "text/plain"},
		Payload:  original,
	}
	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset: %v", err.Error)
	}

	originalProto := asset.GetProtoAsset()

	copy := asset // shallow copy — shares the pointer
	if res := TranscodeAssetPayload(&copy, EncodingGzip); res.Code != OK {
		t.Fatalf("TranscodeAssetPayload: %v", res.Error)
	}

	if Encoding(originalProto.GetPayload().GetPayloadEncoding()) != EncodingUncompressed {
		t.Errorf("original proto encoding was mutated: got %v, want %v",
			originalProto.GetPayload().GetPayloadEncoding(), EncodingUncompressed)
	}
	if string(originalProto.GetPayload().GetPayload()) != string(original) {
		t.Errorf("original proto payload was mutated: got %q, want %q",
			originalProto.GetPayload().GetPayload(), original)
	}

	if copy.GetProtoAsset() == originalProto {
		t.Error("transcoded asset should point to a different proto, not the original")
	}
}

func TestTranscodeAssetPayload_NilAsset(t *testing.T) {
	asset := CatenaAsset{}
	err := TranscodeAssetPayload(&asset, EncodingGzip)
	if err.Code == OK {
		t.Error("expected error for nil asset")
	}
}

func TestTranscodeAssetPayload_NilPayload(t *testing.T) {
	asset := CatenaAsset{asset: &protos.ExternalObjectPayload{Payload: nil}}
	err := TranscodeAssetPayload(&asset, EncodingGzip)
	if err.Code == OK {
		t.Error("expected error for nil payload on non-nil asset")
	}
}

func TestTranscodeAssetPayload_DecodeError(t *testing.T) {
	dp := DataPayload{
		Payload:         []byte("not valid gzip"),
		PayloadEncoding: EncodingGzip,
	}
	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset: %v", err.Error)
	}

	err = TranscodeAssetPayload(&asset, EncodingUncompressed)
	if err.Code == OK {
		t.Error("expected decode error for corrupt gzip payload")
	}
}

func TestTranscodeAssetPayload_EncodeError(t *testing.T) {
	dp := DataPayload{
		Payload: []byte("valid data"),
	}
	asset, err := ToCatenaAsset(dp, true)
	if err.Code != OK {
		t.Fatalf("ToCatenaAsset: %v", err.Error)
	}

	err = TranscodeAssetPayload(&asset, Encoding(99))
	if err.Code == OK {
		t.Error("expected encode error for unsupported target encoding")
	}
}

func TestDecodePayload_CorruptGzip(t *testing.T) {
	_, err := DecodePayload([]byte("not gzip"), EncodingGzip)
	if err == nil {
		t.Error("expected error decoding corrupt gzip via DecodePayload")
	}
}

func TestDecodePayload_CorruptDeflate(t *testing.T) {
	_, err := DecodePayload([]byte("not deflate"), EncodingDeflate)
	if err == nil {
		t.Error("expected error decoding corrupt deflate via DecodePayload")
	}
}

func TestToPayloadFromFS_GzipExtension(t *testing.T) {
	original := []byte("compressed file data")
	gzData, _ := CompressGzip(original)

	fsys := fstest.MapFS{
		"assets/test.gz": {Data: gzData},
	}
	dp, err := ToPayloadFromFS(fsys, "assets/test.gz")
	if err != nil {
		t.Fatalf("ToPayloadFromFS: %v", err)
	}
	if dp.PayloadEncoding != EncodingGzip {
		t.Errorf("expected GZIP encoding, got %d", dp.PayloadEncoding)
	}
}

func TestToPayloadFromFS_DeflateExtension(t *testing.T) {
	original := []byte("compressed file data")
	deflateData, _ := CompressDeflate(original)

	fsys := fstest.MapFS{
		"assets/test.zz": {Data: deflateData},
	}
	dp, err := ToPayloadFromFS(fsys, "assets/test.zz")
	if err != nil {
		t.Fatalf("ToPayloadFromFS: %v", err)
	}
	if dp.PayloadEncoding != EncodingDeflate {
		t.Errorf("expected DEFLATE encoding, got %d", dp.PayloadEncoding)
	}
}

func TestToPayloadFromFS_PlainExtension(t *testing.T) {
	fsys := fstest.MapFS{
		"assets/file.txt": {Data: []byte("plain text")},
	}
	dp, err := ToPayloadFromFS(fsys, "assets/file.txt")
	if err != nil {
		t.Fatalf("ToPayloadFromFS: %v", err)
	}
	if dp.PayloadEncoding != EncodingUncompressed {
		t.Errorf("expected UNCOMPRESSED encoding, got %d", dp.PayloadEncoding)
	}
}

type errWriter struct{}

func (errWriter) Write([]byte) (int, error) {
	return 0, errors.New("intentional write failure")
}

type failAfterBytes struct {
	limit   int
	written int
}

func (w *failAfterBytes) Write(p []byte) (int, error) {
	if w.written+len(p) > w.limit {
		return 0, errors.New("write limit exceeded")
	}
	w.written += len(p)
	return len(p), nil
}

func TestCompressGzipTo_WriteError(t *testing.T) {
	err := compressGzipTo(errWriter{}, []byte("data"))
	if err == nil {
		t.Fatal("expected gzip write error")
	}
	if !strings.Contains(err.Error(), "gzip write") {
		t.Errorf("expected 'gzip write' in error, got: %v", err)
	}
}

func TestCompressGzipTo_CloseError(t *testing.T) {
	data := []byte("hello")
	var measure bytes.Buffer
	gw := gzip.NewWriter(&measure)
	if _, err := gw.Write(data); err != nil {
		t.Fatalf("measure Write: %v", err)
	}
	limit := measure.Len()

	err := compressGzipTo(&failAfterBytes{limit: limit}, data)
	if err == nil {
		t.Fatal("expected gzip close error")
	}
	if !strings.Contains(err.Error(), "gzip close") {
		t.Errorf("expected 'gzip close' in error, got: %v", err)
	}
}

func TestCompressDeflateTo_WriteError(t *testing.T) {
	err := compressDeflateTo(errWriter{}, []byte("data"))
	if err == nil {
		t.Fatal("expected zlib write error")
	}
	if !strings.Contains(err.Error(), "zlib write") {
		t.Errorf("expected 'zlib write' in error, got: %v", err)
	}
}

func TestCompressDeflateTo_CloseError(t *testing.T) {
	data := []byte("hello")
	var measure bytes.Buffer
	zw := zlib.NewWriter(&measure)
	if _, err := zw.Write(data); err != nil {
		t.Fatalf("measure Write: %v", err)
	}
	limit := measure.Len()

	err := compressDeflateTo(&failAfterBytes{limit: limit}, data)
	if err == nil {
		t.Fatal("expected zlib close error")
	}
	if !strings.Contains(err.Error(), "zlib close") {
		t.Errorf("expected 'zlib close' in error, got: %v", err)
	}
}

func TestEncoding_String(t *testing.T) {
	tests := []struct {
		enc  Encoding
		want string
	}{
		{EncodingUncompressed, "UNCOMPRESSED"},
		{EncodingGzip, "GZIP"},
		{EncodingDeflate, "DEFLATE"},
		{Encoding(42), "Encoding(42)"},
		{Encoding(-1), "Encoding(-1)"},
	}

	for _, tt := range tests {
		t.Run(tt.want, func(t *testing.T) {
			if got := tt.enc.String(); got != tt.want {
				t.Errorf("Encoding(%d).String() = %q, want %q", int32(tt.enc), got, tt.want)
			}
		})
	}
}
