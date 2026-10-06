package main

import (
	"context"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	"github.com/rossvideo/catena/sdks/go/pkg/config"
)

// colorHandler is a minimal slog.Handler that writes one line per record,
// colours the level label by severity, and gives the timestamp and message
// their own fixed colours. It exists to show that the SDK accepts
// any *slog.Logger: build whatever handler you like and hand the logger to
// ServerOptions.Logger and, for connection-props, DashboardOptions.Logger.
// Production code would normally reach for
// slog.NewTextHandler, slog.NewJSONHandler, or a third-party handler instead.
//
// color is false for file output so the log file stays free of escape codes.
type colorHandler struct {
	w      io.Writer
	level  slog.Leveler
	color  bool
	attrs  []slog.Attr
	prefix string // dotted group path applied to record attributes
	mu     *sync.Mutex
}

const (
	colorReset  = "\033[0m"
	colorCyan   = "\033[36m"
	colorGreen  = "\033[32m"
	colorYellow = "\033[33m"
	colorRed    = "\033[1;31m"

	// Fixed colours for the non-level parts of the line. Gray keeps the
	// timestamp out of the way; bright white makes the message stand out from
	// the plain key=value attributes that follow it.
	colorTimestamp = "\033[90m"
	colorMessage   = "\033[97m"
)

// newExampleLogger builds the example logger from the same LoggerOptions the
// SDK convenience logger accepts. WriteToConsole colours stderr, WriteToFile
// appends the same text without colour to a timestamped file under LogDir,
// and UseJSON switches both outputs to slog JSON. Silent, or neither output,
// discards records. The returned function closes the log file.
func newExampleLogger(opts config.LoggerOptions) (*slog.Logger, func(), error) {
	if opts.Silent {
		return slog.New(slog.DiscardHandler), func() {}, nil
	}

	var handlers []slog.Handler
	var file *os.File

	if opts.WriteToConsole {
		handlers = append(handlers, newOutputHandler(os.Stderr, opts, true))
	}
	if opts.WriteToFile {
		if err := os.MkdirAll(opts.LogDir, 0755); err != nil {
			return nil, nil, fmt.Errorf("failed to create log directory: %w", err)
		}
		filename := fmt.Sprintf("%s_%s.log", time.Now().Format("20060102_150405"), opts.AppName)
		f, err := os.OpenFile(filepath.Join(opts.LogDir, filename), os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
		if err != nil {
			return nil, nil, fmt.Errorf("failed to open log file: %w", err)
		}
		file = f
		handlers = append(handlers, newOutputHandler(f, opts, false))
	}
	if len(handlers) == 0 {
		handlers = append(handlers, slog.DiscardHandler)
	}

	handler := handlers[0]
	if len(handlers) > 1 {
		handler = &fanoutHandler{handlers: handlers}
	}

	var once sync.Once
	closeLog := func() {
		once.Do(func() {
			if file == nil {
				return
			}
			if err := file.Close(); err != nil {
				fmt.Fprintf(os.Stderr, "logger: failed to close log file: %v\n", err)
			}
		})
	}
	return slog.New(handler), closeLog, nil
}

// newOutputHandler returns a JSON handler when opts.UseJSON is set, otherwise
// the colored text handler. color is ignored for JSON output.
func newOutputHandler(w io.Writer, opts config.LoggerOptions, color bool) slog.Handler {
	if opts.UseJSON {
		return slog.NewJSONHandler(w, &slog.HandlerOptions{Level: opts.Level})
	}
	return newColorHandler(w, opts.Level, color)
}

// newColorHandler returns a handler that writes to w and drops records below
// level. A nil level means Info. color enables the ANSI level, timestamp, and
// message colours.
func newColorHandler(w io.Writer, level slog.Leveler, color bool) *colorHandler {
	if level == nil {
		level = slog.LevelInfo
	}
	return &colorHandler{w: w, level: level, color: color, mu: &sync.Mutex{}}
}

func (h *colorHandler) Enabled(_ context.Context, level slog.Level) bool {
	return level >= h.level.Level()
}

func (h *colorHandler) Handle(_ context.Context, r slog.Record) error {
	var b strings.Builder
	h.writeColored(&b, colorTimestamp, r.Time.Format("15:04:05.000"))
	b.WriteByte(' ')
	h.writeColored(&b, levelColor(r.Level), r.Level.String())
	b.WriteByte(' ')
	h.writeColored(&b, colorMessage, r.Message)

	for _, a := range h.attrs {
		h.writeAttr(&b, "", a)
	}
	r.Attrs(func(a slog.Attr) bool {
		h.writeAttr(&b, h.prefix, a)
		return true
	})
	b.WriteByte('\n')

	h.mu.Lock()
	defer h.mu.Unlock()
	_, err := io.WriteString(h.w, b.String())
	return err
}

func (h *colorHandler) writeColored(b *strings.Builder, color, text string) {
	if h.color {
		b.WriteString(color)
	}
	b.WriteString(text)
	if h.color {
		b.WriteString(colorReset)
	}
}

// WithAttrs pre-renders the attributes under the current group so they appear
// on every record written through the derived handler.
func (h *colorHandler) WithAttrs(attrs []slog.Attr) slog.Handler {
	next := *h
	next.attrs = append(append([]slog.Attr(nil), h.attrs...), attrs...)
	for i := range next.attrs[len(h.attrs):] {
		a := &next.attrs[len(h.attrs)+i]
		if h.prefix != "" {
			a.Key = h.prefix + "." + a.Key
		}
	}
	return &next
}

func (h *colorHandler) WithGroup(name string) slog.Handler {
	if name == "" {
		return h
	}
	next := *h
	if h.prefix == "" {
		next.prefix = name
	} else {
		next.prefix = h.prefix + "." + name
	}
	return &next
}

func (h *colorHandler) writeAttr(b *strings.Builder, prefix string, a slog.Attr) {
	a.Value = a.Value.Resolve()
	if a.Equal(slog.Attr{}) {
		return
	}
	key := a.Key
	if prefix != "" && key != "" {
		key = prefix + "." + key
	}
	if a.Value.Kind() == slog.KindGroup {
		for _, nested := range a.Value.Group() {
			h.writeAttr(b, key, nested)
		}
		return
	}
	b.WriteByte(' ')
	b.WriteString(key)
	b.WriteByte('=')
	b.WriteString(a.Value.String())
}

func levelColor(level slog.Level) string {
	switch {
	case level >= slog.LevelError:
		return colorRed
	case level >= slog.LevelWarn:
		return colorYellow
	case level >= slog.LevelInfo:
		return colorGreen
	default:
		return colorCyan
	}
}

// fanoutHandler writes each record to every handler. Used when console and
// file output are both enabled.
type fanoutHandler struct {
	handlers []slog.Handler
}

func (f *fanoutHandler) Enabled(ctx context.Context, level slog.Level) bool {
	for _, h := range f.handlers {
		if h.Enabled(ctx, level) {
			return true
		}
	}
	return false
}

func (f *fanoutHandler) Handle(ctx context.Context, r slog.Record) error {
	var err error
	for _, h := range f.handlers {
		if !h.Enabled(ctx, r.Level) {
			continue
		}
		err = errors.Join(err, h.Handle(ctx, r.Clone()))
	}
	return err
}

func (f *fanoutHandler) WithAttrs(attrs []slog.Attr) slog.Handler {
	next := make([]slog.Handler, len(f.handlers))
	for i, h := range f.handlers {
		next[i] = h.WithAttrs(attrs)
	}
	return &fanoutHandler{handlers: next}
}

func (f *fanoutHandler) WithGroup(name string) slog.Handler {
	next := make([]slog.Handler, len(f.handlers))
	for i, h := range f.handlers {
		next[i] = h.WithGroup(name)
	}
	return &fanoutHandler{handlers: next}
}
