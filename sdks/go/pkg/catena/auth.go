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
	"errors"
	"fmt"
	"io"
	"log/slog"
	"math"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"time"

	"github.com/MicahParks/jwkset"
	"github.com/MicahParks/keyfunc/v3"
	"github.com/golang-jwt/jwt/v5"

	"github.com/cenkalti/backoff/v5"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

var catenaScopes = []string{
	st2138.ScopeOp,
	st2138.ScopeCfg,
	st2138.ScopeAdm,
	st2138.ScopeMon,
}

type jwtValidator struct {
	options    JwtValidationOptions
	log        *slog.Logger
	keyfunc    jwt.Keyfunc
	validateFn func(tokenString string, parseOptions []jwt.ParserOption) (*jwt.Token, error)
}

type jwtValidatorInterface interface {
	validateJwt(tokenString string) (*jwt.Token, error)
}

// newJwtValidator creates a JWT validator based on the provided options.
// If ValidateSignature is true, it discovers the JWKS endpoint and sets up signature validation.
// If ValidateSignature is false, it only validates claims without verifying the signature.
// The logger is stored on the validator so it is available to every method
func newJwtValidator(ctx context.Context, log *slog.Logger, opts JwtValidationOptions) (jwtValidatorInterface, error) {
	// Use the default HTTP client if none was provided.
	if opts.Http == nil {
		opts.Http = http.DefaultClient
	}
	if log == nil {
		log = slog.Default()
	}
	v := &jwtValidator{
		options: opts,
		log:     log,
	}

	if opts.ValidateSignature {
		jwksKeyFunc, err := v.initializeJWTKeyFunc(ctx)
		if err != nil {
			return nil, fmt.Errorf("failed to initialize JWT keyfunc: %w", err)
		}
		v.keyfunc = jwksKeyFunc

		v.validateFn = v.validateSignatureAndClaims
	} else {
		v.validateFn = v.validateClaims
	}

	return v, nil
}
func (v *jwtValidator) signingMethods() []string {
	return v.options.ResolvedAllowedAlgs()
}

func (v *jwtValidator) initializeJWTKeyFunc(ctx context.Context) (jwt.Keyfunc, error) {

	backoffPolicy := backoff.NewExponentialBackOff()
	backoffPolicy.InitialInterval = 250 * time.Millisecond
	backoffPolicy.RandomizationFactor = 0.5
	backoffPolicy.Multiplier = 2
	backoffPolicy.MaxInterval = 5 * time.Second

	// Define the retry options
	retryOpts := []backoff.RetryOption{
		backoff.WithBackOff(backoffPolicy),
		backoff.WithNotify(func(err error, d time.Duration) {
			v.log.Warn("Failed to initialize JWT keyfunc, retrying...", slog.String("error", err.Error()), slog.Duration("next_retry_in", d))
		}),
	}

	if ctx == nil {
		return nil, fmt.Errorf("nil context provided, context is required to initialize JWT keyfunc")
	}

	discoveryCtx := ctx

	if v.options.StartupRetryMaxElapsedTime == 0 {
		// backoff treats a MaxElapsedTime of 0 as unlimited, this is mapped to a single attempt instead
		retryOpts = append(retryOpts, backoff.WithMaxTries(1))
	} else if v.options.StartupRetryMaxElapsedTime > 0 {
		// Create a context with a timeout to prevent stalled requests
		var cancel context.CancelFunc
		discoveryCtx, cancel = context.WithTimeout(ctx, v.options.StartupRetryMaxElapsedTime)
		defer cancel()

		retryOpts = append(retryOpts, backoff.WithMaxElapsedTime(v.options.StartupRetryMaxElapsedTime))
	} else {
		// Negative values are retired indefinitely, until max elapsedtime
		retryOpts = append(retryOpts, backoff.WithMaxElapsedTime(0))
	}

	// discovery and keyfun creation
	var jwksKeyFunc jwt.Keyfunc
	jwksKeyFunc, err := backoff.Retry(ctx, func() (jwt.Keyfunc, error) {
		newJwskKeyFunc, err := v.createJWTKeyFunc(ctx, discoveryCtx)
		if err != nil {
			return nil, err
		}

		return newJwskKeyFunc, nil
	}, retryOpts...)

	if err != nil {
		return nil, err
	}

	return jwksKeyFunc, nil
}

func (v *jwtValidator) createJWTKeyFunc(ctx context.Context, discoveryCtx context.Context) (jwt.Keyfunc, error) {

	jwksUrl, err := discoverJWKSEndpoint(discoveryCtx, v.options.Issuer, v.options.Http)
	if err != nil {
		return nil, fmt.Errorf("discover jwks endpoint: %w", err)
	}

	// Create a keyfunc that fetches and caches the JWKS from the discovered URL.
	// within the KeyFunc there is a background goroutine that periodically refreshes
	// the JWKS, the ctx passed to NewDefaultOverrideCtx is used to control the lifecycle of that
	// goroutine and any in-flight requests.
	noErrorReturnFirstHTTPReq := false // first fetch error should be returned
	jwksKeyFunc, err := keyfunc.NewDefaultOverrideCtx(ctx, []string{jwksUrl}, keyfunc.Override{
		NoErrorReturnFirstHTTPReq: &noErrorReturnFirstHTTPReq,
	})
	if err != nil {
		// prefix before classifying so the context survives backoff.Retry unwrapping a PermanentError
		return nil, classifyKeyfuncError(ctx, fmt.Errorf("create keyfunc: %w", err))
	}

	return jwksKeyFunc.Keyfunc, nil
}

func classifyKeyfuncError(ctx context.Context, err error) error {

	if err == nil {
		return nil
	}

	if ctx.Err() != nil {
		return backoff.Permanent(ctx.Err())
	}

	if strings.Contains(err.Error(), jwkset.ErrInvalidHTTPStatusCode.Error()) {

		resp, ok := extractJWKSResponse(err)
		if !ok {
			return err
		}

		// reuse the discovery status code rules so both fetches classify the same way
		statusErr := classifyStatusCodeError(resp)
		if statusErr == nil {
			return err
		}

		return statusErr
	}

	if _, ok := errors.AsType[*url.Error](err); ok {
		return err
	}

	if errors.Is(err, io.ErrUnexpectedEOF) {
		return err
	}

	return backoff.Permanent(err)
}

// extractJWKSResponse pulls the status code out of a jwkset.ErrInvalidHTTPStatusCode error to
// create a response object. jwkset only exposes the code in the message ("invalid HTTP status code: 503")
func extractJWKSResponse(err error) (*http.Response, bool) {
	completeStatusCodeErrorMsg := err.Error()
	errorMessage := jwkset.ErrInvalidHTTPStatusCode.Error() + ": "
	index := strings.Index(completeStatusCodeErrorMsg, errorMessage)
	if index < 0 {
		return nil, false
	}

	statusCodeString := completeStatusCodeErrorMsg[index+len(errorMessage):]

	// Validate the status code is an integer
	endIndex := 0
	for endIndex < len(statusCodeString) && statusCodeString[endIndex] >= '0' && statusCodeString[endIndex] <= '9' {
		endIndex++
	}
	if endIndex == 0 {
		return nil, false
	}

	statusCode, convErr := strconv.Atoi(statusCodeString[:endIndex])
	if convErr != nil {
		return nil, false
	}

	return &http.Response{
		StatusCode: statusCode,
		Status:     fmt.Sprintf("%d %s", statusCode, http.StatusText(statusCode)),
		Header:     http.Header{},
	}, true
}

// discoverJWKSEndpoint resolves the JWKS URL from an OpenID Connect issuer.
func discoverJWKSEndpoint(ctx context.Context, issuer string, client *http.Client) (string, error) {
	// guard against a nil client so this function is correct regardless of caller
	if client == nil {
		client = http.DefaultClient
	}

	// process issuer URL: trim whitespace and trailing slashes, and validate it's not empty
	issuer = strings.TrimRight(strings.TrimSpace(issuer), "/")
	if issuer == "" {
		return "", backoff.Permanent(fmt.Errorf("issuer is required"))
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
		return "", backoff.Permanent(fmt.Errorf("build request: %w", err))
	}

	// do it
	resp, err := client.Do(req)
	if err != nil {
		return "", classifyDiscoveryError(ctx, err)
	}
	defer resp.Body.Close()

	// Classify status code errors
	if err := classifyStatusCodeError(resp); err != nil {
		return "", err
	}

	// decode the discovery document and extract the JWKS URI
	if err := json.NewDecoder(resp.Body).Decode(&discoveryDoc); err != nil {
		return "", backoff.Permanent(fmt.Errorf("decode response: %w", err))
	}
	// if the discovery document doesn't contain a jwks_uri, we can't validate signatures
	if discoveryDoc.JwksUri == "" {
		return "", backoff.Permanent(fmt.Errorf("jwks_uri not found in discovery document"))
	}

	return discoveryDoc.JwksUri, nil
}

func classifyStatusCodeError(resp *http.Response) error {
	// check for a error response status
	// 4xx errors are permanent, do not retry except for 408 and 429
	// 429 honours Retry-After when present; otherwise the exponential backoff is used
	// 1xx, 3xx, and 5xx erros are retriable

	switch {
	case resp.StatusCode == http.StatusRequestTimeout:
		return fmt.Errorf("unexpected status %s", resp.Status)

	case resp.StatusCode == http.StatusTooManyRequests:
		return parseTooManyRequestsHeader(resp)

	case resp.StatusCode >= http.StatusBadRequest && resp.StatusCode < http.StatusInternalServerError:
		return backoff.Permanent(fmt.Errorf("Client error: %s", resp.Status))

	case resp.StatusCode >= http.StatusOK && resp.StatusCode < http.StatusMultipleChoices:
		return nil
	default:
		return fmt.Errorf("unexpected status %s", resp.Status)
	}

}

func parseTooManyRequestsHeader(resp *http.Response) error {
	statusErr := fmt.Errorf("unexpected status %s", resp.Status)
	delay, ok := extractRetryAfterDuration(resp.Header.Get("Retry-After"))
	if !ok {
		return statusErr
	}

	retrySecond := int(delay / time.Second)
	if delay%time.Second != 0 {
		retrySecond++
	}
	return fmt.Errorf("%w: %w", statusErr, backoff.RetryAfter(retrySecond))
}

func extractRetryAfterDuration(retryValue string) (time.Duration, bool) {

	retryValue = strings.TrimSpace(retryValue)
	if retryValue == "" {
		return 0, false
	}

	if delaySeconds, err := strconv.ParseInt(retryValue, 10, 64); err == nil {
		if delaySeconds < 0 {
			return 0, false
		}

		if delaySeconds > (math.MaxInt64 / int64(time.Second)) {
			return 0, false
		}

		return time.Duration(delaySeconds) * time.Second, true
	}

	if parsedTime, err := time.Parse(http.TimeFormat, retryValue); err == nil {
		delaySeconds := time.Until(parsedTime)
		if delaySeconds < 0 {
			return 0, false
		}

		return delaySeconds, true
	}

	return 0, false
}

func classifyDiscoveryError(ctx context.Context, err error) error {
	if err == nil {
		return nil
	}

	doErr := fmt.Errorf("perform request: %w", err)

	if ctx.Err() != nil {
		return backoff.Permanent(ctx.Err())
	}

	// http.client.Do wraps transport errors in url.Error
	// those are treated as retriable
	if _, ok := errors.AsType[*url.Error](doErr); ok {
		return doErr
	}

	return backoff.Permanent(doErr)
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
func extractTokenScopes(token *jwt.Token) (map[string]struct{}, map[string]struct{}) {
	readScopes := make(map[string]struct{})
	writeScopes := make(map[string]struct{})
	claims, ok := token.Claims.(jwt.MapClaims)
	if !ok {
		return readScopes, writeScopes
	}

	scopeClaim, ok := claims["scope"]
	if !ok {
		return readScopes, writeScopes
	}

	scopeString, _ := scopeClaim.(string)
	for scopeName := range strings.FieldsSeq(scopeString) {
		if strings.HasSuffix(scopeName, ":w") {
			scopeName = strings.TrimSuffix(scopeName, ":w")
			writeScopes[scopeName] = struct{}{}
		}
		readScopes[scopeName] = struct{}{}
	}

	return readScopes, writeScopes
}
