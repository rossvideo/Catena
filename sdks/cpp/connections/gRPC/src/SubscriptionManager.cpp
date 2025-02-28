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

bool SubscriptionManager::addExactSubscription(const std::string& oid) {
    std::lock_guard<std::mutex> lock(subscriptionMutex_);
    auto [_, inserted] = exactSubscriptions_.insert(oid);
    return inserted;
}

bool SubscriptionManager::addWildcardSubscription(const std::string& baseOid) {
    std::lock_guard<std::mutex> lock(subscriptionMutex_);
    auto [_, inserted] = wildcardSubscriptions_.insert(baseOid);
    return inserted;
}

bool SubscriptionManager::removeExactSubscription(const std::string& oid) {
    std::lock_guard<std::mutex> lock(subscriptionMutex_);
    return exactSubscriptions_.erase(oid) > 0;
}

bool SubscriptionManager::removeWildcardSubscription(const std::string& baseOid) {
    std::lock_guard<std::mutex> lock(subscriptionMutex_);
    return wildcardSubscriptions_.erase(baseOid) > 0;
}

std::vector<std::string> SubscriptionManager::getAllSubscribedOids(catena::common::Device& dm) {
    std::vector<std::string> oids;
    
    // Lock the subscription sets while accessing them
    std::lock_guard<std::mutex> lock(subscriptionMutex_);
    
    // Add exact subscriptions
    for (const auto& oid : exactSubscriptions_) {
        oids.push_back(oid);
    }
    
    // Add wildcard subscriptions (need to expand these to actual matching parameters)
    for (const auto& baseOid : wildcardSubscriptions_) {
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        std::vector<std::unique_ptr<IParam>> params;
        
        {
            // Use the LockGuard class from Device
            Device::LockGuard lg(dm);
            params = dm.getTopLevelParams(rc);
        }
        
        for (auto& param : params) {
            std::string paramOid = param->getOid();
            if (paramOid.find(baseOid) == 0) {
                oids.push_back(paramOid);
            }
        }
    }
    
    return oids;
}

const std::set<std::string>& SubscriptionManager::getExactSubscriptions() {
    return exactSubscriptions_;
}

const std::set<std::string>& SubscriptionManager::getWildcardSubscriptions() {
    return wildcardSubscriptions_;
}

std::mutex& SubscriptionManager::getSubscriptionMutex() {
    return subscriptionMutex_;
} 