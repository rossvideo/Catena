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

/**
 * @file IConnect.h
 * @brief Interface for Connect classes.
 * @author Ben Whitten (benjamin.whitten@rossvideo.com)
 * @copyright Copyright (c) 2025 Ross Video
 */

#pragma once

// common
#include <IParam.h>
// Proto
#include <interface/device.pb.h>
// Std
#include <string>
#include <chrono>

using std::chrono::system_clock;

namespace catena {
namespace common {
 
/**
 * @brief Interface class for the Connect handlers
 */
class IConnect {
  public:
    /**
     * @brief Descructor
     */
    virtual ~IConnect() = default;
    /**
     * @brief Returns the connection's priority.
     */
    virtual uint32_t priority() const = 0;
    /**
     * @brief Returns the object Id.
     */
    virtual uint32_t objectId() const = 0;
    /**
     * @brief Comparison between IConnect objects which returns true if the
     * current priority is less than the priority it's being compared to.
     */
    virtual bool operator<(const IConnect& otherConnection) const = 0;
    /**
     * @brief Returns true if the call has been canceled.
     */
    virtual inline bool isCancelled() = 0;
    /**
     * @brief Forcefully shuts down the connection.
     */
    virtual void shutdown() = 0;
  
  protected:
    /**
     * @brief Updates the response message with parameter values.
     * 
     * @param oid The OID of the updated value
     * @param p The updated parameter
     * @param slot The slot number of the device containing the parameter.
     */
    virtual void updateResponse_(const std::string& oid, const IParam* p, uint32_t slot) = 0;
    /**
     * @brief Updates the response message with an ILanguagePack.
     * 
     * @param l The added ILanguagePack emitted by device.
     * @param slot The slot number of the device containing the language pack.
     */
    virtual void updateResponse_(const ILanguagePack* l, uint32_t slot) = 0;
    /**
     * @brief Crates an authorizer using the jws token.
     *
     * @param jwsToken The client's jws token to create the authorizer with.
     * @param authz True if authorization is enabled, False otherwise.
     */
    virtual void initAuthz_(const std::string& jwsToken, bool authz = false) = 0;
};
 
}; // common
}; // catena
 