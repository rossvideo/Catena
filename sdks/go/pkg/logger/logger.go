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
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

var (
	ErrAlreadyInitialized = errors.New("logger already initialized")

	globalLogger *logger
	initialized  bool
	initMu       sync.Mutex
)

type CloseFunc func()

// logger handles structured logging using slog
type logger struct {
	settings config.LoggerOptions
	file     *os.File
}

// Init initializes the global logger.
// Returns a CloseFunc to clean up resources.
// Returns ErrAlreadyInitialized if called more than once.
func Init(opts config.LoggerOptions) (CloseFunc, error) {
	initMu.Lock()
	defer initMu.Unlock()

	if initialized {
		return nil, ErrAlreadyInitialized
	}

	globalLogger = &logger{settings: opts}
	initErr := globalLogger.setup()
	if initErr != nil {
		// Nil out globalLogger on setup failure to avoid partially initialized state
		globalLogger = nil
		return nil, initErr
	}
	initialized = true
	return close, nil
}

// setup configures the slog handlers based on config and sets slog.SetDefault
func (l *logger) setup() error {
	if l.settings.Silent {
		// Silent mode: discard all logs
		slog.SetDefault(slog.New(slog.NewTextHandler(io.Discard, nil)))
		return nil
	}

	// Create handler options with configured level
	opts := &slog.HandlerOptions{
		Level: l.settings.Level,
	}

	var handlers []slog.Handler

	// Setup console output
	if l.settings.WriteToConsole {
		if l.settings.UseJSON {
			handlers = append(handlers, slog.NewJSONHandler(os.Stderr, opts))
		} else {
			handlers = append(handlers, newCatenaTextHandler(os.Stderr, opts, true))
		}
	}

	// Setup file output
	if l.settings.WriteToFile {
		if err := os.MkdirAll(l.settings.LogDir, 0755); err != nil {
			return fmt.Errorf("failed to create log directory: %w", err)
		}

		// Timestamped filename: YYYYMMDD_HHMMSS_appName.log
		timestamp := time.Now().Format("20060102_150405")
		filename := fmt.Sprintf("%s_%s.log", timestamp, l.settings.AppName)
		logPath := filepath.Join(l.settings.LogDir, filename)

		f, err := os.OpenFile(logPath, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
		if err != nil {
			return fmt.Errorf("failed to open log file: %w", err)
		}
		l.file = f

		if l.settings.UseJSON {
			handlers = append(handlers, slog.NewJSONHandler(f, opts))
		} else {
			handlers = append(handlers, newCatenaTextHandler(f, opts, false))
		}
	}

	// Create fallback handler when no output target is configured.
	if len(handlers) == 0 {
		handlers = append(handlers, slog.NewTextHandler(io.Discard, opts))
	}

	var handler slog.Handler
	if len(handlers) == 1 {
		handler = handlers[0]
	} else {
		handler = &multiHandler{handlers: handlers}
	}

	// Set as default logger for the slog package
	slog.SetDefault(slog.New(handler))

	return nil
}

const (
	ansiReset  = "\033[0m"
	ansiBlue   = "\033[34m"
	ansiGreen  = "\033[32m"
	ansiYellow = "\033[33m"
	ansiRed    = "\033[31m"
)

// multiHandler fans out a single log record to multiple handlers.
type multiHandler struct {
	handlers []slog.Handler
}

func (m *multiHandler) Enabled(ctx context.Context, level slog.Level) bool {
	for _, h := range m.handlers {
		if h.Enabled(ctx, level) {
			return true
		}
	}
	return false
}

func (m *multiHandler) Handle(ctx context.Context, r slog.Record) error {
	var err error
	for _, h := range m.handlers {
		if !h.Enabled(ctx, r.Level) {
			continue
		}
		err = errors.Join(err, h.Handle(ctx, r))
	}
	return err
}

func (m *multiHandler) WithAttrs(attrs []slog.Attr) slog.Handler {
	newHandlers := make([]slog.Handler, 0, len(m.handlers))
	for _, h := range m.handlers {
		newHandlers = append(newHandlers, h.WithAttrs(attrs))
	}
	return &multiHandler{handlers: newHandlers}
}

func (m *multiHandler) WithGroup(name string) slog.Handler {
	newHandlers := make([]slog.Handler, 0, len(m.handlers))
	for _, h := range m.handlers {
		newHandlers = append(newHandlers, h.WithGroup(name))
	}
	return &multiHandler{handlers: newHandlers}
}

// catenaTextHandler writes log lines as: YY-MM-DDTHH:MM:SS.ccZ [LEVEL]: message key=value.
type catenaTextHandler struct {
	w       io.Writer
	opts    *slog.HandlerOptions
	color   bool
	attrs   []slog.Attr
	groups  []string
	writeMu *sync.Mutex
}

func newCatenaTextHandler(w io.Writer, opts *slog.HandlerOptions, color bool) *catenaTextHandler {
	if opts == nil {
		opts = &slog.HandlerOptions{}
	}
	return &catenaTextHandler{
		w:       w,
		opts:    opts,
		color:   color,
		writeMu: &sync.Mutex{},
	}
}

func (h *catenaTextHandler) Enabled(_ context.Context, level slog.Level) bool {
	if h.opts == nil || h.opts.Level == nil {
		return level >= slog.LevelInfo
	}
	return level >= h.opts.Level.Level()
}

func (h *catenaTextHandler) Handle(_ context.Context, r slog.Record) error {
	levelText, levelColor := formatLevel(r.Level)
	levelBlock := "[" + levelText + "]"
	if h.color {
		levelBlock = levelColor + levelBlock + ansiReset
	}

	timestamp := r.Time
	if timestamp.IsZero() {
		timestamp = time.Now()
	}

	var b strings.Builder
	b.WriteString(timestamp.UTC().Format("06-01-02T15:04:05.00Z07:00"))
	b.WriteByte(' ')
	b.WriteString(levelBlock)
	b.WriteByte(':')
	if r.Message != "" {
		b.WriteByte(' ')
		b.WriteString(r.Message)
	}

	allAttrs := make([]slog.Attr, 0, len(h.attrs)+r.NumAttrs())
	allAttrs = append(allAttrs, h.attrs...)
	r.Attrs(func(a slog.Attr) bool {
		allAttrs = append(allAttrs, a)
		return true
	})

	for _, attr := range allAttrs {
		h.appendAttr(&b, h.groups, attr)
	}

	b.WriteByte('\n')

	h.writeMu.Lock()
	defer h.writeMu.Unlock()
	_, err := io.WriteString(h.w, b.String())
	return err
}

func (h *catenaTextHandler) WithAttrs(attrs []slog.Attr) slog.Handler {
	newAttrs := make([]slog.Attr, 0, len(h.attrs)+len(attrs))
	newAttrs = append(newAttrs, h.attrs...)
	newAttrs = append(newAttrs, attrs...)
	return &catenaTextHandler{
		w:       h.w,
		opts:    h.opts,
		color:   h.color,
		attrs:   newAttrs,
		groups:  append([]string(nil), h.groups...),
		writeMu: h.writeMu,
	}
}

func (h *catenaTextHandler) WithGroup(name string) slog.Handler {
	newGroups := append(append([]string(nil), h.groups...), name)
	return &catenaTextHandler{
		w:       h.w,
		opts:    h.opts,
		color:   h.color,
		attrs:   append([]slog.Attr(nil), h.attrs...),
		groups:  newGroups,
		writeMu: h.writeMu,
	}
}

func (h *catenaTextHandler) appendAttr(b *strings.Builder, groups []string, attr slog.Attr) {
	attr.Value = attr.Value.Resolve()
	if attr.Equal(slog.Attr{}) {
		return
	}

	if attr.Value.Kind() == slog.KindGroup {
		nextGroups := groups
		if attr.Key != "" {
			nextGroups = append(append([]string(nil), groups...), attr.Key)
		}
		for _, nested := range attr.Value.Group() {
			h.appendAttr(b, nextGroups, nested)
		}
		return
	}

	if attr.Key == "" {
		return
	}

	fullKey := attr.Key
	if len(groups) > 0 {
		fullPath := append(append([]string(nil), groups...), attr.Key)
		fullKey = strings.Join(fullPath, ".")
	}

	b.WriteByte(' ')
	b.WriteString(fullKey)
	b.WriteByte('=')
	b.WriteString(formatValue(attr.Value))
}

func formatLevel(level slog.Level) (string, string) {
	switch {
	case level <= slog.LevelDebug:
		return "DEBUG", ansiBlue
	case level < slog.LevelWarn:
		return "INFO", ansiGreen
	case level < slog.LevelError:
		return "WARNING", ansiYellow
	default:
		return "ERROR", ansiRed
	}
}

func formatValue(v slog.Value) string {
	switch v.Kind() {
	case slog.KindString:
		return formatString(v.String())
	case slog.KindInt64:
		return strconv.FormatInt(v.Int64(), 10)
	case slog.KindUint64:
		return strconv.FormatUint(v.Uint64(), 10)
	case slog.KindFloat64:
		return strconv.FormatFloat(v.Float64(), 'f', -1, 64)
	case slog.KindBool:
		return strconv.FormatBool(v.Bool())
	case slog.KindDuration:
		return v.Duration().String()
	case slog.KindTime:
		return v.Time().Format(time.RFC3339Nano)
	default:
		return fmt.Sprint(v.Any())
	}
}

func formatString(v string) string {
	if v == "" {
		return `""`
	}
	if strings.ContainsAny(v, " \t\n\r=\"") {
		return strconv.Quote(v)
	}
	return v
}

// close cleans up resources and allows re-initialization.
func close() {
	initMu.Lock()
	defer initMu.Unlock()

	if globalLogger != nil && globalLogger.file != nil {
		if err := globalLogger.file.Close(); err != nil {
			fmt.Fprintf(os.Stderr, "logger: failed to close log file: %v\n", err)
		}
		globalLogger.file = nil
	}
	globalLogger = nil
	initialized = false
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
