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

#include <SubscriptionManager.h>
#include <Device.h>
#include <IParam.h>

namespace catena {
namespace common {

bool SubscriptionManager::isWildcard(const std::string& oid) const {
    return oid.length() >= 2 && oid.substr(oid.length() - 2) == "/*";
}

bool SubscriptionManager::addSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc, Authorizer& authz) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    rc = catena::exception_with_status{"", catena::StatusCode::OK};

    // Check if this is a wildcard subscription
    if (isWildcard(oid)) {
        // Extract base OID (everything before the wildcard)
        std::string baseOid = oid.substr(0, oid.length() - 2); // Remove "/*"
        
        // Add the wildcard subscription
        auto [_, inserted] = wildcardSubscriptions_.insert(oid);
        if (!inserted) {
            rc = catena::exception_with_status("Wildcard subscription already exists for OID: " + oid, 
                                             catena::StatusCode::ALREADY_EXISTS);
            return false;
        }

        // Add subscription for base parameter
        uniqueSubscriptions_.insert(baseOid);

        // Get the base parameter
        std::unique_ptr<IParam> baseParam = dm.getParam(baseOid, rc, authz);
        if (!baseParam) {
            rc = catena::exception_with_status("Failed to get base parameter for wildcard subscription: " + baseOid, 
                                             catena::StatusCode::NOT_FOUND);
            return false;
        }
        
        updateAllSubscribedOids_(dm);
        return true;
    } else {
        // Non-wildcard subscription
        auto [_, inserted] = uniqueSubscriptions_.insert(oid);
        if (!inserted) {
            rc = catena::exception_with_status("Subscription already exists for OID: " + oid, 
                                             catena::StatusCode::ALREADY_EXISTS);
            return false;
        }
        
        updateAllSubscribedOids_(dm);
        return true;
    }
}

bool SubscriptionManager::removeSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc, Authorizer& authz) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    rc = catena::exception_with_status{"", catena::StatusCode::OK};

    if (isWildcard(oid)) {
        // For wildcard removals, we need to remove all matching subscriptions
        std::string baseOid = oid.substr(0, oid.length() - 2); // Remove "/*"
        
        // Check if wildcard subscription exists
        if (wildcardSubscriptions_.find(oid) == wildcardSubscriptions_.end()) {
            rc = catena::exception_with_status("Wildcard subscription not found for OID: " + oid, 
                                             catena::StatusCode::NOT_FOUND);
            return false;
        }

        // Remove the wildcard subscription itself
        wildcardSubscriptions_.erase(oid);

        // Remove all subscriptions that start with the base OID
        auto it = uniqueSubscriptions_.begin();
        while (it != uniqueSubscriptions_.end()) {
            if (it->find(baseOid) == 0) {
                it = uniqueSubscriptions_.erase(it);
            } else {
                ++it;
            }
        }
        
        updateAllSubscribedOids_(dm);
        return true;
    } else {
        // Check if unique subscription exists
        if (uniqueSubscriptions_.find(oid) == uniqueSubscriptions_.end()) {
            rc = catena::exception_with_status("Subscription not found for OID: " + oid, 
                                             catena::StatusCode::NOT_FOUND);
            return false;
        }

        // Remove the subscription
        uniqueSubscriptions_.erase(oid);
        
        updateAllSubscribedOids_(dm);
        return true;
    }
}

void SubscriptionManager::updateAllSubscribedOids_(const Device& dm) {
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
        std::unique_ptr<IParam> baseParam = dm.getParam(basePath, rc);
        if (baseParam) {
            // If we found the base parameter, use visitor to collect all paths
            SubscriptionVisitor visitor(allSubscribedOids_);
            traverseParams(baseParam.get(), basePath, const_cast<Device&>(dm), visitor);
        } else {
            // If base parameter not found, try to find any parameters that start with this path
            std::vector<std::unique_ptr<IParam>> allParams = dm.getTopLevelParams(rc);
            
            for (auto& param : allParams) {
                std::string paramOid = param->getOid();
                if (paramOid.find(basePath) == 0) {
                    if (paramOid.length() == basePath.length() || 
                        paramOid[basePath.length()] == '/') {
                        SubscriptionVisitor visitor(allSubscribedOids_);
                        traverseParams(param.get(), paramOid, const_cast<Device&>(dm), visitor);
                    }
                }
            }
        }
    }
}

std::vector<std::string> SubscriptionManager::getAllSubscribedOids(const Device& dm) const {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    const_cast<SubscriptionManager*>(this)->updateAllSubscribedOids_(dm);
    return allSubscribedOids_;
}

bool SubscriptionManager::isSubscribed(const std::string& oid, const Device& dm) const {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    
    // Check unique subscriptions first
    if (uniqueSubscriptions_.find(oid) != uniqueSubscriptions_.end()) {
        return true;
    }
    
    // Check wildcard subscriptions
    for (const auto& wildcardOid : wildcardSubscriptions_) {
        std::string basePath = wildcardOid.substr(0, wildcardOid.length() - 2);
        if (oid.find(basePath) == 0) {
            if (oid.length() == basePath.length() || oid[basePath.length()] == '/') {
                return true;
            }
        }
    }
    
    return false;
}

const std::set<std::string>& SubscriptionManager::getUniqueSubscriptions() const {
    return uniqueSubscriptions_;
}

const std::set<std::string>& SubscriptionManager::getWildcardSubscriptions() const {
    return wildcardSubscriptions_;
}

} // namespace common
} // namespace catena 