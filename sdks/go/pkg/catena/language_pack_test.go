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
 * @brief Unit tests for language pack handling in the Catena SDK.
 * @file language_pack_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import "testing"

func TestNewLanguagePack(t *testing.T) {
	lp := NewLanguagePack()

	if lp.Proto == nil {
		t.Fatal("expected non-nil Proto")
	}
}

func TestLanguagePackBuilder(t *testing.T) {
	lp := NewLanguagePack().
		WithName("Global Spanish").
		WithWord("greeting", "Hola").
		WithWords(map[string]string{"parting": "Adiós", "greeting": "Buenos días"})

	if lp.GetName() != "Global Spanish" {
		t.Errorf("expected name 'Global Spanish', got %q", lp.GetName())
	}
	// WithWords should merge and overwrite the earlier WithWord entry.
	if lp.GetWords()["greeting"] != "Buenos días" {
		t.Errorf("expected greeting 'Buenos días', got %q", lp.GetWords()["greeting"])
	}
	if lp.GetWords()["parting"] != "Adiós" {
		t.Errorf("expected parting 'Adiós', got %q", lp.GetWords()["parting"])
	}
}

func TestLanguagePackWithWordsInitializesMap(t *testing.T) {
	// WithWords on a pack whose Words map is still nil must initialize it.
	lp := NewLanguagePack().WithWords(map[string]string{"greeting": "Hallo"})

	if lp.GetWords()["greeting"] != "Hallo" {
		t.Errorf("expected greeting 'Hallo', got %q", lp.GetWords()["greeting"])
	}
}

func TestLanguagePackBuilderZeroValue(t *testing.T) {
	// Builder methods must be safe on a zero-value LanguagePack (nil proto).
	lp := LanguagePack{}.WithName("French").WithWord("greeting", "Bonjour")

	if lp.Proto == nil {
		t.Fatal("expected builder to lazily initialize the proto")
	}
	if lp.GetName() != "French" {
		t.Errorf("expected name 'French', got %q", lp.GetName())
	}
	if lp.GetWords()["greeting"] != "Bonjour" {
		t.Errorf("expected greeting 'Bonjour', got %q", lp.GetWords()["greeting"])
	}
}

func TestServer_RegisterGetLanguagePackHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterGetLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) (LanguagePack, StatusResult) {
		handlerCalled = true
		if language != "es" {
			t.Errorf("expected language 'es', got %s", language)
		}
		return NewLanguagePack().WithName("Spanish"), StatusResult{Code: StatusCodeOk}
	})

	lp, status := srv.InvokeGetLanguagePackHandler(0, "es", validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if lp.GetName() != "Spanish" {
		t.Errorf("expected Spanish, got %q", lp.GetName())
	}
}

func TestServer_RegisterCreateLanguagePackHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterCreateLanguagePackHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
		handlerCalled = true
		if language != "fr" {
			t.Errorf("expected language 'fr', got %s", language)
		}
		return StatusResult{Code: StatusCodeOk}
	})

	status := srv.InvokeCreateLanguagePackHandler(0, "fr", NewLanguagePack(), admTestTransportContext(t))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
}

func TestServer_RegisterUpdateLanguagePackHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterUpdateLanguagePackHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
		handlerCalled = true
		return StatusResult{Code: StatusCodeOk}
	})

	status := srv.InvokeUpdateLanguagePackHandler(0, "fr", NewLanguagePack(), admTestTransportContext(t))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
}

func TestServer_RegisterDeleteLanguagePackHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterDeleteLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) StatusResult {
		handlerCalled = true
		return StatusResult{Code: StatusCodeOk}
	})

	status := srv.InvokeDeleteLanguagePackHandler(0, "fr", admTestTransportContext(t))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
}

func TestServer_LanguageHandlersNotRegistered(t *testing.T) {
	srv := newTestServer(t, true)
	ctx := validTestTransportContext(nil)

	if _, status := srv.InvokeGetLanguagePackHandler(0, "es", ctx); status.Code != StatusCodeNotFound {
		t.Errorf("get: expected NotFound, got %v", status.Code)
	}
	if status := srv.InvokeCreateLanguagePackHandler(0, "es", NewLanguagePack(), ctx); status.Code != StatusCodeNotFound {
		t.Errorf("add: expected NotFound, got %v", status.Code)
	}
	if status := srv.InvokeUpdateLanguagePackHandler(0, "es", NewLanguagePack(), ctx); status.Code != StatusCodeNotFound {
		t.Errorf("update: expected NotFound, got %v", status.Code)
	}
	if status := srv.InvokeDeleteLanguagePackHandler(0, "es", ctx); status.Code != StatusCodeNotFound {
		t.Errorf("delete: expected NotFound, got %v", status.Code)
	}
}

func TestServer_LanguageHandlersPermissionDenied(t *testing.T) {
	// "read write all" contains no recognized catena scopes, so it grants
	// neither read nor write. "st2138:cfg" grants read but not write.
	noReadContext := TransportContext{AccessToken: validTestJWTWithoutExecuteCommandScope}
	noWriteContext := TransportContext{AccessToken: validTestJWTWithCfgScope}

	t.Run("get requires read scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterGetLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) (LanguagePack, StatusResult) {
			t.Fatal("handler should not run without read scope")
			return LanguagePack{}, StatusResult{Code: StatusCodeOk}
		})
		if _, status := srv.InvokeGetLanguagePackHandler(0, "es", noReadContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("add requires write scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterCreateLanguagePackHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without write scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeCreateLanguagePackHandler(0, "es", NewLanguagePack(), noWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("update requires write scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterUpdateLanguagePackHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without write scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeUpdateLanguagePackHandler(0, "es", NewLanguagePack(), noWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("delete requires write scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterDeleteLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without write scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeDeleteLanguagePackHandler(0, "es", noWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	// validTestTransportContext carries the op write scope: enough to pass the
	// coarse write gate, but the spec requires adm specifically for mutations.
	opWriteContext := validTestTransportContext(nil)

	t.Run("add requires adm scope, not just write", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterCreateLanguagePackHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without adm scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeCreateLanguagePackHandler(0, "es", NewLanguagePack(), opWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("update requires adm scope, not just write", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterUpdateLanguagePackHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without adm scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeUpdateLanguagePackHandler(0, "es", NewLanguagePack(), opWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("delete requires adm scope, not just write", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterDeleteLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without adm scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeDeleteLanguagePackHandler(0, "es", opWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})
}

// TestServer_LanguageHandlersValidation covers the request validation that the
// server owns so transports don't each repeat it: a language is always
// required, and create/update also require a non-nil pack.
func TestServer_LanguageHandlersValidation(t *testing.T) {
	srv := newTestServer(t, true)
	ctx := admTestTransportContext(t)

	t.Run("get requires language", func(t *testing.T) {
		if _, status := srv.InvokeGetLanguagePackHandler(0, "", ctx); status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument, got %v", status.Code)
		}
	})

	t.Run("add requires language", func(t *testing.T) {
		if status := srv.InvokeCreateLanguagePackHandler(0, "", NewLanguagePack(), ctx); status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument, got %v", status.Code)
		}
	})

	t.Run("add requires pack", func(t *testing.T) {
		if status := srv.InvokeCreateLanguagePackHandler(0, "es", LanguagePack{}, ctx); status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument, got %v", status.Code)
		}
	})

	t.Run("update requires language", func(t *testing.T) {
		if status := srv.InvokeUpdateLanguagePackHandler(0, "", NewLanguagePack(), ctx); status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument, got %v", status.Code)
		}
	})

	t.Run("update requires pack", func(t *testing.T) {
		if status := srv.InvokeUpdateLanguagePackHandler(0, "es", LanguagePack{}, ctx); status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument, got %v", status.Code)
		}
	})

	t.Run("delete requires language", func(t *testing.T) {
		if status := srv.InvokeDeleteLanguagePackHandler(0, "", ctx); status.Code != StatusCodeInvalidArgument {
			t.Errorf("expected InvalidArgument, got %v", status.Code)
		}
	})
}
