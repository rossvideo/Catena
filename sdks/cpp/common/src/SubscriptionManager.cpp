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
bool SubscriptionManager::addSubscription(const std::string& oid, IDevice& dm, exception_with_status& rc, const IAuthorizer& authz) {
    std::lock_guard sg(mtx_);
    rc = catena::exception_with_status{"", catena::StatusCode::OK};
    bool wildcard = isWildcard(oid);
    std::string baseOid;
    std::unique_ptr<IParam> param = nullptr;
    // Getting set of subscriptions for the specified device.
    std::set<std::string>& dmSubscriptions = subscriptions_[dm.slot()];

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
        if (!dmSubscriptions.contains(baseOid)) {
            dmSubscriptions.insert(baseOid);
        // Subscription already exists.
        } else {
            rc = catena::exception_with_status("Subscription already exists for OID: " + baseOid, catena::StatusCode::ALREADY_EXISTS);
        }
    
    // Add all sub params case.
    } else if (wildcard && param) {
        SubscriptionVisitor visitor(dmSubscriptions);
        ParamVisitor::traverseParams(param.get(), baseOid, dm, visitor, authz);

    // Add all params case.
    } else if (wildcard && oid == "/*") {
        std::vector<std::unique_ptr<IParam>> allParams;
        {
            std::lock_guard lg(dm.mutex());
            allParams = dm.getTopLevelParams(rc, authz);
        }
        for (auto& param : allParams) {
            if (authz.readAuthz(*param)) {
                SubscriptionVisitor visitor(dmSubscriptions);
                ParamVisitor::traverseParams(param.get(), "/" + param->getOid(), dm, visitor, authz);
            }
        }
    }

    return rc.status == catena::StatusCode::OK;
}

// Remove a subscription (either unique or wildcard)
bool SubscriptionManager::removeSubscription(const std::string& oid, const IDevice& dm, catena::exception_with_status& rc) {
    std::lock_guard sg(mtx_);
    rc = catena::exception_with_status{"", catena::StatusCode::OK};
    // Getting set of subscriptions for the specified device.
    std::set<std::string>& dmSubscriptions = subscriptions_[dm.slot()];

    // Wildcard case.
    if (isWildcard(oid)) {
        // Expand wildcard and remove all matching OIDs
        std::string basePath = oid.substr(0, oid.length() - 2);
        bool found = false;
        auto it = dmSubscriptions.begin();
        while (it != dmSubscriptions.end()) {
            if (it->starts_with(basePath)) {
                it = dmSubscriptions.erase(it);
                found = true;
            } else {
                ++it;
            }
        }
        if (!found) {
            rc = catena::exception_with_status("Subscription not found for OID: " + oid, catena::StatusCode::NOT_FOUND);
        }
    // Normal case.
    } else {
        auto it = dmSubscriptions.find(oid);
        if (it != dmSubscriptions.end()) {
            dmSubscriptions.erase(it);
        } else {
            rc = catena::exception_with_status("Subscription not found for OID: " + oid, catena::StatusCode::NOT_FOUND);
        }
    }

    return rc.status == catena::StatusCode::OK;
}

// Get all subscribed OIDs
std::set<std::string> SubscriptionManager::getAllSubscribedOids(const IDevice& dm) {
    std::lock_guard sg(mtx_);
    return subscriptions_[dm.slot()];
}

// Returns true if the OID ends with "/*", indicating it's a wildcard subscription
bool SubscriptionManager::isWildcard(const std::string& oid) {
    return oid.length() >= 2 && oid.substr(oid.length() - 2) == "/*";
}

bool SubscriptionManager:: isSubscribed(const std::string& oid, const IDevice& dm) {
    std::lock_guard sg(mtx_);
    return subscriptions_.contains(dm.slot()) && subscriptions_[dm.slot()].contains(oid);
}
