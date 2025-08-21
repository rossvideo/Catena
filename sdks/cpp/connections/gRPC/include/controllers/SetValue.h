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
 * @file SetValue.h
 * @brief Implements the Catena SetValue RPC.
 * @author benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-08-18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/gRPC
#include "controllers/MultiSetValue.h"

namespace catena {
namespace gRPC {

/**
 * @brief CallData class for the SetValue RPC
 *
 * This RPC gets a slot and a single oid, value pair from the client and sets
 * the value of the specified parameter in the specified device.
 */
class SetValue : public MultiSetValue {
  public:
    /**
     * @brief Constructor for the CallData class of the SetValue RPC.
     * Calls proceed() once initialized.
     *
     * @param service Pointer to the ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */ 
    SetValue(IServiceImpl *service, SlotMap& dms, bool ok);

  private:
    /**
     * @brief Requests SetValue from the system.
     * 
     * Helper function to allow reuse of MultiSetValue's proceed().
     */
    void request_() override;
    /**
     * @brief Creates a new SetValue object to serve other clients while
     * processing.
     * 
     * Helper function to allow reuse of MultiSetValue's proceed().
     *
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */ 
    void create_(bool ok) override;
    /**
     * @brief Converts req_ to a MultiSetValuePayload reqs_.
     *   
     * Helper function to allow reuse of proceed().
     */
    void toMulti_() override;

    /**
     * @brief The client's request containing two things:
     * 
     * - The slot specifying the device containing the parameter to update.
     * 
     * - An oid, value pair specifying the parameter to update.
     */
    catena::SingleSetValuePayload req_;
    /**
     * @brief The total # of SetValue objects.
     */
    static int objectCounter_;
};

}; // namespace gRPC
}; // namespace catena
