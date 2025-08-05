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
 * @file IAuthorizer.h
 * @brief Interface for the Authorizer class
 * @author Ben Whitten benjamin.whitten@rossvideo.com
 * @date 25/08/01
 * @copyright Copyright (c) 2025 Ross Video
 */

#include <IParam.h>
#include <IParamDescriptor.h>
#include <Enums.h>

namespace catena {
namespace common {

/**
 * @brief Class for handling authorization information
 */
class IAuthorizer {
  public:
    IAuthorizer() = default;

    /**
     * @brief Authorizer does not have copy semantics
     */
    IAuthorizer(const IAuthorizer&) = delete;
    IAuthorizer& operator=(const IAuthorizer&) = delete;

    /**
     * @brief Authorizer has move semantics
     */
    IAuthorizer(IAuthorizer&&) = default;
    IAuthorizer& operator=(IAuthorizer&&) = default;

    /**
     * @brief Destroy the Authorizer object
     */
    virtual ~IAuthorizer() = default;

    /**
     * @brief Check if the client has read authorization for a param
     * @param param The param to check for authorization
     * @return true if the client has read authorization
     */
    virtual bool readAuthz(const IParam& param) const = 0;

    /**
     * @brief Check if the client has read authorization for a param descriptor
     * @param pd The param descriptor to check for authorization
     * @return true if the client has read authorization
     */
    virtual bool readAuthz(const IParamDescriptor& pd) const = 0;

    /**
     * @brief Check if the client has read authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has read authorization
     */
    virtual bool readAuthz(const std::string& scope) const = 0;

    /**
     * @brief Check if the client has read authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has read authorization
     */
    virtual bool readAuthz(const Scopes_e& scope) const = 0;

    /**
     * @brief Check if the client has write authorization for a param
     * @param param The param to check for authorization
     * @return true if the client has write authorization
     */
    virtual bool writeAuthz(const IParam& param) const = 0;

    /**
     * @brief Check if the client has write authorization for a param
     * descriptor
     * @param pd The param descriptor to check for authorization
     * @return true if the client has write authorization
     */
    virtual bool writeAuthz(const IParamDescriptor& pd) const = 0;

    /**
     * @brief Check if the client has write authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has write authorization
     */
    virtual bool writeAuthz(const std::string& scope) const = 0;

    /**
     * @brief Check if the client has read authorization for a scope
     * @param scope The scope to check for authorization
     * @return true if the client has read authorization
     */
    virtual bool writeAuthz(const Scopes_e& scope) const = 0;
};

} // namespace common
} // namespace catena
