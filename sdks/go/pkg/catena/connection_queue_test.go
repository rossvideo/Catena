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

package catena

import (
	"bytes"
	"context"
	"log/slog"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func TestNewConnectionQueue(t *testing.T) {
	cq := newConnectionQueue(10, nil)

	if cq == nil {
		t.Fatal("NewConnectionQueue returned nil")
	}
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections, got %d", cq.connectionCount())
	}
}

func TestNewConnectionQueueWithOptions_InjectedClock(t *testing.T) {
	frozenClock := time.Date(2026, 11, 11, 12, 0, 0, 0, time.UTC)

	t.Run("accepts token that expires after the injected clock", func(t *testing.T) {
		cq := newConnectionQueueWithOptions(0, nil, withClock(func() time.Time { return frozenClock })).(*connectionQueue)
		owner := &stubTransport{tb: t}

		valid := HandlerContext{tokenExpiry: frozenClock.Add(time.Second)}
		conn, res := cq.registerOwnedConnection(owner, valid, nil)
		if res.Code != StatusCodeOk || conn == nil {
			t.Fatalf("injected clock should accept a token that expires after now, got conn=%v res=%v", conn, res)
		}
		if got := cq.getConnectionExpiry(conn); !got.Equal(valid.tokenExpiry) {
			t.Errorf("expiry = %v, want %v", got, valid.tokenExpiry)
		}
	})

	t.Run("rejects token expired before the injected clock", func(t *testing.T) {
		cq := newConnectionQueueWithOptions(0, nil, withClock(func() time.Time { return frozenClock }))
		owner := &stubTransport{tb: t}

		expired := HandlerContext{tokenExpiry: frozenClock.Add(-time.Second)}
		conn, res := cq.registerOwnedConnection(owner, expired, nil)
		if res.Code != StatusCodeUnauthenticated || conn != nil {
			t.Fatalf("injected clock should reject an already-expired token, got conn=%v res=%v", conn, res)
		}
		if res.Error != "Token expired" {
			t.Errorf("error = %q, want %q", res.Error, "Token expired")
		}
		if cq.connectionCount() != 0 {
			t.Errorf("expected 0 connections after rejected expiry, got %d", cq.connectionCount())
		}
	})
}

func TestConnectionQueue_Options(t *testing.T) {
	t.Run("applies clock, sweep interval, and grace period", func(t *testing.T) {
		frozen := time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC)
		grace := 250 * time.Millisecond
		cq := newConnectionQueueWithOptions(0, nil, withClock(func() time.Time { return frozen }), withExpirySweepInterval(0), withExpiryGracePeriod(grace)).(*connectionQueue)

		if !cq.now().Equal(frozen) {
			t.Errorf("clock = %v, want %v", cq.now(), frozen)
		}
		if cq.expirySweepInterval != 0 {
			t.Errorf("sweep interval = %v, want 0", cq.expirySweepInterval)
		}
		if cq.expiryGracePeriod != grace {
			t.Errorf("grace period = %v, want %v", cq.expiryGracePeriod, grace)
		}
	})

	t.Run("ignores a nil option", func(t *testing.T) {
		grace := time.Millisecond
		cq := newConnectionQueueWithOptions(0, nil, nil, withExpirySweepInterval(0), withExpiryGracePeriod(grace)).(*connectionQueue)

		if cq.expirySweepInterval != 0 {
			t.Errorf("sweep interval = %v, want 0", cq.expirySweepInterval)
		}
		if cq.expiryGracePeriod != grace {
			t.Errorf("grace period = %v, want %v", cq.expiryGracePeriod, grace)
		}
	})

	t.Run("does not start the sweeper when the interval is zero", func(t *testing.T) {
		cq := newConnectionQueueWithOptions(3, nil, withExpirySweepInterval(0)).(*connectionQueue)

		select {
		case <-cq.expirySweeperDone:
		default:
			t.Fatal("sweeper done channel should be closed when the sweep interval is zero")
		}
		if cq.expiryGracePeriod != defaultExpiryGracePeriod {
			t.Errorf("grace period = %v, want default %v", cq.expiryGracePeriod, defaultExpiryGracePeriod)
		}
		if cq.maxConnections != 3 {
			t.Errorf("max connections = %d, want 3", cq.maxConnections)
		}
	})
}

func TestConnectionQueue_RegisterDeregister(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	owner := &stubTransport{tb: t}

	conn, res := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res.Code != StatusCodeOk {
		t.Fatalf("RegisterConnection returned error: %v", res)
	}
	if conn == nil {
		t.Fatal("RegisterConnection returned nil connection")
	}
	if conn.ID <= 0 {
		t.Fatal("RegisterConnection returned non-positive ID")
	}
	if conn.Updates == nil {
		t.Error("connection updates channel should be initialized")
	}
	if conn.Done == nil {
		t.Error("connection done channel should be initialized")
	}
	if cq.connectionCount() != 1 {
		t.Errorf("expected 1 connection, got %d", cq.connectionCount())
	}

	cq.deregisterConnection(conn.ID)
	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after deregister, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_RegisterConnection_InitialUpdate(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	owner := &stubTransport{tb: t}

	initialUpdate := &protos.PushUpdates{
		Slot: 0,
		Kind: &protos.PushUpdates_Value{
			Value: &protos.PushUpdates_PushValue{
				Oid: "initial/param",
			},
		},
	}

	conn, res := cq.registerOwnedConnection(owner, HandlerContext{}, initialUpdate)
	if res.Code != StatusCodeOk {
		t.Fatalf("RegisterConnection returned error: %v", res)
	}
	if conn == nil {
		t.Fatal("RegisterConnection returned nil connection")
	}
	if conn.ID <= 0 {
		t.Fatal("RegisterConnection returned non-positive ID")
	}

	select {
	case received := <-conn.Updates:
		pv, ok := received.Kind.(*protos.PushUpdates_Value)
		if !ok || pv.Value.GetOid() != "initial/param" {
			t.Errorf("expected OID 'initial/param' in initial update")
		}
	case <-time.After(time.Second):
		t.Error("did not receive initial update")
	}
}

func TestConnectionQueue_MaxConnections(t *testing.T) {
	cq := newConnectionQueue(2, nil)
	owner := &stubTransport{tb: t}

	conn1, res1 := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res1.Code != StatusCodeOk || conn1 == nil {
		t.Fatal("first connection should succeed")
	}

	conn2, res2 := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res2.Code != StatusCodeOk || conn2 == nil {
		t.Fatal("second connection should succeed")
	}

	conn3, res3 := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res3.Code != StatusCodeResourceExhausted || conn3 != nil {
		t.Error("third connection should be rejected (max 2)")
	}

	if cq.connectionCount() != 2 {
		t.Errorf("expected 2 connections, got %d", cq.connectionCount())
	}

	cq.deregisterConnection(conn1.ID)
	conn4, res4 := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res4.Code != StatusCodeOk || conn4 == nil {
		t.Error("should be able to connect after one disconnects")
	}
}

func TestConnectionQueue_RegisterOwnedConnection_ExpiredToken(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		cq := newConnectionQueue(0, nil).(*connectionQueue)
		owner := &stubTransport{tb: t}

		valid := HandlerContext{tokenExpiry: time.Now().Add(time.Hour)}
		conn, res := cq.registerOwnedConnection(owner, valid, nil)
		if res.Code != StatusCodeOk || conn == nil {
			t.Fatalf("unexpired token should register, got conn=%v res=%v", conn, res)
		}
		if got := cq.getConnectionExpiry(conn); !got.Equal(valid.tokenExpiry) {
			t.Errorf("expiry = %v, want %v", got, valid.tokenExpiry)
		}
	})

	t.Run("fail", func(t *testing.T) {
		cq := newConnectionQueue(0, nil)
		owner := &stubTransport{tb: t}

		expired := HandlerContext{tokenExpiry: time.Now().Add(-time.Second)}
		conn, res := cq.registerOwnedConnection(owner, expired, nil)
		if res.Code != StatusCodeUnauthenticated || conn != nil {
			t.Fatalf("expired token should be rejected, got conn=%v res=%v", conn, res)
		}
		if res.Error != "Token expired" {
			t.Errorf("error = %q, want %q", res.Error, "Token expired")
		}
		if cq.connectionCount() != 0 {
			t.Errorf("expected 0 connections after rejected expiry, got %d", cq.connectionCount())
		}
	})
}

func TestConnectionQueue_SetMaxConnections(t *testing.T) {
	cq := newConnectionQueue(1, nil)
	owner := &stubTransport{tb: t}

	cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	conn2, res2 := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res2.Code != StatusCodeResourceExhausted || conn2 != nil {
		t.Error("second connection should be rejected")
	}

	cq.setMaxConnections(2)
	conn3, res3 := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res3.Code != StatusCodeOk || conn3 == nil {
		t.Error("should succeed after increasing limit")
	}
}

func TestConnectionQueue_NotifyUpdate(t *testing.T) {
	t.Run("delivers to every connection", func(t *testing.T) {
		cq := newConnectionQueue(0, nil)

		owner := &stubTransport{tb: t}
		handlerContext := HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeOp: {}},
			authzEnabled: true,
		}

		conn1, _ := cq.registerOwnedConnection(owner, handlerContext, nil)
		conn2, _ := cq.registerOwnedConnection(owner, handlerContext, nil)

		update := &protos.PushUpdates{
			Slot: 0,
			Kind: &protos.PushUpdates_Value{
				Value: &protos.PushUpdates_PushValue{
					Oid: "test/param",
				},
			},
		}

		cq.notifyUpdate(update, st2138.ScopeOp)

		checkUpdate := func(name string, conn *Connection) {
			select {
			case received := <-conn.Updates:
				pv, ok := received.Kind.(*protos.PushUpdates_Value)
				if !ok || pv.Value.GetOid() != "test/param" {
					t.Errorf("%s: expected OID 'test/param'", name)
				}
			case <-time.After(time.Second):
				t.Errorf("%s: did not receive update", name)
			}
		}

		checkUpdate("conn1", conn1)
		checkUpdate("conn2", conn2)
	})

	t.Run("filters value updates by scope", func(t *testing.T) {
		cq := newConnectionQueue(0, nil)
		owner := &stubTransport{tb: t}

		matchingConn, _ := cq.registerOwnedConnection(owner, HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeMon: {}},
			authzEnabled: true,
		}, nil)
		matchingWriteConn, _ := cq.registerOwnedConnection(owner, HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeMon: {}},
			writeScopes:  map[string]struct{}{st2138.ScopeMon: {}},
			authzEnabled: true,
		}, nil)
		nonMatchingConn, _ := cq.registerOwnedConnection(owner, HandlerContext{
			readScopes:   map[string]struct{}{st2138.ScopeCfg: {}},
			authzEnabled: true,
		}, nil)

		update := &protos.PushUpdates{
			Slot: 0,
			Kind: &protos.PushUpdates_Value{
				Value: &protos.PushUpdates_PushValue{Oid: "test/param"},
			},
		}

		cq.notifyUpdate(update, st2138.ScopeMon)

		select {
		case <-matchingConn.Updates:
		case <-time.After(time.Second):
			t.Fatal("matching connection did not receive value update")
		}

		select {
		case <-matchingWriteConn.Updates:
		case <-time.After(time.Second):
			t.Fatal("matching write-scope connection did not receive value update")
		}

		select {
		case <-nonMatchingConn.Updates:
			t.Fatal("non-matching connection should not receive value update")
		default:
		}
	})

	t.Run("skips expired connection", func(t *testing.T) {
		now := time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC)
		cq := newConnectionQueueWithOptions(0, nil, withClock(func() time.Time { return now }))
		owner := &stubTransport{tb: t}

		expiredConnection, res := cq.registerOwnedConnection(owner, HandlerContext{tokenExpiry: now.Add(time.Minute)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("registerOwnedConnection: %v", res)
		}
		liveConnection, res := cq.registerOwnedConnection(owner, HandlerContext{tokenExpiry: now.Add(time.Hour)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("registerOwnedConnection: %v", res)
		}

		now = now.Add(2 * time.Minute) // the first connection is now expired, the second connection is still alive

		cq.notifyUpdate(&protos.PushUpdates{
			Slot: 0,
			Kind: &protos.PushUpdates_Value{Value: &protos.PushUpdates_PushValue{Oid: "p"}},
		}, "")

		if len(expiredConnection.Updates) != 0 {
			t.Error("expired connection must not receive the update")
		}
		if len(liveConnection.Updates) != 1 {
			t.Error("unexpired connection must still receive the update")
		}
	})
}

func TestIsValueUpdate_NilUpdate(t *testing.T) {
	if isValueUpdate(nil) {
		t.Error("expected nil update not to be treated as value update")
	}
}

func TestConnectionQueue_Shutdown(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	owner := &stubTransport{tb: t}

	conn1, _ := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	conn2, _ := cq.registerOwnedConnection(owner, HandlerContext{}, nil)

	go func() {
		<-conn1.Done
		cq.deregisterConnection(conn1.ID)
	}()

	go func() {
		<-conn2.Done
		cq.deregisterConnection(conn2.ID)
	}()

	cq.shutdown(context.Background())

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_Shutdown_RejectsNewConnections(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	owner := &stubTransport{tb: t}

	conn, _ := cq.registerOwnedConnection(owner, HandlerContext{}, nil)

	go func() {
		<-conn.Done
		cq.deregisterConnection(conn.ID)
	}()

	cq.shutdown(context.Background())

	conn, res := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
	if res.Code != StatusCodeInternal || conn != nil {
		t.Error("should reject new connections after shutdown")
	}
}

func TestConnectionQueue_Shutdown_StopsExpirySweeper(t *testing.T) {
	// shutdown waits for the sweeper to exit, so a sweeper that never stops would
	// hang it. Run it with a timeout so a leak fails the test instead.
	shutdownWithin := func(t *testing.T, cq *connectionQueue) {
		t.Helper()
		done := make(chan struct{})

		go func() {
			cq.shutdown(context.Background())
			close(done)
		}()

		select {
		case <-done:
		case <-time.After(2 * time.Second):
			t.Fatal("shutdown did not return, the sweeper may not have stopped")
		}
	}

	t.Run("sweeper exits when the queue shuts down", func(t *testing.T) {
		cq := newConnectionQueueWithOptions(0, nil, withExpirySweepInterval(5*time.Millisecond)).(*connectionQueue)

		select {
		case <-cq.expirySweeperDone:
			t.Fatal("sweeper should be running before shutdown")
		default:
		}

		shutdownWithin(t, cq)

		select {
		case <-cq.expirySweeperDone:
		default:
			t.Fatal("sweeper is still running after shutdown")
		}
	})
}

func TestConnectionQueue_ShutdownOwner(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	ownerA := &stubTransport{tb: t}
	ownerB := &stubTransport{tb: t}

	connA, _ := cq.registerOwnedConnection(ownerA, HandlerContext{}, nil)
	connB, _ := cq.registerOwnedConnection(ownerB, HandlerContext{}, nil)

	go func() {
		<-connA.Done
		cq.deregisterConnection(connA.ID)
	}()

	select {
	case <-connB.Done:
		t.Fatal("unexpected shutdown signal for unrelated owner")
	default:
	}

	cq.shutdownOwner(context.Background(), ownerA)

	if cq.connectionCount() != 1 {
		t.Fatalf("expected one remaining connection after owner shutdown, got %d", cq.connectionCount())
	}

	select {
	case <-connB.Done:
		t.Fatal("unexpected shutdown signal for unrelated owner")
	default:
	}

	cq.deregisterConnection(connB.ID)
}

func TestConnectionQueue_ShutdownConnection_AlreadyClosed(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	owner := &stubTransport{tb: t}

	conn, _ := cq.registerOwnedConnection(owner, HandlerContext{}, nil)

	go func() {
		<-conn.Done
		cq.deregisterConnection(conn.ID)
	}()

	close(conn.Done) // simulate connection already closed

	cq.shutdownConnection(context.Background(), conn)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConectionQueue_ShutdownConnection_Deadline(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	owner := &stubTransport{tb: t}

	conn, _ := cq.registerOwnedConnection(owner, HandlerContext{}, nil)

	// don't deregister

	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()
	cq.shutdownConnection(ctx, conn)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_ShutdownConnection_OneDeadline(t *testing.T) {
	cq := newConnectionQueue(0, nil)
	ownerA := &stubTransport{tb: t}
	ownerB := &stubTransport{tb: t}

	connA, _ := cq.registerOwnedConnection(ownerA, HandlerContext{}, nil)
	connB, _ := cq.registerOwnedConnection(ownerB, HandlerContext{}, nil)

	// only properly deregsiter connA, leave connB hanging

	go func() {
		<-connA.Done
		cq.deregisterConnection(connA.ID)
	}()

	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()
	cq.shutdownConnection(ctx, connA)
	cq.shutdownConnection(ctx, connB)

	if cq.connectionCount() != 0 {
		t.Errorf("expected 0 connections after shutdown, got %d", cq.connectionCount())
	}
}

func TestConnectionQueue_InjectedLogger(t *testing.T) {
	var buf bytes.Buffer
	log := slog.New(slog.NewJSONHandler(&buf, &slog.HandlerOptions{Level: slog.LevelInfo}))

	cq := newConnectionQueue(10, log)
	conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{}, nil)
	if res.Code != StatusCodeOk {
		t.Fatalf("registerOwnedConnection: %v", res)
	}
	cq.deregisterConnection(conn.ID)

	out := buf.String()
	if !strings.Contains(out, `"msg":"Streaming connection registered"`) {
		t.Fatalf("expected registration log, got %s", out)
	}
	if !strings.Contains(out, `"component":"connection-queue"`) {
		t.Fatalf("expected component=connection-queue, got %s", out)
	}
}

func TestConnectionQueue_LoggerSilent(t *testing.T) {
	var buf bytes.Buffer
	defaultLogger := slog.Default()
	slog.SetDefault(slog.New(slog.NewJSONHandler(&buf, &slog.HandlerOptions{Level: slog.LevelDebug})))
	t.Cleanup(func() { slog.SetDefault(defaultLogger) })
	cq := newConnectionQueue(10, nil)
	conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{}, nil)
	if res.Code != StatusCodeOk {
		t.Fatalf("registerOwnedConnection: %v", res)
	}
	cq.deregisterConnection(conn.ID)

	out := buf.String()
	if strings.Contains(out, `"msg":"Streaming connection registered"`) {
		t.Fatalf("expected no registration log, got %s", out)
	}
	if strings.Contains(out, `"component":"connection-queue"`) {
		t.Fatalf("expected no component=connection-queue, got %s", out)
	}
}

func TestConnectionCloseReason_String(t *testing.T) {
	t.Run("none", func(t *testing.T) {
		if got := CloseReasonNone.String(); got != "none" {
			t.Errorf("CloseReasonNone.String() = %q, want %q", got, "none")
		}
	})

	t.Run("shutdown", func(t *testing.T) {
		if got := CloseReasonShutdown.String(); got != "shutdown" {
			t.Errorf("CloseReasonShutdown.String() = %q, want %q", got, "shutdown")
		}
	})

	t.Run("expired", func(t *testing.T) {
		if got := CloseReasonExpired.String(); got != "expired" {
			t.Errorf("CloseReasonExpired.String() = %q, want %q", got, "expired")
		}
	})

	t.Run("unknown", func(t *testing.T) {
		if got := ConnectionCloseReason(99).String(); got != "unknown" {
			t.Errorf("ConnectionCloseReason(99).String() = %q, want %q", got, "unknown")
		}
	})
}

func TestConnection_RecordCloseReason(t *testing.T) {
	t.Run("records reason when not closing", func(t *testing.T) {
		conn := &Connection{}
		if !conn.recordCloseReason(CloseReasonShutdown) {
			t.Fatal("expected recordCloseReason to succeed")
		}
		if conn.CloseReason() != CloseReasonShutdown {
			t.Errorf("close reason = %s, want shutdown", conn.CloseReason())
		}
	})

	t.Run("rejects when already closing", func(t *testing.T) {
		conn := &Connection{closeReason: CloseReasonShutdown}
		if conn.recordCloseReason(CloseReasonExpired) {
			t.Fatal("expected markClosing to fail when a reason is already set")
		}
		if conn.CloseReason() != CloseReasonShutdown {
			t.Errorf("close reason = %s, want shutdown", conn.CloseReason())
		}
	})
}

func TestConnectionQueue_SetConnectionExpiryWithMutexHeld(t *testing.T) {
	t.Run("sets expiry on registered connection", func(t *testing.T) {
		cq := newConnectionQueue(0, nil).(*connectionQueue)
		conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("registerOwnedConnection: %v", res)
		}

		expiry := time.Now().Add(time.Hour).UTC()
		cq.mu.Lock()
		ok := cq.setConnectionExpiryWithMutexHeld(conn, expiry)
		cq.mu.Unlock()
		if !ok {
			t.Fatal("expected setConnectionExpiryWithMutexHeld to succeed")
		}
		if got := cq.getConnectionExpiry(conn); !got.Equal(expiry) {
			t.Errorf("expiry = %v, want %v", got, expiry)
		}
	})

	t.Run("rejects unregistered connection", func(t *testing.T) {
		cq := newConnectionQueue(0, nil).(*connectionQueue)
		conn := &Connection{ID: 1}
		expiry := time.Now().Add(time.Hour)

		cq.mu.Lock()
		ok := cq.setConnectionExpiryWithMutexHeld(conn, expiry)
		cq.mu.Unlock()
		if ok {
			t.Fatal("expected setConnectionExpiryWithMutexHeld to fail for an unregistered connection")
		}
		if got := cq.getConnectionExpiry(conn); !got.IsZero() {
			t.Errorf("expiry = %v, want zero", got)
		}
	})

	t.Run("rejects connection that is already closing", func(t *testing.T) {
		cq := newConnectionQueue(0, nil).(*connectionQueue)
		conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("registerOwnedConnection: %v", res)
		}
		if !conn.recordCloseReason(CloseReasonShutdown) {
			t.Fatal("expected recordCloseReason to succeed")
		}

		expiry := time.Now().Add(time.Hour)
		cq.mu.Lock()
		ok := cq.setConnectionExpiryWithMutexHeld(conn, expiry)
		cq.mu.Unlock()
		if ok {
			t.Fatal("expected setConnectionExpiryWithMutexHeld to fail for a closing connection")
		}
		if got := cq.getConnectionExpiry(conn); !got.IsZero() {
			t.Errorf("expiry = %v, want zero", got)
		}
	})
}

func TestConnectionQueue_SetConnectionExpiry(t *testing.T) {
	t.Run("sets expiry on registered connection", func(t *testing.T) {
		cq := newConnectionQueue(0, nil).(*connectionQueue)
		conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("registerOwnedConnection: %v", res)
		}

		expiry := time.Now().Add(time.Hour).UTC()
		if !cq.setConnectionExpiry(conn, expiry) {
			t.Fatal("expected setConnectionExpiry to succeed")
		}
		if got := cq.getConnectionExpiry(conn); !got.Equal(expiry) {
			t.Errorf("expiry = %v, want %v", got, expiry)
		}
	})

	t.Run("rejects unregistered connection", func(t *testing.T) {
		cq := newConnectionQueue(0, nil).(*connectionQueue)
		conn := &Connection{ID: 1}
		expiry := time.Now().Add(time.Hour)

		if cq.setConnectionExpiry(conn, expiry) {
			t.Fatal("expected setConnectionExpiry to fail for an unregistered connection")
		}
		if got := cq.getConnectionExpiry(conn); !got.IsZero() {
			t.Errorf("expiry = %v, want zero", got)
		}
	})
}

// manualClock is a connection-queue clock the sweeper can read while a test moves it forward.
type manualClock struct {
	mu  sync.Mutex
	now time.Time
}

func newManualClock(now time.Time) *manualClock {
	return &manualClock{now: now}
}

func (c *manualClock) Now() time.Time {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.now
}

func (c *manualClock) Advance(d time.Duration) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.now = c.now.Add(d)
}

func TestConnectionQueue_SweepExpiredConnections(t *testing.T) {
	waitForCount := func(t *testing.T, cq connectionQueueInterface, want int) {
		t.Helper()
		deadline := time.Now().Add(2 * time.Second)
		for time.Now().Before(deadline) {
			if cq.connectionCount() == want {
				return
			}
			time.Sleep(5 * time.Millisecond)
		}
		t.Fatalf("connection count = %d, want %d", cq.connectionCount(), want)
	}

	stopConnectionQueue := func(cq connectionQueueInterface) {
		t.Helper()
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		cq.shutdown(ctx)
	}

	t.Run("removes only expired connections", func(t *testing.T) {
		clock := newManualClock(time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC))
		cq := newConnectionQueueWithOptions(0, nil,
			withClock(clock.Now),
			withExpirySweepInterval(0),
			withExpiryGracePeriod(time.Second),
		).(*connectionQueue)
		t.Cleanup(func() { stopConnectionQueue(cq) })
		owner := &stubTransport{tb: t}

		expired, res := cq.registerOwnedConnection(owner, HandlerContext{tokenExpiry: clock.Now().Add(time.Minute)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("register expired connection: %v", res)
		}
		liveConnection, res := cq.registerOwnedConnection(owner, HandlerContext{tokenExpiry: clock.Now().Add(time.Hour)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("register live connection: %v", res)
		}
		permanentConnection, res := cq.registerOwnedConnection(owner, HandlerContext{}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("register connection without expiry: %v", res)
		}

		if got := cq.sweepExpiredConnections(); got != 0 {
			t.Fatalf("sweep before expiry = %d, want 0", got)
		}

		clock.Advance(2 * time.Minute)
		if got := cq.sweepExpiredConnections(); got != 1 {
			t.Fatalf("sweep = %d, want 1", got)
		}
		if got := cq.sweepExpiredConnections(); got != 0 {
			t.Fatalf("sweep of an already closing connection = %d, want 0", got)
		}

		go func() {
			<-expired.Done
			cq.deregisterConnection(expired.ID)
		}()
		waitForCount(t, cq, 2)

		if expired.CloseReason() != CloseReasonExpired {
			t.Errorf("expired close reason = %s, want expired", expired.CloseReason())
		}
		select {
		case <-liveConnection.Done:
			t.Fatal("unexpired connection was swept")
		default:
		}
		select {
		case <-permanentConnection.Done:
			t.Fatal("connection without expiry was swept")
		default:
		}
		cq.deregisterConnection(liveConnection.ID)
		cq.deregisterConnection(permanentConnection.ID)
	})

	t.Run("skips connections while the queue is shutting down", func(t *testing.T) {
		clock := newManualClock(time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC))
		cq := newConnectionQueueWithOptions(0, nil,
			withClock(clock.Now),
			withExpirySweepInterval(0),
		).(*connectionQueue)
		t.Cleanup(func() { stopConnectionQueue(cq) })

		conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{tokenExpiry: clock.Now().Add(time.Minute)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("registerOwnedConnection: %v", res)
		}

		clock.Advance(2 * time.Minute)

		// Hold the connection in the queue until the sweep has run. shutdown blocks until then.
		release := make(chan struct{})
		go func() {
			<-conn.Done
			<-release
			cq.deregisterConnection(conn.ID)
		}()

		shutdownDone := make(chan struct{})
		go func() {
			cq.shutdown(context.Background())
			close(shutdownDone)
		}()

		select {
		case <-conn.Done:
		case <-time.After(time.Second):
			t.Fatal("shutdown did not signal the connection")
		}

		if got := cq.sweepExpiredConnections(); got != 0 {
			t.Fatalf("sweep during shutdown = %d, want 0", got)
		}
		if cq.connectionCount() != 1 {
			t.Fatalf("connection count = %d, want 1", cq.connectionCount())
		}
		if conn.CloseReason() != CloseReasonShutdown {
			t.Errorf("close reason = %s, want shutdown", conn.CloseReason())
		}

		close(release)
		select {
		case <-shutdownDone:
		case <-time.After(time.Second):
			t.Fatal("shutdown did not finish after the connection deregistered")
		}
		if cq.connectionCount() != 0 {
			t.Fatalf("connection count = %d, want 0", cq.connectionCount())
		}
	})

	t.Run("sweeps expired connections from the ticker loop", func(t *testing.T) {
		clock := newManualClock(time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC))
		cq := newConnectionQueueWithOptions(0, nil,
			withClock(clock.Now),
			withExpirySweepInterval(10*time.Millisecond),
		).(*connectionQueue)
		t.Cleanup(func() { stopConnectionQueue(cq) })
		owner := &stubTransport{tb: t}

		expired, res := cq.registerOwnedConnection(owner, HandlerContext{tokenExpiry: clock.Now().Add(time.Minute)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("register expired connection: %v", res)
		}
		liveConnection, res := cq.registerOwnedConnection(owner, HandlerContext{tokenExpiry: clock.Now().Add(time.Hour)}, nil)
		if res.Code != StatusCodeOk {
			t.Fatalf("register live connection: %v", res)
		}

		go func() {
			<-expired.Done
			cq.deregisterConnection(expired.ID)
		}()

		clock.Advance(2 * time.Minute)
		waitForCount(t, cq, 1)

		if expired.CloseReason() != CloseReasonExpired {
			t.Errorf("expired close reason = %s, want expired", expired.CloseReason())
		}

		select {
		case <-liveConnection.Done:
			t.Fatal("unexpired connection was swept")
		default:
		}

		cq.deregisterConnection(liveConnection.ID)
	})

	t.Run("sweep honours newly set connection expiry", func(t *testing.T) {
		t.Run("token expiry is shortened", func(t *testing.T) {
			clock := newManualClock(time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC))
			cq := newConnectionQueueWithOptions(0, nil,
				withClock(clock.Now),
				withExpirySweepInterval(10*time.Millisecond),
			).(*connectionQueue)
			t.Cleanup(func() { stopConnectionQueue(cq) })

			conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{tokenExpiry: clock.Now().Add(time.Hour)}, nil)
			if res.Code != StatusCodeOk {
				t.Fatalf("register connection: %v", res)
			}
			if !cq.setConnectionExpiry(conn, clock.Now().Add(time.Minute)) {
				t.Fatal("expected setConnectionExpiry to shorten the connection")
			}

			go func() {
				<-conn.Done
				cq.deregisterConnection(conn.ID)
			}()

			// Past the shortened expiry, and still before the original one.
			clock.Advance(2 * time.Minute)
			waitForCount(t, cq, 0)

			if conn.CloseReason() != CloseReasonExpired {
				t.Errorf("close reason = %s, want expired", conn.CloseReason())
			}

		})

		t.Run("token expiry is extended", func(t *testing.T) {
			clock := newManualClock(time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC))
			cq := newConnectionQueueWithOptions(0, nil,
				withClock(clock.Now),
				withExpirySweepInterval(10*time.Millisecond),
			).(*connectionQueue)
			t.Cleanup(func() { stopConnectionQueue(cq) })

			conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{tokenExpiry: clock.Now().Add(time.Minute)}, nil)
			if res.Code != StatusCodeOk {
				t.Fatalf("register connection: %v", res)
			}

			if !cq.setConnectionExpiry(conn, clock.Now().Add(time.Hour)) {
				t.Fatal("expected setConnectionExpiry to extend the connection")
			}

			go func() {
				<-conn.Done
				cq.deregisterConnection(conn.ID)
			}()

			clock.Advance(2 * time.Minute)

			if got := cq.sweepExpiredConnections(); got != 0 {
				t.Fatalf("sweep at the original expiry = %d, want 0", got)
			}

			select {
			case <-conn.Done:
				t.Fatal("connection with an extended expiry was swept at its original expiry")
			default:
			}

			clock.Advance(time.Hour)
			waitForCount(t, cq, 0)

			if conn.CloseReason() != CloseReasonExpired {
				t.Errorf("close reason = %s, want expired", conn.CloseReason())
			}
		})
	})

	// The tests below are meant to be run with -race:
	// go test -race -count=1 -v -run 'TestConnectionQueue_SweepExpiredConnections/Test_Race_Conditions' ./pkg/catena/
	t.Run("Test Race Conditions", func(t *testing.T) {
		raceStart := time.Date(2026, time.November, 11, 12, 0, 0, 0, time.UTC)

		// runConcurrently releases every function at the same moment and waits for all of them.
		runConcurrently := func(funcs ...func()) {
			var ready, done sync.WaitGroup
			start := make(chan struct{})
			ready.Add(len(funcs))
			done.Add(len(funcs))

			for _, fn := range funcs {
				go func(fn func()) {
					defer done.Done()
					ready.Done()
					<-start
					fn()
				}(fn)
			}

			ready.Wait()
			close(start)
			done.Wait()
		}

		// waitForCleanup waits for every shutdown goroutine spawned by the sweep.
		waitForCleanup := func(t *testing.T, cq *connectionQueue) {
			t.Helper()

			finished := make(chan struct{})

			go func() {
				cq.expiryCleanupWg.Wait()
				close(finished)
			}()

			select {
			case <-finished:
			case <-time.After(2 * time.Second):
				t.Fatal("expiry cleanup goroutines did not finish")
			}
		}

		readState := func(cq *connectionQueue, conn *Connection) (ConnectionCloseReason, time.Time) {
			cq.mu.Lock()
			defer cq.mu.Unlock()
			return conn.closeReason, conn.expiry
		}

		// The client disconnects (deregisters) at the same time a sweep
		// marks the connection to be expired.
		t.Run("deregister racing with the sweep", func(t *testing.T) {
			for i := 0; i < 200; i++ {
				clock := newManualClock(raceStart)
				cq := newConnectionQueueWithOptions(0, nil,
					withClock(clock.Now),
					withExpirySweepInterval(0),
				).(*connectionQueue)

				var releases atomic.Int32
				conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{
					tokenExpiry: clock.Now().Add(time.Minute),
					ctxCancel:   func() { releases.Add(1) },
				}, nil)
				if res.Code != StatusCodeOk {
					t.Fatalf("iteration %d: register: %v", i, res)
				}
				clock.Advance(2 * time.Minute)

				swept := -1
				runConcurrently(
					func() { swept = cq.sweepExpiredConnections() },
					func() { cq.deregisterConnection(conn.ID) },
				)

				if swept != 0 && swept != 1 {
					t.Fatalf("iteration %d: sweep = %d, want 0 or 1", i, swept)
				}
				waitForCleanup(t, cq)

				if cq.connectionCount() != 0 {
					t.Fatalf("iteration %d: connection count = %d, want 0", i, cq.connectionCount())
				}
				if got := releases.Load(); got != 1 {
					t.Fatalf("iteration %d: connection released %d times, want 1", i, got)
				}
			}
		})

		// A token refresh extends the expiry while the sweep is running
		t.Run("setConnectionExpiry racing with the sweep", func(t *testing.T) {
			for i := 0; i < 200; i++ {
				clock := newManualClock(raceStart)
				cq := newConnectionQueueWithOptions(0, nil,
					withClock(clock.Now),
					withExpirySweepInterval(0),
				).(*connectionQueue)

				conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{tokenExpiry: clock.Now().Add(time.Minute)}, nil)
				if res.Code != StatusCodeOk {
					t.Fatalf("iteration %d: register: %v", i, res)
				}
				clock.Advance(2 * time.Minute)
				newExpiry := clock.Now().Add(time.Hour)

				var swept int
				var extended bool
				runConcurrently(
					func() { swept = cq.sweepExpiredConnections() },
					func() { extended = cq.setConnectionExpiry(conn, newExpiry) },
				)

				reason, expiry := readState(cq, conn)
				if extended {
					if swept != 0 || reason != CloseReasonNone {
						t.Fatalf("iteration %d: refresh accepted but swept=%d reason=%s", i, swept, reason)
					}
					if !expiry.Equal(newExpiry) {
						t.Fatalf("iteration %d: expiry = %v, want %v", i, expiry, newExpiry)
					}

					select {
					case <-conn.Done:
						t.Fatalf("iteration %d: connection with an accepted refresh was closed", i)
					default:
					}

					cq.deregisterConnection(conn.ID)
				} else {
					if swept != 1 || reason != CloseReasonExpired {
						t.Fatalf("iteration %d: refresh rejected but swept=%d reason=%s", i, swept, reason)
					}

					go func() {
						<-conn.Done
						cq.deregisterConnection(conn.ID)
					}()
				}

				waitForCleanup(t, cq)

				if cq.connectionCount() != 0 {
					t.Fatalf("iteration %d: connection count = %d, want 0", i, cq.connectionCount())
				}
			}
		})

		// The queue is shut down while the sweep loop is expiring the connection
		t.Run("shutdown racing with the sweep loop", func(t *testing.T) {
			for i := 0; i < 100; i++ {
				clock := newManualClock(raceStart)
				cq := newConnectionQueueWithOptions(0, nil,
					withClock(clock.Now),
					withExpirySweepInterval(time.Millisecond),
					withExpiryGracePeriod(time.Second),
				).(*connectionQueue)

				conn, res := cq.registerOwnedConnection(&stubTransport{tb: t}, HandlerContext{tokenExpiry: clock.Now().Add(time.Minute)}, nil)
				if res.Code != StatusCodeOk {
					t.Fatalf("iteration %d: register: %v", i, res)
				}

				go func() {
					<-conn.Done
					cq.deregisterConnection(conn.ID)
				}()

				shutdownDone := make(chan struct{})
				var raced sync.WaitGroup
				raced.Add(2)
				start := make(chan struct{})

				// The clock advanced before the shutdown to expire the connection
				// This is detected by the sweep loop that is running concurrently in the background
				go func() {
					defer raced.Done()
					<-start
					clock.Advance(2 * time.Minute)
				}()

				// The shutdown races with the sweep loop that is running
				go func() {
					defer raced.Done()
					<-start

					// Vary the delay so shutdown lands before, during and after a sweep tick.
					time.Sleep(time.Duration(i%10) * 200 * time.Microsecond)

					cq.shutdown(context.Background())
					close(shutdownDone)
				}()
				close(start)

				select {
				case <-shutdownDone:
				case <-time.After(2 * time.Second):
					t.Fatalf("iteration %d: shutdown did not return", i)
				}
				raced.Wait()

				reason, _ := readState(cq, conn)
				if reason != CloseReasonExpired && reason != CloseReasonShutdown {
					t.Fatalf("iteration %d: close reason = %s, want expired or shutdown", i, reason)
				}

				select {
				case <-conn.Done:
				default:
					t.Fatalf("iteration %d: Done was not closed", i)
				}
				if cq.connectionCount() != 0 {
					t.Fatalf("iteration %d: connection count = %d, want 0", i, cq.connectionCount())
				}

			}
		})

		t.Run("shutdownOwner racing with the sweep", func(t *testing.T) {
			for i := 0; i < 200; i++ {
				clock := newManualClock(raceStart)
				cq := newConnectionQueueWithOptions(0, nil,
					withClock(clock.Now),
					withExpirySweepInterval(0),
				).(*connectionQueue)

				ownerA := &stubTransport{tb: t}
				ownerB := &stubTransport{tb: t}

				var releases atomic.Int32
				connA, res := cq.registerOwnedConnection(ownerA, HandlerContext{
					tokenExpiry: clock.Now().Add(time.Minute),
					ctxCancel:   func() { releases.Add(1) },
				}, nil)
				if res.Code != StatusCodeOk {
					t.Fatalf("iteration %d: register owner A: %v", i, res)
				}
				connB, res := cq.registerOwnedConnection(ownerB, HandlerContext{}, nil)
				if res.Code != StatusCodeOk {
					t.Fatalf("iteration %d: register owner B: %v", i, res)
				}

				go func() {
					<-connA.Done
					cq.deregisterConnection(connA.ID)
				}()

				clock.Advance(2 * time.Minute)

				runConcurrently(
					func() { cq.sweepExpiredConnections() },
					func() { cq.shutdownOwner(context.Background(), ownerA) },
				)
				waitForCleanup(t, cq)

				reasonA, _ := readState(cq, connA)
				if reasonA != CloseReasonExpired && reasonA != CloseReasonShutdown {
					t.Fatalf("iteration %d: close reason = %s, want expired or shutdown", i, reasonA)
				}
				select {
				case <-connA.Done:
				default:
					t.Fatalf("iteration %d: Done was not closed", i)
				}
				if got := releases.Load(); got != 1 {
					t.Fatalf("iteration %d: connection released %d times, want 1", i, got)
				}

				// A seperately owned connection is left as is
				if cq.connectionCount() != 1 {
					t.Fatalf("iteration %d: connection count = %d, want 1 (owner B)", i, cq.connectionCount())
				}
				if reasonB, _ := readState(cq, connB); reasonB != CloseReasonNone {
					t.Fatalf("iteration %d: owner B close reason = %s, want none", i, reasonB)
				}
				select {
				case <-connB.Done:
					t.Fatalf("iteration %d: owner B connection was closed", i)
				default:
				}

				cq.deregisterConnection(connB.ID)
			}
		})
	})
}
