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
 * @file UpdateSubscriptions.h
 * @brief Implements Catena gRPC UpdateSubscriptions
 * @author john.naylor@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-02-27
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/gRPC
#include "CallData.h"

// common
#include <ISubscriptionManager.h>

namespace catena {
namespace gRPC {

/**
 * @brief CallData class for the UpdateSubscriptions RPC
 */
class UpdateSubscriptions : public CallData {
  public:
    /**
     * @brief Constructor for the CallData class of the UpdateSubscriptions RPC
     * gRPC. Calls proceed() once initialized.
     * @param service The CatenaServiceImpl instance
     * @param dm The device model
     * @param ok Flag indicating if initialization was successful
     */
    UpdateSubscriptions(ICatenaServiceImpl *service, SlotMap& dms, bool ok);

    /**
     * @brief Manages the steps of the UpdateSubscriptions gRPC command
     * through the state variable status. Returns the value of the
     * parameter specified by the user.
     */
    void proceed(bool ok) override;

  private:
    /**
     * @brief Helper method to process a subscription
     * @param baseOid The base OID 
     * @param authz The authorizer to use for access control
     */
    void processSubscription_(const std::string& baseOid, catena::common::Authorizer& authz);

    /**
     * @brief Helper method to send all currently subscribed parameters
     * @param authz The authorizer to use for access control
     */
    void sendSubscribedParameters_(catena::common::Authorizer& authz);

    /**
     * @brief The request payload.
     */
    catena::UpdateSubscriptionsPayload req_;
    /**
     * @brief gRPC async response writer.
     */
    ServerAsyncWriter<catena::DeviceComponent_ComponentParam> writer_;
    /**
     * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
     */
    CallStatus status_;
    /**
     * @brief Map of slot numbers to device pointers.
     */
    SlotMap dms_;
    /**
     * @brief The device we are updating the subscriptions for.
     */
    IDevice* dm_ = nullptr;
    /**
     * @brief Shared ptr to the authorizer object so that we can maintain
     * ownership of raw ptr throughout call lifetime without use of "new"
     * keyword. 
     */
    std::shared_ptr<catena::common::Authorizer> sharedAuthz_;
    /**
     * @brief Ptr to the authorizer object. Raw as to not attempt to delete in
     * case of kAuthzDisabled.
     */
    catena::common::Authorizer* authz_;
    /**
     * @brief The set of currently subscribed OIDs from sub manager.
     */
    std::set<std::string> subbedOids_{};
    /**
     * @brief Iterator for the set of subscribed OIDs.
     */
    std::set<std::string>::iterator it_;

    /**
     * @brief The object's unique id counter.
     */
    static int objectCounter_;
    /**
     * @brief The object's unique id.
     */
    int objectId_;
};

}; // namespace gRPC
}; // namespace catena
