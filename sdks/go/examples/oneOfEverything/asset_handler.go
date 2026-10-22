package main

import (
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
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
		srv.RegisterReadAssetHandler(slot, func(ctx catena.HandlerContext, req catena.ReadAssetRequest, stream catena.Stream[st2138.Asset]) catena.StatusResult {
			ctx.Logger().Info("Asset download request", "slot", req.Slot, "fqoid", req.Fqoid)
			val, ok := assets.Load(req.Fqoid)
			if !ok {
				ctx.Logger().Warn("Asset not found", "slot", req.Slot, "fqoid", req.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found: "+req.Fqoid)
			}

			stored := val.(storedAsset)

			// SendAssetChunks streams the stored asset payload to the client,
			// splitting the embedded payload into chunks when necessary. The
			// first chunk carries asset metadata/digest/encoding and the cachable
			// flag. URL-based assets and small embedded assets are sent as a single
			// chunk while larger assets are split into multiple chunks.
			return catena.SendAssetChunks(req.Slot, req.Fqoid, stream, stored.payload, stored.cachable)
		})

		// POST / CreateAsset: create a new asset, conflict if one already exists.
		srv.RegisterCreateAssetHandler(slot, func(ctx catena.HandlerContext, req catena.CreateAssetRequest) catena.StatusResult {
			ctx.Logger().Info("Asset load request", "slot", req.Slot, "fqoid", req.Fqoid)
			payload, err := st2138.FromAsset(req.Asset)
			if err != nil {
				ctx.Logger().Error("Failed to convert asset to payload", "slot", req.Slot, "fqoid", req.Fqoid, "error", err)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid asset payload: "+err.Error())
			}
			entry := storedAsset{payload: payload, cachable: req.Asset.Proto.GetCachable()}
			if _, loaded := assets.LoadOrStore(req.Fqoid, entry); loaded {
				ctx.Logger().Warn("Asset already exists", "slot", req.Slot, "fqoid", req.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeAlreadyExists, "asset already exists: "+req.Fqoid)
			}
			ctx.Logger().Info("Asset load complete", "slot", req.Slot, "fqoid", req.Fqoid, "size", len(payload.Payload))
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		// PUT / UpdateAsset: replace an existing asset, not found if missing.
		srv.RegisterUpdateAssetHandler(slot, func(ctx catena.HandlerContext, req catena.UpdateAssetRequest) catena.StatusResult {
			ctx.Logger().Info("Asset overwrite request", "slot", req.Slot, "fqoid", req.Fqoid)
			payload, err := st2138.FromAsset(req.Asset)
			if err != nil {
				ctx.Logger().Error("Failed to convert asset to payload", "slot", req.Slot, "fqoid", req.Fqoid, "error", err)
				return catena.StatusWithCode(catena.StatusCodeInvalidArgument, "invalid asset payload: "+err.Error())
			}
			if _, ok := assets.Load(req.Fqoid); !ok {
				ctx.Logger().Warn("Asset not found", "slot", req.Slot, "fqoid", req.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found: "+req.Fqoid)
			}
			assets.Store(req.Fqoid, storedAsset{payload: payload, cachable: req.Asset.Proto.GetCachable()})
			ctx.Logger().Info("Asset overwrite complete", "slot", req.Slot, "fqoid", req.Fqoid, "size", len(payload.Payload))
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		// DELETE / DeleteAsset: remove an existing asset, not found if missing.
		srv.RegisterDeleteAssetHandler(slot, func(ctx catena.HandlerContext, req catena.DeleteAssetRequest) catena.StatusResult {
			ctx.Logger().Info("Asset delete request", "slot", req.Slot, "fqoid", req.Fqoid)
			if _, ok := assets.LoadAndDelete(req.Fqoid); !ok {
				ctx.Logger().Warn("Asset not found", "slot", req.Slot, "fqoid", req.Fqoid)
				return catena.StatusWithCode(catena.StatusCodeNotFound, "asset not found: "+req.Fqoid)
			}
			ctx.Logger().Info("Asset delete complete", "slot", req.Slot, "fqoid", req.Fqoid)
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}
