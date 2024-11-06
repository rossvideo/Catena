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
 * @file AuthzInfo.h
 * @brief Class for handling authorization information
 * @author John R. Naylor john.naylor@rossvideo.com
 * @author John Danen
 * @date 2024-09-18
 * @copyright Copyright (c) 2024 Ross Video
 */

// common
#include <Path.h>
#include <Enums.h>
#include <ParamDescriptor.h>


namespace catena {
namespace common {

/**
 * @brief Class for handling authorization information
 */
class AuthzInfo {
    using ParamDescriptor = catena::common::ParamDescriptor;
  public:

    /**
     * @brief Construct a new AuthzInfo object
     * @param pd the ParamDescriptor of the object
     * @param scope the scope of the object
     */
    AuthzInfo(const ParamDescriptor& pd, const std::string& scope)
      : pd_{pd}, clientScope_{scope}{}

    /**
     * @brief Destroy the AuthzInfo object
     */
    virtual ~AuthzInfo() = default;

    /**
     * @brief Creates a AuthzInfo object for a sub parameter
     * @param oid the oid of the sub parameter
     * @return AuthzInfo the AuthzInfo object for the sub parameter
     */
    AuthzInfo subParamInfo(std::string oid) const {
        return AuthzInfo(pd_.getSubParam(oid), clientScope_);
    }

    /**
     * @brief Check if the client has read authorization
     * @return true if the client has read authorization
     */
    bool readAuthz() const {
        /**
         * @todo implement readAuthz
         */
        return true;
    }

    /**
     * @brief Check if the client has write authorization
     * @return true if the client has write authorization
     */
    bool writeAuthz() const {
        if (pd_.readOnly()) {
            return false;
        }
        /**
         * @todo implement writeAuthz
         */
        return true;
    }

    const catena::common::IConstraint* getConstraint() const {
        return pd_.getConstraint();
    }

  private:
    const ParamDescriptor& pd_;
    const std::string clientScope_; /**< the scope of the object */
};

} // namespace common
} // namespace catena