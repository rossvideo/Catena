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

        // If base param is a STRUCT or STRUCT_ARRAY, get its children
        if (baseParam->type().value() == catena::ParamType::STRUCT || 
            baseParam->type().value() == catena::ParamType::STRUCT_ARRAY) {
            
            const auto& descriptor = baseParam->getDescriptor();
            const auto& subParams = descriptor.getAllSubParams();

            // For STRUCT_ARRAY, we need to iterate through array indices
            if (baseParam->type().value() == catena::ParamType::STRUCT_ARRAY) {
                for (size_t i = 0; i < baseParam->size(); i++) {
                    // Create path with array index
                    Path path(baseOid);
                    path.push_back(i);

                    // Get array element
                    auto arrayElem = dm.getParam(path.toString(true), rc);
                    if (!arrayElem) {
                        std::cout << "Failed to get array element at index " << i << ": " << rc.what() << std::endl;
                        continue;
                    }

                    // Add subscription for array element
                    uniqueSubscriptions_.insert(path.toString(true));

                    // Process each child of the array element
                    for (const auto& [childOid, childDesc] : subParams) {
                        Path childPath = path;
                        childPath.push_back(childOid);
                        
                        auto childParam = dm.getParam(childPath.toString(true), rc);
                        if (childParam) {
                            uniqueSubscriptions_.insert(childPath.toString(true));
                        }
                    }
                }
            } else { // STRUCT type
                // Process each child directly
                for (const auto& [childOid, childDesc] : subParams) {
                    Path childPath(baseOid);
                    childPath.push_back(childOid);

                    auto childParam = dm.getParam(childPath.toString(true), rc);
                    if (childParam) {
                        uniqueSubscriptions_.insert(childPath.toString(true));
                    }
                }
            }
        }
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
            // If we found the base parameter, add it and all its children
            allSubscribedOids_.push_back(basePath);

            // If base parameter is an array, process each element
            if (baseParam->isArrayType()) {
                uint32_t array_length = baseParam->size();
                for (uint32_t i = 0; i < array_length; i++) {
                    Path indexed_path{basePath, std::to_string(i)};
                    auto indexed_param = dm.getParam(indexed_path.toString(), rc);
                    if (indexed_param) {
                        allSubscribedOids_.push_back(indexed_path.toString());
                        processChildren_(indexed_path.toString(), indexed_param.get(), dm);
                    }
                }
            } else {
                processChildren_(basePath, baseParam.get(), dm);
            }
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
                        allSubscribedOids_.push_back(paramOid);
                        processChildren_(paramOid, param.get(), dm);
                    }
                }
            }
        }
    }
}

void SubscriptionManager::processChildren_(const std::string& parent_path, IParam* current_param, catena::common::Device& dm) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    const auto& descriptor = current_param->getDescriptor();
    
    for (const auto& [child_name, child_desc] : descriptor.getAllSubParams()) {
        if (child_name.empty() || child_name[0] == '/') continue;
        
        Path child_path{parent_path, child_name};
        auto sub_param = dm.getParam(child_path.toString(), rc);
        
        if (rc.status == catena::StatusCode::OK && sub_param) {
            allSubscribedOids_.push_back(child_path.toString());

            // If this child is an array, process its elements
            if (sub_param->isArrayType()) {
                uint32_t array_length = sub_param->size();
                for (uint32_t i = 0; i < array_length; i++) {
                    Path indexed_path{child_path.toString(), i};
                    auto indexed_param = dm.getParam(indexed_path.toString(), rc);
                    if (indexed_param) {
                        allSubscribedOids_.push_back(indexed_path.toString());
                        processChildren_(indexed_path.toString(), indexed_param.get(), dm);
                    }
                }
            } else {
                processChildren_(child_path.toString(), sub_param.get(), dm);
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