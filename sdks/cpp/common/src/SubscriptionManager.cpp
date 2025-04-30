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
#include <IDevice.h>
#include <IParam.h>
#include <Status.h>

// Use the correct namespaces
using catena::common::IDevice;
using catena::common::IParam;
using catena::common::Path;
using catena::common::ParamVisitor;

namespace catena {
namespace common {

// Add a subscription (unique or wildcard)
bool SubscriptionManager::addSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc) {
    subscriptionLock_.lock();
    rc = catena::exception_with_status{"", catena::StatusCode::OK};
    if (isWildcard(oid)) {
        // Expand wildcard and add all matching OIDs
        std::string basePath = oid.substr(0, oid.length() - 2);
        std::unique_ptr<IParam> baseParam;
        {
            std::lock_guard lg(dm.mutex());
            baseParam = dm.getParam(basePath, rc);
        }
        if (baseParam) {
            SubscriptionVisitor visitor(subscriptions_);
            ParamVisitor::traverseParams(baseParam.get(), basePath, dm, visitor);
        } else {
            std::vector<std::unique_ptr<IParam>> allParams;
            {
                std::lock_guard lg(dm.mutex());
                allParams = dm.getTopLevelParams(rc);
            }
            for (auto& param : allParams) {
                std::string paramOid = param->getOid();
                if (paramOid.find(basePath) == 0) {
                    if (paramOid.length() == basePath.length() || 
                        paramOid[basePath.length()] == '/') {
                        SubscriptionVisitor visitor(subscriptions_);
                        ParamVisitor::traverseParams(param.get(), paramOid, dm, visitor);
                    }
                }
            }
        }
    } else {
        subscriptions_.insert(oid);
    }
    subscriptionLock_.unlock();
    return true;
}

// Remove a subscription (either unique or wildcard)
bool SubscriptionManager::removeSubscription(const std::string& oid, IDevice& dm, catena::exception_with_status& rc) {
    subscriptionLock_.lock();
    rc = catena::exception_with_status{"", catena::StatusCode::OK};
    if (isWildcard(oid)) {
        // Expand wildcard and remove all matching OIDs
        std::string basePath = oid.substr(0, oid.length() - 2);
        bool found = false;
        auto it = subscriptions_.begin();
        while (it != subscriptions_.end()) {
            if (it->find(basePath) == 0) {
                it = subscriptions_.erase(it);
                found = true;
            } else {
                ++it;
            }
        }
        if (!found) {
            rc = catena::exception_with_status("Subscription not found for OID: " + oid, 
                                             catena::StatusCode::NOT_FOUND);
            subscriptionLock_.unlock();
            return false;
        }
        subscriptionLock_.unlock();
        return true;
    } else {
        auto ait = subscriptions_.find(oid);
        if (ait != subscriptions_.end()) {
            subscriptions_.erase(ait);
            subscriptionLock_.unlock();
            return true;
        }
        rc = catena::exception_with_status("Subscription not found for OID: " + oid, 
                                         catena::StatusCode::NOT_FOUND);
        subscriptionLock_.unlock();
        return false;
    }
}

// Get all subscribed OIDs
const std::set<std::string>& SubscriptionManager::getAllSubscribedOids(IDevice& dm) {
    return subscriptions_;
}

// Returns true if the OID ends with "/*", indicating it's a wildcard subscription
bool SubscriptionManager::isWildcard(const std::string& oid) {
    return oid.length() >= 2 && oid.substr(oid.length() - 2) == "/*";
}

} // namespace common
} // namespace catena 