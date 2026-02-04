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
 * @brief Test for Settings and configuration parsing.
 * @file config_test.go
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-12-09
 */

package logger

import (
	"log/slog"
	"os"
	"testing"
)

func TestSettingsFields(t *testing.T) {
	stg := Settings{
		AppName:        "myapp",
		LogDir:         "/var/log/myapp",
		Silent:         true,
		Level:          LevelError,
		WriteToFile:    false,
		WriteToConsole: true,
		UseJSON:        true,
	}

	if stg.AppName != "myapp" {
		t.Errorf("expected AppName 'myapp', got %q", stg.AppName)
	}
	if stg.LogDir != "/var/log/myapp" {
		t.Errorf("expected LogDir '/var/log/myapp', got %q", stg.LogDir)
	}
	if stg.Silent != true {
		t.Errorf("expected Silent true, got %v", stg.Silent)
	}
	if stg.Level != LevelError {
		t.Errorf("expected Level LevelError, got %v", stg.Level)
	}
	if stg.WriteToFile != false {
		t.Errorf("expected WriteToFile false, got %v", stg.WriteToFile)
	}
	if stg.WriteToConsole != true {
		t.Errorf("expected WriteToConsole true, got %v", stg.WriteToConsole)
	}
	if stg.UseJSON != true {
		t.Errorf("expected UseJSON true, got %v", stg.UseJSON)
	}
}

func TestParseLevel(t *testing.T) {
	tests := []struct {
		input    string
		expected slog.Level
	}{
		{"debug", slog.LevelDebug},
		{"DEBUG", slog.LevelDebug},
		{"info", slog.LevelInfo},
		{"INFO", slog.LevelInfo},
		{"warn", slog.LevelWarn},
		{"warning", slog.LevelWarn},
		{"WARNING", slog.LevelWarn},
		{"error", slog.LevelError},
		{"ERROR", slog.LevelError},
		{"invalid", slog.LevelError}, // default
		{"", slog.LevelError},        // default
	}

	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			result := ParseLevel(tt.input)
			if result != tt.expected {
				t.Errorf("ParseLevel(%q) = %v, want %v", tt.input, result, tt.expected)
			}
		})
	}
}

func TestParseVerbosity(t *testing.T) {
	tests := []struct {
		name     string
		args     []string
		expected slog.Level
	}{
		{"no verbosity", []string{"--config", "file.yaml"}, slog.LevelError},
		{"single -v", []string{"-v", "--config"}, slog.LevelWarn},
		{"double -vv", []string{"-vv", "--config"}, slog.LevelInfo},
		{"triple -vvv", []string{"-vvv"}, slog.LevelDebug},
		{"quad -vvvv", []string{"-vvvv"}, slog.LevelDebug}, // caps at debug
		{"mixed args", []string{"-v", "file", "-v"}, slog.LevelInfo},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			result := ParseVerbosity(tt.args)
			if result != tt.expected {
				t.Errorf("ParseVerbosity(%v) = %v, want %v", tt.args, result, tt.expected)
			}
		})
	}
}

func TestParseFromEnv(t *testing.T) {
	// Clean up env vars after test
	defer func() {
		os.Unsetenv("TEST_LOG_DIR")
		os.Unsetenv("TEST_SILENT")
		os.Unsetenv("TEST_LOG_LEVEL")
		os.Unsetenv("TEST_LOG_FILE")
		os.Unsetenv("TEST_LOG_CONSOLE")
		os.Unsetenv("TEST_LOG_JSON")
	}()

	t.Run("default values", func(t *testing.T) {
		stg := ParseFromEnv("NONEXISTENT")
		if stg.AppName != "catena" {
			t.Errorf("expected default AppName 'catena', got %q", stg.AppName)
		}
		if stg.LogDir != "./logs" {
			t.Errorf("expected default LogDir './logs', got %q", stg.LogDir)
		}
	})

	t.Run("with env vars", func(t *testing.T) {
		os.Setenv("TEST_LOG_DIR", "/custom/logs")
		os.Setenv("TEST_SILENT", "true")
		os.Setenv("TEST_LOG_LEVEL", "debug")
		os.Setenv("TEST_LOG_FILE", "false")
		os.Setenv("TEST_LOG_CONSOLE", "true")
		os.Setenv("TEST_LOG_JSON", "true")

		stg := ParseFromEnv("TEST")
		if stg.LogDir != "/custom/logs" {
			t.Errorf("expected LogDir '/custom/logs', got %q", stg.LogDir)
		}
		if !stg.Silent {
			t.Error("expected Silent true")
		}
		if stg.Level != slog.LevelDebug {
			t.Errorf("expected Level LevelDebug, got %v", stg.Level)
		}
		if stg.WriteToFile {
			t.Error("expected WriteToFile false")
		}
		if !stg.WriteToConsole {
			t.Error("expected WriteToConsole true")
		}
		if !stg.UseJSON {
			t.Error("expected UseJSON true")
		}
	})
}

func TestParseBoolValues(t *testing.T) {
	tests := []struct {
		input    string
		expected bool
	}{
		{"true", true},
		{"TRUE", true},
		{"True", true},
		{"1", true},
		{"yes", true},
		{"YES", true},
		{"on", true},
		{"ON", true},
		{"false", false},
		{"FALSE", false},
		{"0", false},
		{"no", false},
		{"off", false},
		{"", false},
		{"invalid", false},
	}

	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			result := parseBool(tt.input)
			if result != tt.expected {
				t.Errorf("parseBool(%q) = %v, want %v", tt.input, result, tt.expected)
			}
		})
	}
}

func TestParseSettingsWithVerbosity(t *testing.T) {
	tests := []struct {
		name         string
		envLevel     string
		osArgs       []string
		expectedLevel slog.Level
	}{
		{
			name:         "no CLI flags, use env level",
			envLevel:     "info",
			osArgs:       []string{"program"},
			expectedLevel: slog.LevelInfo,
		},
		{
			name:         "CLI more verbose than env",
			envLevel:     "error",
			osArgs:       []string{"program", "-vv"},
			expectedLevel: slog.LevelInfo,
		},
		{
			name:         "env more verbose than CLI",
			envLevel:     "debug",
			osArgs:       []string{"program", "-v"},
			expectedLevel: slog.LevelDebug,
		},
		{
			name:         "CLI overrides with -vvv",
			envLevel:     "warn",
			osArgs:       []string{"program", "-vvvv"},
			expectedLevel: slog.LevelDebug,
		},
		{
			name:         "no env level set, CLI provides",
			envLevel:     "",
			osArgs:       []string{"program", "-v"},
			expectedLevel: slog.LevelWarn,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Save and restore os.Args
			oldArgs := os.Args
			defer func() { os.Args = oldArgs }()
			os.Args = tt.osArgs

			// Set env var
			if tt.envLevel != "" {
				os.Setenv("TEST_LOG_LEVEL", tt.envLevel)
			} else {
				os.Unsetenv("TEST_LOG_LEVEL")
			}
			defer os.Unsetenv("TEST_LOG_LEVEL")

			stg := ParseSettingsWithVerbosity("TEST")
			if stg.Level != tt.expectedLevel {
				t.Errorf("expected level %v, got %v", tt.expectedLevel, stg.Level)
			}
		})
	}
}
