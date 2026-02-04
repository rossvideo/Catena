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

package logger

import (
	"log/slog"
	"testing"
)

func TestLevelString(t *testing.T) {
	tests := []struct {
		level    Level
		expected string
	}{
		{LevelDebug, "DEBUG"},
		{LevelInfo, "INFO"},
		{LevelWarning, "WARN"},
		{LevelError, "ERROR"},
	}

	for _, tt := range tests {
		t.Run(tt.expected, func(t *testing.T) {
			result := tt.level.String()
			if result != tt.expected {
				t.Errorf("Level(%d).String() = %q, want %q", tt.level, result, tt.expected)
			}
		})
	}
}

func TestLevelOrder(t *testing.T) {
	if LevelDebug >= LevelInfo {
		t.Error("LevelDebug should be less than LevelInfo")
	}
	if LevelInfo >= LevelWarning {
		t.Error("LevelInfo should be less than LevelWarning")
	}
	if LevelWarning >= LevelError {
		t.Error("LevelWarning should be less than LevelError")
	}
}

func TestLevelComparison(t *testing.T) {
	tests := []struct {
		name      string
		logLevel  Level
		minLevel  Level
		shouldLog bool
	}{
		{"debug at debug", LevelDebug, LevelDebug, true},
		{"info at debug", LevelInfo, LevelDebug, true},
		{"warning at debug", LevelWarning, LevelDebug, true},
		{"error at debug", LevelError, LevelDebug, true},
		{"debug at info", LevelDebug, LevelInfo, false},
		{"info at info", LevelInfo, LevelInfo, true},
		{"warning at info", LevelWarning, LevelInfo, true},
		{"error at info", LevelError, LevelInfo, true},
		{"debug at warning", LevelDebug, LevelWarning, false},
		{"info at warning", LevelInfo, LevelWarning, false},
		{"warning at warning", LevelWarning, LevelWarning, true},
		{"error at warning", LevelError, LevelWarning, true},
		{"debug at error", LevelDebug, LevelError, false},
		{"info at error", LevelInfo, LevelError, false},
		{"warning at error", LevelWarning, LevelError, false},
		{"error at error", LevelError, LevelError, true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			shouldLog := tt.logLevel >= tt.minLevel
			if shouldLog != tt.shouldLog {
				t.Errorf("logLevel(%v) >= minLevel(%v) = %v, want %v",
					tt.logLevel, tt.minLevel, shouldLog, tt.shouldLog)
			}
		})
	}
}

func TestLevelConstants(t *testing.T) {
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

func TestLevelAlias(t *testing.T) {
	var l Level = slog.LevelInfo
	if l != LevelInfo {
		t.Errorf("Level should be aliased to slog.Level")
	}
}
