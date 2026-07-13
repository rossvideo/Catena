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
	lp := NewLanguagePack("es")

	if lp.Proto == nil {
		t.Fatal("expected non-nil Proto")
	}
	if lp.Proto.GetLanguage() != "es" {
		t.Errorf("expected language 'es', got %q", lp.Proto.GetLanguage())
	}
	if lp.Proto.GetLanguagePack() == nil {
		t.Fatal("expected non-nil inner LanguagePack")
	}
}

func TestLanguagePackBuilder(t *testing.T) {
	lp := NewLanguagePack("es").
		WithName("Global Spanish").
		WithWord("greeting", "Hola").
		WithWords(map[string]string{"parting": "Adiós", "greeting": "Buenos días"})

	pack := lp.Proto.GetLanguagePack()
	if pack.GetName() != "Global Spanish" {
		t.Errorf("expected name 'Global Spanish', got %q", pack.GetName())
	}
	// WithWords should merge and overwrite the earlier WithWord entry.
	if pack.GetWords()["greeting"] != "Buenos días" {
		t.Errorf("expected greeting 'Buenos días', got %q", pack.GetWords()["greeting"])
	}
	if pack.GetWords()["parting"] != "Adiós" {
		t.Errorf("expected parting 'Adiós', got %q", pack.GetWords()["parting"])
	}
}

func TestLanguagePackWithWordsInitializesMap(t *testing.T) {
	// WithWords on a pack whose Words map is still nil must initialize it.
	lp := NewLanguagePack("de").WithWords(map[string]string{"greeting": "Hallo"})

	if lp.Proto.GetLanguagePack().GetWords()["greeting"] != "Hallo" {
		t.Errorf("expected greeting 'Hallo', got %q", lp.Proto.GetLanguagePack().GetWords()["greeting"])
	}
}

func TestLanguagePackBuilderZeroValue(t *testing.T) {
	// Builder methods must be safe on a zero-value LanguagePack (nil proto).
	lp := LanguagePack{}.WithName("French").WithWord("greeting", "Bonjour")

	if lp.Proto == nil || lp.Proto.GetLanguagePack() == nil {
		t.Fatal("expected builder to lazily initialize the proto")
	}
	if lp.Proto.GetLanguagePack().GetName() != "French" {
		t.Errorf("expected name 'French', got %q", lp.Proto.GetLanguagePack().GetName())
	}
	if lp.Proto.GetLanguagePack().GetWords()["greeting"] != "Bonjour" {
		t.Errorf("expected greeting 'Bonjour', got %q", lp.Proto.GetLanguagePack().GetWords()["greeting"])
	}
}

func TestServer_RegisterLanguagePackHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) (LanguagePack, StatusResult) {
		handlerCalled = true
		if language != "es" {
			t.Errorf("expected language 'es', got %s", language)
		}
		return NewLanguagePack(language).WithName("Spanish"), StatusResult{Code: StatusCodeOk}
	})

	lp, status := srv.InvokeLanguagePackHandler(0, "es", validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
	if lp.Proto.GetLanguagePack().GetName() != "Spanish" {
		t.Errorf("expected Spanish, got %q", lp.Proto.GetLanguagePack().GetName())
	}
}

func TestServer_RegisterAddLanguageHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterAddLanguageHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
		handlerCalled = true
		if language != "fr" {
			t.Errorf("expected language 'fr', got %s", language)
		}
		return StatusResult{Code: StatusCodeOk}
	})

	status := srv.InvokeAddLanguageHandler(0, "fr", NewLanguagePack("fr"), validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
}

func TestServer_RegisterUpdateLanguageHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterUpdateLanguageHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
		handlerCalled = true
		return StatusResult{Code: StatusCodeOk}
	})

	status := srv.InvokeUpdateLanguageHandler(0, "fr", NewLanguagePack("fr"), validTestTransportContext(nil))

	if !handlerCalled {
		t.Error("registered handler was not called")
	}
	if status.Code != StatusCodeOk {
		t.Errorf("expected OK status, got %v", status.Code)
	}
}

func TestServer_RegisterDeleteLanguageHandler(t *testing.T) {
	srv := newTestServer(t, true)

	handlerCalled := false
	srv.RegisterDeleteLanguageHandler(0, func(slot uint16, language string, ctx HandlerContext) StatusResult {
		handlerCalled = true
		return StatusResult{Code: StatusCodeOk}
	})

	status := srv.InvokeDeleteLanguageHandler(0, "fr", validTestTransportContext(nil))

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

	if _, status := srv.InvokeLanguagePackHandler(0, "es", ctx); status.Code != StatusCodeNotFound {
		t.Errorf("get: expected NotFound, got %v", status.Code)
	}
	if status := srv.InvokeAddLanguageHandler(0, "es", NewLanguagePack("es"), ctx); status.Code != StatusCodeNotFound {
		t.Errorf("add: expected NotFound, got %v", status.Code)
	}
	if status := srv.InvokeUpdateLanguageHandler(0, "es", NewLanguagePack("es"), ctx); status.Code != StatusCodeNotFound {
		t.Errorf("update: expected NotFound, got %v", status.Code)
	}
	if status := srv.InvokeDeleteLanguageHandler(0, "es", ctx); status.Code != StatusCodeNotFound {
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
		srv.RegisterLanguagePackHandler(0, func(slot uint16, language string, ctx HandlerContext) (LanguagePack, StatusResult) {
			t.Fatal("handler should not run without read scope")
			return LanguagePack{}, StatusResult{Code: StatusCodeOk}
		})
		if _, status := srv.InvokeLanguagePackHandler(0, "es", noReadContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("add requires write scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterAddLanguageHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without write scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeAddLanguageHandler(0, "es", NewLanguagePack("es"), noWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("update requires write scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterUpdateLanguageHandler(0, func(slot uint16, language string, languagePack LanguagePack, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without write scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeUpdateLanguageHandler(0, "es", NewLanguagePack("es"), noWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("delete requires write scope", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterDeleteLanguageHandler(0, func(slot uint16, language string, ctx HandlerContext) StatusResult {
			t.Fatal("handler should not run without write scope")
			return StatusResult{Code: StatusCodeOk}
		})
		if status := srv.InvokeDeleteLanguageHandler(0, "es", noWriteContext); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})
}

func TestServer_LanguageHandlersAccessDenied(t *testing.T) {
	ctx := validTestTransportContext(nil)
	denyAccess := func(endpointType EndpointType, hctx HandlerContext) bool { return false }

	t.Run("get", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterAccessHandler(denyAccess)
		if _, status := srv.InvokeLanguagePackHandler(0, "es", ctx); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("add", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterAccessHandler(denyAccess)
		if status := srv.InvokeAddLanguageHandler(0, "es", NewLanguagePack("es"), ctx); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("update", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterAccessHandler(denyAccess)
		if status := srv.InvokeUpdateLanguageHandler(0, "es", NewLanguagePack("es"), ctx); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})

	t.Run("delete", func(t *testing.T) {
		srv := newTestServer(t, true)
		srv.RegisterAccessHandler(denyAccess)
		if status := srv.InvokeDeleteLanguageHandler(0, "es", ctx); status.Code != StatusCodePermissionDenied {
			t.Errorf("expected PermissionDenied, got %v", status.Code)
		}
	})
}
