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
 * @brief HandlerContext is the context passed to all Catena handlers, providing access to the request context and other information.
 * @file handler_context.go
 * @copyright Copyright © 2026 Ross Video Ltd
 */

package catena

import (
	"context"
	"slices"

	"github.com/golang-jwt/jwt/v5"
)

// TransportContext is the raw, per-request input a transport hands to the
// server: the caller's access token, the request metadata (e.g. headers), and
// the request (or stream) context. The server resolves it - validating the
// token and applying authorization - into the HandlerContext that handlers
// actually receive.
type TransportContext struct {
	AccessToken string
	Metadata    map[string][]string
	Ctx         context.Context
}

// HandlerContext gives a handler everything it needs to know about the request
// it is serving. Use it to:
//   - check the caller's authorization with the scope helpers
//     (HasReadScope/HasWriteScope/HasAnyReadScope/HasAnyWriteScope). When
//     authorization is disabled these all return true.
//   - read the request Metadata (for example, transport headers).
//   - respect cancellation and deadlines through Context() (see below),
//     which matters most for long-running or streaming handlers.
//
// Treat it as read-only; the server fills it in before your handler runs.
type HandlerContext struct {
	Token        *jwt.Token
	readScopes   map[string]struct{}
	writeScopes  map[string]struct{}
	Metadata     map[string][]string
	authzEnabled bool
	ctx          context.Context
	// ctxCancel tears down ctx and unregisters its shutdown watcher. The gate
	// builds ctx and stashes cancel here; whoever owns the end of the request
	// (invokeHandler's defer, GetSlots, or a torn-down streaming Connection)
	// calls release(). Do not call this field directly - use release() so the
	// zero HandlerContext (nil cancel) stays safe.
	ctxCancel context.CancelFunc
}

// HasReadScope reports whether the caller was granted the named read scope.
// A write grant implies read: holding "foo:w" satisfies HasReadScope("foo") as
// well as HasWriteScope("foo"). It returns true for every scope when
// authorization is disabled.
func (ctx HandlerContext) HasReadScope(scopeName string) bool {
	if !ctx.authzEnabled {
		return true
	}
	_, ok := ctx.readScopes[scopeName]
	return ok
}

// HasWriteScope reports whether the caller was granted the named write scope.
// It returns true for every scope when authorization is disabled.
func (ctx HandlerContext) HasWriteScope(scopeName string) bool {
	if !ctx.authzEnabled {
		return true
	}
	_, ok := ctx.writeScopes[scopeName]
	return ok
}

// HasAnyWriteScope reports whether the caller holds at least one write scope,
// i.e. whether they are allowed to make any change at all. Use it as a coarse
// gate before the more specific HasWriteScope check.
func (ctx HandlerContext) HasAnyWriteScope() bool {
	return slices.ContainsFunc(catenaScopes, ctx.HasWriteScope)
}

// HasAnyReadScope reports whether the caller holds at least one read scope,
// i.e. whether they are allowed to read anything at all. Use it as a coarse
// gate before the more specific HasReadScope check.
func (ctx HandlerContext) HasAnyReadScope() bool {
	return slices.ContainsFunc(catenaScopes, ctx.HasReadScope)
}

// HasAnyWriteScopeExceptMonitor reports whether the caller holds a write scope
// other than the monitor scope - that is, op, cfg, or adm write. Per ST 2138
// the asset mutations (LoadAsset/OverwriteAsset/DeleteAsset) accept adm:w,
// op:w, or cfg:w but never mon:w, so HasAnyWriteScope is too permissive for
// them. It returns true for every scope when authorization is disabled.
func (ctx HandlerContext) HasAnyWriteScopeExceptMonitor() bool {
	if !ctx.authzEnabled {
		return true
	}
	return ctx.HasWriteScope(ScopeOp) || ctx.HasWriteScope(ScopeCfg) || ctx.HasWriteScope(ScopeAdm)
}

// Context returns the context.Context for this request. It is Done when either
// the request (or stream) ends or the server begins shutting down, whichever
// happens first, and it carries any deadline and values from the request.
// Long-running and streaming handlers should select on ctx.Done() (or pass ctx
// to cancellable calls) so they stop promptly when the caller goes away or the
// server is stopping.
func (ctx HandlerContext) Context() context.Context {
	return ctx.ctx
}

// release cancels the request context and unregisters its shutdown watcher. It
// is safe to call multiple times and on a zero HandlerContext (nil ctxCancel),
// so callers can defer it unconditionally after the gate.
func (ctx HandlerContext) release() {
	if ctx.ctxCancel != nil {
		ctx.ctxCancel()
	}
}
