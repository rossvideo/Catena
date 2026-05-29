package config

import (
	"bytes"
	"encoding/json"
	"log/slog"
	"reflect"
	"testing"
)

func TestDefaultOptions(t *testing.T) {
	opts := defaultRuntimeOptions()

	if opts != (RuntimeOptions{
		Server: ServerOptions{
			IsDev:          false,
			MaxConnections: 100,
		},
		Logger: LoggerOptions{
			AppName:        "catena",
			LogDir:         "./logs",
			Silent:         false,
			Level:          slog.LevelInfo,
			WriteToFile:    true,
			WriteToConsole: true,
			UseJSON:        false,
		},
	}) {
		t.Errorf("defaultRuntimeOptions() returned unexpected value: %+v", opts)
	}
}

func TestRuntimeOptions_LogValuer(t *testing.T) {
	opts := RuntimeOptions{
		Server: ServerOptions{
			IsDev:          true,
			MaxConnections: 123,
		},
		Logger: LoggerOptions{
			AppName:        "test-app",
			Silent:         false,
			LogDir:         "/tmp/test-logs",
			Level:          slog.LevelDebug,
			WriteToFile:    true,
			WriteToConsole: true,
			UseJSON:        true,
		},
	}

	var buf bytes.Buffer

	log := slog.New(slog.NewJSONHandler(&buf, &slog.HandlerOptions{
		Level: slog.LevelDebug,
	}))

	log.Info("runtime options", "options", opts)

	var entry map[string]any
	dec := json.NewDecoder(bytes.NewReader(buf.Bytes()))
	dec.UseNumber()

	if err := dec.Decode(&entry); err != nil {
		t.Fatalf("decode slog output: %v\noutput: %s", err, buf.String())
	}

	got, ok := entry["options"].(map[string]any)
	if !ok {
		t.Fatalf("options field missing or wrong type: %#v", entry["options"])
	}

	want := map[string]any{
		"dev":             true,
		"max_connections": json.Number("123"),
		"logger": map[string]any{
			"app_name":         "test-app",
			"silent":           false,
			"log_dir":          "/tmp/test-logs",
			"level":            "DEBUG",
			"write_to_file":    true,
			"write_to_console": true,
			"use_json":         true,
		},
	}

	if !reflect.DeepEqual(got, want) {
		t.Fatalf("logged options mismatch\n got: %#v\nwant: %#v", got, want)
	}
}
