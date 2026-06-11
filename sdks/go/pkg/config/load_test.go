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
 * @brief tests for loading configuration for the Catena SDK.
 * @file load_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-29
 */

package config

import (
	"errors"
	"flag"
	"log/slog"
	"reflect"
	"strconv"
	"testing"
)

func TestInitOptions(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		// just test that it doesn't error and sets the options, the actual parsing is tested in loadParser
		opts, err := InitOptions("test_app", []string{})

		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		// should return default options with the app name set
		defaultOpts := defaultRuntimeOptions()
		defaultOpts.Logger.AppName = "test_app"
		if !reflect.DeepEqual(opts, defaultOpts) {
			t.Errorf("Expected options to be %v got: %v", defaultOpts, opts)
		}
	})

	t.Run("help", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"-h"})

		if err != ErrHelp {
			t.Errorf("Expected help error got: %v", err)
		}
		if !reflect.DeepEqual(opts, RuntimeOptions{}) {
			t.Errorf("Expected options to be empty got: %v", opts)
		}
	})

	t.Run("invalid flag", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"-invalid"})

		if err == nil {
			t.Errorf("Expected error got nil")
		}
		if !reflect.DeepEqual(opts, RuntimeOptions{}) {
			t.Errorf("Expected options to be empty got: %v", opts)
		}
	})

	t.Run("invalid env var", func(t *testing.T) {
		t.Setenv("CATENA_DEV_MODE", "notabool")
		opts, err := InitOptions("test_app", []string{})

		if err == nil {
			t.Errorf("Expected error got nil")
		}
		if !reflect.DeepEqual(opts, RuntimeOptions{}) {
			t.Errorf("Expected options to be empty got: %v", opts)
		}
	})

	t.Run("cli overrides env var", func(t *testing.T) {
		t.Setenv("CATENA_MAX_CONNECTIONS", "10")
		opts, err := InitOptions("test_app", []string{"--max-connections=20"})

		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.MaxConnections != 20 {
			t.Errorf("Expected MaxConnections to be 20 got: %v", opts.Server.MaxConnections)
		}
	})

	t.Run("verbosity flags set log level", func(t *testing.T) {
		tests := []struct {
			name     string
			args     []string
			expected slog.Level
		}{
			{"default", []string{}, slog.LevelInfo}, // default log level is INFO
			{"warn", []string{"-v"}, slog.LevelWarn},
			{"info", []string{"-vv"}, slog.LevelInfo},
			{"debug", []string{"-vvv"}, slog.LevelDebug},
		}
		for _, tt := range tests {
			t.Run(tt.name, func(t *testing.T) {
				opts, err := InitOptions("test_app", tt.args)
				if err != nil {
					t.Errorf("Expected no error got: %v", err)
				}
				if opts.Logger.Level != tt.expected {
					t.Errorf("Expected log level to be %v got: %v", tt.expected, opts.Logger.Level)
				}
			})
		}
	})

	t.Run("log level priority over verbosity flags", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"-vvv", "--log-level=error"})
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Logger.Level != slog.LevelError {
			t.Errorf("Expected log level to be %v got: %v", slog.LevelError, opts.Logger.Level)
		}
	})

	t.Run("authz can be disabled with explicit false", func(t *testing.T) {
		opts, err := InitOptions("test_app", []string{"--authz=false"})
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if opts.Server.AuthzEnabled {
			t.Errorf("Expected AuthzEnabled to be false")
		}
	})
}

func makeTestLoader(t *testing.T) *configLoader {
	t.Helper()
	return &configLoader{
		flags: flag.NewFlagSet("TEST", flag.ContinueOnError),
	}
}

func TestLoader_Bool(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := true
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err != nil {
			t.Errorf("Expected no error got: %v", loader.err)
		}
		if loader.flags.Lookup("test-bool") == nil {
			t.Errorf("Expected flag 'test-bool' to be registered")
		}
		if val != true {
			t.Errorf("Expected val to be true got: %v", val)
		}
	})

	t.Run("env var parsing", func(t *testing.T) {
		tests := []struct {
			input    string
			expected bool
		}{
			{"true", true},
			{"TrUe", true},
			{"1", true},
			{"yes", true},
			{"YES", true},
			{"on", true},
			{"false", false},
			{"False", false},
			{"0", false},
			{"no", false},
			{"off", false},
			{"oFF", false},
		}
		for _, tt := range tests {
			loader := makeTestLoader(t)
			t.Run(tt.input, func(t *testing.T) {
				val := false
				t.Setenv("TEST_BOOL", tt.input)
				loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
				if loader.err != nil {
					t.Errorf("Expected no error got: %v", loader.err)
				}
				if val != tt.expected {
					t.Errorf("Expected val to be %v got: %v", tt.expected, val)
				}
			})
		}
	})

	t.Run("env var parsing error", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := true
		t.Setenv("TEST_BOOL", "notabool")
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if !val {
			t.Errorf("Expected val to be unchanged at true got: %v", val)
		}
	})

	t.Run("skips parsing on existing error", func(t *testing.T) {
		loader := makeTestLoader(t)
		loader.err = strconv.ErrSyntax
		val := true
		t.Setenv("TEST_BOOL", "false")
		loader.extractBool("TEST_BOOL", "test-bool", "Test boolean flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if !val {
			t.Errorf("Expected val to be unchanged at true got: %v", val)
		}
	})
}

func TestLoader_Int(t *testing.T) {
	loader := makeTestLoader(t)
	val := 12
	t.Setenv("TEST_INT", "34")
	loader.extractInt("TEST_INT", "test-int", "Test int flag", &val)
	if loader.err != nil {
		t.Errorf("Expected no error got: %v", loader.err)
	}
	if loader.flags.Lookup("test-int") == nil {
		t.Errorf("Expected flag 'test-int' to be registered")
	}
	if val != 34 {
		t.Errorf("Expected val to be 34 got: %d", val)
	}
}

func TestLoader_String(t *testing.T) {
	loader := makeTestLoader(t)
	val := "default"
	t.Setenv("TEST_STRING", "hello")
	loader.extractString("TEST_STRING", "test-string", "Test string flag", &val)
	if loader.err != nil {
		t.Errorf("Expected no error got: %v", loader.err)
	}
	if loader.flags.Lookup("test-string") == nil {
		t.Errorf("Expected flag 'test-string' to be registered")
	}
	if val != "hello" {
		t.Errorf("Expected val to be 'hello' got: %s", val)
	}
}

func TestLoader_LogLevel(t *testing.T) {
	tests := []struct {
		input    string
		expected slog.Level
	}{
		{"debug", slog.LevelDebug},
		{"INFO", slog.LevelInfo},
		{"Warn", slog.LevelWarn},
		{"ERROR", slog.LevelError},
	}
	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			loader := makeTestLoader(t)
			val := slog.LevelInfo
			t.Setenv("TEST_LOG_LEVEL", tt.input)
			loader.extractLogLevel("TEST_LOG_LEVEL", "test-log-level", "Test log level flag", &val)
			if loader.err != nil {
				t.Errorf("Expected no error got: %v", loader.err)
			}
			if val != tt.expected {
				t.Errorf("Expected val to be %v got: %v", tt.expected, val)
			}
		})
	}

	t.Run("invalid log level", func(t *testing.T) {
		loader := makeTestLoader(t)
		val := slog.LevelInfo
		t.Setenv("TEST_LOG_LEVEL", "notalevel")
		loader.extractLogLevel("TEST_LOG_LEVEL", "test-log-level", "Test log level flag", &val)
		if loader.err == nil {
			t.Errorf("Expected error got nil")
		}
		if val != slog.LevelInfo {
			t.Errorf("Expected val to be unchanged at LevelInfo got: %v", val)
		}
	})
}

func TestLoadParser(t *testing.T) {
	t.Run("success", func(t *testing.T) {
		flags := flag.NewFlagSet("TEST", flag.ContinueOnError)
		val := 12
		err := loadParser(nil, flags, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if err != nil {
			t.Errorf("Expected no err got: %v", err)
		}
		if val != 12 {
			t.Errorf("Expected val to be 12 got: %d", val)
		}
		if flags.Lookup("test") == nil {
			t.Errorf("Expected flag 'test' to be registered")
		}
	})

	t.Run("env var parsing", func(t *testing.T) {
		flags := flag.NewFlagSet("TEST", flag.ContinueOnError)
		val := 12
		t.Setenv("TEST_ENV", "34")
		err := loadParser(nil, flags, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if err != nil {
			t.Errorf("Expected no err got: %v", err)
		}
		if val != 34 {
			t.Errorf("Expected val to be 34 got: %d", val)
		}
	})

	t.Run("env var parsing error", func(t *testing.T) {
		flags := flag.NewFlagSet("TEST", flag.ContinueOnError)
		val := 12
		t.Setenv("TEST_ENV", "notanint")
		err := loadParser(nil, flags, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if err == nil {
			t.Errorf("Expected error got nil")
		}
	})

	t.Run("skips parsing on existing error", func(t *testing.T) {
		flags := flag.NewFlagSet("TEST", flag.ContinueOnError)
		val := 12
		testErr := errors.New("test error")
		err := loadParser(testErr, flags, "TEST_ENV", "test", "Test flag", &val, strconv.Atoi)

		if err != testErr {
			t.Errorf("Expected error to be %v got: %v", testErr, err)
		}
		if val != 12 {
			t.Errorf("Expected val to be unchanged at 12 got: %d", val)
		}
	})
}

func TestParserValue(t *testing.T) {
	t.Run("new", func(t *testing.T) {
		val := 12
		parser := newParserValue(&val, strconv.Atoi)
		if parser.val != &val {
			t.Errorf("Expected val pointer to be %v got: %v", &val, parser.val)
		}
		if parser.parser == nil {
			t.Errorf("Expected parser to be set got nil")
		}
	})

	t.Run("String", func(t *testing.T) {
		val := 12
		parser := newParserValue(&val, strconv.Atoi)
		str := parser.String()
		if str != "12" {
			t.Errorf("Expected String() to return '12' got: %s", str)
		}
	})

	t.Run("String nil", func(t *testing.T) {
		parser := newParserValue[any](nil, nil)
		str := parser.String()
		if str != "" {
			t.Errorf("Expected String() to return '' got: %s", str)
		}
	})

	t.Run("Set", func(t *testing.T) {
		val := 12
		called := false
		parser := newParserValue(&val, func(s string) (int, error) {
			called = true
			if s != "input" {
				t.Errorf("Expected parser to be called with 'input' got: %s", s)
			}
			return 34, nil
		})
		err := parser.Set("input")
		if err != nil {
			t.Errorf("Expected no error got: %v", err)
		}
		if !called {
			t.Errorf("Expected parser to be called")
		}
		if val != 34 {
			t.Errorf("Expected val to be 34 got: %d", val)
		}
	})

	t.Run("Set parser error", func(t *testing.T) {
		val := 12
		parser := newParserValue(&val, func(s string) (int, error) {
			return 0, errors.New("parse error")
		})
		err := parser.Set("34")
		if err == nil {
			t.Errorf("Expected error got nil")
		}
		if val != 12 {
			t.Errorf("Expected val to be unchanged at 12 got: %d", val)
		}
	})
}
