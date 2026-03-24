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
 * @date 2025-12-09
 */

package logger

import (
	"log/slog"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/internal/testutil"
)

func TestInit(t *testing.T) {
	t.Run("creates log directory", func(t *testing.T) {
		Close()
		dir, cleanup := testutil.TempDir(t, "logger-test")
		defer cleanup()

		logDir := filepath.Join(dir, "logs")
		err := Init(Settings{
			AppName:        "test",
			LogDir:         logDir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("Init failed: %v", err)
		}
		defer Close()

		if _, err := os.Stat(logDir); os.IsNotExist(err) {
			t.Error("log directory was not created")
		}
	})

	t.Run("creates log file with timestamp", func(t *testing.T) {
		Close()
		dir, cleanup := testutil.TempDir(t, "logger-test")
		defer cleanup()

		err := Init(Settings{
			AppName:        "myapp",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("Init failed: %v", err)
		}
		defer Close()

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
		Close()
		// Try to create a log directory in a non-existent parent with no permissions
		err := Init(Settings{
			AppName:        "test",
			LogDir:         "/nonexistent/path/that/should/fail/logs",
			WriteToFile:    true,
			WriteToConsole: false,
		})
		if err == nil {
			t.Error("expected error for invalid log directory")
			Close()
		}
	})

	t.Run("only initializes once", func(t *testing.T) {
		Close()
		dir, cleanup := testutil.TempDir(t, "logger-test")
		defer cleanup()

		err := Init(Settings{
			AppName:        "first",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("first Init failed: %v", err)
		}

		// Second init should return ErrAlreadyInitialized
		err = Init(Settings{
			AppName:        "second",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelDebug,
		})
		if err != ErrAlreadyInitialized {
			t.Errorf("expected ErrAlreadyInitialized, got %v", err)
		}

		// Verify only one log file exists (from first init)
		entries, err := os.ReadDir(dir)
		if err != nil {
			t.Fatalf("failed to read dir: %v", err)
		}
		if len(entries) != 1 {
			t.Errorf("expected 1 file, got %d", len(entries))
		}
		if !strings.Contains(entries[0].Name(), "first") {
			t.Errorf("expected filename to contain 'first', got %s", entries[0].Name())
		}

		Close()
	})
}

func TestLogToFile(t *testing.T) {
	Close()
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Settings{
		AppName:        "filetest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelInfo,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}

	Info("test log message to file")
	Close()

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

	testutil.AssertContains(t, string(content), "test log message to file")
}

func TestClose(t *testing.T) {
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	// First initialization
	Close()
	err := Init(Settings{
		AppName:        "first",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelInfo,
	})
	if err != nil {
		t.Fatalf("first Init failed: %v", err)
	}

	Info("first log")
	Close()

	// Second initialization should work after Close
	err = Init(Settings{
		AppName:        "second",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelInfo,
	})
	if err != nil {
		t.Fatalf("second Init failed: %v", err)
	}

	Info("second log")
	Close()

	// Should have two log files
	entries, err := os.ReadDir(dir)
	if err != nil {
		t.Fatalf("failed to read dir: %v", err)
	}
	if len(entries) != 2 {
		t.Errorf("expected 2 files, got %d", len(entries))
	}
}

func TestNoLoggerInitialized(t *testing.T) {
	Close()

	// These should not panic when logger is not initialized
	// (slog.Default() should handle it gracefully)
	Debug("test")
	Info("test")
	Warning("test")
	Error("test")
}

func TestGetLogger(t *testing.T) {
	logger := GetLogger()
	if logger == nil {
		t.Error("GetLogger should return non-nil logger")
	}
}

func TestGetNamed(t *testing.T) {
	Close()
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Settings{
		AppName:        "namedtest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer Close()

	namedLogger := GetNamed("mycomponent")
	if namedLogger == nil {
		t.Error("GetNamed should return non-nil logger")
	}
	// Named logger should work without panicking
	namedLogger.Info("named log message")
}

func TestWith(t *testing.T) {
	Close()
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Settings{
		AppName:        "withtest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer Close()

	// With should return a logger with attributes
	loggerWith := With("key", "value")
	if loggerWith == nil {
		t.Error("With should return non-nil logger")
	}
	loggerWith.Info("log with attribute")
}

func TestWithGroup(t *testing.T) {
	Close()
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Settings{
		AppName:        "grouptest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer Close()

	// WithGroup should return a logger with a group prefix
	groupLogger := WithGroup("mygroup")
	if groupLogger == nil {
		t.Error("WithGroup should return non-nil logger")
	}
	groupLogger.Info("log in group")
}

func TestSilentMode(t *testing.T) {
	Close()
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Settings{
		AppName:        "silenttest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Silent:         true,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer Close()

	// In silent mode, nothing should be logged
	Debug("debug message")
	Info("info message")
	Warning("warning message")
	Error("error message")

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
	Close()
	dir, cleanup := testutil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Settings{
		AppName:        "jsontest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelInfo,
		UseJSON:        true,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}

	Info("json log message")
	Close()

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

func TestDefaultSettings(t *testing.T) {
	settings := DefaultSettings()

	if settings.AppName != "catena" {
		t.Errorf("expected AppName 'catena', got %q", settings.AppName)
	}
	if settings.LogDir != "./logs" {
		t.Errorf("expected LogDir './logs', got %q", settings.LogDir)
	}
	if settings.Silent != false {
		t.Errorf("expected Silent false, got %v", settings.Silent)
	}
	if settings.Level != slog.LevelError {
		t.Errorf("expected Level LevelError, got %v", settings.Level)
	}
	if settings.WriteToFile != true {
		t.Errorf("expected WriteToFile true, got %v", settings.WriteToFile)
	}
	if settings.WriteToConsole != true {
		t.Errorf("expected WriteToConsole true, got %v", settings.WriteToConsole)
	}
}

func TestDebugSettings(t *testing.T) {
	settings := DebugSettings()

	if settings.Level != slog.LevelDebug {
		t.Errorf("expected Level LevelDebug, got %v", settings.Level)
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
