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

package catena

import (
	"net/http"
)

// StatusResult represents an HTTP outcome (success or error) that the server can render uniformly.
type StatusResult struct {
	Status  int    // e.g., 200, 204, 400, 404, 405, 501, 500
	Payload any    // optional body to JSON-encode on success (2xx). If nil and Status is 204, no body.
	Message string // optional error message for non-2xx statuses
}

// Convenience constructors
func OK(payload any) StatusResult { return StatusResult{Status: http.StatusOK, Payload: payload} }
func NoContent() StatusResult     { return StatusResult{Status: http.StatusNoContent} }
func BadRequest(msg string) StatusResult {
	return StatusResult{Status: http.StatusBadRequest, Message: msg}
}
func NotFound(msg string) StatusResult {
	return StatusResult{Status: http.StatusNotFound, Message: msg}
}
func MethodNotAllowed(msg string) StatusResult {
	return StatusResult{Status: http.StatusMethodNotAllowed, Message: msg}
}
func NotImplemented(msg string) StatusResult {
	return StatusResult{Status: http.StatusNotImplemented, Message: msg}
}
func InternalServerError(msg string) StatusResult {
	return StatusResult{Status: http.StatusInternalServerError, Message: msg}
}
