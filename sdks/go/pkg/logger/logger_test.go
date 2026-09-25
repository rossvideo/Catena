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
	"log/slog"
	"os"
	"path/filepath"
	"strings"
	"testing"

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
