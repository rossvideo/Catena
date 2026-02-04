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
	"encoding/json"
	"testing"
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
