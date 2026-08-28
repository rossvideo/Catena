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
 * @brief Tests for HandlerContext
 * @file handler_context_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"context"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestHandlerContext(t *testing.T) {
	t.Run("Context", func(t *testing.T) {
		handler := HandlerContext{
			// make a non-trivial context to ensure it is returned unchanged
			ctx: context.WithValue(context.Background(), struct{}{}, "sentinal"),
		}
		// make sure Context() is a straight getter
		if handler.Context() != handler.ctx {
			t.Fatal("expected Context() to return the original context")
		}
	})

	t.Run("AuthzEnabled", func(t *testing.T) {
		enabled := HandlerContext{authzEnabled: true}
		if !enabled.AuthzEnabled() {
			t.Fatal("expected AuthzEnabled to return true when authz is enabled")
		}

		disabled := HandlerContext{authzEnabled: false}
		if disabled.AuthzEnabled() {
			t.Fatal("expected AuthzEnabled to return false when authz is disabled")
		}
	})

	t.Run("ScopeChecksUseSeparateReadAndWriteScopes", func(t *testing.T) {
		readOnly := HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeMon: {}},
			authzEnabled: true,
		}
		if !readOnly.HasReadScope(st2138.ScopeMon) {
			t.Fatal("expected read scope to satisfy read scope check")
		}
		if readOnly.HasAnyWriteScope() {
			t.Fatal("read scope should not satisfy write access")
		}
		if readOnly.HasWriteScope(st2138.ScopeMon) {
			t.Fatal("read scope should not satisfy write scope check")
		}

		writeOnly := HandlerContext{
			writeScopes:  map[string]struct{}{st2138.ScopeCfg: {}},
			authzEnabled: true,
		}
		if writeOnly.HasAnyReadScope() {
			t.Fatal("write scope should not satisfy read access unless it is also in read scopes")
		}
		if !writeOnly.HasAnyWriteScope() {
			t.Fatal("expected write scope to satisfy write access")
		}
		if writeOnly.HasReadScope(st2138.ScopeCfg) {
			t.Fatal("write scope should not satisfy read scope check")
		}

		parsedWrite := HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeCfg: {}},
			writeScopes:  map[string]struct{}{st2138.ScopeCfg: {}},
			authzEnabled: true,
		}
		if !parsedWrite.HasAnyReadScope() {
			t.Fatal("parsed write scope should satisfy read access because parsing adds it to read scopes")
		}
		if !parsedWrite.HasAnyWriteScope() {
			t.Fatal("parsed write scope should satisfy write access")
		}
	})

	t.Run("RequireScopeGeneratesStatusBoilerplate", func(t *testing.T) {
		authorized := HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeAdm: {}},
			writeScopes:  map[string]struct{}{st2138.ScopeAdm: {}},
			authzEnabled: true,
		}
		if res := authorized.RequireReadScope(st2138.ScopeAdm); res.IsError() {
			t.Fatalf("expected OK when read scope is held, got %v", res)
		}
		if res := authorized.RequireWriteScope(st2138.ScopeAdm); res.IsError() {
			t.Fatalf("expected OK when write scope is held, got %v", res)
		}

		denied := HandlerContext{authzEnabled: true}
		if res := denied.RequireReadScope(st2138.ScopeAdm); res.Code != StatusCodePermissionDenied {
			t.Fatalf("expected PermissionDenied without read scope, got %v", res.Code)
		}
		if res := denied.RequireWriteScope(st2138.ScopeAdm); res.Code != StatusCodePermissionDenied {
			t.Fatalf("expected PermissionDenied without write scope, got %v", res.Code)
		}

		// With authorization disabled the scope checks pass for any scope.
		disabled := HandlerContext{}
		if res := disabled.RequireReadScope(st2138.ScopeAdm); res.IsError() {
			t.Fatalf("expected OK when authz disabled, got %v", res)
		}
		if res := disabled.RequireWriteScope(st2138.ScopeAdm); res.IsError() {
			t.Fatalf("expected OK when authz disabled, got %v", res)
		}
	})
}
