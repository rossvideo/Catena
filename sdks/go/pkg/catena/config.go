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
 * @brief Configuration for the Catena SDK.
 * @file config.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-01-21
 */

package catena

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Environment represents dev or prod mode for the SDK.
type Environment string

const (
	EnvDev  Environment = "dev"  // development mode
	EnvProd Environment = "prod" // production mode
)

var currentEnv Environment = EnvProd // default to prod

// parseEnvString parses an environment string to Environment type.
func parseEnvString(s string) Environment {
	switch strings.ToLower(strings.TrimSpace(s)) {
	case "dev", "development":
		return EnvDev
	case "prod", "production":
		return EnvProd
	default:
		return EnvProd
	}
}

// IsDev returns true if the SDK is running in development mode.
func IsDev() bool {
	return currentEnv == EnvDev
}

// IsProd returns true if the SDK is running in production mode.
func IsProd() bool {
	return currentEnv == EnvProd
}

// GetEnv returns the current environment (dev or prod).
func GetEnv() Environment {
	return currentEnv
}

// SetEnv allows programmatic override of the environment.
func SetEnv(env Environment) {
	currentEnv = env
}

// Config holds all Catena SDK configuration.
type Config struct {
	Prefix string        // Environment variable prefix (e.g., "CATENA" or "MYAPP")
	Env    Environment   // dev or prod (default: prod)
	Logger logger.Config // Logger configuration
	Port   int           // HTTP server port (default: 6254)
}

// DefaultConfig returns sensible defaults with the given prefix.
func DefaultConfig(prefix string) Config {
	if prefix == "" {
		prefix = "CATENA"
	}
	return Config{
		Prefix: prefix,
		Env:    EnvProd,
		Logger: logger.DefaultConfig(),
		Port:   6254,
	}
}

// parseConfig parses all configuration from environment variables using the given prefix.
func parseConfig(prefix string) Config {
	if prefix == "" {
		prefix = "CATENA"
	}

	cfg := DefaultConfig(prefix)
	cfg.Logger = logger.ParseFromEnv(prefix)

	// Parse environment (dev/prod)
	if v := os.Getenv(prefix + "_ENV"); v != "" {
		cfg.Env = parseEnvString(v)
	}

	// Parse port
	if v := os.Getenv(prefix + "_PORT"); v != "" {
		port, err := strconv.Atoi(v)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: invalid %s_PORT value %q: %v\n", prefix, v, err)
			os.Exit(1)
		}
		if port <= 0 || port > 65535 {
			fmt.Fprintf(os.Stderr, "Error: %s_PORT value %d is out of valid range (1-65535)\n", prefix, port)
			os.Exit(1)
		}
		cfg.Port = port
	}

	return cfg
}

// parseConfigWithVerbosity parses config and applies CLI verbosity flags (-v, -vv, -vvv).
func parseConfigWithVerbosity(prefix string) Config {
	cfg := parseConfig(prefix)
	cfg.Logger = logger.ParseConfigWithVerbosity(cfg.Prefix)
	return cfg
}

// InitSDK parses configuration from environment variables and initializes all Catena SDK systems.
func InitSDK(prefix string, appName ...string) (Config, error) {
	cfg := parseConfigWithVerbosity(prefix)
	if len(appName) > 0 && appName[0] != "" {
		cfg.Logger.AppName = appName[0]
	}
	currentEnv = cfg.Env
	if err := logger.Init(cfg.Logger); err != nil {
		return Config{}, err
	}
	return cfg, nil
}

// Close cleans up all Catena SDK resources.
func Close() {
	logger.Close()
}
