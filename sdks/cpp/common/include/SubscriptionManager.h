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
 * @brief Centralized manager for parameter subscriptions in Catena
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-02
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <set>
#include <string>
#include <vector>
#include <memory>
#include <IDevice.h>
#include <IParam.h>
#include <ParamVisitor.h>
#include <ISubscriptionManager.h>

namespace catena {
namespace common {

using catena::common::IDevice;
using catena::common::IParam;

/**
 * @brief Class for managing parameter subscriptions in Catena
 */
class SubscriptionManager : public ISubscriptionManager {
public:
    /**
     * @brief Constructor
     */
    SubscriptionManager() = default;

    /**
     * @brief Add an OID subscription
     * @param oid The OID to subscribe to (can be either a unique OID like "/param" or a wildcard like "/param/*")
     * @param dm The device model to use 
     * @param rc The status code to return if the operation fails
     * @return true if the subscription was added, false if it already existed
     */
    bool addSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc) override;

    /**
     * @brief Remove an OID subscription
     * @param oid The OID to unsubscribe from (can be either a unique OID or a wildcard like "/param/*")
     * @param dm The device model to use
     * @param rc The status code to return if the operation fails
     * @return true if the subscription was removed, false if it didn't exist
     */
    bool removeSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc) override;

    /**
     * @brief Get all subscribed OIDs, including expanding wildcard subscriptions
     * @param dm The device model to use 
     * @return Reference to the vector of all subscribed OIDs
     */
    const std::set<std::string>& getAllSubscribedOids(IDevice& dm) override;

    /**
     * @brief Get all unique subscriptions
     * @return Reference to the set of unique subscriptions
     */
    const std::set<std::string>& getUniqueSubscriptions() override;

    /**
     * @brief Get all wildcard subscriptions
     * @return Reference to the set of wildcard subscriptions (OIDs ending with "/*")
     */
    const std::set<std::string>& getWildcardSubscriptions() override;

    /**
     * @brief Check if an OID is a wildcard subscription
     * @param oid The OID to check
     * @return true if the OID is greater than or equal to 2 characters and ends with "/*"
     */
    bool isWildcard(const std::string& oid) override;

private:
    /**
     * @brief Mutex for subscription data access
     */
    mutable std::mutex mtx_;

    /**
     * @brief Lock for protecting subscription data access
     */
    mutable std::unique_lock<std::mutex> subscriptionLock_{mtx_, std::defer_lock};

    /**
     * @brief Visitor class for collecting subscribed OIDs
     */
    class SubscriptionVisitor : public catena::common::IParamVisitor {
        public:
            /**
             * @brief Constructor for the SubscriptionVisitor class
             * @param oids The vector of subscribed OIDs
             */
            explicit SubscriptionVisitor(std::set<std::string>& oids) : oids_(oids) {}
            
            /**
             * @brief Visit a parameter
             * @param param The parameter to visit
             * @param path The path of the parameter
             */
            void visit(catena::common::IParam* param, const std::string& path) override {
                oids_.insert(path);
            }
            
            /**
             * @brief Visit an array
             * @param param The array to visit
             * @param path The path of the array
             * @param length The length of the array
             */
            void visitArray(catena::common::IParam* param, const std::string& path, uint32_t length) override {}

        private:
            /**
             * @brief The vector of subscribed OIDs within the visitor
             */
            std::set<std::string>& oids_;
    };


    /**
     * @brief Set of unique subscriptions
     */
    std::set<std::string> uniqueSubscriptions_; 

    /**
     * @brief Set of wildcard subscriptions
     */
    std::set<std::string> wildcardSubscriptions_;

    /**
     * @brief Vector of all subscribed OIDs
     */
    std::set<std::string> allSubscribedOids_;

    /**
     * @brief Update the combined list of all subscribed OIDs
     * @param dm The device model to use for expanding wildcard subscriptions
     */
    void updateAllSubscribedOids_(IDevice& dm);
};

} // namespace common
} // namespace catena 