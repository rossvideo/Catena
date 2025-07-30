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
 * @author benjamin.whitten@rossvideo.com
 * @date 2025-06-11
 * @copyright Copyright © 2025 Ross Video Ltd
 */


// common
#include <IDevice.h>
#include <IParam.h>
#include <ParamVisitor.h>
#include <ISubscriptionManager.h>
#include <Authorizer.h>

// std
#include <set>
#include <string>

namespace catena {
namespace common {

/**
 * @brief Class for managing parameter subscriptions in Catena
 */
class SubscriptionManager : public ISubscriptionManager {
  public:
    /**
     * @brief Constructor with device for calculating subscription limit
     * @param device The device to calculate the maximum possible subscriptions for
     * @param authz The authorizer to use for permission checking
     */
    explicit SubscriptionManager(const IDevice& device, Authorizer& authz);

    /**
     * @brief Add an OID subscription
     * @param oid The OID to subscribe to (can be either a unique OID like "/param" or a wildcard like "/param/*")
     * @param dm The device model to use 
     * @param rc The status code to return if the operation fails
     * @param authz The authorizer to use for checking permissions
     * @return true if the subscription was added, false if it already existed
     */
    bool addSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc, Authorizer& authz = Authorizer::kAuthzDisabled) override;

    /**
     * @brief Remove an OID subscription
     * @param oid The OID to unsubscribe from (can be either a unique OID or a wildcard like "/param/*")
     * @param dm The device model to use
     * @param rc The status code to return if the operation fails
     * @return true if the subscription was removed, false if it didn't exist
     */
    bool removeSubscription(const std::string& oid, const IDevice& dm, exception_with_status& rc) override;

    /**
     * @brief Get all subscribed OIDs, including expanding wildcard subscriptions
     * @param dm The device model to use 
     * @return A copy of the set of all subscribed OIDs
     * 
     * Since subManager does not expose its mutex, it is required to return a
     * copy of the set in order to avoid race conditions in the async API
     * calls.
     */
    std::set<std::string> getAllSubscribedOids(const IDevice& dm) override;

    /**
     * @brief Check if an OID is a wildcard subscription
     * @param oid The OID to check
     * @return true if the OID is greater than or equal to 2 characters and ends with "/*"
     */
    bool isWildcard(const std::string& oid) override;

    /**
     * @brief Check if an OID is subscribed to
     * @param dm The device model to use 
     * @param oid The OID to check
     * @return true if the OID is already subscribed to
     */
    bool isSubscribed(const std::string& oid, const IDevice& dm) override;



    /**
     * @brief Get the current number of subscriptions for a device
     * @param dm The device model to use
     * @return The current number of subscriptions
     */
    uint32_t getCurrentSubscriptionCount(const IDevice& dm) const override;

    /**
     * @brief Get the maximum number of subscriptions allowed per device
     * @return The current limit
     */
    uint32_t getMaxSubscriptions() const override { return max_subscriptions_per_device_; }

  private:
    /**
     * @brief Mutex for subscription data access
     */
    mutable std::mutex mtx_;

    /**
     * @brief Maximum number of subscriptions allowed per device
     */
    uint32_t max_subscriptions_per_device_ = 0;

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
                // Skip array elements (paths ending with indices) to prevent invalid paths
                Path pathObj = Path(path);
                if (!pathObj.back_is_index()) {
                    oids_.insert(path);
                }
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
     * @brief Set of all active subscriptions (unique and expanded wildcards)
     */
    std::unordered_map<uint32_t, std::set<std::string>> subscriptions_;

    /**
     * @brief Update the combined list of all subscribed OIDs
     * @param dm The device model to use for expanding wildcard subscriptions
     */
    void updateAllSubscribedOids_(IDevice& dm);
};

} // namespace common
} // namespace catena 