/*
 * Copyright 2025 Ross Video Ltd
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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
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
 * @file logger.go
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-12-09
 */

package logger

import (
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/rossvideo/catena/sdks/go/internal/testil"
)

func TestInit(t *testing.T) {
	t.Run("creates log directory", func(t *testing.T) {
		Reset()
		dir, cleanup := testil.TempDir(t, "logger-test")
		defer cleanup()

		logDir := filepath.Join(dir, "logs")
		err := Init(Config{
			AppName:        "test",
			LogDir:         logDir,
			WriteToFile:    true,
			WriteToConsole: false,
			MinLevel:       LevelInfo,
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
		Reset()
		dir, cleanup := testil.TempDir(t, "logger-test")
		defer cleanup()

		err := Init(Config{
			AppName:        "myapp",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			MinLevel:       LevelInfo,
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
		Reset()
		// Try to create a log directory in a non-existent parent with no permissions
		err := Init(Config{
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
		Reset()
		dir, cleanup := testil.TempDir(t, "logger-test")
		defer cleanup()

		err := Init(Config{
			AppName:        "first",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			MinLevel:       LevelInfo,
		})
		if err != nil {
			t.Fatalf("first Init failed: %v", err)
		}

		// Second init should be ignored (no error, but no change)
		err = Init(Config{
			AppName:        "second",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			MinLevel:       LevelDebug,
		})
		if err != nil {
			t.Fatalf("second Init failed: %v", err)
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

func TestLogLevels(t *testing.T) {
	tests := []struct {
		name         string
		minLevel     Level
		logFunc      func(string, ...any)
		logLevel     string
		shouldAppear bool
	}{
		{"debug at debug level", LevelDebug, Debug, "DEBUG", true},
		{"info at debug level", LevelDebug, Info, "INFO", true},
		{"warning at debug level", LevelDebug, Warning, "WARNING", true},
		{"error at debug level", LevelDebug, Error, "ERROR", true},

		{"debug at info level", LevelInfo, Debug, "DEBUG", false},
		{"info at info level", LevelInfo, Info, "INFO", true},
		{"warning at info level", LevelInfo, Warning, "WARNING", true},
		{"error at info level", LevelInfo, Error, "ERROR", true},

		{"debug at warning level", LevelWarning, Debug, "DEBUG", false},
		{"info at warning level", LevelWarning, Info, "INFO", false},
		{"warning at warning level", LevelWarning, Warning, "WARNING", true},
		{"error at warning level", LevelWarning, Error, "ERROR", true},

		{"debug at error level", LevelError, Debug, "DEBUG", false},
		{"info at error level", LevelError, Info, "INFO", false},
		{"warning at error level", LevelError, Warning, "WARNING", false},
		{"error at error level", LevelError, Error, "ERROR", true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			Reset()

			buf := testil.NewLogBuffer()
			err := Init(Config{
				AppName:        "test",
				LogDir:         "",
				WriteToFile:    false,
				WriteToConsole: false,
				MinLevel:       tt.minLevel,
			})
			if err != nil {
				t.Fatalf("Init failed: %v", err)
			}

			// Inject our test buffer as writer
			SetWriters(buf)

			tt.logFunc("test message")

			if tt.shouldAppear {
				if !buf.Contains(tt.logLevel) {
					t.Errorf("expected log to contain %q, got: %s", tt.logLevel, buf.String())
				}
				if !buf.Contains("test message") {
					t.Errorf("expected log to contain 'test message', got: %s", buf.String())
				}
			} else {
				if buf.Contains(tt.logLevel) {
					t.Errorf("expected log to NOT contain %q, got: %s", tt.logLevel, buf.String())
				}
			}
		})
	}
}

func TestLogFormat(t *testing.T) {
	Reset()

	buf := testil.NewLogBuffer()
	err := Init(Config{
		AppName:        "test",
		LogDir:         "",
		WriteToFile:    false,
		WriteToConsole: false,
		MinLevel:       LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}

	SetWriters(buf)

	Info("hello %s %d", "world", 42)

	output := buf.String()

	// Check format: timestamp [LEVEL] message
	if !strings.Contains(output, "[INFO]") {
		t.Errorf("expected [INFO] in output, got: %s", output)
	}
	if !strings.Contains(output, "hello world 42") {
		t.Errorf("expected 'hello world 42' in output, got: %s", output)
	}
	// Check timestamp format (should start with YYYY-MM-DD)
	if len(output) < 10 || output[4] != '-' || output[7] != '-' {
		t.Errorf("expected timestamp at start of output, got: %s", output)
	}
}

func TestSilentMode(t *testing.T) {
	Reset()

	buf := testil.NewLogBuffer()
	err := Init(Config{
		AppName:        "test",
		LogDir:         "",
		WriteToFile:    false,
		WriteToConsole: false,
		Silent:         true,
		MinLevel:       LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}

	SetWriters(buf)

	// In silent mode, MinLevel is set to Error
	Debug("debug message")
	Info("info message")
	Warning("warning message")
	Error("error message")

	output := buf.String()

	// Only error should appear in silent mode
	testil.AssertNotContains(t, output, "debug message")
	testil.AssertNotContains(t, output, "info message")
	testil.AssertNotContains(t, output, "warning message")
	testil.AssertContains(t, output, "error message")
}

func TestNoLoggerInitialized(t *testing.T) {
	Reset()

	// These should not panic when logger is not initialized
	Debug("test")
	Info("test")
	Warning("test")
	Error("test")
}

func TestLogToFile(t *testing.T) {
	Reset()
	dir, cleanup := testil.TempDir(t, "logger-test")
	defer cleanup()

	err := Init(Config{
		AppName:        "filetest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		MinLevel:       LevelInfo,
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

	testil.AssertContains(t, string(content), "test log message to file")
	testil.AssertContains(t, string(content), "[INFO]")
}

func TestReset(t *testing.T) {
	dir, cleanup := testil.TempDir(t, "logger-test")
	defer cleanup()

	// First initialization
	Reset()
	err := Init(Config{
		AppName:        "first",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		MinLevel:       LevelInfo,
	})
	if err != nil {
		t.Fatalf("first Init failed: %v", err)
	}

	Info("first log")
	Reset()

	// Second initialization should work after reset
	err = Init(Config{
		AppName:        "second",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		MinLevel:       LevelInfo,
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

