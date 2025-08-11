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
#include <Authorizer.h>
#include <ISubscriptionManager.h>
#include "IConnect.h"
#include <Logger.h>

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
  public:
    /**
     * @brief Descructor
     */
    virtual ~Connect() = default;
    /**
     * @brief Returns the connection's priority.
     */
    uint32_t priority() const override { return priority_; }
    /**
     * @brief Returns the object Id.
     */
    uint32_t objectId() const override { return objectId_; }
    /**
     * @brief Returns true if this has less prioirty than otherConnection.
     */
    bool operator<(const IConnect& otherConnection) const override {
        return priority_ < otherConnection.priority() ||
               (priority_ == otherConnection.priority() && objectId_ >= otherConnection.objectId());
    }
    /**
     * @brief Forcefully shuts down the connection.
     */
    void shutdown() override {
        shutdown_ = true;
        hasUpdate_ = true;
        cv_.notify_one();
    };

  protected:
    /**
     * @brief Constructor
     */
    Connect() = delete;
    /**
     * @brief Constructor for the Connect class.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param authz true if authorization is enabled, false otherwise.
     * @param jwsToken The client's JWS token.
     * @param subscriptionManager The subscription manager.
     */
    Connect(SlotMap& dms, ISubscriptionManager& subscriptionManager) : 
        dms_{dms}, 
        subscriptionManager_{subscriptionManager},
        detailLevel_{catena::Device_DetailLevel_UNSET} {}
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

    // IConnect::IsCanceled to be defined by the derived class

    /**
     * @brief Updates the response message with parameter values and handles 
     * authorization checks.
     * 
     * @param oid - The OID of the value to update
     * @param p - The parameter to update
     */
    void updateResponse_(const std::string& oid, const IParam* p, uint32_t slot) override {
        try {
            // If Connect was cancelled, notify client and end process
            if (isCancelled()) {
                hasUpdate_ = true;
                cv_.notify_one();

            } else if (authz_->readAuthz(*p)) {
                // Map of detail levels to their update logic
                const std::unordered_map<catena::Device_DetailLevel, std::function<bool()>> detailLevelMap {
                    {catena::Device_DetailLevel_FULL, [&]() {
                        // Always update for FULL detail level
                        return true;
                    }},
                    {catena::Device_DetailLevel_MINIMAL, [&]() {
                        // For MINIMAL, only update if it's in the minimal set
                        return p->getDescriptor().minimalSet();
                    }},
                    {catena::Device_DetailLevel_SUBSCRIPTIONS, [&]() {
                        // Update if OID is subscribed or in minimal set
                        return p->getDescriptor().minimalSet() || (dms_[slot] && subscriptionManager_.isSubscribed(oid, *dms_[slot]));
                    }},
                    {catena::Device_DetailLevel_COMMANDS, [&]() {
                        // For COMMANDS, only update command parameters
                        return p->getDescriptor().isCommand();
                    }},
                    {catena::Device_DetailLevel_NONE, [&]() {
                        // Don't send any updates
                        return false;
                    }}
                };

                if (detailLevelMap.contains(detailLevel_) && detailLevelMap.at(detailLevel_)()) {
                    res_.Clear();
                    res_.set_slot(slot);
                    res_.mutable_value()->set_oid(oid);    
                    catena::Value* value = res_.mutable_value()->mutable_value();
            
                    catena::exception_with_status rc{"", catena::StatusCode::OK};
                    rc = p->toProto(*value, *authz_);
                    //If the param conversion was successful, send the update
                    if (rc.status == catena::StatusCode::OK) {
                        hasUpdate_ = true;
                        cv_.notify_one();
                    }
                }
            }
        } catch(catena::exception_with_status& why) {
            // if an error is thrown, no update is pushed to the client
            DEBUG_LOG << "Failed to send SetValue update: " << why.what();
        }
    }
    
    /**
     * @brief Updates the response message with an ILanguagePack and
     * handles authorization checks.
     * 
     * @param l The added ILanguagePack emitted by device.
     */
    void updateResponse_(const ILanguagePack* l, uint32_t slot) override {
        try {
            // If Connect was cancelled, notify client and end process.
            if (isCancelled()){
                hasUpdate_ = true;
                cv_.notify_one();
            } else if (authz_->readAuthz(Scopes_e::kMonitor)) {
                // Updating res_'s device_component and pushing update.
                res_.Clear();
                res_.set_slot(slot);
                auto pack = res_.mutable_device_component()->mutable_language_pack();
                l->toProto(*pack->mutable_language_pack());
                hasUpdate_ = true;
                cv_.notify_one();
            }
        } catch(catena::exception_with_status& why){
            // if an error is thrown, no update is pushed to the client
            DEBUG_LOG << "Failed to send language pack update: " << why.what();
        }
    }

    /**
     * @brief Sets up the authorizer object with the jwsToken and calculates
     * connection priority.
     * 
     * priority_ = scope * 2 + write + (adm:w && forceConnection)
     * 
     * @param jwsToken The jwsToken to use for authorization.
     * @param authz true if authorization is enabled, false otherwise.
     */
    void initAuthz_(const std::string& jwsToken, bool authz = false) override {
        if (authz) {
            sharedAuthz_ = std::make_shared<catena::common::Authorizer>(jwsToken);
            authz_ = sharedAuthz_.get();
            // Throw error for non-admin clients trying to force a connection.
            if (forceConnection_ && !authz_->writeAuthz(Scopes_e::kAdmin)) {
                throw catena::exception_with_status("adm:w scope required to force a connection", catena::StatusCode::PERMISSION_DENIED);
            }
            // Setting up their connection priority.
            for (uint32_t i = static_cast<uint32_t>(Scopes_e::kAdmin); i >= static_cast<uint32_t>(Scopes_e::kMonitor); i -= 1) {             
                // Client has read
                if (authz_->readAuthz(static_cast<Scopes_e>(i))) {
                    priority_ = 2 * i + forceConnection_;
                    // Also has write
                    if (authz_->writeAuthz(static_cast<Scopes_e>(i))) {
                        priority_ += 1;
                    }
                    break;
                }
            }
        } else {
            authz_ = &catena::common::Authorizer::kAuthzDisabled;
        }
    }

    /**
     * @brief The priority of the connection.
     * 
     * priority_ = scope * 2 + write + (adm:w && forceConnection)
     */
    uint32_t priority_ = 0;
    /**
     * @brief Shared ptr to maintain ownership of authorizer.
     */
    std::shared_ptr<catena::common::Authorizer> sharedAuthz_;
    /**
     * @brief The authorizer object containing client's scopes.
     */
    catena::common::Authorizer* authz_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
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
     * Only applicable if client has admin:w scope.
     */
    bool forceConnection_;
    /**
     * @brief Flag indicating whether to shut down the connection.
     */
    bool shutdown_ = false;
    /**
     * @brief ID of the Connect object
     */
    uint32_t objectId_ = 0;
};

}; // common
}; // catena
