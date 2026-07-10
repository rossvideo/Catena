package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// registerLanguageHandlers wires the AddLanguage and LanguagePackRequest
// endpoints for every slot. Packs are stored in ExampleState so that a language
// added at runtime can be queried back and also shows up in DeviceRequest.
func registerLanguageHandlers(srv catena.Server, state *ExampleState) {
	for _, slot := range slotList {
		srv.RegisterAddLanguageHandler(slot, func(slot uint16, language string, pack catena.LanguagePack, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("AddLanguage", "slot", slot, "language", language, "name", pack.GetName())
			state.addLanguagePack(slot, language, pack)
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		srv.RegisterLanguagePackHandler(slot, func(slot uint16, language string, ctx catena.HandlerContext) (catena.LanguagePack, catena.StatusResult) {
			logger.Info("LanguagePackRequest", "slot", slot, "language", language)
			pack, ok := state.languagePack(slot, language)
			if !ok {
				return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeNotFound, "language not found: "+language)
			}
			return pack, catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}
