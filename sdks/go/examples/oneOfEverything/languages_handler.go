package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// registerLanguagesHandlers wires the Languages endpoint for each slot. The
// handler returns the language codes the device model supports; adopters would
// derive this from their own language packs. Here a small static list is used.
func registerLanguagesHandlers(srv catena.Server) {
	supported := []string{"en", "fr"}
	for _, slot := range slotList {
		srv.RegisterListLanguagesHandler(slot, func(slot uint16, ctx catena.HandlerContext) ([]string, catena.StatusResult) {
			logger.Info("ListLanguages", "slot", slot)
			return supported, catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}
