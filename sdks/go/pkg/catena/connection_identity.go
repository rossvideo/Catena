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
 * @brief Connection Identity management
 * @file connection_identity.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Omar Shah (omar.shah@rossvideo.com)
 * @date 2026-11-25
 */

package catena

import (
	"errors"

	"github.com/golang-jwt/jwt/v5"
)

// connectionIdentity represents the connection identity
type connectionIdentity struct {
	sid      string
	sub      string
	clientId string
}

// kind returns the kind of connection identity
func (id connectionIdentity) kind() string {
	switch {
	case id.sid != "":
		return "sid"
	case id.sub != "" && id.clientId != "":
		return "sub"
	default:
		return "unknown"
	}
}

// deriveConnectionIdentity derives the connection identity from a JWT token.
// It returns a connectionIdentity using the token claims. The sid is used if
// available, otherwise sub and clientId are used
func deriveConnectionIdentity(token *jwt.Token) (connectionIdentity, error) {
	if token == nil || token.Claims == nil {
		return connectionIdentity{}, errors.New("token or claims are nil")
	}
	claims, ok := token.Claims.(jwt.MapClaims)
	if !ok {
		return connectionIdentity{}, errors.New("invalid token claims")
	}

	// Extract and return the connection identity if sid is available
	if sid, _ := claims["sid"].(string); sid != "" {
		return connectionIdentity{sid: sid}, nil
	}

	//  Use sub and clientId if sid is not available
	sub, _ := claims["sub"].(string)
	clientId, _ := claims["azp"].(string)
	if clientId == "" {
		// Use client_id claim if azp is not available
		clientId, _ = claims["client_id"].(string)
	}

	if sub == "" || clientId == "" {
		return connectionIdentity{}, errors.New("sub or clientId are empty")
	}

	return connectionIdentity{sub: sub, clientId: clientId}, nil
}
