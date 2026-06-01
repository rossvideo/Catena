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
 * @brief Configuration for the Catena SDK.
 * @file config_test.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-04
 */

package catena

import (
	"os"
	"testing"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func TestEnvironmentConstants(t *testing.T) {
	if EnvDev != "dev" {
		t.Errorf("expected EnvDev='dev', got %q", EnvDev)
	}
	if EnvProd != "prod" {
		t.Errorf("expected EnvProd='prod', got %q", EnvProd)
	}
}

func TestParseEnvString(t *testing.T) {
	tests := []struct {
		input    string
		expected Environment
	}{
		{"dev", EnvDev},
		{"DEV", EnvDev},
		{"Dev", EnvDev},
		{"development", EnvDev},
		{"DEVELOPMENT", EnvDev},
		{"  dev  ", EnvDev},
		{"prod", EnvProd},
		{"PROD", EnvProd},
		{"Prod", EnvProd},
		{"production", EnvProd},
		{"PRODUCTION", EnvProd},
		{"  prod  ", EnvProd},
		{"", EnvProd},        // empty defaults to prod
		{"unknown", EnvProd}, // unknown defaults to prod
		{"staging", EnvProd}, // unknown defaults to prod
	}

	for _, tt := range tests {
		t.Run(tt.input, func(t *testing.T) {
			result := parseEnvString(tt.input)
			if result != tt.expected {
				t.Errorf("parseEnvString(%q) = %q, want %q", tt.input, result, tt.expected)
			}
		})
	}
}

func TestSetEnv_GetEnv(t *testing.T) {
	// Save original state
	original := GetEnv()
	defer SetEnv(original)

	// Test setting to dev
	SetEnv(EnvDev)
	if GetEnv() != EnvDev {
		t.Errorf("expected GetEnv()=EnvDev after SetEnv(EnvDev)")
	}

	// Test setting to prod
	SetEnv(EnvProd)
	if GetEnv() != EnvProd {
		t.Errorf("expected GetEnv()=EnvProd after SetEnv(EnvProd)")
	}
}

func TestIsDev_IsProd(t *testing.T) {
	// Save original state
	original := GetEnv()
	defer SetEnv(original)

	// Test dev mode
	SetEnv(EnvDev)
	if !IsDev() {
		t.Error("expected IsDev()=true when env is EnvDev")
	}
	if IsProd() {
		t.Error("expected IsProd()=false when env is EnvDev")
	}

	// Test prod mode
	SetEnv(EnvProd)
	if IsDev() {
		t.Error("expected IsDev()=false when env is EnvProd")
	}
	if !IsProd() {
		t.Error("expected IsProd()=true when env is EnvProd")
	}
}

func TestDefaultConfig(t *testing.T) {
	cfg := DefaultConfig("")
	if cfg.Prefix != "CATENA" {
		t.Errorf("expected default prefix 'CATENA', got %q", cfg.Prefix)
	}
	if cfg.Env != EnvProd {
		t.Errorf("expected default env EnvProd, got %q", cfg.Env)
	}
	if cfg.Port != 6254 {
		t.Errorf("expected default port 6254, got %d", cfg.Port)
	}
	if !cfg.AuthzEnabled {
		t.Error("expected AuthzEnabled to default to true")
	}
}

func TestDefaultConfig_CustomPrefix(t *testing.T) {
	cfg := DefaultConfig("MYAPP")
	if cfg.Prefix != "MYAPP" {
		t.Errorf("expected prefix 'MYAPP', got %q", cfg.Prefix)
	}
}

func TestDefaultConfig_LoggerSettings(t *testing.T) {
	cfg := DefaultConfig("TEST")
	defaultLogger := logger.DefaultSettings()

	if cfg.Logger.Level != defaultLogger.Level {
		t.Errorf("expected logger level %v, got %v", defaultLogger.Level, cfg.Logger.Level)
	}
}

func TestParseConfig_DefaultValues(t *testing.T) {
	// Clear any existing env vars
	os.Unsetenv("TEST_ENV")
	os.Unsetenv("TEST_PORT")
	os.Unsetenv("TEST_AUTHZ_ENABLED")

	cfg := parseConfig("TEST")
	if cfg.Prefix != "TEST" {
		t.Errorf("expected prefix 'TEST', got %q", cfg.Prefix)
	}
	if cfg.Env != EnvProd {
		t.Errorf("expected default env EnvProd, got %q", cfg.Env)
	}
	if cfg.Port != 6254 {
		t.Errorf("expected default port 6254, got %d", cfg.Port)
	}
}

func TestParseConfig_EnvVar(t *testing.T) {
	os.Setenv("TESTCFG_ENV", "dev")
	defer os.Unsetenv("TESTCFG_ENV")

	cfg := parseConfig("TESTCFG")
	if cfg.Env != EnvDev {
		t.Errorf("expected env EnvDev from env var, got %q", cfg.Env)
	}
}

func TestParseConfig_PortVar(t *testing.T) {
	os.Setenv("TESTCFG_PORT", "8080")
	defer os.Unsetenv("TESTCFG_PORT")

	cfg := parseConfig("TESTCFG")
	if cfg.Port != 8080 {
		t.Errorf("expected port 8080 from env var, got %d", cfg.Port)
	}
}

func TestParseConfig_AuthzEnabledVar(t *testing.T) {
	os.Setenv("TESTCFG_AUTHZ_ENABLED", "false")
	defer os.Unsetenv("TESTCFG_AUTHZ_ENABLED")

	cfg := parseConfig("TESTCFG")
	if cfg.AuthzEnabled {
		t.Error("expected AuthzEnabled=false from env var")
	}
}

func TestParseConfig_EmptyPrefix(t *testing.T) {
	cfg := parseConfig("")
	if cfg.Prefix != "CATENA" {
		t.Errorf("expected default prefix 'CATENA' for empty input, got %q", cfg.Prefix)
	}
}

func TestOptionsDefaults(t *testing.T) {
	opt := Options{}
	if opt.Prefix != "" {
		t.Errorf("expected empty default prefix, got %q", opt.Prefix)
	}
	if opt.AppName != "" {
		t.Errorf("expected empty default AppName, got %q", opt.AppName)
	}
}

func TestInitOptions_DefaultPrefix(t *testing.T) {
	// Clear env vars that might interfere
	os.Unsetenv("CATENA_ENV")
	os.Unsetenv("CATENA_PORT")
	os.Unsetenv("CATENA_LOG_LEVEL")
	os.Unsetenv("CATENA_LOG_FILE")

	cfg, err := InitOptions()
	if err != nil {
		t.Fatalf("InitOptions() failed: %v", err)
	}
	defer Close()

	if cfg.Prefix != "CATENA" {
		t.Errorf("expected default prefix 'CATENA', got %q", cfg.Prefix)
	}
}

func TestInitOptions_CustomPrefix(t *testing.T) {
	os.Unsetenv("MYPREFIX_ENV")
	os.Unsetenv("MYPREFIX_PORT")
	os.Unsetenv("MYPREFIX_LOG_LEVEL")
	os.Unsetenv("MYPREFIX_LOG_FILE")

	cfg, err := InitOptions(Options{Prefix: "MYPREFIX"})
	if err != nil {
		t.Fatalf("InitOptions() failed: %v", err)
	}
	defer Close()

	if cfg.Prefix != "MYPREFIX" {
		t.Errorf("expected prefix 'MYPREFIX', got %q", cfg.Prefix)
	}
}

func TestInitOptions_CustomAppName(t *testing.T) {
	os.Unsetenv("CATENA_ENV")
	os.Unsetenv("CATENA_PORT")
	os.Unsetenv("CATENA_LOG_LEVEL")
	os.Unsetenv("CATENA_LOG_FILE")

	cfg, err := InitOptions(Options{AppName: "TestApp"})
	if err != nil {
		t.Fatalf("InitOptions() failed: %v", err)
	}
	defer Close()

	if cfg.Logger.AppName != "TestApp" {
		t.Errorf("expected AppName 'TestApp', got %q", cfg.Logger.AppName)
	}
}

func TestInitOptions_SetsCurrentEnv(t *testing.T) {
	// Save original
	original := GetEnv()
	defer SetEnv(original)

	os.Setenv("TESTINIT_ENV", "dev")
	defer os.Unsetenv("TESTINIT_ENV")

	_, err := InitOptions(Options{Prefix: "TESTINIT"})
	if err != nil {
		t.Fatalf("InitOptions() failed: %v", err)
	}
	defer Close()

	if !IsDev() {
		t.Error("expected IsDev()=true after InitOptions with dev env")
	}
}

func TestClose(t *testing.T) {
	// Initialize first
	os.Unsetenv("CATENA_ENV")
	os.Unsetenv("CATENA_PORT")
	os.Unsetenv("CATENA_LOG_LEVEL")
	os.Unsetenv("CATENA_LOG_FILE")

	_, err := InitOptions()
	if err != nil {
		t.Fatalf("InitOptions() failed: %v", err)
	}

	// Should not panic
	Close()
}

func TestConfigStruct_Fields(t *testing.T) {
	cfg := Config{
		Prefix:       "TEST",
		Env:          EnvDev,
		Port:         9000,
		AuthzEnabled: true,
		Logger: logger.Settings{
			Level: logger.LevelDebug,
		},
	}

	if cfg.Prefix != "TEST" {
		t.Errorf("expected Prefix='TEST', got %q", cfg.Prefix)
	}
	if cfg.Env != EnvDev {
		t.Errorf("expected Env=EnvDev, got %q", cfg.Env)
	}
	if cfg.Port != 9000 {
		t.Errorf("expected Port=9000, got %d", cfg.Port)
	}
	if cfg.Logger.Level != logger.LevelDebug {
		t.Errorf("expected Logger.Level=LevelDebug, got %v", cfg.Logger.Level)
	}
	if !cfg.AuthzEnabled {
		t.Errorf("expected AuthzEnabled=true, got %v", cfg.AuthzEnabled)
	}
}

func TestEnvironmentType(t *testing.T) {
	// Test that Environment is a string type
	var env Environment = "custom"
	if string(env) != "custom" {
		t.Errorf("Environment should be convertible to string")
	}
}
