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

package main

import (
	"fmt"
	"time"

	"github.com/golang-jwt/jwt/v5"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

const authzSlot = 3

func registerAuthz(srv catena.Server) {
	const (
		userAccess  = "user_access"
		claims      = "claims"
		rolesTable  = "roles_table"
		authzStatus = "authz_status"
	)

	makeUserAccess := func(ctx catena.HandlerContext) *st2138.Param {
		accessMap := map[string]any{}
		if ctx.HasWriteScope(st2138.ScopeMon) {
			accessMap["mon"] = "Read\nWrite"
		} else if ctx.HasReadScope(st2138.ScopeMon) {
			accessMap["mon"] = "Read\n---"
		} else {
			accessMap["mon"] = "---\n---"
		}
		if ctx.HasWriteScope(st2138.ScopeOp) {
			accessMap["op"] = "Read\nWrite"
		} else if ctx.HasReadScope(st2138.ScopeOp) {
			accessMap["op"] = "Read\n---"
		} else {
			accessMap["op"] = "---\n---"
		}
		if ctx.HasWriteScope(st2138.ScopeCfg) {
			accessMap["cfg"] = "Read\nWrite"
		} else if ctx.HasReadScope(st2138.ScopeCfg) {
			accessMap["cfg"] = "Read\n---"
		} else {
			accessMap["cfg"] = "---\n---"
		}
		if ctx.HasWriteScope(st2138.ScopeAdm) {
			accessMap["adm"] = "Read\nWrite"
		} else if ctx.HasReadScope(st2138.ScopeAdm) {
			accessMap["adm"] = "Read\n---"
		} else {
			accessMap["adm"] = "---\n---"
		}
		return st2138.NewParamStruct(accessMap).
			WithReadOnly(true).
			WithName(st2138.PolyglotText{"en": "Scopes"}).
			WithParam("mon", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Monitor"))).
			WithParam("op", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Operate"))).
			WithParam("cfg", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Configure"))).
			WithParam("adm", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Administer")))
	}

	makeRolesTable := func() *st2138.Param {
		roles := []map[string]any{
			{"role": "commissioner", "mon": "Read\nWrite", "op": "Read\nWrite", "cfg": "Read\nWrite", "adm": "Read\nWrite"},
			{"role": "it-admin", "mon": "Read\nWrite", "op": "Read\n---", "cfg": "Read\nWrite", "adm": "---\n---"},
			{"role": "journalist", "mon": "Read\nWrite", "op": "Read\nWrite", "cfg": "Read\n---", "adm": "---\n---"},
			{"role": "runner", "mon": "Read\n---", "op": "Read\n---", "cfg": "---\n---", "adm": "---\n---"},
			{"role": "producer", "mon": "Read\nWrite", "op": "Read\n---", "cfg": "Read\n---", "adm": "---\n---"},
			{"role": "tech-director", "mon": "Read\nWrite", "op": "Read\nWrite", "cfg": "Read\nWrite", "adm": "Read\n---"},
		}
		return st2138.NewParamStructArray(roles).
			WithReadOnly(true).
			WithParam("role", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Role"))).
			WithParam("mon", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Monitor"))).
			WithParam("op", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Operate"))).
			WithParam("cfg", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Configure"))).
			WithParam("adm", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Administer")))
	}

	makeClaims := func(ctx catena.HandlerContext) *st2138.Param {
		valMap := make([]map[string]any, 0)
		if ctx.Token != nil {
			claims, ok := ctx.Token.Claims.(jwt.MapClaims)
			if ok {
				for k, v := range claims {
					if sec, ok := v.(float64); ok && (k == "exp" || k == "iat") {
						v = time.Unix(int64(sec), 0).UTC().Format(time.RFC3339)
					}
					valMap = append(valMap, map[string]any{
						"key":   k,
						"value": fmt.Sprint(v),
					})
				}
			}
		}
		return st2138.NewParamStructArray(valMap).
			WithReadOnly(true).
			WithName(st2138.PolyglotText{"en": "JWT Claims"}).
			WithParam("key", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Key"))).
			WithParam("value", st2138.NewParamString().WithName(st2138.NewPolyglotText("en", "Value")))
	}

	// Shown in place of the scope/claims params when authz is disabled: the
	// scope helpers grant everything, so per-user access can't be reported.
	makeAuthzStatus := func() *st2138.Param {
		return st2138.NewParamString("Authorization is disabled; scope information cannot be shown.").
			WithReadOnly(true).
			WithName(st2138.PolyglotText{"en": "Authorization"})
	}

	srv.RegisterProductStruct(authzSlot, catena.ProductStruct{
		Name:         "Authz Demo",
		Vendor:       "Ross Video",
		Version:      "v1.0.0",
		SerialNumber: "1234567890",
	})

	srv.RegisterGetDeviceHandler(authzSlot, func(slot uint16, ctx catena.HandlerContext, stream catena.Stream[st2138.DeviceComponent]) catena.StatusResult {
		if err := stream.Send(
			st2138.ComponentDevice(st2138.NewDevice(authzSlot).
				WithDefaultScope(st2138.ScopeMon).
				WithMultiSetEnabled(false).
				WithAccessScopes(st2138.ScopeMon, st2138.ScopeOp, st2138.ScopeCfg, st2138.ScopeAdm).
				WithMenuGroup("status", st2138.NewMenuGroup().WithName(st2138.PolyglotText{"en": "Status"})).
				WithMenuGroup("config", st2138.NewMenuGroup().WithName(st2138.PolyglotText{"en": "Config"}))),
		); err != nil {
			return catena.StatusError(err)
		}

		// hardcode so we're not at the mercy of map iteration order
		loginOids := []string{userAccess, claims}
		if !ctx.AuthzEnabled() {
			loginOids = []string{authzStatus}
		}
		if err := stream.Send(
			st2138.ComponentMenu("status/login", st2138.NewMenu().
				WithName(st2138.PolyglotText{"en": "Login"}).
				WithParamOids(loginOids...),
			),
		); err != nil {
			return catena.StatusError(err)
		}

		if err := stream.Send(
			st2138.ComponentMenu("config/roles", st2138.NewMenu().
				WithName(st2138.PolyglotText{"en": "Roles"}).
				// hardcode so we're not at the mercy of map iteration order
				WithParamOids(
					rolesTable,
				),
			),
		); err != nil {
			return catena.StatusError(err)
		}

		if ctx.HasReadScope(st2138.ScopeMon) {
			if ctx.AuthzEnabled() {
				if err := stream.Send(st2138.ComponentParam(userAccess, makeUserAccess(ctx))); err != nil {
					return catena.StatusError(err)
				}
				if err := stream.Send(st2138.ComponentParam(claims, makeClaims(ctx))); err != nil {
					return catena.StatusError(err)
				}
			} else if err := stream.Send(st2138.ComponentParam(authzStatus, makeAuthzStatus())); err != nil {
				return catena.StatusError(err)
			}
			if err := stream.Send(st2138.ComponentParam(rolesTable, makeRolesTable())); err != nil {
				return catena.StatusError(err)
			}
		}
		return catena.StatusOk()
	})

	srv.RegisterGetParamHandler(authzSlot, func(slot uint16, fqoid string, ctx catena.HandlerContext) (st2138.Param, catena.StatusResult) {
		if ctx.HasReadScope(st2138.ScopeMon) {
			switch fqoid {
			case userAccess:
				if ctx.AuthzEnabled() {
					return catena.Reply(*makeUserAccess(ctx))
				}
			case claims:
				if ctx.AuthzEnabled() {
					return catena.Reply(*makeClaims(ctx))
				}
			case authzStatus:
				if !ctx.AuthzEnabled() {
					return catena.Reply(*makeAuthzStatus())
				}
			case rolesTable:
				return catena.Reply(*makeRolesTable())
			}
		}
		return catena.ReplyError[st2138.Param](catena.StatusCodeNotFound, "param not found")
	})

	srv.RegisterGetValueHandler(authzSlot, func(slot uint16, fqoid string, ctx catena.HandlerContext) (st2138.Value, catena.StatusResult) {
		if ctx.HasReadScope(st2138.ScopeMon) {
			switch fqoid {
			case userAccess:
				if ctx.AuthzEnabled() {
					return catena.Reply(st2138.Value{Proto: makeUserAccess(ctx).Proto.Value})
				}
			case claims:
				if ctx.AuthzEnabled() {
					return catena.Reply(st2138.Value{Proto: makeClaims(ctx).Proto.Value})
				}
			case authzStatus:
				if !ctx.AuthzEnabled() {
					return catena.Reply(st2138.Value{Proto: makeAuthzStatus().Proto.Value})
				}
			case rolesTable:
				return catena.Reply(st2138.Value{Proto: makeRolesTable().Proto.Value})
			}
		}
		return catena.ReplyError[st2138.Value](catena.StatusCodeNotFound, "value not found")
	})

	srv.RegisterSetValueHandler(authzSlot, func(slot uint16, entries []catena.SetValueEntry, ctx catena.HandlerContext) catena.StatusResult {
		if len(entries) > 1 {
			return catena.StatusWithCode(catena.StatusCodePermissionDenied, "multi set not allowed")
		}

		return catena.StatusWithCode(catena.StatusCodePermissionDenied, "write not allowed")
	})
}
