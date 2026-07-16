package main

import (
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerAssetHandlers(srv catena.Server, assets *sync.Map) {
	// Slot 0-2: direct map-backed lookup, matching the sync.Map data-model example.
	for _, slot := range slotList {
		srv.RegisterGetAssetHandler(slot, func(slot uint16, fqoid string, ctx catena.HandlerContext) (catena.Asset, catena.StatusResult) {
			logger.Info("Asset download request", "slot", slot, "fqoid", fqoid)
			val, ok := assets.Load(fqoid)
			if !ok {
				logger.Warning("Asset not found", "slot", slot, "fqoid", fqoid)
				return catena.ReplyError[catena.Asset](catena.StatusCodeNotFound, "asset not found: "+fqoid)
			}

			payload := val.(catena.DataPayload)
			catenaAsset, res := catena.ToAsset(payload, true)
			if res.Code != catena.StatusCodeOk {
				logger.Error("Failed to convert payload to asset", "slot", slot, "fqoid", fqoid, "error", res.Error)
				return catena.ReplyError[catena.Asset](catena.StatusCodeInternal, "failed to convert asset: "+res.Error)
			}

			logger.Info("Asset download complete", "slot", slot, "fqoid", fqoid, "size", len(payload.Payload))
			return catena.Reply(catenaAsset)
		})
	}
}
