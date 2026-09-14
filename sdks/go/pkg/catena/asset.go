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

package catena

import (
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// assetChunkSize is the maximum number of embedded payload bytes streamed per
// ReadAsset chunk. Kept small enough to bound per-message memory but large
// enough that small assets go out in a single chunk.
const assetChunkSize = 64 * 1024

// Stream the asset in chunks to demonstrate large-object delivery:
// the first chunk carries the metadata, digest, encoding, and
// cachable flag plus the first slice of payload bytes; later
// chunks carry only payload bytes (never the URL: a payload can
// have one or the other, and continuation chunks only ever exist
// for embedded payloads), which the client concatenates in send
// order. Assets up to assetChunkSize (and URL-kind assets, which
// have no embedded bytes) go out as a single chunk.
func SendAssetChunks(slot uint16, fqoid string, stream Stream[st2138.Asset], payload st2138.DataPayload, cachable bool) StatusResult {

	data := payload.Payload
	sent := 0

	for first := true; first || sent < len(data); first = false {
		end := min(sent+assetChunkSize, len(data))

		dp := st2138.DataPayload{Payload: data[sent:end]}
		if first {
			// First chunk preserves the original metadata/digest/encoding/url
			dp = payload
			dp.Payload = data[sent:end]
		}

		chunk, err := st2138.ToAsset(dp, cachable)
		if err != nil {
			logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", err)
			return StatusWithCode(StatusCodeInternal, "failed to convert asset: "+err.Error())
		}
		if err := stream.Send(chunk); err != nil {
			logger.Warning("Asset download stream closed", "slot", slot, "fqoid", fqoid, "error", err)
			return StatusWithCode(StatusCodeInternal, "failed to send asset: "+err.Error())
		}
		sent = end
	}

	logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(data))
	return StatusWithCode(StatusCodeOk, "")

}
