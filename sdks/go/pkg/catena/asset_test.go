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
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

package catena

import (
	"errors"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestSendAssetChunks(t *testing.T) {
	t.Run("SingleEmbeddedPayloadChunk", func(t *testing.T) {
		stream := &sliceStream[st2138.Asset]{}
		payload := st2138.DataPayload{
			Metadata:        map[string]string{"file-name": "test.txt"},
			Digest:          []byte("digest"),
			PayloadEncoding: st2138.EncodingGzip,
			Payload:         []byte("small asset"),
		}

		result := SendAssetChunks(1, "asset", stream, payload, true)
		if result.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v", result)
		}

		if len(stream.Items) != 1 {
			t.Fatalf("expected 1 chunk, got %d", len(stream.Items))
		}

		chunk := stream.Items[0].Proto
		if !chunk.GetCachable() {
			t.Error("expected cachable flag on chunk")
		}
		if recMetaData := chunk.GetPayload().GetMetadata()["file-name"]; recMetaData != "test.txt" {
			t.Errorf("file name = %q, want %q", recMetaData, "test.txt")
		}
		if recDigest := chunk.GetPayload().GetDigest(); string(recDigest) != "digest" {
			t.Errorf("digest = %q, want %q", recDigest, "digest")
		}
		if recEncoding := chunk.GetPayload().GetPayloadEncoding(); int32(recEncoding) != int32(st2138.EncodingGzip) {
			t.Errorf("payload encoding = %v, want %v, %q", recEncoding, st2138.EncodingGzip, st2138.EncodingGzip.String())
		}
		if recPayload := chunk.GetPayload().GetPayload(); string(recPayload) != "small asset" {
			t.Errorf("payload = %q, want %q", recPayload, "small asset")
		}
	})

	t.Run("SingleURLPayloadChunk", func(t *testing.T) {
		stream := &sliceStream[st2138.Asset]{}
		payload := st2138.ToPayloadFromURL("https://example.com/test.txt")

		result := SendAssetChunks(1, "asset", stream, payload, false)
		if result.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v", result)
		}

		if len(stream.Items) != 1 {
			t.Fatalf("expected 1 chunk, got %d", len(stream.Items))
		}

		chunkPayload := stream.Items[0].Proto.GetPayload()
		if recUrl := chunkPayload.GetUrl(); recUrl != payload.Url {
			t.Errorf("URL = %q, want %q", recUrl, payload.Url)
		}
	})

	t.Run("InvalidDataPayload", func(t *testing.T) {
		stream := &sliceStream[st2138.Asset]{}
		payload := st2138.DataPayload{
			Metadata:        map[string]string{"file-name": "test.txt"},
			Digest:          []byte("digest"),
			PayloadEncoding: st2138.EncodingGzip,
			Payload:         []byte("test"),
			Url:             "https://example.com/test.txt", // Invalid: both payload and URL set
		}

		result := SendAssetChunks(1, "asset", stream, payload, true)
		if result.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL, got %v", result)
		}

		if len(stream.Items) != 0 {
			t.Fatalf("expected no chunks, got %d", len(stream.Items))
		}
	})
}

func TestSendAssetChunksWithSize(t *testing.T) {
	t.Run("MultipleEmbeddedChunks", func(t *testing.T) {
		stream := &sliceStream[st2138.Asset]{}
		payload := st2138.DataPayload{
			Metadata:        map[string]string{"file-name": "test.txt"},
			Digest:          []byte("digest"),
			PayloadEncoding: st2138.EncodingGzip,
			Payload:         []byte("0123456789"),
		}

		result := SendAssetChunksWithSize(1, "asset", stream, payload, true, 3)
		if result.Code != StatusCodeOk {
			t.Fatalf("expected OK, got %v", result)
		}

		if len(stream.Items) != 4 {
			t.Fatalf("expected 4 chunks, got %d", len(stream.Items))
		}

		var aggregatePayload []byte
		for i, chunk := range stream.Items {
			assetPayload := chunk.Proto.GetPayload()
			aggregatePayload = append(aggregatePayload, assetPayload.GetPayload()...)

			if i == 0 {
				// First chunk should contain metadata, digest, and encoding, and cachable information
				if !chunk.Proto.GetCachable() {
					t.Errorf("chunk %d cachable = false, want true", i)
				}
				if recMetaData := assetPayload.GetMetadata()["file-name"]; recMetaData != "test.txt" {
					t.Errorf("chunk %d file name = %q, want %q", i, recMetaData, "test.txt")
				}
				if recDigest := assetPayload.GetDigest(); string(recDigest) != "digest" {
					t.Errorf("chunk %d digest = %q, want %q", i, recDigest, "digest")
				}
				if recEncoding := assetPayload.GetPayloadEncoding(); int32(recEncoding) != int32(st2138.EncodingGzip) {
					t.Errorf("chunk %d payload encoding = %v, want %v, %q", i, recEncoding, st2138.EncodingGzip, st2138.EncodingGzip.String())
				}

				continue
			}

			if len(assetPayload.GetMetadata()) != 0 {
				t.Errorf("chunk %d unexpectedly contained metadata", i)
			}
			if len(assetPayload.GetDigest()) != 0 {
				t.Errorf("chunk %d unexpectedly contained digest", i)
			}
			if int32(assetPayload.GetPayloadEncoding()) != int32(st2138.EncodingUncompressed) {
				t.Errorf("chunk %d unexpectedly contained payload encoding", i)
			}
		}

		if string(aggregatePayload) != string(payload.Payload) {
			t.Errorf("concatenated payload = %q, want %q", aggregatePayload, payload.Payload)
		}
	})

	t.Run("SendWithInvalidChunkSize", func(t *testing.T) {
		stream := &sliceStream[st2138.Asset]{}
		payload := st2138.DataPayload{Payload: []byte("test")}

		result := SendAssetChunksWithSize(1, "asset", stream, payload, false, 0)
		if result.Code != StatusCodeInvalidArgument {
			t.Fatalf("expected InvalidArgument, got %v", result)
		}

		if len(stream.Items) != 0 {
			t.Fatalf("expected no chunks, got %d", len(stream.Items))
		}
	})

	t.Run("SendFailureStopsImmediately", func(t *testing.T) {
		stream := &sliceStream[st2138.Asset]{Err: errors.New("testError"), FailAfter: 2}
		payload := st2138.DataPayload{Payload: []byte("0123456789")}

		result := SendAssetChunksWithSize(1, "asset", stream, payload, false, 3)

		if result.Code != StatusCodeInternal {
			t.Fatalf("expected INTERNAL, got %v", result)
		}

		if len(stream.Items) != 2 {
			t.Fatalf("expected 2 chunks before failure, got %d", len(stream.Items))
		}
	})
}
