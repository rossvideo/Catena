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
#include <iostream>

namespace catena {
namespace common {

bool SubscriptionManager::isWildcard(const std::string& oid) {
    return oid.length() >= 2 && oid.substr(oid.length() - 2) == "/*";
}

bool SubscriptionManager::addSubscription(const std::string& oid, Device& dm) {
    exception_with_status rc{"", StatusCode::OK};

    if (isWildcard(oid)) {
        // For wildcard subscriptions, we need to handle both the wildcard and base path
        std::string baseOid = oid.substr(0, oid.length() - 2);
        auto [_, inserted] = wildcardSubscriptions_.insert(oid);
        if (!inserted) return false;

        // Add the base path to unique subscriptions to ensure we track it
        uniqueSubscriptions_.insert(baseOid);

        // Get the base parameter to traverse its children
        std::unique_ptr<IParam> baseParam;
        {
            Device::LockGuard lg(dm);
            baseParam = dm.getParam(baseOid, rc);
        }
        
        if (!baseParam) {
            std::cout << "Failed to get base parameter for wildcard subscription: " << rc.what() << std::endl;
            return false;
        }
        
        // Use the visitor pattern to collect all child OIDs
        SubscriptionVisitor visitor(allSubscribedOids_);
        traverseParams(baseParam.get(), baseOid, dm, visitor);

        return true;
    } else {
        // For non-wildcard subscriptions, just add to unique subscriptions
        auto [_, inserted] = uniqueSubscriptions_.insert(oid);
        return inserted;
    }
}

bool SubscriptionManager::removeSubscription(const std::string& oid) {
    if (isWildcard(oid)) {
        std::string baseOid = oid.substr(0, oid.length() - 2);
        
        // First remove the wildcard subscription
        if (!wildcardSubscriptions_.erase(oid)) {
            return false;
        }

        // Then remove any unique subscriptions that were under this wildcard path
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
        // For non-wildcard subscriptions, just remove from unique subscriptions
        return uniqueSubscriptions_.erase(oid) > 0;
    }
}

void SubscriptionManager::updateAllSubscribedOids_(Device& dm) {
    // Clear existing OIDs to rebuild from scratch
    allSubscribedOids_.clear();
    
    // Add all unique subscriptions first
    allSubscribedOids_.insert(allSubscribedOids_.end(), 
                             uniqueSubscriptions_.begin(), 
                             uniqueSubscriptions_.end());
    
    // Then process all wildcard subscriptions
    for (const auto& wildcardOid : wildcardSubscriptions_) {
        exception_with_status rc{"", StatusCode::OK};
        
        std::string basePath = wildcardOid.substr(0, wildcardOid.length() - 2);
        
        // Try to get the base parameter first
        std::unique_ptr<IParam> baseParam;
        {
            Device::LockGuard lg(dm);
            baseParam = dm.getParam(basePath, rc);
        }
        
        if (baseParam) {
            // If we found the base parameter, traverse its children
            SubscriptionVisitor visitor(allSubscribedOids_);
            traverseParams(baseParam.get(), basePath, dm, visitor);
        } else {
            // If base parameter not found, search through all top-level params
            std::vector<std::unique_ptr<IParam>> allParams;
            {
                Device::LockGuard lg(dm);
                allParams = dm.getTopLevelParams(rc);
            }
            
            // Look for parameters that match the wildcard pattern
            for (auto& param : allParams) {
                std::string paramOid = param->getOid();
                if (paramOid.find(basePath) == 0) {
                    // Check if this is a direct child or deeper descendant
                    if (paramOid.length() == basePath.length() || 
                        paramOid[basePath.length()] == '/') {
                        SubscriptionVisitor visitor(allSubscribedOids_);
                        traverseParams(param.get(), paramOid, dm, visitor);
                    }
                }
            }
        }
    }
}

const std::vector<std::string>& SubscriptionManager::getAllSubscribedOids(Device& dm) {
    updateAllSubscribedOids_(dm);
    return allSubscribedOids_;
}

const std::set<std::string>& SubscriptionManager::getUniqueSubscriptions() {
    return uniqueSubscriptions_;
}

const std::set<std::string>& SubscriptionManager::getWildcardSubscriptions() {
    return wildcardSubscriptions_;
}

void SubscriptionManager::SubscriptionVisitor::visit(IParam* param, const std::string& path) {
    oids_.push_back(path);
}

void SubscriptionManager::SubscriptionVisitor::visitArray(IParam* param, const std::string& path, uint32_t length) {
    // No special handling needed for arrays
}

} // namespace common
} // namespace catena 