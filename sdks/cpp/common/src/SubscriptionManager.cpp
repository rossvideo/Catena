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
#include <Device.h>
#include <IParam.h>

namespace catena {
namespace common {

bool SubscriptionManager::addSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    
    // Check if parameter exists
    const IParam* param = dm.getParam(oid);
    if (!param) {
        rc = catena::exception_with_status("Parameter not found: " + oid, catena::StatusCode::NOT_FOUND);
        return false;
    }

    // Check if subscriptions are enabled
    if (!dm.subscriptions()) {
        rc = catena::exception_with_status("Subscriptions not enabled for device", catena::StatusCode::FAILED_PRECONDITION);
        return false;
    }

    // Add subscription
    subscriptions_[dm.id()].insert(oid);
    return true;
}

bool SubscriptionManager::removeSubscription(const std::string& oid, Device& dm, catena::exception_with_status& rc) {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    
    // Check if parameter exists
    const IParam* param = dm.getParam(oid);
    if (!param) {
        rc = catena::exception_with_status("Parameter not found: " + oid, catena::StatusCode::NOT_FOUND);
        return false;
    }

    // Check if subscriptions are enabled
    if (!dm.subscriptions()) {
        rc = catena::exception_with_status("Subscriptions not enabled for device", catena::StatusCode::FAILED_PRECONDITION);
        return false;
    }

    // Remove subscription
    auto it = subscriptions_.find(dm.id());
    if (it != subscriptions_.end()) {
        it->second.erase(oid);
        return true;
    }

    rc = catena::exception_with_status("No subscriptions found for device", catena::StatusCode::NOT_FOUND);
    return false;
}

std::vector<std::string> SubscriptionManager::getAllSubscribedOids(const Device& dm) const {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    
    auto it = subscriptions_.find(dm.id());
    if (it != subscriptions_.end()) {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    
    return std::vector<std::string>();
}

bool SubscriptionManager::isSubscribed(const std::string& oid, const Device& dm) const {
    std::lock_guard<std::mutex> lock(subscriptionsMutex_);
    
    auto it = subscriptions_.find(dm.id());
    if (it != subscriptions_.end()) {
        return it->second.find(oid) != it->second.end();
    }
    
    return false;
}

} // namespace common
} // namespace catena 