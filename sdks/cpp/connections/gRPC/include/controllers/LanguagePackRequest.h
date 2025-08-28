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
 * @file LanguagePackRequest.h
 * @brief Implements the Catena LanguagePackRequest RPC.
 * @author benjamin.whitten@rossvideo.com
 * @date 2025-08-18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/gRPC
#include "CallData.h"

namespace catena {
namespace gRPC {

/**
 * @brief CallData class for the LanguagePackRequest RPC
 * 
 * This RPC gets a slot and a language id from the client and returns the
 * specified language pack from the specified device.
 */
class LanguagePackRequest : public CallData {
  public:
    /**
     * @brief Constructor for the CallData class of the LanguagePackRequest RPC.
     * Calls proceed() once initialized.
     *
     * @param service Pointer to the ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */
    LanguagePackRequest(IServiceImpl *service, SlotMap& dms, bool ok);
    /**
     * @brief Manages the steps of the LanguagePackRequest RPC through the state
     * variable status.
     *
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */
    void proceed(bool ok) override;
  private:
    /**
     * @brief The client's request containing two things:
     * 
     * - A slot specifying the device to retireve the language pack from.
     * 
     * - The id of the language pack to retrieve (e.g. "es" for Global Spanish).
     */
    st2138::LanguagePackRequestPayload req_;
    /**
     * @brief The RPC response writer for writing back to the client.
     */
    grpc::ServerAsyncResponseWriter<::st2138::DeviceComponent_ComponentLanguagePack> responder_;
    /**
     * @brief The RPC's state (kCreate, kProcess, kFinish, etc.).
     */
    CallStatus status_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
    /**
     * @brief The object's unique id.
     */
    int objectId_;
    /**
     * @brief The total # of LanguagePackRequest objects.
     */
    static int objectCounter_;
};

}; // namespace gRPC
}; // namespace catena
