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
 * @brief Implements controller for the REST connect endpoint.
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
#include <ILanguagePack.h>
#include <utils.h>
#include <Authorizer.h>
#include <SubscriptionManager.h>

// Connections/REST
#include "interface/ISocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"

#include <Logger.h>

namespace catena {
namespace REST {

/**
 * @brief Controller class for the Connect REST endpoint.
 * 
 * This controller supports one method:
 * 
 * - GET: Connects the client to each device in the service and writes
 * updates whenever one of their ValueSetByClient, ValueSetByServer, or
 * LanguageAddedPushUpdate signals is emitted. Whether or not a PushUpdate is
 * written to the client also depends on their specified DetailLevel.
 */
class Connect : public ICallData, public catena::common::Connect {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;
    using SlotMap = catena::common::SlotMap;
    using SignalMap = catena::common::SignalMap;

    /**
     * @brief Constructor for the Connect endpoint controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object used to read the client's request.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    Connect(tcp::socket& socket, ISocketReader& context, SlotMap& dms);
    /**
     * @brief Destructor for the Connect controller.
     */
    virtual ~Connect();
    /**
     * @brief The Connect endpoint's main process.
     */
    void proceed() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object used to read the client's request.
     * @param dms A map of slots to ptrs to their corresponding device.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, SlotMap& dms) {
      return new Connect(socket, context, dms);
    }
    
    /**
     * @brief Signal emitted in cases which require all open connections to be
     * shut down such as service shutdown.
     */
    static vdk::signal<void()> shutdownSignal_;
    /**
     * @brief Returns true if the connection has been cancelled.
     */
    inline bool isCancelled() override;
    
  private:
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      DEBUG_LOG << "Connect::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok;
    }

    /**
     * @brief The socket to write the response stream to.
     */
    tcp::socket& socket_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SSEWriter writer_;
    /**
     * @brief The ISocketReader object used to read the client's request.
     * 
     * This is used to get two things from the client:
     * 
     * - The detail level of updates the they want to recieve.
     * 
     * - A flag indicating whether the client wants to force a connection to
     * the service.
     */
    ISocketReader& context_;
    /**
     * @brief The mutex to lock the controller while writing.
     */
    std::mutex mtx_;
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
     * @brief ID of the shutdown signal for the Connect object
     */
    unsigned int shutdownSignalId_;

    /**
     * @brief The total # of connect endpoint controller objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
