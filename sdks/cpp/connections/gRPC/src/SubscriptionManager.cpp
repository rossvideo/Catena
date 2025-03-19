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

#include <SubscriptionManager.h>
#include <iostream>
#include <Device.h>
#include <IParam.h>
#include <Status.h>

// Use the correct namespaces
using catena::common::Device;
using catena::common::IParam;
using catena::common::Path;

namespace catena {
namespace grpc {

// Returns true if the OID ends with "/*", indicating it's a wildcard subscription
bool SubscriptionManager::isWildcard(const std::string& oid) {
    return oid.length() >= 2 && oid.substr(oid.length() - 2) == "/*";
}

// Add a subscription (either unique or wildcard). Returns true if added, false if already exists
bool SubscriptionManager::addSubscription(const std::string& oid, catena::common::Device& dm) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};

    // Check if this is a wildcard subscription
    if (isWildcard(oid)) {
        // Extract base OID (everything before the wildcard)
        std::string baseOid = oid.substr(0, oid.length() - 2); // Remove "/*"
        
        // Add the wildcard subscription
        auto [_, inserted] = wildcardSubscriptions_.insert(oid);
        if (!inserted) return false;

        // Add subscription for base parameter
        uniqueSubscriptions_.insert(baseOid);

        // Get the base parameter
        std::unique_ptr<IParam> baseParam;
        {
            catena::common::Device::LockGuard lg(dm);
            baseParam = dm.getParam(baseOid, rc);
        }
        
        if (!baseParam) {
            std::cout << "Failed to get base parameter for wildcard subscription: " << rc.what() << std::endl;
            return false;
        }
        
        //Process all child parameters of the base parameter
        SubscriptionVisitor visitor(allSubscribedOids_);
        catena::common::traverseParams(baseParam.get(), baseOid, dm, visitor);

        return true;
    } else {
        // Non-wildcard subscription
        auto [_, inserted] = uniqueSubscriptions_.insert(oid);
        return inserted;
    }
}

// Remove a subscription (either unique or wildcard). Returns true if removed, false if not found
bool SubscriptionManager::removeSubscription(const std::string& oid) {
    if (isWildcard(oid)) {
        // For wildcard removals, we need to remove all matching subscriptions
        std::string baseOid = oid.substr(0, oid.length() - 2); // Remove "/*"
        
        // Remove the wildcard subscription itself
        if (!wildcardSubscriptions_.erase(oid)) {
            return false;
        }

        // Remove all subscriptions that start with the base OID
        auto it = uniqueSubscriptions_.begin();
        while (it != uniqueSubscriptions_.end()) {
            if (it->find(baseOid) == 0) {
                it = uniqueSubscriptions_.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    } else {
        return uniqueSubscriptions_.erase(oid) > 0;
    }
}

// Update the list of all subscribed OIDs by combining unique and wildcard subscriptions
void SubscriptionManager::updateAllSubscribedOids_(catena::common::Device& dm) {
    allSubscribedOids_.clear();
    
    // Add unique subscriptions
    allSubscribedOids_.insert(allSubscribedOids_.end(), 
                             uniqueSubscriptions_.begin(), 
                             uniqueSubscriptions_.end());
    
    // Add wildcard subscriptions 
    for (const auto& wildcardOid : wildcardSubscriptions_) {
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        
        // Remove the "/*" from the wildcard OID to get base path
        std::string basePath = wildcardOid.substr(0, wildcardOid.length() - 2);
        
        // First try to get the base parameter
        std::unique_ptr<IParam> baseParam;
        {
            catena::common::Device::LockGuard lg(dm);
            baseParam = dm.getParam(basePath, rc);
        }
        
        if (baseParam) {
            // If we found the base parameter, use visitor to collect all paths
            SubscriptionVisitor visitor(allSubscribedOids_);
            catena::common::traverseParams(baseParam.get(), basePath, dm, visitor);
        } else {
            // If base parameter not found, try to find any parameters that start with this path
            std::vector<std::unique_ptr<IParam>> allParams;
            {
                catena::common::Device::LockGuard lg(dm);
                allParams = dm.getTopLevelParams(rc);
            }
            
            for (auto& param : allParams) {
                std::string paramOid = param->getOid();
                if (paramOid.find(basePath) == 0) {
                    if (paramOid.length() == basePath.length() || 
                        paramOid[basePath.length()] == '/') {
                        SubscriptionVisitor visitor(allSubscribedOids_);
                        catena::common::traverseParams(param.get(), paramOid, dm, visitor);
                    }
                }
            }
        }
    }
}

// Get all subscribed OIDs, including wildcard subscriptions
const std::vector<std::string>& SubscriptionManager::getAllSubscribedOids(catena::common::Device& dm) {
    updateAllSubscribedOids_(dm);
    return allSubscribedOids_;
}

// Get the set of unique (non-wildcard) subscriptions
const std::set<std::string>& SubscriptionManager::getUniqueSubscriptions() {
    return uniqueSubscriptions_;
}

// Get the set of wildcard subscriptions (OIDs ending with "/*")
const std::set<std::string>& SubscriptionManager::getWildcardSubscriptions() {
    return wildcardSubscriptions_;
}

} // namespace grpc
} // namespace catena 