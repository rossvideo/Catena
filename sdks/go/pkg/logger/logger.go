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
 * @brief Logger using log/slog with configurable levels.
 * @file logger.go
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-12-09
 */
package logger

import (
	"context"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"os"
	"path/filepath"
	"sync"
	"time"
)

var (
	ErrAlreadyInitialized = errors.New("logger already initialized")

	globalLogger *logger
	initialized  bool
	initMu       sync.Mutex
)

// logger handles structured logging using slog
type logger struct {
	config Config
	file   *os.File
}

// Init initializes the global logger.
// Returns ErrAlreadyInitialized if called more than once.
func Init(cfg Config) error {
	initMu.Lock()
	defer initMu.Unlock()

	if initialized {
		return ErrAlreadyInitialized
	}

	globalLogger = &logger{config: cfg}
	initErr := globalLogger.setup()
	if initErr != nil {
		// Nil out globalLogger on setup failure to avoid partially initialized state
		globalLogger = nil
		return initErr
	}
	initialized = true
	return nil
}

// setup configures the slog handlers based on config and sets slog.SetDefault
func (l *logger) setup() error {
	if l.config.Silent {
		// Silent mode: discard all logs
		slog.SetDefault(slog.New(slog.NewTextHandler(io.Discard, nil)))
		return nil
	}

	var writers []io.Writer

	// Setup console output
	if l.config.WriteToConsole {
		writers = append(writers, os.Stderr)
	}

	// Setup file output
	if l.config.WriteToFile {
		if err := os.MkdirAll(l.config.LogDir, 0755); err != nil {
			return fmt.Errorf("failed to create log directory: %w", err)
		}

		// Timestamped filename: YYYYMMDD_HHMMSS_appName.log
		timestamp := time.Now().Format("20060102_150405")
		filename := fmt.Sprintf("%s_%s.log", timestamp, l.config.AppName)
		logPath := filepath.Join(l.config.LogDir, filename)

		f, err := os.OpenFile(logPath, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
		if err != nil {
			return fmt.Errorf("failed to open log file: %w", err)
		}
		l.file = f
		writers = append(writers, f)
	}

	// Combine writers
	var output io.Writer
	if len(writers) == 0 {
		output = io.Discard
	} else if len(writers) == 1 {
		output = writers[0]
	} else {
		output = io.MultiWriter(writers...)
	}

	// Create handler options with configured level
	opts := &slog.HandlerOptions{
		Level: l.config.Level,
	}

	// Create handler based on format preference
	var handler slog.Handler
	if l.config.UseJSON {
		handler = slog.NewJSONHandler(output, opts)
	} else {
		handler = slog.NewTextHandler(output, opts)
	}

	// Set as default logger for the slog package
	slog.SetDefault(slog.New(handler))

	return nil
}

// Close cleans up resources
func Close() {
	if globalLogger != nil && globalLogger.file != nil {
		if err := globalLogger.file.Close(); err != nil {
			fmt.Fprintf(os.Stderr, "logger: failed to close log file: %v\n", err)
		}
	}
}

// GetLogger returns the underlying slog.Logger for advanced usage.
// Always returns a valid logger (falls back to slog.Default()).
func GetLogger() *slog.Logger {
	return slog.Default()
}

// GetNamed returns a logger with a component/name attribute.
// This is useful for identifying which component generated a log message.
// Example: logger.GetNamed("server") returns a logger that adds "logger"="server" to all messages.
func GetNamed(name string) *slog.Logger {
	return slog.Default().With("logger", name)
}

// Debug logs a debug message (only visible when Level is LevelDebug)
func Debug(msg string, args ...any) {
	slog.Debug(msg, args...)
}

// Info logs an info message
func Info(msg string, args ...any) {
	slog.Info(msg, args...)
}

// Warning logs a warning message
func Warning(msg string, args ...any) {
	slog.Warn(msg, args...)
}

// Error logs an error message
func Error(msg string, args ...any) {
	slog.Error(msg, args...)
}

// With returns a logger with additional attributes
func With(args ...any) *slog.Logger {
	return slog.Default().With(args...)
}

// WithGroup returns a logger with a group prefix
func WithGroup(name string) *slog.Logger {
	return slog.Default().WithGroup(name)
}

// DebugContext logs a debug message with context
func DebugContext(ctx context.Context, msg string, args ...any) {
	slog.DebugContext(ctx, msg, args...)
}

// InfoContext logs an info message with context
func InfoContext(ctx context.Context, msg string, args ...any) {
	slog.InfoContext(ctx, msg, args...)
}

// WarningContext logs a warning message with context
func WarningContext(ctx context.Context, msg string, args ...any) {
	slog.WarnContext(ctx, msg, args...)
}

// ErrorContext logs an error message with context
func ErrorContext(ctx context.Context, msg string, args ...any) {
	slog.ErrorContext(ctx, msg, args...)
}
