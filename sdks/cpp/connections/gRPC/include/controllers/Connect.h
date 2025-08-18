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
 * @file Connect.h
 * @brief Implements the Catena Connect RPC
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
#include <rpc/Connect.h>
#include <ILanguagePack.h>

namespace catena {
namespace gRPC {

/**
 * @brief CallData class for the Connect RPC
 *
 * This RPC connects the client to each device in the service and writes
 * updates whenever one of their ValueSetByClient, ValueSetByServer, or
 * LanguageAddedPushUpdate signals is emitted.
 *
 * Whether or not a PushUpdate is written to the client also depends on their
 * specified DetailLevel.
 */
class Connect : public CallData, public catena::common::Connect {
  public:
    /**
     * @brief Constructor for the CallData class of the Connect RPC.
     * Calls proceed() once initialized.
     *
     * @param service Pointer to the ServiceImpl.
     * @param dms A map of slots to ptrs to their corresponding device.
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */ 
    Connect(IServiceImpl *service, SlotMap& dms, bool ok);
    /**
     * @brief Manages the steps of the Connect RPC through the state variable
     * status.
     *
     * @param ok Flag indicating the status of the service and call.
     * Will be false if either has been shutdown/cancelled.
     */
    void proceed(bool ok) override;
    /**
     * @brief Returns true if the connection has been cancelled.
     */
    bool isCancelled() override;

  private:
    /**
     * @brief The client's request containing two things:
     * 
     * - The detail level of updates they want to receive.
     * 
     * - A flag indicating whether the client wants to force a connection to
     * the service.
     */
    catena::ConnectPayload req_;
    /**
     * @brief The RPC response writer for writing back to the client.
     */
    ServerAsyncWriter<catena::PushUpdates> writer_;
    /**
     * @brief The RPC's state (kCreate, kProcess, kFinish, etc.).
     */
    CallStatus status_;
    /**
     * @brief The mutex to lock the RPC while writing.
     */
    std::mutex mtx_;
    /**
     * @brief The total # of Connect objects.
     */
    static int objectCounter_;

    /**
     * @brief A list of ids for the operations waiting for valueSetByClient to
     * be emitted.
     */
    SignalMap valueSetByClientIds_;
    /**
     * @brief A list of ids for the operations waiting for valueSetByServer to
     * be emitted.
     */
    SignalMap valueSetByServerIds_;
    /**
     * @brief A list of ids for the operations waiting for
     * languageAddedPushUpdate to be emitted.
     */
    SignalMap languageAddedIds_;

    /**
     * @brief Signal emitted in cases which require all open connections to be
     * shut down such as service shutdown.
     */
    static vdk::signal<void()> shutdownSignal_;
    /**
     * @brief ID of the shutdown signal for the Connect object
    */
    unsigned int shutdownSignalId_ = 0;
};

}; // namespace gRPC
}; // namespace catena
