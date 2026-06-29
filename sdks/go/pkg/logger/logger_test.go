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
 * @date 2026-05-14
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

func TestInit(t *testing.T) {
	t.Run("creates log directory", func(t *testing.T) {
		close()
		dir := t.TempDir()

		logDir := filepath.Join(dir, "logs")
		_, err := Init(config.LoggerOptions{
			AppName:        "test",
			LogDir:         logDir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("Init failed: %v", err)
		}
		defer close()

		if _, err := os.Stat(logDir); os.IsNotExist(err) {
			t.Error("log directory was not created")
		}
	})

	t.Run("creates log file with timestamp", func(t *testing.T) {
		close()
		dir := t.TempDir()

		_, err := Init(config.LoggerOptions{
			AppName:        "myapp",
			LogDir:         dir,
			WriteToFile:    true,
			WriteToConsole: false,
			Level:          LevelInfo,
		})
		if err != nil {
			t.Fatalf("Init failed: %v", err)
		}
		defer close()

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

		close()
		// Try to create a log directory in a non-existent parent with no permissions
		_, err := Init(config.LoggerOptions{
			AppName:        "test",
			LogDir:         "/nonexistent/path/that/should/fail/logs",
			WriteToFile:    true,
			WriteToConsole: false,
		})
		if err == nil {
			t.Error("expected error for invalid log directory")
			close()
		}
	})

	t.Run("only initializes once", func(t *testing.T) {
		close()
		dir := t.TempDir()

		_, err := Init(config.LoggerOptions{
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
		_, err = Init(config.LoggerOptions{
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

		close()
	})
}

func TestLogToFile(t *testing.T) {
	close()
	dir := t.TempDir()

	_, err := Init(config.LoggerOptions{
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
	close()

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

func TestClose(t *testing.T) {
	dir := t.TempDir()

	// First initialization
	close()
	_, err := Init(config.LoggerOptions{
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
	close()

	// Second initialization should work after Close
	_, err = Init(config.LoggerOptions{
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
	close()

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
	close()

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
	close()
	dir := t.TempDir()

	_, err := Init(config.LoggerOptions{
		AppName:        "namedtest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer close()

	namedLogger := GetNamed("mycomponent")
	if namedLogger == nil {
		t.Error("GetNamed should return non-nil logger")
	}
	// Named logger should work without panicking
	namedLogger.Info("named log message")
}

func TestWith(t *testing.T) {
	close()
	dir := t.TempDir()

	_, err := Init(config.LoggerOptions{
		AppName:        "withtest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer close()

	// With should return a logger with attributes
	loggerWith := With("key", "value")
	if loggerWith == nil {
		t.Error("With should return non-nil logger")
	}
	loggerWith.Info("log with attribute")
}

func TestWithGroup(t *testing.T) {
	close()
	dir := t.TempDir()

	_, err := Init(config.LoggerOptions{
		AppName:        "grouptest",
		LogDir:         dir,
		WriteToFile:    true,
		WriteToConsole: false,
		Level:          LevelDebug,
	})
	if err != nil {
		t.Fatalf("Init failed: %v", err)
	}
	defer close()

	// WithGroup should return a logger with a group prefix
	groupLogger := WithGroup("mygroup")
	if groupLogger == nil {
		t.Error("WithGroup should return non-nil logger")
	}
	groupLogger.Info("log in group")
}

func TestSilentMode(t *testing.T) {
	close()
	dir := t.TempDir()

	_, err := Init(config.LoggerOptions{
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
	defer close()

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
	close()
	dir := t.TempDir()

	_, err := Init(config.LoggerOptions{
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
	close()

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
