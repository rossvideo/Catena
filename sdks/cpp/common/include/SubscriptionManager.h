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
 * @file SubscriptionManager.h
 * @brief Manages parameter subscriptions for devices
 * @author john.naylor@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-02-27
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#include <Device.h>
#include <IParam.h>

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace catena {
namespace common {

/**
 * @brief Manages parameter subscriptions for devices
 */
class SubscriptionManager {
public:
    /**
     * @brief Constructor
     */
    SubscriptionManager() = default;

    /**
     * @brief Destructor
     */
    ~SubscriptionManager() = default;

    /**
     * @brief Add a subscription for a parameter
     * @param oid The OID to subscribe to
     * @param dm The device model
     * @param rc Return code for error handling
     * @return true if subscription was added successfully
     */
    bool addSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc);

    /**
     * @brief Remove a subscription for a parameter
     * @param oid The OID to unsubscribe from
     * @param dm The device model
     * @param rc Return code for error handling
     * @return true if subscription was removed successfully
     */
    bool removeSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc);

    /**
     * @brief Get all subscribed OIDs for a device
     * @param dm The device model
     * @return Vector of subscribed OIDs
     */
    std::vector<std::string> getAllSubscribedOids(const Device& dm) const;

    /**
     * @brief Check if a parameter is subscribed
     * @param oid The OID to check
     * @param dm The device model
     * @return true if the parameter is subscribed
     */
    bool isSubscribed(const std::string& oid, const Device& dm) const;

private:
    /**
     * @brief Map of device IDs to their subscribed OIDs
     */
    std::unordered_map<std::string, std::unordered_set<std::string>> subscriptions_;

    /**
     * @brief Mutex to protect subscriptions_
     */
    mutable std::mutex subscriptionsMutex_;
};

} // namespace common
} // namespace catena 