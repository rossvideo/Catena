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
 * @brief Logger struct for logging to console and file.
 * @file logger.go
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025-12-09
 */
package logger

import (
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sync"
	"time"
)

var (
	globalLogger *Logger
	once         sync.Once
)

// Logger handles logging to console and file
type Logger struct {
	mu      sync.Mutex
	config  Config
	file    *os.File
	writers []io.Writer
}

// Init initializes the global logger (matches C++ Logger::init)
func Init(cfg Config) error {
	var initErr error
	once.Do(func() {
		globalLogger = &Logger{config: cfg}
		initErr = globalLogger.setup()
	})
	return initErr
}

func (l *Logger) setup() error {
	if l.config.Silent {
		l.config.MinLevel = LevelError
	}

	// Setup console output
	if l.config.WriteToConsole {
		l.writers = append(l.writers, os.Stderr)
	}

	// Setup file output (matches C++ FileLogSink)
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
		l.writers = append(l.writers, f)
	}

	return nil
}

// Close cleans up resources (matches C++ Logger destructor)
func Close() {
	if globalLogger != nil && globalLogger.file != nil {
		if err := globalLogger.file.Close(); err != nil {
			fmt.Fprintf(os.Stderr, "logger: failed to close log file: %v\n", err)
		}
	}
}

// log writes a message at the given level
func (l *Logger) log(level Level, format string, args ...any) {
	if level < l.config.MinLevel {
		return
	}

	l.mu.Lock()
	defer l.mu.Unlock()

	// Format: timestamp [LEVEL] message
	timestamp := time.Now().Format("2006-01-02 15:04:05.000")
	msg := fmt.Sprintf(format, args...)
	entry := fmt.Sprintf("%s [%s] %s\n", timestamp, level, msg)

	for _, w := range l.writers {
		w.Write([]byte(entry))
	}
}

// Package-level convenience functions (similar to LOG(INFO), LOG(ERROR), etc.)
func Debug(format string, args ...any) {
	if globalLogger != nil {
		globalLogger.log(LevelDebug, format, args...)
	}
}

func Info(format string, args ...any) {
	if globalLogger != nil {
		globalLogger.log(LevelInfo, format, args...)
	}
}

func Warning(format string, args ...any) {
	if globalLogger != nil {
		globalLogger.log(LevelWarning, format, args...)
	}
}

func Error(format string, args ...any) {
	if globalLogger != nil {
		globalLogger.log(LevelError, format, args...)
	}
}
