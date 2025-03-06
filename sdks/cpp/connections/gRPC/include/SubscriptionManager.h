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
 * @brief Centralized manager for parameter subscriptions in gRPC connections
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-03-25
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#include <set>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <Device.h>

namespace catena {
namespace grpc {

/**
 * @brief Class for managing parameter subscriptions in gRPC connections
 */
class SubscriptionManager {
public:
    /**
     * @brief Constructor
     */
    SubscriptionManager() = default;

    /**
     * @brief Add an OID subscription
     * @param oid The OID to subscribe to
     * @return true if the subscription was added, false if it already existed
     */
    bool addSubscription(const std::string& oid);

    /**
     * @brief Add a wildcard OID subscription
     * @param baseOid The base OID without the wildcard character
     * @return true if the subscription was added, false if it already existed
     */
    bool addWildcardSubscription(const std::string& baseOid);

    /**
     * @brief Remove an OID subscription
     * @param oid The OID to unsubscribe from
     * @return true if the subscription was removed, false if it didn't exist
     */
    bool removeSubscription(const std::string& oid);

    /**
     * @brief Remove a wildcard OID subscription
     * @param baseOid The base OID without the wildcard character
     * @return true if the subscription was removed, false if it didn't exist
     */
    bool removeWildcardSubscription(const std::string& baseOid);

    /**
     * @brief Get all subscribed OIDs, including expanding wildcard subscriptions
     * @param dm The device model to use for expanding wildcard subscriptions
     * @return Vector of all subscribed OIDs
     */
    std::vector<std::string> getAllSubscribedOids(catena::common::Device& dm);

    /**
     * @brief Get all exact subscriptions
     * @return Reference to the set of exact subscriptions
     */
    const std::set<std::string>& getExactSubscriptions();

    /**
     * @brief Get all wildcard subscriptions
     * @return Reference to the set of wildcard subscriptions
     */
    const std::set<std::string>& getWildcardSubscriptions();

    /**
     * @brief Get the mutex for protecting access to the subscription sets
     * @return Reference to the subscription mutex
     */
    std::mutex& getSubscriptionMutex();

private:
    std::set<std::string> exactSubscriptions_;
    std::set<std::string> wildcardSubscriptions_;
    std::mutex subscriptionMutex_;
};

} // namespace grpc
} // namespace catena 