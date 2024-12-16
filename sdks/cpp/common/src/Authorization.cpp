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

// placeholder for disabled authorization
static const std::vector<std::string> kAuthzDisabledScope = {""};

// initialize the disabled authorization object
Authorizer Authorizer::kAuthzDisabled = {kAuthzDisabledScope};

/**
 * @brief Check if the client has read authorization
 * @return true if the client has read authorization
 */
bool Authorizer::readAuthz(const IParam& param) const {
    if (this == &kAuthzDisabled) {
        return true; // no authorization required
    }

    const std::string& scope = param.getScope();
    if (std::find(clientScopes_.get().begin(), clientScopes_.get().end(), scope) == clientScopes_.get().end()) {
        return false;
    }
    return true;
}

bool Authorizer::readAuthz(const ParamDescriptor& pd) const {
    if (this == &kAuthzDisabled) {
        return true; // no authorization required
    }

    const std::string& scope = pd.getScope();
    if (std::find(clientScopes_.get().begin(), clientScopes_.get().end(), scope) == clientScopes_.get().end()) {
        return false;
    }
    return true;
}

/**
 * @brief Check if the client has write authorization
 * @return true if the client has write authorization
 */
bool Authorizer::writeAuthz(const IParam& param) const {
    if (param.readOnly()) {
        return false;
    }

    if (this == &kAuthzDisabled) {
        return true; // no authorization required
    }

    const std::string scope = param.getScope() + ":w";
    if (std::find(clientScopes_.get().begin(), clientScopes_.get().end(), scope) == clientScopes_.get().end()) {
        return false;
    }
    return true;
}

bool Authorizer::writeAuthz(const ParamDescriptor& pd) const {
    if (pd.readOnly()) {
        return false;
    }

    if (this == &kAuthzDisabled) {
        return true; // no authorization required
    }

    const std::string scope = pd.getScope() + ":w";
    if (std::find(clientScopes_.get().begin(), clientScopes_.get().end(), scope) == clientScopes_.get().end()) {
        return false;
    }
    return true;
}
