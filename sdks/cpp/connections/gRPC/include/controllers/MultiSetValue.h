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
 * @file MultiSetValue.h
 * @brief Generic CallData class for the Catena SetValue and MultiSetValue RPCs.
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @author benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-08-18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/gRPC
#include "CallData.h"

namespace catena {
namespace gRPC {

/**
 * @brief Generic CallData class for the SetValue and MultiSetValue RPCs.
 *
 * This RPC gets a slot and any number of oid, value pairs from the client and
 * sets the value of each specified parameter in the specified device. 
 */
class MultiSetValue : public CallData {
  public:
    /**
     * @brief Constructor for the CallData class of the MultiSetValue RPC.
     * Calls proceed() once initialized.
     *
     * @param service Pointer to the ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */ 
    MultiSetValue(IServiceImpl *service, SlotMap& dms, bool ok);
    /**
     * @brief Manages the steps of the MultiSetValue RPC through the state
     * variable status.
     *
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */
    void proceed(bool ok) override;
  protected:
    /**
     * @brief Construct for the CallData class for both MultiSetValue and
     * SetValue which additionally sets the objectId. 
     *
     * @param service Pointer to the ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     * @param objectId objectCounter_ + 1
     */ 
    MultiSetValue(IServiceImpl *service, SlotMap& dms, bool ok, int objectId);
    /**
     * @brief Requests MultiSetValue from the service.
     *   
     * Helper function to allow reuse of proceed().
     */
    virtual void request_();
    /**
     * @brief Creates a new MultiSetValue object to serve other clients
     * while processing.
     *   
     * Helper function to allow reuse of proceed().
     *
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */ 
    virtual void create_(bool ok);
    /**
     * @brief Converts req_ to a MultiSetValuePayload reqs_.
     *   
     * Helper function to allow reuse of proceed().
     * 
     * @note Does nothing in MultiSetValue.
     */
    virtual void toMulti_() { return; }

    /**
     * @brief Name of childclass to specify RPC in console notifications.
     */
    std::string typeName;
    /**
     * @brief The client's request containing two things:
     * 
     * - The slot specifying the device containing the parameters to update.
     * 
     * - Any number of oid, value pairs specifying the parameters to update.
     */
    catena::MultiSetValuePayload reqs_;
    /**
     * @brief The RPC response writer for writing back to the client.
     */
    ServerAsyncResponseWriter<catena::Empty> responder_;
    /**
     * @brief The RPC's state (kCreate, kProcess, kFinish, etc.).
     */
    CallStatus status_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
    /**
     * The status of the transaction for use in responder.finish functions.
     */
    Status errorStatus_;
    /**
     * @brief The object's unique id.
     */
    int objectId_;
  private:
    /**
     * @brief The total # of MultiSetValue objects.
     */
    static int objectCounter_;
};

}; // namespace gRPC
}; // namespace catena
