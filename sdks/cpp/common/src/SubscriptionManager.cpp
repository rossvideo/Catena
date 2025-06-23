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
using catena::common::SubscriptionManager;

// Add a subscription (unique or wildcard)
bool SubscriptionManager::addSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc, Authorizer& authz) {
    std::lock_guard sg(mtx_);
    rc = catena::exception_with_status{"", catena::StatusCode::OK};
    bool result = false;

    // Resource limit checks
    if (checkResourceLimits_(oid, dm, rc)) {
        bool wildcard = isWildcard(oid);
        std::string baseOid;
        std::unique_ptr<IParam> param = nullptr;

        // Getting base Oid if wildcard.
        if (wildcard) {
            baseOid = oid.substr(0, oid.length() - 2);
        } else {
            baseOid = oid;
        }

        // Making sure the oid exists unless client is subbing to all params.
        if (!wildcard || oid != "/*") {
            std::lock_guard lg(dm.mutex());
            param = dm.getParam(baseOid, rc, authz);
        }

        // Normal case.
        if (!wildcard && param) {
            // Add the subscription if it doesn't already exist.
            if (!subscriptions_.contains(baseOid)) {
                subscriptions_.insert(baseOid);
                result = true;  // Success
            } else {
                rc = catena::exception_with_status("Subscription already exists for OID: " + baseOid, catena::StatusCode::ALREADY_EXISTS);
                result = false;
            }
        
        // Add all sub params case.
        } else if (wildcard && param) {
            SubscriptionVisitor visitor(subscriptions_);
            ParamVisitor::traverseParams(param.get(), baseOid, dm, visitor);
            result = true;

        // Add all params case.
        } else if (wildcard && oid == "/*") {
            std::vector<std::unique_ptr<IParam>> allParams;
            {
                std::lock_guard lg(dm.mutex());
                allParams = dm.getTopLevelParams(rc);
            }
            for (auto& param : allParams) {
                if (authz.readAuthz(*param)) {
                    SubscriptionVisitor visitor(subscriptions_);
                    ParamVisitor::traverseParams(param.get(), "/" + param->getOid(), dm, visitor);
                }
            }
            result = true;
        }
    }

    return result && (rc.status == catena::StatusCode::OK);
}

// Remove a subscription (unique or wildcard)
bool SubscriptionManager::removeSubscription(const std::string& oid, const IDevice& dm, exception_with_status& rc) {
    std::lock_guard sg(mtx_);
    rc = catena::exception_with_status{"", catena::StatusCode::OK};
    bool result = false;

    // Wildcard case.
    if (isWildcard(oid)) {
        // Expand wildcard and remove all matching OIDs
        std::string basePath = oid.substr(0, oid.length() - 2);
        bool found = false;
        auto it = subscriptions_.begin();
        while (it != subscriptions_.end()) {
            if (it->starts_with(basePath)) {
                it = subscriptions_.erase(it);
                found = true;
            } else {
                ++it;
            }
        }
        if (found) {
            result = true;
        } else {
            rc = catena::exception_with_status("Subscription not found for OID: " + oid, catena::StatusCode::NOT_FOUND);
        }
    // Normal case.
    } else {
        auto it = subscriptions_.find(oid);
        if (it != subscriptions_.end()) {
            subscriptions_.erase(it);
            result = true;
        } else {
            rc = catena::exception_with_status("Subscription not found for OID: " + oid, catena::StatusCode::NOT_FOUND);
        }
    }

    return result && (rc.status == catena::StatusCode::OK);
}

// Get all subscribed OIDs
std::set<std::string> SubscriptionManager::getAllSubscribedOids(const IDevice& dm) {
    std::lock_guard sg(mtx_);
    return subscriptions_;
}

// Returns true if the OID ends with "/*", indicating it's a wildcard subscription
bool SubscriptionManager::isWildcard(const std::string& oid) const {
    return oid.length() >= 3 && oid.substr(oid.length() - 2) == "/*";
}

bool SubscriptionManager:: isSubscribed(const std::string& oid, const IDevice& dm) {
    std::lock_guard sg(mtx_);
    return subscriptions_.contains(oid);
}

// Check if adding a subscription would exceed resource limits
bool SubscriptionManager::checkResourceLimits_(const std::string& oid, const IDevice& dm, exception_with_status& rc) const {
    // Check total subscription count - use device's default max length as the limit
    size_t maxSubscriptions = dm.default_max_length();
    if (subscriptions_.size() >= maxSubscriptions) {
        rc = catena::exception_with_status("Maximum number of subscriptions (" + std::to_string(maxSubscriptions) + ") exceeded", catena::StatusCode::RESOURCE_EXHAUSTED);
        return false;
    }

    // Check wildcard expansion limits - use device's default total length as the limit
    if (isWildcard(oid)) {
        size_t maxWildcardExpansion = dm.default_total_length();
        size_t actualExpansion = countWildcardExpansion_(oid, dm);
        if (actualExpansion > maxWildcardExpansion) {
            rc = catena::exception_with_status("Wildcard expansion would exceed limit (" + std::to_string(maxWildcardExpansion) + "): would expand to " + std::to_string(actualExpansion) + " OIDs", catena::StatusCode::RESOURCE_EXHAUSTED);
            return false;
        }
    }

    return true;
}

// Count the actual number of OIDs that would be expanded from a wildcard
size_t SubscriptionManager::countWildcardExpansion_(const std::string& wildcardOid, const IDevice& dm) const {
    if (!isWildcard(wildcardOid)) {
        return 1; // Not a wildcard, so only 1 OID
    }

    std::string baseOid = wildcardOid.substr(0, wildcardOid.length() - 2);

    // For "/*" wildcard, count all top-level parameters
    if (wildcardOid == "/*") {
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        std::vector<std::unique_ptr<IParam>> allParams;
        {
            std::lock_guard lg(const_cast<IDevice&>(dm).mutex());
            allParams = dm.getTopLevelParams(rc);
        }
        
        size_t totalCount = 0;
        for (auto& param : allParams) {
            totalCount += ParamVisitor::countParams(param.get(), "/" + param->getOid(), const_cast<IDevice&>(dm));
        }
        return totalCount;
    }
    // For other wildcards, count parameters under the base path
    else {
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        std::unique_ptr<IParam> param = dm.getParam(baseOid, rc);
        if (param) {
            return ParamVisitor::countParams(param.get(), baseOid, const_cast<IDevice&>(dm));
        }
    }

    return 0;
}

