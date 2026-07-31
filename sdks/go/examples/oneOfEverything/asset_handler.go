package main

import (
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

// storedAsset bundles the converted DataPayload with the cachable flag from the
// original ExternalObjectPayload. DataPayload alone cannot carry cachable, so
// tracking it here lets GET faithfully round-trip the value supplied on POST/PUT.
type storedAsset struct {
	payload  st2138.DataPayload
	cachable bool
}

// assetChunkSize is the maximum number of embedded payload bytes streamed per
// ReadAsset chunk. Kept small enough to bound per-message memory but large
// enough that small assets go out in a single chunk.
const assetChunkSize = 64 * 1024

func registerAssetHandlers(srv catena.Server, assets *sync.Map) {
	// Slot 0-2: direct map-backed lookup, matching the sync.Map data-model example.
	for _, slot := range slotList {
		srv.RegisterReadAssetHandler(slot, func(slot uint16, fqoid string, ctx catena.HandlerContext, stream catena.Stream[st2138.Asset]) catena.StatusResult {
			logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)
			val, ok := assets.Load(fqoid)
			if !ok {
				logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found: "+fqoid)
			}

			// Stream the asset in chunks to demonstrate large-object delivery:
			// the first chunk carries the metadata, digest, encoding, and
			// cachable flag plus the first slice of payload bytes; later
			// chunks carry only payload bytes, which the client concatenates
			// in send order. Assets up to assetChunkSize (and URL-kind assets,
			// which have no embedded bytes) go out as a single chunk.
			stored := val.(storedAsset)
			data := stored.payload.Payload
			sent := 0
			for first := true; first || sent < len(data); first = false {
				end := min(sent+assetChunkSize, len(data))
				dp := st2138.DataPayload{Url: stored.payload.Url, Payload: data[sent:end]}
				if first {
					dp = stored.payload
					dp.Payload = data[sent:end]
				}
				chunk, err := st2138.ToAsset(dp, stored.cachable)
				if err != nil {
					logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", err)
					return catena.StatusWithCode(catena.StatusCodeInternal, "failed to convert asset: "+err.Error())
				}
				if err := stream.Send(chunk); err != nil {
					logger.Warning("Asset download stream closed", "slot", slot, "fqoid", fqoid, "error", err)
					return catena.StatusWithCode(catena.StatusCodeInternal, "failed to send asset: "+err.Error())
				}
				sent = end
			}

			logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(data))
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		// POST / CreateAsset: create a new asset, conflict if one already exists.
		srv.RegisterCreateAssetHandler(slot, func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("Asset load request", "slot", slot, "fqoid", fqoid)
			payload, err := st2138.FromAsset(asset)
			if err != nil {
				logger.Error("Failed to convert asset to payload", "slot", slot, "fqoid", fqoid, "error", err)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid asset payload: "+err.Error())
			}
			entry := storedAsset{payload: payload, cachable: asset.Proto.GetCachable()}
			if _, loaded := assets.LoadOrStore(fqoid, entry); loaded {
				logger.Warning("Asset already exists", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.StatusCodeAlreadyExists, "asset already exists: "+fqoid)
			}
			logger.Info("Asset load complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		// PUT / UpdateAsset: replace an existing asset, not found if missing.
		srv.RegisterUpdateAssetHandler(slot, func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("Asset overwrite request", "slot", slot, "fqoid", fqoid)
			payload, err := st2138.FromAsset(asset)
			if err != nil {
				logger.Error("Failed to convert asset to payload", "slot", slot, "fqoid", fqoid, "error", err)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid asset payload: "+err.Error())
			}
			if _, ok := assets.Load(fqoid); !ok {
				logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found: "+fqoid)
			}
			assets.Store(fqoid, storedAsset{payload: payload, cachable: asset.Proto.GetCachable()})
			logger.Info("Asset overwrite complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		// DELETE / DeleteAsset: remove an existing asset, not found if missing.
		srv.RegisterDeleteAssetHandler(slot, func(slot uint16, fqoid string, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("Asset delete request", "slot", slot, "fqoid", fqoid)
			if _, ok := assets.LoadAndDelete(fqoid); !ok {
				logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found: "+fqoid)
			}
			logger.Info("Asset delete complete", "slot", slot, "fqoid", fqoid)
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}
