package main

import (
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// LanguageStore holds language packs per slot, keyed by language tag (e.g.
// "en", "nl"). It backs the AddLanguage/LanguagePackRequest endpoints (gRPC and
// REST) plus the REST-only update and delete language endpoints, so a pack
// added at runtime can be queried back, changed, or removed.
type LanguageStore struct {
	mu    sync.RWMutex
	packs map[uint16]map[string]catena.LanguagePack
}

// NewLanguageStore seeds each slot with a "Global English" pack so a
// LanguagePackRequest has something to return before any AddLanguage call.
func NewLanguageStore(slots []uint16) *LanguageStore {
	s := &LanguageStore{packs: make(map[uint16]map[string]catena.LanguagePack)}
	for _, slot := range slots {
		s.packs[slot] = map[string]catena.LanguagePack{
			"en": catena.NewLanguagePack().
				WithName("Global English").
				WithWords(map[string]string{
					"greeting": "Hello",
					"parting":  "Goodbye",
				}),
		}
	}
	return s
}

// Set stores (or overwrites) a language pack for a slot/language.
func (s *LanguageStore) Set(slot uint16, language string, pack catena.LanguagePack) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if s.packs[slot] == nil {
		s.packs[slot] = make(map[string]catena.LanguagePack)
	}
	s.packs[slot][language] = pack
}

// Get returns the stored language pack for a slot/language.
func (s *LanguageStore) Get(slot uint16, language string) (catena.LanguagePack, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	packs, ok := s.packs[slot]
	if !ok {
		return catena.LanguagePack{}, false
	}
	pack, ok := packs[language]
	return pack, ok
}

// Has reports whether a slot/language pack exists.
func (s *LanguageStore) Has(slot uint16, language string) bool {
	_, ok := s.Get(slot, language)
	return ok
}

// Delete removes a slot/language pack, reporting whether it existed.
func (s *LanguageStore) Delete(slot uint16, language string) bool {
	s.mu.Lock()
	defer s.mu.Unlock()
	packs, ok := s.packs[slot]
	if !ok {
		return false
	}
	if _, ok := packs[language]; !ok {
		return false
	}
	delete(packs, language)
	return true
}

// registerLanguagePackHandlers wires the language-pack endpoints for each slot.
// get + add are exercised by both gRPC and REST; update + delete are REST-only
// since the gRPC service has no such RPCs.
func registerLanguagePackHandlers(srv catena.Server) {
	languages := NewLanguageStore(slotList)
	for _, slot := range slotList {
		srv.RegisterGetLanguagePackHandler(slot, func(slot uint16, language string, ctx catena.HandlerContext) (catena.LanguagePack, catena.StatusResult) {
			logger.Info("LanguagePackRequest", "slot", slot, "language", language)
			pack, ok := languages.Get(slot, language)
			if !ok {
				return catena.LanguagePack{}, catena.StatusWithCode(catena.StatusCodeNotFound, "language not found: "+language)
			}
			return pack, catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		srv.RegisterCreateLanguagePackHandler(slot, func(slot uint16, language string, pack catena.LanguagePack, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("AddLanguage", "slot", slot, "language", language, "name", pack.GetName())
			languages.Set(slot, language, pack)
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		srv.RegisterUpdateLanguagePackHandler(slot, func(slot uint16, language string, pack catena.LanguagePack, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("UpdateLanguage", "slot", slot, "language", language, "name", pack.GetName())
			if !languages.Has(slot, language) {
				return catena.StatusWithCode(catena.StatusCodeNotFound, "language not found: "+language)
			}
			languages.Set(slot, language, pack)
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})

		srv.RegisterDeleteLanguagePackHandler(slot, func(slot uint16, language string, ctx catena.HandlerContext) catena.StatusResult {
			logger.Info("DeleteLanguage", "slot", slot, "language", language)
			if !languages.Delete(slot, language) {
				return catena.StatusWithCode(catena.StatusCodeNotFound, "language not found: "+language)
			}
			return catena.StatusWithCode(catena.StatusCodeOk, "")
		})
	}
}
