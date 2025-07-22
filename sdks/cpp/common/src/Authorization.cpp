/*
 * Copyright 2024 Ross Video Ltd
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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

#include <Authorization.h>

using catena::common::Authorizer;

// initialize the disabled authorization object with private constructor.
Authorizer Authorizer::kAuthzDisabled;

Authorizer::Authorizer(const std::string& JWSToken) {
    try {
        // Decoding the token and extracting scopes.
        jwt::decoded_jwt<jwt::traits::kazuho_picojson> decodedToken = jwt::decode(JWSToken);

        auto claims = decodedToken.get_payload_json();
        for (picojson::value::object::const_iterator it = claims.begin(); it != claims.end(); ++it) {
            if (it->first == "scope"){
                std::string scopeClaim = it->second.get<std::string>();
                std::istringstream iss(scopeClaim);
                while (std::getline(iss, scopeClaim, ' ')) {
                    clientScopes_.insert(scopeClaim);
                }
            }
        }
    // Catch error.
    } catch (...) {
        throw catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    }
}

bool Authorizer::hasAuthz_(const std::string& scope) const {
    // no authorization required or scope found in client scopes.
    return this == &kAuthzDisabled || clientScopes_.contains(scope);
}

/**
 * Check if the client has write authorization
 */
bool Authorizer::writeAuthz(const std::string& scope) const {
    return hasAuthz_(scope + ":w");
}

bool Authorizer::writeAuthz(const IParam& param) const {
    return !param.readOnly() && writeAuthz(param.getScope());
}

bool Authorizer::writeAuthz(const IParamDescriptor& pd) const {
    return !pd.readOnly() && writeAuthz(pd.getScope());
}

/*
 * Check if the client has read authorization
 */
bool Authorizer::readAuthz(const std::string& scope) const {
    return hasAuthz_(scope) || writeAuthz(scope);
}

bool Authorizer::readAuthz(const IParam& param) const {
    return readAuthz(param.getScope());
}

bool Authorizer::readAuthz(const IParamDescriptor& pd) const {
    return readAuthz(pd.getScope());
}
