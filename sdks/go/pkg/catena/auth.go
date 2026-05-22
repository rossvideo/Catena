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
 * @brief JWT validation helpers
 * @file auth.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-22
 */

package catena

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strings"
	"time"

	"github.com/MicahParks/keyfunc/v3"
	"github.com/golang-jwt/jwt/v5"
)

// JwtValidationOptions controls optional claim validation and HTTP behavior.
type JwtValidationOptions struct {
	AllowedAlgs       []string
	Audience          string
	Issuer            string
	Leeway            time.Duration
	ValidateSignature bool

	Http *http.Client
}

type jwtValidator struct {
	options    JwtValidationOptions
	keyfunc    jwt.Keyfunc
	validateFn func(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error)
}

type jwtValidatorInterface interface {
	validateJwt(tokenString string) (*jwt.Token, error)
}

func newJwtValidator(ctx context.Context, opts JwtValidationOptions) (jwtValidatorInterface, error) {
	v := &jwtValidator{
		options: opts,
	}

	if opts.ValidateSignature {
		jwksUrl, err := discoverJWKSEndpoint(ctx, opts.Issuer, opts.Http)
		if err != nil {
			return nil, fmt.Errorf("discover jwks endpoint: %w", err)
		}

		keyFunc, err := keyfunc.NewDefaultCtx(ctx, []string{jwksUrl})
		if err != nil {
			return nil, fmt.Errorf("create keyfunc: %w", err)
		}
		v.keyfunc = keyFunc.Keyfunc
		v.validateFn = v.validateSignature
	} else {
		v.validateFn = v.validateClaims
	}

	return v, nil
}
func (v *jwtValidator) signingMethods() []string {
	if len(v.options.AllowedAlgs) == 0 {
		return []string{
			jwt.SigningMethodRS256.Alg(),
			jwt.SigningMethodRS384.Alg(),
			jwt.SigningMethodRS512.Alg(),
			jwt.SigningMethodPS256.Alg(),
			jwt.SigningMethodPS384.Alg(),
			jwt.SigningMethodPS512.Alg(),
			jwt.SigningMethodES256.Alg(),
			jwt.SigningMethodES384.Alg(),
			jwt.SigningMethodES512.Alg(),
			jwt.SigningMethodEdDSA.Alg(),
			jwt.SigningMethodHS256.Alg(),
		}
	}
	return append([]string(nil), v.options.AllowedAlgs...)
}

// discoverJWKSEndpoint resolves the JWKS URL from an OpenID Connect issuer.
func discoverJWKSEndpoint(ctx context.Context, issuer string, client *http.Client) (string, error) {
	issuer = strings.TrimRight(strings.TrimSpace(issuer), "/")
	if issuer == "" {
		return "", fmt.Errorf("issuer is required")
	}

	discoveryURL := issuer + "/.well-known/openid-configuration"
	discoveryDoc := struct {
		JwksUri string `json:"jwks_uri"`
	}{
		JwksUri: "",
	}

	req, err := http.NewRequestWithContext(ctx, http.MethodGet, discoveryURL, nil)
	if err != nil {
		return "", fmt.Errorf("build request: %w", err)
	}

	resp, err := client.Do(req)
	if err != nil {
		return "", fmt.Errorf("perform request: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode < http.StatusOK || resp.StatusCode >= http.StatusMultipleChoices {
		return "", fmt.Errorf("unexpected status %s", resp.Status)
	}

	if err := json.NewDecoder(resp.Body).Decode(&discoveryDoc); err != nil {
		return "", fmt.Errorf("decode response: %w", err)
	}
	if discoveryDoc.JwksUri == "" {
		return "", fmt.Errorf("jwks_uri not found in discovery document")
	}

	return discoveryDoc.JwksUri, nil
}

// ValidateJWT verifies a JWT signature against the provided JWKS URL.
// It fetches, caches, and auto-refreshes the JWKS as needed.
func (v *jwtValidator) validateJwt(tokenString string) (*jwt.Token, error) {
	tokenString = strings.TrimSpace(tokenString)
	if tokenString == "" {
		return nil, fmt.Errorf("jwt is required")
	}

	parseOptions := []jwt.ParserOption{jwt.WithValidMethods(v.signingMethods())}
	if v.options.Issuer != "" {
		parseOptions = append(parseOptions, jwt.WithIssuer(v.options.Issuer))
	}
	if v.options.Audience != "" {
		parseOptions = append(parseOptions, jwt.WithAudience(v.options.Audience))
	}
	if v.options.Leeway > 0 {
		parseOptions = append(parseOptions, jwt.WithLeeway(v.options.Leeway))
	}

	if v.validateFn == nil {
		return nil, fmt.Errorf("jwt validator is not properly initialized")
	}

	token, err := v.validateFn(tokenString, parseOptions)
	if err != nil {
		return nil, fmt.Errorf("validate jwt: %w", err)
	}

	return token, nil
}

func (v *jwtValidator) validateClaims(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
	claims := jwt.MapClaims{}
	parser := jwt.NewParser(parseOptions...)
	token, _, err := parser.ParseUnverified(tokenString, claims)
	if err != nil {
		return nil, err
	}
	validator := jwt.NewValidator(parseOptions...)
	if err := validator.Validate(claims); err != nil {
		return nil, err
	}
	return token, nil
}

func (v *jwtValidator) validateSignature(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
	claims := jwt.MapClaims{}
	if v.keyfunc == nil {
		return nil, fmt.Errorf("keyfunc is not initialized")
	}
	return jwt.ParseWithClaims(tokenString, claims, v.keyfunc, parseOptions...)
}
