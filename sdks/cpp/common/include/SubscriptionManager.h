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
 * @file SubscriptionManager.h
 * @brief Manages parameter subscriptions for devices
 * @author john.naylor@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-02
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <Device.h>
#include <IParam.h>
#include <Authorization.h>
#include <ParamVisitor.h>

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>

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
     * @param authz The authorizer to use for parameter access
     * @return true if subscription was added successfully
     */
    bool addSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc, Authorizer& authz);

    /**
     * @brief Remove a subscription for a parameter
     * @param oid The OID to unsubscribe from
     * @param dm The device model
     * @param rc Return code for error handling
     * @param authz The authorizer to use for parameter access
     * @return true if subscription was removed successfully
     */
    bool removeSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc, Authorizer& authz);

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

    /**
     * @brief Get the set of unique (non-wildcard) subscriptions
     * @return Set of unique subscription OIDs
     */
    const std::set<std::string>& getUniqueSubscriptions() const;

    /**
     * @brief Get the set of wildcard subscriptions
     * @return Set of wildcard subscription OIDs
     */
    const std::set<std::string>& getWildcardSubscriptions() const;

private:
    /**
     * @brief Internal visitor class for collecting OIDs during parameter traversal
     */
    class SubscriptionVisitor : public ParamVisitor {
        public:
            /**
             * @brief Constructor
             * @param oids Vector to store collected OIDs
             */
            explicit SubscriptionVisitor(std::vector<std::string>& oids) : oids_(oids) {}

            /**
             * @brief Visit a parameter
             * @param param The parameter to visit
             * @param path The path to the parameter
             */
            void visit(IParam* param, const std::string& path) override {
                oids_.push_back(path);
            }

            /**
             * @brief Visit an array
             * @param param The array to visit
             * @param path The path to the array
             * @param length The length of the array
             */
            void visitArray(IParam* param, const std::string& path, uint32_t length) override {
                oids_.push_back(path);
            }

        private:
            std::vector<std::string>& oids_;
    };

    /**
     * @brief Check if an OID is a wildcard subscription
     * @param oid The OID to check
     * @return true if the OID ends with "/*"
     */
    bool isWildcard(const std::string& oid) const;

    /**
     * @brief Update the list of all subscribed OIDs
     * @param dm The device model
     */
    void updateAllSubscribedOids_(const Device& dm);

    /**
     * @brief Map of device IDs to their subscribed OIDs
     */
    std::unordered_map<std::string, std::unordered_set<std::string>> subscriptions_;

    /**
     * @brief Set of unique (non-wildcard) subscriptions
     */
    std::set<std::string> uniqueSubscriptions_;

    /**
     * @brief Set of wildcard subscriptions (OIDs ending with "/*")
     */
    std::set<std::string> wildcardSubscriptions_;

    /**
     * @brief Vector of all subscribed OIDs (including wildcard expansions)
     */
    std::vector<std::string> allSubscribedOids_;

    /**
     * @brief Mutex to protect subscriptions_
     */
    mutable std::mutex subscriptionsMutex_;
};

} // namespace common
} // namespace catena 