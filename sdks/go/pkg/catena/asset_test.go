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
 * @date 2026-02-04
 */

package catena

import (
	"crypto/sha256"
	"encoding/json"
	"os"
	"path/filepath"
	"testing"
	"testing/fstest"
)

func TestToCatenaAsset_WithPayload(t *testing.T) {
	dp := DataPayload{
		Metadata: map[string]string{
			"content-type": "image/png",
			"filename":     "test.png",
		},
		Digest:          []byte{0x01, 0x02, 0x03},
		PayloadEncoding: 0, // UNCOMPRESSED
		Payload:         []byte("fake image data"),
	}

	asset, err := ToCatenaAsset(dp, true)
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
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
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
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
	if err == nil {
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
	if err == nil {
		t.Error("expected error when neither payload nor url are provided")
	}
}

func TestToCatenaAsset_WithGzipEncoding(t *testing.T) {
	dp := DataPayload{
		Payload:         []byte("compressed data"),
		PayloadEncoding: 1, // GZIP
	}

	asset, err := ToCatenaAsset(dp, true)
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
	}

	proto := asset.GetProtoAsset()
	if proto == nil {
		t.Fatal("expected non-nil proto asset")
	}

	payload := proto.GetPayload()
	if payload == nil {
		t.Fatal("expected non-nil payload")
	}
	if payload.GetPayloadEncoding() != 1 {
		t.Errorf("expected GZIP encoding (1), got %v", payload.GetPayloadEncoding())
	}
}

func TestToCatenaAsset_WithDeflateEncoding(t *testing.T) {
	dp := DataPayload{
		Payload:         []byte("compressed data"),
		PayloadEncoding: 2, // DEFLATE
	}

	asset, err := ToCatenaAsset(dp, true)
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
	}

	proto := asset.GetProtoAsset()
	payload := proto.GetPayload()
	if payload.GetPayloadEncoding() != 2 {
		t.Errorf("expected DEFLATE encoding (2), got %v", payload.GetPayloadEncoding())
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
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
	}

	jsonData, err := asset.ToJSON()
	if err != nil {
		t.Fatalf("ToJSON error: %v", err)
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
	if err != nil {
		t.Fatalf("ToJSON error: %v", err)
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
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
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
	if err != nil {
		t.Fatalf("ToCatenaAsset error: %v", err)
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
		PayloadEncoding: 1,
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
	if dp.PayloadEncoding != 1 {
		t.Errorf("expected PayloadEncoding 1, got %d", dp.PayloadEncoding)
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

	if dp.PayloadEncoding != 0 {
		t.Errorf("expected PayloadEncoding 0 (UNCOMPRESSED), got %d", dp.PayloadEncoding)
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
