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
 * @brief Opt-in CORS middleware for the Catena REST transport.
 * @file cors.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-09-14
 */

package rest

import (
	"net/http"
	"slices"
	"strconv"
	"strings"
	"time"
)

// SDK-owned CORS baselines. Operators append via ExtraAllowed* only.
var requiredMethods = []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"}
var requiredHeaders = []string{"Content-Type", "Authorization", "Accept", "Language", "Detail-Level"}

// withCORS wraps next with opt-in CORS. Empty AllowedOrigins leaves the
// handler unchanged so a gateway that already injects CORS is not doubled.
func (t *Transport) withCORS(next http.Handler) http.Handler {
	if len(t.allowedOrigins) == 0 {
		return next
	}

	methods := strings.Join(unionCORSMethods(requiredMethods, t.extraAllowedMethods), ", ")
	headers := strings.Join(unionCORSHeaders(requiredHeaders, t.extraAllowedHeaders), ", ")
	maxAge := strconv.FormatInt(int64(t.corsMaxAge/time.Second), 10)

	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		applyCORSOrigin(w, t.allowedOrigins, r.Header.Get("Origin"))
		// Handle preflight requests
		if r.Method == http.MethodOptions {
			w.Header().Set("Access-Control-Allow-Methods", methods)
			w.Header().Set("Access-Control-Allow-Headers", headers)
			w.Header().Set("Access-Control-Max-Age", maxAge)
			w.WriteHeader(http.StatusNoContent)
			return
		}
		// Handle actual requests
		next.ServeHTTP(w, r)
	})
}

func applyCORSOrigin(w http.ResponseWriter, allowed []string, origin string) {
	switch {
	case origin == "" || origin == "null":
		return
	case slices.Contains(allowed, "*"):
		w.Header().Set("Access-Control-Allow-Origin", "*")
	case slices.Contains(allowed, origin):
		w.Header().Set("Access-Control-Allow-Origin", origin)
		w.Header().Add("Vary", "Origin")
	}
}

func unionCORSMethods(required, extra []string) []string {
	return unionCORSTokens(required, extra, true)
}

func unionCORSHeaders(required, extra []string) []string {
	return unionCORSTokens(required, extra, false)
}

// unionCORSTokens concatenates required then extra, dropping empty tokens and
// case-insensitive duplicates. Methods are stored upper-cased; headers keep
// first-seen casing.
func unionCORSTokens(required, extra []string, upper bool) []string {
	seen := make(map[string]struct{}, len(required)+len(extra))
	out := make([]string, 0, len(required)+len(extra))
	for _, item := range append(append([]string{}, required...), extra...) {
		item = strings.TrimSpace(item)
		if item == "" {
			continue
		}
		key := strings.ToLower(item)
		if _, ok := seen[key]; ok {
			continue
		}
		seen[key] = struct{}{}
		if upper {
			out = append(out, strings.ToUpper(item))
		} else {
			out = append(out, item)
		}
	}
	return out
}
