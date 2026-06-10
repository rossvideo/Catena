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

	"github.com/MicahParks/keyfunc/v3"
	"github.com/golang-jwt/jwt/v5"
)

const (
	ScopeOp  = "st2138:op"
	ScopeCfg = "st2138:cfg"
	ScopeAdm = "st2138:adm"
	ScopeMon = "st2138:mon"
)

var catenaScopes = []string{
	ScopeOp,
	ScopeCfg,
	ScopeAdm,
	ScopeMon,
}

type jwtValidator struct {
	options    JwtValidationOptions
	keyfunc    jwt.Keyfunc
	validateFn func(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error)
}

type jwtValidatorInterface interface {
	validateJwt(tokenString string) (*jwt.Token, error)
}

// newJwtValidator creates a JWT validator based on the provided options.
// If ValidateSignature is true, it discovers the JWKS endpoint and sets up signature validation.
// If ValidateSignature is false, it only validates claims without verifying the signature.
func newJwtValidator(ctx context.Context, opts JwtValidationOptions) (jwtValidatorInterface, error) {
	v := &jwtValidator{
		options: opts,
	}

	// Use the default HTTP client if none was provided.
	if v.options.Http == nil {
		v.options.Http = http.DefaultClient
	}

	// determine whether to validate signature based on options
	if opts.ValidateSignature {
		// If signature validation is enabled, we need to discover the JWKS endpoint and set up the keyfunc.
		jwksUrl, err := discoverJWKSEndpoint(ctx, opts.Issuer, opts.Http)
		if err != nil {
			return nil, fmt.Errorf("discover jwks endpoint: %w", err)
		}

		// Create a keyfunc that fetches and caches the JWKS from the discovered URL.
		// within the KeyFunc there is a background goroutine that periodically refreshes
		// the JWKS, the ctx passed to NewDefaultCtx is used to control the lifecycle of that
		// goroutine and any in-flight requests.
		keyFunc, err := keyfunc.NewDefaultCtx(ctx, []string{jwksUrl})
		if err != nil {
			return nil, fmt.Errorf("create keyfunc: %w", err)
		}

		// store the keyfunc and set the validateFn to validate both signature and claims
		v.keyfunc = keyFunc.Keyfunc
		v.validateFn = v.validateSignatureAndClaims
	} else {
		// not validating signature, just validate claims
		v.validateFn = v.validateClaims
	}

	return v, nil
}
func (v *jwtValidator) signingMethods() []string {
	return v.options.ResolvedAllowedAlgs()
}

// discoverJWKSEndpoint resolves the JWKS URL from an OpenID Connect issuer.
func discoverJWKSEndpoint(ctx context.Context, issuer string, client *http.Client) (string, error) {
	// process issuer URL: trim whitespace and trailing slashes, and validate it's not empty
	issuer = strings.TrimRight(strings.TrimSpace(issuer), "/")
	if issuer == "" {
		return "", fmt.Errorf("issuer is required")
	}

	// construct the discovery URL and fetch the OpenID Connect discovery document
	discoveryURL := issuer + "/.well-known/openid-configuration"
	// the discovery document should contain a "jwks_uri" field that tells us where to fetch
	// the JWKS for validating JWT signatures
	discoveryDoc := struct {
		JwksUri string `json:"jwks_uri"`
	}{
		JwksUri: "",
	}

	// make a request using the client and ctx to allow for cancellation and timeouts
	req, err := http.NewRequestWithContext(ctx, http.MethodGet, discoveryURL, nil)
	if err != nil {
		return "", fmt.Errorf("build request: %w", err)
	}

	// do it
	resp, err := client.Do(req)
	if err != nil {
		return "", fmt.Errorf("perform request: %w", err)
	}
	defer resp.Body.Close()

	// check for a successful response
	if resp.StatusCode < http.StatusOK || resp.StatusCode >= http.StatusMultipleChoices {
		return "", fmt.Errorf("unexpected status %s", resp.Status)
	}

	// decode the discovery document and extract the JWKS URI
	if err := json.NewDecoder(resp.Body).Decode(&discoveryDoc); err != nil {
		return "", fmt.Errorf("decode response: %w", err)
	}
	// if the discovery document doesn't contain a jwks_uri, we can't validate signatures
	if discoveryDoc.JwksUri == "" {
		return "", fmt.Errorf("jwks_uri not found in discovery document")
	}

	return discoveryDoc.JwksUri, nil
}

// ValidateJWT verifies a JWT signature against the provided JWKS URL.
// It fetches, caches, and auto-refreshes the JWKS as needed.
func (v *jwtValidator) validateJwt(tokenString string) (*jwt.Token, error) {
	// process the token string: trim whitespace and validate it's not empty
	tokenString = strings.TrimSpace(tokenString)
	if tokenString == "" {
		return nil, fmt.Errorf("jwt is required")
	}

	// build the parser options based on the validator options.
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

	// quick check to make sure this was initialized properly
	if v.validateFn == nil {
		return nil, fmt.Errorf("jwt validator is not properly initialized")
	}

	// do the validation using the approprate function
	// have the if up in the ctor means we don't have to check the options every time
	token, err := v.validateFn(tokenString, parseOptions)
	if err != nil {
		return nil, fmt.Errorf("validate jwt: %w", err)
	}

	return token, nil
}

// validateClaims parses the JWT without validating the signature, and validates the claims based on the provided parser options.
func (v *jwtValidator) validateClaims(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
	claims := jwt.MapClaims{}
	parser := jwt.NewParser(parseOptions...)
	// just parse unverifed so it doesn't check the signature
	token, _, err := parser.ParseUnverified(tokenString, claims)
	if err != nil {
		return nil, err
	}
	// then we separately validate the claims
	validator := jwt.NewValidator(parseOptions...)
	if err := validator.Validate(claims); err != nil {
		return nil, err
	}
	return token, nil
}

// validateSignatureAndClaims parses the JWT, validates the signature using the keyfunc, and validates the claims based on the provided parser options.
func (v *jwtValidator) validateSignatureAndClaims(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error) {
	claims := jwt.MapClaims{}
	if v.keyfunc == nil {
		return nil, fmt.Errorf("keyfunc is not initialized")
	}
	// super simple check, the jwt lib handles everything if we have a keyfunc
	token, err := jwt.ParseWithClaims(tokenString, claims, v.keyfunc, parseOptions...)
	if err != nil {
		return nil, err
	}
	return token, nil
}

// extractTokenScopes extracts the scopes from the token and returns them as a map of strings.
// The map is keyed by the scope name and the value is a struct{} to act as a set in Go
// The function returns the read and write scopes as separate maps.
func extractTokenScopes(token *jwt.Token) (map[string]struct{}, map[string]struct{}, StatusResult) {
	readScopes := make(map[string]struct{})
	writeScopes := make(map[string]struct{})
	claims, ok := token.Claims.(jwt.MapClaims)
	if !ok {
		return readScopes, writeScopes, StatusWithCode(StatusCodeOk, "")
	}

	scopeClaim, ok := claims["scope"]
	if !ok {
		return readScopes, writeScopes, StatusWithCode(StatusCodeOk, "")
	}

	scopeString, _ := scopeClaim.(string)
	for scopeName := range strings.FieldsSeq(scopeString) {
		if strings.HasSuffix(scopeName, ":w") {
			scopeName = strings.TrimSuffix(scopeName, ":w")
			writeScopes[scopeName] = struct{}{}
		}
		readScopes[scopeName] = struct{}{}
	}

	return readScopes, writeScopes, StatusWithCode(StatusCodeOk, "")
}
