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
 * @brief Helper functions test for the proto builders.
 * @file helpers_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-10-26
 */

package st2138

import (
	"errors"
	"testing"
)

type stubBuilder struct {
	value string
	err   error
	calls int
}

func (b *stubBuilder) build() (string, error) {
	b.calls++
	if b.err != nil {
		return "", b.err
	}
	return b.value, nil
}

func TestBuildChild(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		var parentErr error
		child := &stubBuilder{value: "built"}
		got, ok := buildChild(&parentErr, child, (*stubBuilder).build, "WithStub")
		if !ok {
			t.Fatal("expected ok")
		}
		if got != "built" {
			t.Errorf("expected 'built', got %q", got)
		}
		if parentErr != nil {
			t.Errorf("expected no parent error, got %v", parentErr)
		}
	})

	t.Run("parent error short-circuits", func(t *testing.T) {
		parentErr := errors.New("parent failed")
		child := &stubBuilder{value: "built"}
		got, ok := buildChild(&parentErr, child, (*stubBuilder).build, "WithStub")
		if ok {
			t.Fatal("expected not ok")
		}
		if got != "" {
			t.Errorf("expected zero value, got %q", got)
		}
		if parentErr.Error() != "parent failed" {
			t.Errorf("expected parent error unchanged, got %v", parentErr)
		}
		if child.calls != 0 {
			t.Errorf("expected child not built, got %d calls", child.calls)
		}
	})

	t.Run("nil child", func(t *testing.T) {
		var parentErr error
		got, ok := buildChild(&parentErr, (*stubBuilder)(nil), (*stubBuilder).build, "WithStub")
		if ok {
			t.Fatal("expected not ok")
		}
		if got != "" {
			t.Errorf("expected zero value, got %q", got)
		}
		if parentErr == nil || parentErr.Error() != "WithStub: child cannot be nil" {
			t.Errorf("expected nil child error, got %v", parentErr)
		}
	})

	t.Run("child error propagates", func(t *testing.T) {
		var parentErr error
		childErr := errors.New("child failed")
		child := &stubBuilder{err: childErr}
		got, ok := buildChild(&parentErr, child, (*stubBuilder).build, "WithStub")
		if ok {
			t.Fatal("expected not ok")
		}
		if got != "" {
			t.Errorf("expected zero value, got %q", got)
		}
		if !errors.Is(parentErr, childErr) {
			t.Errorf("expected child error on parent, got %v", parentErr)
		}
	})
}

func TestPut(t *testing.T) {
	m := make(map[string]string)
	put(&m, "key", "value")
	if m["key"] != "value" {
		t.Errorf("expected value to be 'value', got %s", m["key"])
	}
}

func TestPut_NilMap(t *testing.T) {
	var m map[string]string
	put(&m, "key", "value")
	if m["key"] != "value" {
		t.Errorf("expected value to be 'value', got %s", m["key"])
	}
}
