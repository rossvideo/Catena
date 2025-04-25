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
 * @brief Implements REST Connect RPC.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include <rpc/Connect.h>
#include <rpc/TimeNow.h>
#include <Status.h>
#include <IParam.h>
#include <IDevice.h>
#include <utils.h>
#include <Authorization.h>
#include <SubscriptionManager.h>

// Connections/REST
#include "SocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the Connect REST RPC.
 */
class Connect : public ICallData, public catena::common::Connect {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;

    /**
     * @brief Constructor for the Connect RPC.
     *
     * @param socket The socket to write the response stream to.
     * @param context The SocketReader object.
     * @param dm The device to connect to.
     */ 
    Connect(tcp::socket& socket, SocketReader& context, IDevice& dm);
    /**
     * Connect main process
     */
    void proceed() override;
    /**
     * Finishes the Connect process by disconnecting listeners.
     */
    void finish() override;
    /**
     * @brief Creates a new rpc object for use with GenericFactory.
     * 
     * @param socket The socket to write the response stream to.
     * @param context The SocketReader object.
     * @param dm The device to connect to.
     */
    static ICallData* makeOne(tcp::socket& socket, SocketReader& context, IDevice& dm) {
      return new Connect(socket, context, dm);
    }
    
    /**
     * @brief Signal emitted in the case of an error which requires the all
     * open connections to be shut down.
     */
    static vdk::signal<void()> shutdownSignal_;
  private:
    /**
     * @brief Helper function to write status messages to the API console.
     * 
     * @param status The current state of the RPC (kCreate, kFinish, etc.)
     * @param ok The status of the RPC (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      std::cout << "Connect::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok
                << std::endl;
    }
    /**
     * @brief Returns true if the RPC was cancelled.
     */
    inline bool isCancelled() override { return !this->socket_.is_open(); }

    /**
     * @brief The socket to write the response stream to.
     */
    tcp::socket& socket_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SSEWriter writer_;
    /**
     * @brief The mutex to for locking the object while writing
     */
    std::mutex mtx_;

    /**
     * @brief Id of operation waiting for valueSetByClient to be emitted.
     * Used when ending the connection.
     */
    unsigned int valueSetByClientId_;
    /**
     * @brief Id of operation waiting for valueSetByServer to be emitted.
     * Used when ending the connection.
     */
    unsigned int valueSetByServerId_;
    /**
     * @brief Id of operation waiting for languageAddedPushUpdate to be
     * emitted. Used when ending the connection.
     */
    unsigned int languageAddedId_;
    /**
     * @brief ID of the shutdown signal for the Connect object
    */
    unsigned int shutdownSignalId_;
    /**
     * @brief Flag to indicate when the shutdown signal has been recieved.
     */
    bool shutdown_;
    
    /**
     * @brief ID of the Connect object
     */
    int objectId_;
    /**
     * @brief The total # of Connect objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
