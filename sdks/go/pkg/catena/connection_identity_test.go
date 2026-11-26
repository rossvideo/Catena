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
 * @brief Connection Identity management tests
 * @file connection_identity_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Omar Shah (omar.shah@rossvideo.com)
 * @date 2026-11-25
 */

package catena

import (
	"testing"

	"github.com/golang-jwt/jwt/v5"
)

func TestDeriveConnectionIdentity(t *testing.T) {
	newToken := func(claims jwt.Claims) *jwt.Token {
		return jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	}

	t.Run("Nil token returns error", func(t *testing.T) {
		got, err := deriveConnectionIdentity(nil)
		if err == nil {
			t.Fatal("expected error for nil token, got nil")
		}
		if got != (connectionIdentity{}) {
			t.Errorf("expected zero identity, got %+v", got)
		}
	})

	t.Run("Nil claims returns error", func(t *testing.T) {
		got, err := deriveConnectionIdentity(&jwt.Token{})
		if err == nil {
			t.Fatal("expected error for nil claims, got nil")
		}
		if got != (connectionIdentity{}) {
			t.Errorf("expected zero identity, got %+v", got)
		}
	})

	t.Run("Non MapClaims type returns error", func(t *testing.T) {
		got, err := deriveConnectionIdentity(newToken(jwt.RegisteredClaims{}))
		if err == nil {
			t.Fatal("expected error for non-MapClaims claims, got nil")
		}
		if got != (connectionIdentity{}) {
			t.Errorf("expected zero identity, got %+v", got)
		}
	})

	t.Run("uses sid when present", func(t *testing.T) {
		token := newToken(jwt.MapClaims{
			"sid": "session-1",
			"sub": "user-1",
			"azp": "client-1",
		})
		got, err := deriveConnectionIdentity(token)
		if err != nil {
			t.Fatalf("unexpected error: %v", err)
		}
		if want := (connectionIdentity{sid: "session-1"}); got != want {
			t.Errorf("expected %+v, got %+v", want, got)
		}
	})

	t.Run("uses sub and clientId when sid is not present", func(t *testing.T) {
		token := newToken(jwt.MapClaims{
			"sub": "user-1",
			"azp": "client-1",
		})
		got, err := deriveConnectionIdentity(token)
		if err != nil {
			t.Fatalf("unexpected error: %v", err)
		}
		if want := (connectionIdentity{sub: "user-1", clientId: "client-1"}); got != want {
			t.Errorf("expected %+v, got %+v", want, got)
		}
	})

	t.Run("uses client_id when azp is not present", func(t *testing.T) {
		token := newToken(jwt.MapClaims{
			"sub":       "user-1",
			"client_id": "client-2",
		})
		got, err := deriveConnectionIdentity(token)
		if err != nil {
			t.Fatalf("unexpected error: %v", err)
		}
		if want := (connectionIdentity{sub: "user-1", clientId: "client-2"}); got != want {
			t.Errorf("expected %+v, got %+v", want, got)
		}
	})

	t.Run("azp takes precedence over client_id", func(t *testing.T) {
		token := newToken(jwt.MapClaims{
			"sub":       "user-1",
			"azp":       "client-1",
			"client_id": "client-2",
		})
		got, err := deriveConnectionIdentity(token)
		if err != nil {
			t.Fatalf("unexpected error: %v", err)
		}
		if want := (connectionIdentity{sub: "user-1", clientId: "client-1"}); got != want {
			t.Errorf("expected %+v, got %+v", want, got)
		}
	})

	t.Run("Missing sub returns error", func(t *testing.T) {
		token := newToken(jwt.MapClaims{"azp": "client-1"})
		got, err := deriveConnectionIdentity(token)
		if err == nil {
			t.Fatal("expected error for missing sub, got nil")
		}
		if got != (connectionIdentity{}) {
			t.Errorf("expected zero identity, got %+v", got)
		}
	})

	t.Run("Missing azp and client_id returns error", func(t *testing.T) {
		token := newToken(jwt.MapClaims{"sub": "user-1"})
		got, err := deriveConnectionIdentity(token)
		if err == nil {
			t.Fatal("expected error for missing client id, got nil")
		}
		if got != (connectionIdentity{}) {
			t.Errorf("expected zero identity, got %+v", got)
		}
	})
}

func TestConnectionIdentity_Kind(t *testing.T) {
	t.Run("sid when a session id is set", func(t *testing.T) {
		if got := (connectionIdentity{sid: "session-1"}).kind(); got != "sid" {
			t.Errorf("kind() = %q, want %q", got, "sid")
		}
	})

	t.Run("sub when sub and client id are set", func(t *testing.T) {
		if got := (connectionIdentity{sub: "user-1", clientId: "client-1"}).kind(); got != "sub" {
			t.Errorf("kind() = %q, want %q", got, "sub")
		}
	})

	t.Run("unknown when the identity is empty", func(t *testing.T) {
		if got := (connectionIdentity{}).kind(); got != "unknown" {
			t.Errorf("kind() = %q, want %q", got, "unknown")
		}
	})
}
