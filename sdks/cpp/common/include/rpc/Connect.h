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
 * @file Connect.h
 * @brief Pure virtual class for Connect RPCs used to share update code.
 * @author Ben Whitten (benjamin.whitten@rossvideo.com)
 * @copyright Copyright (c) 2025 Ross Video
 */

#pragma once

// common
#include <Tags.h>
#include <Enums.h>
#include <Status.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <IDevice.h>
#include <Authorization.h>
#include <ISubscriptionManager.h>
#include "IConnect.h"

#include <interface/device.pb.h>

#include <string>
#include <condition_variable>

namespace catena {
namespace common {

/**
 * @brief Pure virtual class for Connect RPCs.
 * Contains functions for updating the response message with parameter values.
 * Can only be inherited.
 * 
 * @todo Implement subscriptionManager.
 */
class Connect : public IConnect {
  protected:
    /**
     * @brief Constructor
     */
    Connect() = delete;
    /**
     * @brief Constructor for the Connect class.
     * @param dm The device manager.
     * @param authz true if authorization is enabled, false otherwise.
     * @param jwsToken The client's JWS token.
     * @param subscriptionManager The subscription manager.
     */
    Connect(IDevice& dm, bool authz, const std::string& jwsToken, ISubscriptionManager& subscriptionManager) : 
        dm_{dm}, 
        subscriptionManager_{subscriptionManager},
        detailLevel_{catena::Device_DetailLevel_UNSET} {
        if (authz) {
            sharedAuthz_ = std::make_shared<catena::common::Authorizer>(jwsToken);
            authz_ = sharedAuthz_.get();
        } else {
            authz_ = &catena::common::Authorizer::kAuthzDisabled;
        }
    }
    /**
     * @brief Connect does not have copy semantics
     */
    Connect(const Connect&) = delete;
    Connect& operator=(const Connect&) = delete;
    /**
     * @brief Connect does not have move semantics
     */
    Connect(Connect&&) = delete;
    Connect& operator=(Connect&&) = delete;
    /**
     * @brief Destructor
     */
    ~Connect() = default;

    // IConnect::IsCanceled to be defined by the derived class

    /**
     * @brief Updates the response message with parameter values and handles 
     * authorization checks.
     * 
     * @param oid - The OID of the value to update
     * @param idx - The index of the value to update
     * @param p - The parameter to update
     */
    void updateResponse_(const std::string& oid, size_t idx, const IParam* p) override {
        try {
            // If Connect was cancelled, notify client and end process
            if (this->isCancelled()) {
                this->hasUpdate_ = true;
                this->cv_.notify_one();
                return;
            }

            if (this->authz_ != &catena::common::Authorizer::kAuthzDisabled && !this->authz_->readAuthz(*p)) {
                return;
            }

            // Get all subscribed OIDs
            auto subscribedOids = this->subscriptionManager_.getAllSubscribedOids(this->dm_);

            // Check if we should process this update based on detail level
            bool should_update = false;
            
            // Map of detail levels to their update logic
            const std::unordered_map<catena::Device_DetailLevel, std::function<void()>> detailLevelMap {
                {catena::Device_DetailLevel_FULL, [&]() {
                    // Always update for FULL detail level
                    should_update = true;
                }},
                {catena::Device_DetailLevel_MINIMAL, [&]() {
                    // For MINIMAL, only update if it's in the minimal set
                    should_update = p->getDescriptor().minimalSet();
                }},
                {catena::Device_DetailLevel_SUBSCRIPTIONS, [&]() {
                    // Update if OID is subscribed or in minimal set
                    should_update = p->getDescriptor().minimalSet() || 
                           (std::find(subscribedOids.begin(), subscribedOids.end(), oid) != subscribedOids.end());
                }},
                {catena::Device_DetailLevel_COMMANDS, [&]() {
                    // For COMMANDS, only update command parameters
                    should_update = p->getDescriptor().isCommand();
                }},
                {catena::Device_DetailLevel_NONE, [&]() {
                    // Don't send any updates
                    should_update = false;
                }}
            };

            if (detailLevelMap.contains(this->detailLevel_)) {
                detailLevelMap.at(this->detailLevel_)();
            } else {
                should_update = false;
            }
    
            if (!should_update) {
                return;
            }
    
    
            this->res_.mutable_value()->set_oid(oid);
            this->res_.mutable_value()->set_element_index(idx);
            
            catena::Value* value = this->res_.mutable_value()->mutable_value();
    
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            rc = p->toProto(*value, *authz_);
            //If the param conversion was successful, send the update
            if (rc.status == catena::StatusCode::OK) {
                this->hasUpdate_ = true;
                this->cv_.notify_one();
            }
        } catch(catena::exception_with_status& why) {
            // if an error is thrown, no update is pushed to the client
        }
    }
    
    /**
     * @brief Updates the response message with a ComponentLanguagePack and
     * handles authorization checks.
     * 
     * @param l The added ComponentLanguagePack emitted by device.
     */
    void updateResponse_(const IDevice::ComponentLanguagePack& l) override {
        try {
            // If Connect was cancelled, notify client and end process.
            if (this->isCancelled()){
                this->hasUpdate_ = true;
                this->cv_.notify_one();
                return;
            }
            // Returning if authorization is enabled and the client does not have monitor scope.
            if (this->authz_ != &catena::common::Authorizer::kAuthzDisabled
                && !this->authz_->hasAuthz(Scopes().getForwardMap().at(Scopes_e::kMonitor))) {
                return;
            }
            // Updating res_'s device_component and pushing update.
            auto pack = this->res_.mutable_device_component()->mutable_language_pack();
            pack->set_language(l.language());
            pack->mutable_language_pack()->CopyFrom(l.language_pack());
            this->hasUpdate_ = true;
            this->cv_.notify_one();
        } catch(catena::exception_with_status& why){
            // if an error is thrown, no update is pushed to the client
        }
    }

    /**
     * @brief Shared ptr to maintain ownership of authorizer.
     */
    std::shared_ptr<catena::common::Authorizer> sharedAuthz_;
    /**
     * @brief The authorizer object containing client's scopes.
     */
    catena::common::Authorizer* authz_;
    /**
     * @brief The connected device.
     */
    IDevice& dm_;
    /**
     * @brief Bool indicating whether the child has an update to write.
     */
    bool hasUpdate_ = false;
    /**
     * @brief A condition variable used to wait for an update.
     */
    std::condition_variable cv_;
    /**
     * @brief Server response (updates).
     */
    catena::PushUpdates res_;
    /**
     * @brief The language of the response.
     */
    std::string language_;
    /**
     * @brief The detail level of the response.
     */
    catena::Device_DetailLevel detailLevel_;
    /**
     * @brief The subscription manager.
     */
    ISubscriptionManager& subscriptionManager_;
    /**
     * @brief The user agent of the client.
     * 
     * No idea what this is used for and if its even needed here.
     */
    std::string userAgent_;
    /**
     * @brief Flag to force a connection.
     * 
     * No idea what this is used for and if its even needed here.
     */
    bool forceConnection_;
};

}; // common
}; // catena
