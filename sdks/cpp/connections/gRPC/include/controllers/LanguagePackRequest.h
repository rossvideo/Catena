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
 * @brief Implements Catena gRPC LanguagePackRequest
 * @author benjamin.whitten@rossvideo.com
 * @date 2025-01-29
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

// connections/gRPC
#include "CallData.h"

namespace catena {
namespace gRPC {

/**
* @brief CallData class for the LanguagePackRequest RPC
*/
class LanguagePackRequest : public CallData {
  public:
    /**
     * @brief Constructor for the CallData class of the LanguagePackRequest
     * gRPC. Calls proceed() once initialized.
     *
     * @param service - Pointer to the parent ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok - Flag to check if the command was successfully executed.
     */ 
    LanguagePackRequest(IServiceImpl *service, SlotMap& dms, bool ok);
    /**
     * @brief Manages the steps of the LanguagePackRequest command through
     * the state variable status.
     *
     * @param service - Pointer to the parent ServiceImpl.
     * @param ok - Flag to check if the command was successfully executed.
     */
    void proceed(bool ok) override;
  private:
    /**
     * @brief Server request (slot, languageId).
     */
    catena::LanguagePackRequestPayload req_;
    /**
     * @brief gRPC async response writer.
     */
    grpc::ServerAsyncResponseWriter<::catena::DeviceComponent_ComponentLanguagePack> responder_;
    /**
     * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
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
