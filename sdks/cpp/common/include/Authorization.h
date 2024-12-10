#pragma once

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
/**
 * @file Authorization.h
 * @brief Class for handling authorization information
 * @author John R. Naylor john.naylor@rossvideo.com
 * @author John Danen
 * @date 2024-09-18
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <Path.h>
#include <Enums.h>
#include <IParam.h>
#include <ParamDescriptor.h>


namespace catena {
namespace common {

/**
 * @brief Class for handling authorization information
 */
class Authorizer {
  public:
    /**
     * @brief Special Authorizer object that disables authorization
     */
    static Authorizer kAuthzDisabled;

    /**
     * @brief Construct a new Authorizer object
     * @param pd the ParamDescriptor of the object
     * @param scope the scope of the object
     */
    Authorizer(const std::vector<std::string>& clientScopes)
        : clientScopes_{clientScopes} {}

    Authorizer(const Authorizer&) = delete;
    Authorizer& operator=(const Authorizer&) = delete;
    Authorizer(Authorizer&&) = default;
    Authorizer& operator=(Authorizer&&) = default;

    /**
     * @brief Destroy the Authorizer object
     */
    virtual ~Authorizer() = default;

    /**
     * @brief Check if the client has read authorization
     * @return true if the client has read authorization
     */
    bool readAuthz(const IParam& param) const;

    /**
     * @brief Check if the client has read authorization
     * @return true if the client has read authorization
     */
    bool readAuthz(const ParamDescriptor& pd) const;

    /**
     * @brief Check if the client has write authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz(const IParam& param) const;

    /**
     * @brief Check if the client has write authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz(const ParamDescriptor& pd) const;

  private:
    const std::vector<std::string>& clientScopes_;
};

} // namespace common
} // namespace catena