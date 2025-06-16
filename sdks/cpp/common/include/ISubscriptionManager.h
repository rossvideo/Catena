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
 * @file ISubscriptionManager.h
 * @brief Interface for managing parameter subscriptions in Catena
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-24
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <set>
#include <string>
#include <memory>
#include <IDevice.h>
#include <IParam.h>

namespace catena {
namespace common {

using catena::common::IDevice;
using catena::common::IParam;
using catena::common::Authorizer;

/**
 * @brief Interface for managing parameter subscriptions in Catena
 */
class ISubscriptionManager {
  public:
    virtual ~ISubscriptionManager() = default;

    /**
     * @brief Add an OID subscription
     * @param oid The OID to subscribe to (can be either a unique OID like "/param" or a wildcard like "/param/*")
     * @param dm The device model to use 
     * @param rc The status code to return if the operation fails
     * @param authz The authorizer to use for checking permissions
     * @return true if the subscription was added, false if it already existed
     */
    virtual bool addSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc, Authorizer& authz = Authorizer::kAuthzDisabled) = 0;

    /**
     * @brief Remove an OID subscription
     * @param oid The OID to unsubscribe from (can be either a unique OID or a wildcard like "/param/*")
     * @param dm The device model to use
     * @param rc The status code to return if the operation fails
     * @return true if the subscription was removed, false if it didn't exist
     */
    virtual bool removeSubscription(const std::string& oid, const IDevice& dm, exception_with_status& rc) = 0;

    /**
     * @brief Get all subscribed OIDs, including expanding wildcard subscriptions
     * @param dm The device model to use 
     * @return A copy of the set of all subscribed OIDs
     * 
     * Since subManager does not expose its mutex, it is required to return a
     * copy of the set in order to avoid race conditions in the async API
     * calls.
     */
    virtual std::set<std::string> getAllSubscribedOids(const IDevice& dm) = 0;

    /**
     * @brief Check if an OID is a wildcard subscription
     * @param oid The OID to check
     * @return true if the OID is greater than or equal to 2 characters and ends with "/*"
     */
    virtual bool isWildcard(const std::string& oid) = 0;

    /**
     * @brief Check if an OID is subscribed to
     * @param dm The device model to use 
     * @param oid The OID to check
     * @return true if the OID is already subscribed to
     */
    virtual bool isSubscribed(const std::string& oid, const IDevice& dm) = 0;
};

} // namespace common
} // namespace catena 