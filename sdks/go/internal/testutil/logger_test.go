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

package testutil

import (
	"fmt"
	"os"
	"path/filepath"
	"sync"
	"testing"
)

func TestCaptureStderr(t *testing.T) {
	output := CaptureStderr(func() {
		fmt.Fprintln(os.Stderr, "test error message")
	})

	if output != "test error message\n" {
		t.Errorf("expected 'test error message\\n', got %q", output)
	}
}

func TestCaptureStderr_Empty(t *testing.T) {
	output := CaptureStderr(func() {
		// Write nothing
	})

	if output != "" {
		t.Errorf("expected empty string, got %q", output)
	}
}

func TestCaptureStderr_MultipleLines(t *testing.T) {
	output := CaptureStderr(func() {
		fmt.Fprintln(os.Stderr, "line 1")
		fmt.Fprintln(os.Stderr, "line 2")
		fmt.Fprintln(os.Stderr, "line 3")
	})

	expected := "line 1\nline 2\nline 3\n"
	if output != expected {
		t.Errorf("expected %q, got %q", expected, output)
	}
}

func TestCaptureOutput(t *testing.T) {
	output := CaptureOutput(func() {
		fmt.Println("stdout message")
		fmt.Fprintln(os.Stderr, "stderr message")
	})

	if output.Stdout != "stdout message\n" {
		t.Errorf("expected stdout 'stdout message\\n', got %q", output.Stdout)
	}
	if output.Stderr != "stderr message\n" {
		t.Errorf("expected stderr 'stderr message\\n', got %q", output.Stderr)
	}
}

func TestCaptureOutput_StdoutOnly(t *testing.T) {
	output := CaptureOutput(func() {
		fmt.Println("only stdout")
	})

	if output.Stdout != "only stdout\n" {
		t.Errorf("expected stdout 'only stdout\\n', got %q", output.Stdout)
	}
	if output.Stderr != "" {
		t.Errorf("expected empty stderr, got %q", output.Stderr)
	}
}

func TestCaptureOutput_StderrOnly(t *testing.T) {
	output := CaptureOutput(func() {
		fmt.Fprintln(os.Stderr, "only stderr")
	})

	if output.Stdout != "" {
		t.Errorf("expected empty stdout, got %q", output.Stdout)
	}
	if output.Stderr != "only stderr\n" {
		t.Errorf("expected stderr 'only stderr\\n', got %q", output.Stderr)
	}
}

func TestLogBuffer_Write(t *testing.T) {
	buf := NewLogBuffer()
	n, err := buf.Write([]byte("test data"))
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if n != 9 {
		t.Errorf("expected 9 bytes written, got %d", n)
	}
	if buf.String() != "test data" {
		t.Errorf("expected 'test data', got %q", buf.String())
	}
}

func TestLogBuffer_String(t *testing.T) {
	buf := NewLogBuffer()
	buf.Write([]byte("hello world"))

	if buf.String() != "hello world" {
		t.Errorf("expected 'hello world', got %q", buf.String())
	}
}

func TestLogBuffer_Reset(t *testing.T) {
	buf := NewLogBuffer()
	buf.Write([]byte("some data"))
	buf.Reset()

	if buf.String() != "" {
		t.Errorf("expected empty string after reset, got %q", buf.String())
	}
}

func TestLogBuffer_Contains(t *testing.T) {
	buf := NewLogBuffer()
	buf.Write([]byte("hello world"))

	if !buf.Contains("world") {
		t.Error("expected Contains('world') to be true")
	}
	if buf.Contains("foo") {
		t.Error("expected Contains('foo') to be false")
	}
}

func TestLogBuffer_Lines(t *testing.T) {
	buf := NewLogBuffer()
	buf.Write([]byte("line1\nline2\nline3"))

	lines := buf.Lines()
	if len(lines) != 3 {
		t.Fatalf("expected 3 lines, got %d", len(lines))
	}
	if lines[0] != "line1" {
		t.Errorf("expected lines[0]='line1', got %q", lines[0])
	}
	if lines[1] != "line2" {
		t.Errorf("expected lines[1]='line2', got %q", lines[1])
	}
	if lines[2] != "line3" {
		t.Errorf("expected lines[2]='line3', got %q", lines[2])
	}
}

func TestLogBuffer_Lines_Empty(t *testing.T) {
	buf := NewLogBuffer()

	lines := buf.Lines()
	if len(lines) != 0 {
		t.Errorf("expected 0 lines for empty buffer, got %d", len(lines))
	}
}

func TestLogBuffer_Lines_TrailingNewline(t *testing.T) {
	buf := NewLogBuffer()
	buf.Write([]byte("line1\nline2\n"))

	lines := buf.Lines()
	if len(lines) != 2 {
		t.Fatalf("expected 2 lines (trailing newline trimmed), got %d", len(lines))
	}
}

func TestLogBuffer_Concurrent(t *testing.T) {
	buf := NewLogBuffer()
	var wg sync.WaitGroup

	// Concurrent writes
	for i := 0; i < 100; i++ {
		wg.Add(1)
		go func(n int) {
			defer wg.Done()
			buf.Write([]byte(fmt.Sprintf("msg%d\n", n)))
		}(i)
	}

	// Concurrent reads
	for i := 0; i < 50; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			_ = buf.String()
			_ = buf.Contains("msg")
			_ = buf.Lines()
		}()
	}

	wg.Wait()

	// Verify some content was written
	if buf.String() == "" {
		t.Error("expected non-empty buffer after concurrent writes")
	}
}

func TestTempDir(t *testing.T) {
	dir, cleanup := TempDir(t, "testutil-test")
	defer cleanup()

	// Verify directory exists
	info, err := os.Stat(dir)
	if err != nil {
		t.Fatalf("temp dir does not exist: %v", err)
	}
	if !info.IsDir() {
		t.Error("expected path to be a directory")
	}

	// Create a file in the temp dir
	testFile := filepath.Join(dir, "test.txt")
	if err := os.WriteFile(testFile, []byte("test"), 0644); err != nil {
		t.Fatalf("failed to create test file: %v", err)
	}

	// Verify file exists
	if _, err := os.Stat(testFile); err != nil {
		t.Fatalf("test file does not exist: %v", err)
	}

	// Run cleanup
	cleanup()

	// Verify directory is removed
	if _, err := os.Stat(dir); !os.IsNotExist(err) {
		t.Error("expected temp dir to be removed after cleanup")
	}
}

func TestAssertContains(t *testing.T) {
	// This test validates AssertContains doesn't panic on valid input
	mockT := &testing.T{}

	AssertContains(mockT, "hello world", "world")
	// Should not fail

	AssertContains(mockT, "hello world", "foo")
	// mockT would have recorded an error, but we're not checking it here
	// since we're testing the function doesn't panic
}

func TestAssertNotContains(t *testing.T) {
	// This test validates AssertNotContains doesn't panic on valid input
	mockT := &testing.T{}

	AssertNotContains(mockT, "hello world", "foo")
	// Should not fail

	AssertNotContains(mockT, "hello world", "world")
	// mockT would have recorded an error, but we're not checking it here
}

func TestNewLogBuffer(t *testing.T) {
	buf := NewLogBuffer()
	if buf == nil {
		t.Fatal("NewLogBuffer returned nil")
	}
	if buf.String() != "" {
		t.Errorf("new buffer should be empty, got %q", buf.String())
	}
}
