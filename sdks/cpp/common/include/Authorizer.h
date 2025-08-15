#pragma once

/*
 * Copyright 2025 Ross Video Ltd
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

/**
 * @file Authorizer.h
 * @brief Class for handling authorization information
 * @author John R. Naylor john.naylor@rossvideo.com
 * @author John Danen
 * @date 2024-09-18
 * @author Ben Whitten benjamin.whitten@rossvideo.com
 * @author Zuhayr Sarker zuhayr.sarker@rossvideo.com
 * @date 2025-02-20
 * @copyright Copyright (c) 2025 Ross Video
 */

// common
#include <IAuthorizer.h>

#include <jwt-cpp/jwt.h>
#include <functional>
#include <set>

/**
 * @brief top level namespace for Catena. Functionality at this scope includes
 * the protoc generated classes.
 * Most everything else is in child namespaces such as common, meta, etc.
 */
namespace catena {
/**
 * @brief Functionality that can be shared between the different connection
 * management types
 */
namespace common {

/**
 * @brief Class for handling authorization information.
 * 
 * This class takes a valid JWS token and extracts the "scope" and "exp" claims
 * to later check client access.
 * The actual authentication portion of the JWS token is handled externally and
 * the class just assumes that the token is good when it recieves it.
 */
class Authorizer : public IAuthorizer {
  public:
    /**
     * @brief The scopes of the object
     */
    using ClientScopes = std::set<std::string>;
    /**
     * @brief Special Authorizer object that disables authorization
     */
    static Authorizer kAuthzDisabled;

    /**
     * @brief Constructs a new Authorizer object by extracting the "scope" and
     * "exp" claims from the JWS token.
     * from a JWS token.
     * 
     * The JWS token is assumed to be valid as authentication is handled
     * externally.
     * @param JWSToken The JWS token to extract scopes from.
     * @throw Throws an catena::exception_with_status UNAUTHENTICATED if the
     * authorizer fails to decoded the JWS Token.
     */
    Authorizer(const std::string& JWSToken);

    /**
     * @brief Authorizer does not have copy semantics 
     */
    Authorizer(const Authorizer&) = delete;
    Authorizer& operator=(const Authorizer&) = delete;

    /**
     * @brief Authorizer has move semantics
     */
    Authorizer(Authorizer&&) = default;
    Authorizer& operator=(Authorizer&&) = default;

    /**
     * @brief Destroy the Authorizer object
     */
    virtual ~Authorizer() = default;

    /**
     * @brief Check if the client has read authorization for a param
     * @param param The param to check for authorization
     * @return true if the client has read authorization
     */
    bool readAuthz(const IParam& param) const override;

    /**
     * @brief Check if the client has read authorization for a param descriptor
     * @param pd The param descriptor to check for authorization
     * @return true if the client has read authorization
     */
    bool readAuthz(const IParamDescriptor& pd) const override;

    /**
     * @brief Check if the client has read authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has read authorization
     */
    bool readAuthz(const std::string& scope) const override;

    /**
     * @brief Check if the client has read authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has read authorization
     */
    bool readAuthz(const Scopes_e& scope) const override;

    /**
     * @brief Check if the client has write authorization for a param
     * @param param The param to check for authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz(const IParam& param) const override;

    /**
     * @brief Check if the client has write authorization for a param
     * descriptor
     * @param pd The param descriptor to check for authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz(const IParamDescriptor& pd) const override;

    /**
     * @brief Check if the client has write authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz(const std::string& scope) const override;

    /**
     * @brief Check if the client has read authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has read authorization
     */
    bool writeAuthz(const Scopes_e& scope) const override;
    /**
     * @brief Returns true if the JWS token is expired.
     */
    bool isExpired() const override;

  protected:
	  /**
     * @brief Constructor for kAuthzDisabled authorizer. 
     */
    Authorizer() : clientScopes_{{""}} {}
    /**
     * @brief Checks if the client has the specified authorization
     * @param scope The scope to check for authorization
     * @return true if the client has the specified authorization
     */
    bool hasAuthz_(const std::string& scope) const;
    /**
     * @brief The exp or expiry date field from the JWS token. 0 by default.
     */
    uint32_t exp_{0};
    /**
     * @brief Client scopes extracted from a valid JWS token.
     */
  	ClientScopes clientScopes_;
};

} // namespace common
} // namespace catena
