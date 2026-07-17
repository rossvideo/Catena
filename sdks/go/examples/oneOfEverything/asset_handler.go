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

func registerAssetHandlers(srv catena.Server, assets *sync.Map) {
	// Slot 0-2: direct map-backed lookup, matching the sync.Map data-model example.
	for _, slot := range slotList {
		srv.RegisterReadAssetHandler(slot, func(slot uint16, fqoid string, ctx catena.HandlerContext) (st2138.Asset, catena.StatusResult) {
			logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)
			val, ok := assets.Load(fqoid)
			if !ok {
				logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
				return catena.ReplyError[st2138.Asset](catena.StatusCodeNotFound, "asset not found: "+fqoid)
			}

			stored := val.(storedAsset)
			catenaAsset, res := st2138.ToAsset(stored.payload, stored.cachable)
			if res.Code != catena.StatusCodeOk {
				logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", res.Error)
				return catena.ReplyError[st2138.Asset](catena.StatusCodeInternal, "failed to convert asset: "+res.Error)
			}

			logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(stored.payload.Payload))
			return catena.Reply(catenaAsset)
		})

		// POST / CreateAsset: create a new asset, conflict if one already exists.
		srv.RegisterCreateAssetHandler(slot, func(slot uint16, fqoid string, asset st2138.Asset, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("Asset load request", "slot", slot, "fqoid", fqoid)
			payload, res := st2138.FromAsset(asset)
			if res.Code != catena.StatusCodeOk {
				logger.Error("Failed to convert asset to payload", "slot", slot, "fqoid", fqoid, "error", res.Error)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid asset payload: "+res.Error)
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
			payload, res := st2138.FromAsset(asset)
			if res.Code != catena.StatusCodeOk {
				logger.Error("Failed to convert asset to payload", "slot", slot, "fqoid", fqoid, "error", res.Error)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid asset payload: "+res.Error)
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
