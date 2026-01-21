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
	"os"
	"strings"
)

// Environment represents dev or prod mode for the SDK.
type Environment string

const (
	EnvDev  Environment = "dev"  // development mode
	EnvProd Environment = "prod" // production mode
)

var currentEnv Environment = EnvProd // default to prod

func init() {
	if v := os.Getenv("CATENA_ENV"); v != "" {
		switch strings.ToLower(strings.TrimSpace(v)) {
		case "dev", "development":
			currentEnv = EnvDev
		case "prod", "production":
			currentEnv = EnvProd
		}
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
