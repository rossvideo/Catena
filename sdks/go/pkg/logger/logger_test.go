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
 * @brief Test utilities for the Catena Go SDK.
 * @file logger_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-09-25
 */

package logger

import (
	"bytes"
	"context"
	"errors"
	"io"
	"log/slog"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

func TestNew(t *testing.T) {
	t.Run("creates log directory", func(t *testing.T) {
		dir := t.TempDir()
		logDir := filepath.Join(dir, "logs")
		_, closeFn, err := New(config.LoggerOptions{
			AppName:        "test",
			LogDir:         logDir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		defer closeFn()

		if _, err := os.Stat(logDir); os.IsNotExist(err) {
			t.Error("log directory was not created")
		}
	})

	t.Run("creates log file with timestamp", func(t *testing.T) {
		dir := t.TempDir()
		_, closeFn, err := New(config.LoggerOptions{
			AppName:        "myapp",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		defer closeFn()

		// Check that a log file was created
		entries, err := os.ReadDir(dir)
		if err != nil {
			t.Fatalf("failed to read dir: %v", err)
		}
		if len(entries) != 1 {
			t.Fatalf("expected 1 file, got %d", len(entries))
		}

		filename := entries[0].Name()
		if !strings.HasSuffix(filename, "_myapp.log") {
			t.Errorf("expected filename to end with _myapp.log, got %s", filename)
		}
	})

	t.Run("handles invalid log directory", func(t *testing.T) {
		if os.Geteuid() == 0 {
			t.Skip("skipping permission error test when running as root")
		}

		// Try to create a log directory in a non-existent parent with no permissions
		_, _, err := New(config.LoggerOptions{
			AppName:        "test",
			LogDir:         "/nonexistent/path/that/should/fail/logs",
			WriteToFile:    true,
			WriteToConsole: false,
		})
		if err == nil {
			t.Error("expected error for invalid log directory")
		}
	})

	t.Run("does not mutate slog.Default", func(t *testing.T) {
		logDefault := slog.Default()
		dir := t.TempDir()
		_, closeFn, err := New(config.LoggerOptions{
			AppName:        "default-check",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		defer closeFn()

		if slog.Default() != logDefault {
			t.Error("New must not call slog.SetDefault")
		}
	})

	t.Run("allows multiple independent loggers", func(t *testing.T) {
		dir := t.TempDir()
		first, closeFirst, err := New(config.LoggerOptions{
			AppName:        "first",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("first New failed: %v", err)
		}
		defer closeFirst()

		second, closeSecond, err := New(config.LoggerOptions{
			AppName:        "second",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelDebug,
		})
		if err != nil {
			t.Fatalf("second New failed: %v", err)
		}
		defer closeSecond()

		if first == nil || second == nil {
			t.Fatal("expected both loggers to be non-nil")
		}
		entries, err := os.ReadDir(dir)
		if err != nil {
			t.Fatalf("failed to read dir: %v", err)
		}
		if len(entries) != 2 {
			t.Errorf("expected 2 files, got %d", len(entries))
		}
	})

	t.Run("Fallback handler when no outputs are configured", func(t *testing.T) {
		log, closeFn, err := New(config.LoggerOptions{})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		if closeFn == nil {
			t.Fatal("CloseFunc must be non-nil")
		}
		// Should not panic
		log.Info("discarded")
		closeFn()
	})
}

func TestCloseFunc(t *testing.T) {
	t.Run("closes the file opened by New", func(t *testing.T) {
		dir := t.TempDir()
		log, closeFn, err := New(config.LoggerOptions{
			AppName:        "close",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}

		log.Info("before close")
		closeFn()

		entries, err := os.ReadDir(dir)
		if err != nil {
			t.Fatalf("failed to read dir: %v", err)
		}
		if len(entries) != 1 {
			t.Fatalf("expected 1 file, got %d", len(entries))
		}
		path := filepath.Join(dir, entries[0].Name())

		before, err := os.ReadFile(path)
		if err != nil {
			t.Fatalf("failed to read log file: %v", err)
		}
		if !strings.Contains(string(before), "before close") {
			t.Fatalf("expected log line before close, got %s", before)
		}

		log.Info("after close")
		after, err := os.ReadFile(path)
		if err != nil {
			t.Fatalf("failed to read log file: %v", err)
		}
		if strings.Contains(string(after), "after close") {
			t.Fatal("CloseFunc should close the file so later records are not written")
		}
	})

	t.Run("second call is a no-op", func(t *testing.T) {
		dir := t.TempDir()
		_, closeFn, err := New(config.LoggerOptions{
			AppName:        "close-twice",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		closeFn()
		closeFn() // must not panic
	})

	t.Run("silent logger close is a no-op", func(t *testing.T) {
		_, closeFn, err := New(config.LoggerOptions{Silent: true})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		if closeFn == nil {
			t.Fatal("CloseFunc must be non-nil")
		}
		closeFn()
	})
}

func TestLogToFile(t *testing.T) {
	dir := t.TempDir()

	log, closeFn, err := New(config.LoggerOptions{
		AppName:        "filetest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelInfo,
	})
	if err != nil {
		t.Fatalf("New failed: %v", err)
	}

	log.Info("test log message to file")
	closeFn()

	// Find and read the log file
	entries, err := os.ReadDir(dir)
	if err != nil {
		t.Fatalf("failed to read dir: %v", err)
	}
	if len(entries) != 1 {
		t.Fatalf("expected 1 file, got %d", len(entries))
	}

	content, err := os.ReadFile(filepath.Join(dir, entries[0].Name()))
	if err != nil {
		t.Fatalf("failed to read log file: %v", err)
	}

	if !strings.Contains(string(content), "test log message to file") {
		t.Errorf("expected %q to contain %q", string(content), "test log message to file")
	}
}

func TestLogToConsole(t *testing.T) {
	dir := t.TempDir()
	stderr := captureStderr(t, func() {
		log, closeFn, err := New(config.LoggerOptions{
			AppName:        "console-test",
			LogDir:         dir,
			WriteToFile:    false,
			WriteToConsole: true,
			Level:          LevelDebug,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		log.With("component", "rest-transport").Info("listening", "port", 9080)
		closeFn()
	})

	expectedOutput := ansiGreen + "[INFO]" + ansiReset + ": listening component=rest-transport port=9080"
	if !strings.Contains(stderr, expectedOutput) {
		t.Fatalf("console output = %q, want %q", stderr, expectedOutput)
	}
}

func TestSilentMode(t *testing.T) {
	dir := t.TempDir()

	log, closeFn, err := New(config.LoggerOptions{
		AppName:        "silenttest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Silent:         true,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("New failed: %v", err)
	}
	defer closeFn()

	log.Debug("debug message")
	log.Info("info message")
	log.Warn("warning message")
	log.Error("error message")

	// Check that log file is empty or doesn't exist
	entries, err := os.ReadDir(dir)
	if err != nil {
		t.Fatalf("failed to read dir: %v", err)
	}
	// In silent mode, no log file should be created since WriteToFile is
	// effectively disabled by the silent setting
	// (The implementation discards all logs in silent mode)
	if len(entries) > 0 {
		content, err := os.ReadFile(filepath.Join(dir, entries[0].Name()))
		if err == nil && len(content) > 0 {
			t.Errorf("expected no logs in silent mode, but got: %s", string(content))
		}
	}
}

func TestJSONOutput(t *testing.T) {
	dir := t.TempDir()

	log, closeFn, err := New(config.LoggerOptions{
		AppName:        "jsontest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelInfo,
		UseJSON:        true,
	})
	if err != nil {
		t.Fatalf("New failed: %v", err)
	}

	log.Info("json log message")
	closeFn()

	// Check the log file contains JSON
	entries, err := os.ReadDir(dir)
	if err != nil {
		t.Fatalf("failed to read dir: %v", err)
	}
	if len(entries) != 1 {
		t.Fatalf("expected 1 file, got %d", len(entries))
	}

	content, err := os.ReadFile(filepath.Join(dir, entries[0].Name()))
	if err != nil {
		t.Fatalf("failed to read log file: %v", err)
	}

	// JSON output should contain curly braces
	if !strings.Contains(string(content), "{") {
		t.Errorf("expected JSON output, got: %s", string(content))
	}
}

func TestConsoleJSONOutput(t *testing.T) {
	stderr := captureStderr(t, func() {
		log, closeFn, err := New(config.LoggerOptions{
			WriteToConsole: true,
			WriteToFile:    false,
			UseJSON:        true,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("New failed: %v", err)
		}
		log.Info("json console")
		closeFn()
	})
	if !strings.Contains(stderr, `"msg":"json console"`) {
		t.Fatalf("console JSON output = %q", stderr)
	}
}

func TestLoggerLevelConstants(t *testing.T) {
	// Verify level constants match slog levels
	if LevelDebug != slog.LevelDebug {
		t.Errorf("LevelDebug = %d, want %d", LevelDebug, slog.LevelDebug)
	}
	if LevelInfo != slog.LevelInfo {
		t.Errorf("LevelInfo = %d, want %d", LevelInfo, slog.LevelInfo)
	}
	if LevelWarning != slog.LevelWarn {
		t.Errorf("LevelWarning = %d, want %d", LevelWarning, slog.LevelWarn)
	}
	if LevelError != slog.LevelError {
		t.Errorf("LevelError = %d, want %d", LevelError, slog.LevelError)
	}
}

func TestCatenaTextHandler_Enabled(t *testing.T) {
	ctx := context.Background()

	t.Run("nil options default to log level info", func(t *testing.T) {
		h := newCatenaTextHandler(&bytes.Buffer{}, nil, false)
		if h.Enabled(ctx, slog.LevelDebug) {
			t.Error("debug should be disabled when options are nil")
		}
		if !h.Enabled(ctx, slog.LevelInfo) {
			t.Error("info should be enabled when options are nil")
		}
	})

	t.Run("configured level", func(t *testing.T) {
		h := newCatenaTextHandler(&bytes.Buffer{}, &slog.HandlerOptions{Level: slog.LevelWarn}, false)
		if h.Enabled(ctx, slog.LevelInfo) {
			t.Error("info should be disabled at warn level")
		}
		if !h.Enabled(ctx, slog.LevelWarn) {
			t.Error("warn should be enabled at warn level")
		}
	})
}

func TestCatenaTextHandler_Handle(t *testing.T) {
	when := time.Date(2026, 10, 10, 10, 10, 10, 100*int(time.Millisecond), time.UTC)
	ctx := context.Background()

	t.Run("formats level message and values", func(t *testing.T) {
		var buf bytes.Buffer
		h := newCatenaTextHandler(&buf, &slog.HandlerOptions{Level: slog.LevelDebug}, false)
		record := slog.NewRecord(when, slog.LevelInfo, "hello", 0)
		record.AddAttrs(
			slog.String("name", "plain"),
			slog.String("empty", ""),
			slog.String("spaced", "hello world"),
			slog.String("quoted", `say "hi"`),
			slog.Int64("count", -3),
			slog.Uint64("size", 42),
			slog.Float64("ratio", 1.25),
			slog.Bool("ok", true),
			slog.Duration("elapsed", 1500*time.Millisecond),
			slog.Time("at", when),
			slog.Any("raw", struct{ N int }{N: 2}),
			slog.Any("resolved", fixedLogValue{}),
		)
		if err := h.Handle(ctx, record); err != nil {
			t.Fatalf("Handle: %v", err)
		}

		got := buf.String()
		wantParts := []string{
			"26-10-10T10:10:10.10Z [INFO]: hello",
			"name=plain",
			`empty=""`,
			`spaced="hello world"`,
			`quoted="say \"hi\""`,
			"count=-3",
			"size=42",
			"ratio=1.25",
			"ok=true",
			"elapsed=1.5s",
			"at=" + when.Format(time.RFC3339Nano),
			"raw={2}",
			"resolved=from-valuer",
		}
		for _, part := range wantParts {
			if !strings.Contains(got, part) {
				t.Errorf("output missing %q\n got: %s", part, got)
			}
		}
		if !strings.HasSuffix(got, "\n") {
			t.Errorf("output should end with a newline: %q", got)
		}
	})

	t.Run("colors the level and stamps a zero time", func(t *testing.T) {
		var buf bytes.Buffer
		h := newCatenaTextHandler(&buf, &slog.HandlerOptions{Level: slog.LevelDebug}, true)
		record := slog.NewRecord(time.Time{}, slog.LevelError, "", 0)
		if err := h.Handle(ctx, record); err != nil {
			t.Fatalf("Handle: %v", err)
		}
		got := buf.String()
		if !strings.Contains(got, ansiRed+"[ERROR]"+ansiReset+":") {
			t.Errorf("expected colored error level, got %q", got)
		}
		if strings.Contains(got, ": ") {
			t.Errorf("empty message should not add a trailing space, got %q", got)
		}
	})

	levels := []struct {
		level slog.Level
		text  string
		color string
	}{
		{slog.LevelDebug, "DEBUG", ansiBlue},
		{slog.LevelInfo, "INFO", ansiGreen},
		{slog.LevelWarn, "WARNING", ansiYellow},
		{slog.LevelError, "ERROR", ansiRed},
	}
	for _, tt := range levels {
		t.Run(tt.text, func(t *testing.T) {
			var buf bytes.Buffer
			h := newCatenaTextHandler(&buf, &slog.HandlerOptions{Level: slog.LevelDebug}, true)
			record := slog.NewRecord(when, tt.level, "msg", 0)
			if err := h.Handle(ctx, record); err != nil {
				t.Fatalf("Handle: %v", err)
			}
			want := tt.color + "[" + tt.text + "]" + ansiReset
			if !strings.Contains(buf.String(), want) {
				t.Errorf("expected %q in %q", want, buf.String())
			}
		})
	}

	t.Run("returns write errors", func(t *testing.T) {
		h := newCatenaTextHandler(failWriter{}, &slog.HandlerOptions{Level: slog.LevelInfo}, false)
		err := h.Handle(ctx, slog.NewRecord(when, slog.LevelInfo, "fail", 0))
		if !errors.Is(err, errWrite) {
			t.Fatalf("Handle error = %v, want %v", err, errWrite)
		}
	})
}

func TestCatenaTextHandler_WithAttrsAndGroups(t *testing.T) {
	when := time.Date(2026, 10, 10, 10, 10, 10, 0, time.UTC)
	ctx := context.Background()
	var buf bytes.Buffer
	parent := newCatenaTextHandler(&buf, &slog.HandlerOptions{Level: slog.LevelDebug}, false)

	child := parent.WithAttrs([]slog.Attr{slog.String("component", "log-test")}).(*catenaTextHandler)
	grouped := child.WithGroup("testGroup").(*catenaTextHandler)
	nested := grouped.WithGroup("nestedGroup").(*catenaTextHandler)

	if len(parent.attrs) != 0 || len(parent.groups) != 0 {
		t.Fatal("WithAttrs and WithGroup must not mutate the parent handler")
	}
	if got := child.attrs; len(got) != 1 || got[0].Key != "component" {
		t.Fatalf("child attrs = %#v, want %#v", got, []slog.Attr{slog.String("component", "log-test")})
	}
	if strings.Join(nested.groups, ".") != "testGroup.nestedGroup" {
		t.Fatalf("groups = %v, want %v", nested.groups, []string{"testGroup", "nestedGroup"})
	}
	if child.writeMu != parent.writeMu {
		t.Fatal("derived handlers should share the write mutex")
	}

	record := slog.NewRecord(when, slog.LevelDebug, "call", 0)
	record.AddAttrs(
		slog.Attr{},
		slog.Int("id", 7),
		slog.Group("http",
			slog.String("method", "GET"),
			slog.Attr{},
		),
		slog.Group("", slog.String("flat", "yes")),
		slog.Attr{Value: slog.IntValue(1)},
	)
	if err := nested.Handle(ctx, record); err != nil {
		t.Fatalf("Handle: %v", err)
	}

	got := buf.String()
	for _, part := range []string{
		"component=log-test",
		"testGroup.nestedGroup.id=7",
		"testGroup.nestedGroup.http.method=GET",
		"testGroup.nestedGroup.flat=yes",
	} {
		if !strings.Contains(got, part) {
			t.Errorf("output missing %q\n got: %s", part, got)
		}
	}
	if strings.Contains(got, " =") || strings.Contains(got, "=1 ") {
		t.Errorf("empty attributes should be skipped, got %s", got)
	}
}

func TestMultiHandler(t *testing.T) {
	when := time.Date(2026, 10, 10, 10, 10, 10, 0, time.UTC)
	ctx := context.Background()

	t.Run("fans out records to multiple handlers", func(t *testing.T) {
		var infoBuf, errorBuf bytes.Buffer
		infoH := newCatenaTextHandler(&infoBuf, &slog.HandlerOptions{Level: slog.LevelInfo}, false)
		errorH := newCatenaTextHandler(&errorBuf, &slog.HandlerOptions{Level: slog.LevelError}, false)
		handler := (&multiHandler{handlers: []slog.Handler{infoH, errorH}}).
			WithAttrs([]slog.Attr{slog.String("component", "test-case")}).
			WithGroup("slot")

		if handler.Enabled(ctx, slog.LevelDebug) {
			t.Error("debug should be disabled when every child rejects it")
		}
		if !handler.Enabled(ctx, slog.LevelInfo) {
			t.Error("info should be enabled when one child accepts it")
		}

		infoRecord := slog.NewRecord(when, slog.LevelInfo, "started", 0)
		infoRecord.AddAttrs(slog.Int("id", 1))
		if err := handler.Handle(ctx, infoRecord); err != nil {
			t.Fatalf("Handle info: %v", err)
		}
		errorRecord := slog.NewRecord(when, slog.LevelError, "failed", 0)
		if err := handler.Handle(ctx, errorRecord); err != nil {
			t.Fatalf("Handle error: %v", err)
		}
		expectedInfoOutput := "[INFO]: started slot.component=test-case slot.id=1"
		expectedErrorOutput := "[ERROR]: failed slot.component=test-case"

		if !strings.Contains(infoBuf.String(), expectedInfoOutput) {
			t.Fatalf("info handler output = %q, want %q", infoBuf.String(), expectedInfoOutput)
		}
		if errorBuf.Len() == 0 || !strings.Contains(errorBuf.String(), expectedErrorOutput) {
			t.Fatalf("error handler output = %q, want %q", errorBuf.String(), expectedErrorOutput)
		}
		if strings.Contains(errorBuf.String(), "started") {
			t.Fatalf("error handler should skip info records, got %q", errorBuf.String())
		}
	})

	t.Run("Handler errors handled separately", func(t *testing.T) {
		var buf bytes.Buffer
		okH := newCatenaTextHandler(&buf, &slog.HandlerOptions{Level: slog.LevelInfo}, false)
		badH := newCatenaTextHandler(failWriter{}, &slog.HandlerOptions{Level: slog.LevelInfo}, false)
		handler := &multiHandler{handlers: []slog.Handler{okH, badH}}

		err := handler.Handle(ctx, slog.NewRecord(when, slog.LevelInfo, "partial", 0))
		if !errors.Is(err, errWrite) {
			t.Fatalf("Handle error = %v, want %v", err, errWrite)
		}
		if !strings.Contains(buf.String(), "partial") {
			t.Fatalf("successful handler should still write, got %q", buf.String())
		}
	})
}

type fixedLogValue struct{}

func (fixedLogValue) LogValue() slog.Value {
	return slog.StringValue("from-valuer")
}

var errWrite = errors.New("write failed")

type failWriter struct{}

func (failWriter) Write([]byte) (int, error) {
	return 0, errWrite
}

func captureStderr(t *testing.T, fn func()) string {
	t.Helper()
	old := os.Stderr
	t.Cleanup(func() { os.Stderr = old })
	r, w, err := os.Pipe()
	if err != nil {
		t.Fatalf("pipe: %v", err)
	}
	os.Stderr = w

	done := make(chan string, 1)
	go func() {
		b, _ := io.ReadAll(r)
		done <- string(b)
	}()

	fn()
	_ = w.Close()
	os.Stderr = old
	return <-done
}
