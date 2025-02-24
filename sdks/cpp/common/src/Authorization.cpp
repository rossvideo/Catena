/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.
*/
//

/**
 * @file Authorization.cpp
 * @brief Implements the Authorizer class
 * @author John Danen
 * @date 2024-12-06
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <Authorization.h>

using catena::common::Authorizer;

// initialize the disabled authorization object
Authorizer Authorizer::kAuthzDisabled;

Authorizer::Authorizer(const std::string& JWSToken, const std::string& keycloakServer, const std::string& realm) {
    std::string jwks = fetchJWKS("https://" + keycloakServer + "/realms/" + realm + "/protocol/openid-connect/certs");
    try {
        // Decoding token, constructing verifier, and verifying the token.
        jwt::decoded_jwt<jwt::traits::kazuho_picojson> decodedToken = jwt::decode(JWSToken);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::rs256(PUBLIC_KEY, "", "", ""))
            .with_type("at+jwt")
            //.with_issuer("https://catena.rossvideo.com")
            //.with_audience("https://catena.rossvideo.com")
            .leeway(300); // 5 minutes of leeway for nbf and exp.
        verifier.verify(decodedToken); // Throws error if invalid.

        // Extracting scopes from the decoded token's claims.
        auto claims = decodedToken.get_payload_json();
        for (picojson::value::object::const_iterator it = claims.begin(); it != claims.end(); ++it) {
            if (it->first == "scope"){
                std::string scopeClaim = it->second.get<std::string>();
                std::istringstream iss(scopeClaim);
                while (std::getline(iss, scopeClaim, ' ')) {
                    clientScopes_.push_back(scopeClaim);
                }
            }
        }
        
    // Catch error thrown by verifier.verify().
    } catch (jwt::error::token_verification_exception err) {
        throw catena::exception_with_status(err.what(), catena::StatusCode::UNAUTHENTICATED);
    // Catch any other errors (probably from jwt::decode())
    } catch (...) {
        throw catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    }
}

size_t Authorizer::curlWriteFunction(char* contents, size_t size, size_t numMembers, void* output) {
    size_t realSize = size * numMembers;
    std::string *str = (std::string *)output;
    str->append(contents, size * numMembers);
    return realSize;
}

std::string Authorizer::fetchJWKS(const std::string& url) {
    // Initializing CURL.
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw catena::exception_with_status("Failed to initialize curl", catena::StatusCode::INTERNAL);
    }
    // Setting up CURL options.
    std::string jwks;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&jwks);
    // Running CURL and returning the jwks.
    CURLcode res =  curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        throw catena::exception_with_status("Failed to fetch JWKS", catena::StatusCode::INTERNAL);
    }
    return jwks;
}

bool Authorizer::hasAuthz(const std::string& scope) const {
    if (this == &kAuthzDisabled) {
        return true; // no authorization required
    }

    if (std::find(clientScopes_.begin(), clientScopes_.end(), scope) == clientScopes_.end()) {
        return false;
    }
    return true;
}

/**
 * @brief Check if the client has read authorization
 * @return true if the client has read authorization
 */
bool Authorizer::readAuthz(const IParam& param) const {
    const std::string& scope = param.getScope();
    return hasAuthz(scope) || writeAuthz(param);
}

bool Authorizer::readAuthz(const ParamDescriptor& pd) const {
    const std::string& scope = pd.getScope();
    return hasAuthz(scope) || writeAuthz(pd);
}

/**
 * @brief Check if the client has write authorization
 * @return true if the client has write authorization
 */
bool Authorizer::writeAuthz(const IParam& param) const {
    if (param.readOnly()) {
        return false;
    }

    const std::string scope = param.getScope() + ":w";
    return hasAuthz(scope);
}

bool Authorizer::writeAuthz(const ParamDescriptor& pd) const {
    if (pd.readOnly()) {
        return false;
    }

    const std::string scope = pd.getScope() + ":w";
    return hasAuthz(scope);
}
