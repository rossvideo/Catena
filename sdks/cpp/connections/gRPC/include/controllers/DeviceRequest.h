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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @file DeviceRequest.h
 * @brief Implements the Catena DeviceRequest RPC.
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2025-08-18
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
 * @brief CallData class for the DeviceRequest RPC
 *
 * This RPC gets a slot and detail level from the client and writes back the
 * specified device as a stream of components.
 */
class DeviceRequest : public CallData {
  public:
    /**
     * @brief Constructor for the CallData class of the DeviceRequest RPC.
     * Calls proceed() once initialized.
     *
     * @param service Pointer to the ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */
    DeviceRequest(IServiceImpl *service, SlotMap& dms, bool ok);
    /**
     * @brief Manages the steps of the DeviceRequest RPC through the state
     * variable status.
     *
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.   
     */
    void proceed(bool ok) override;

  private:
    /**
     * @brief The client's request containing two/three things:
     * 
     * - A slot specifying the device to request the components of.
     * 
     * - The detail level to return this request in.
     * 
     * - Optionally a set of specific param oids to get from the device if the
     * detail level is set to SUBSCRIPTIONS.
     */
    st2138::DeviceRequestPayload req_;
    /**
     * @brief The RPC response writer for writing back to the client.
     */
    ServerAsyncWriter<st2138::DeviceComponent> writer_;
    /**
     * @brief The device serializer coroutine recieved from a call to
     * Device.getComponentSerializer().
     */
    std::unique_ptr<IDevice::IDeviceSerializer> serializer_ = nullptr;
    /**
     * @brief The RPC's state (kCreate, kProcess, kFinish, etc.)
     */
    CallStatus status_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
    /**
     * @brief The device at the specified slot in dms_.
     */
    IDevice* dm_ = nullptr;
    /**
     * @brief A shared pointer which maintains ownership over a newly created
     * Authozier object.
     * 
     * This is seperate from authz_ to avoid any attempts to delete
     * kAuthzDisabled if authorization is disabled.
     */
    std::shared_ptr<catena::common::Authorizer> sharedAuthz_;
    /**
     * @brief Ptr to the authorizer object to use for the RPC.
     */
    catena::common::Authorizer* authz_;
    /**
     * @brief The set of subscribed OIDs for use if the detail level is set to
     * SUBSCRIPTIONS.
     */
    std::set<std::string> subscribedOids_;
    /**
     * @brief The object's unique id.
     */
    int objectId_;
    /**
     * @brief The total # of DeviceRequest objects.
     */
    static int objectCounter_;
};

}; // namespace gRPC
}; // namespace catena
