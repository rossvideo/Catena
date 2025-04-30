#pragma once

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
 * @brief Implements Catena REST UpdateSubscriptions
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-28
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// connections/REST
#include <ISubscriptionManager.h>

// common
#include <IDevice.h>
#include <rpc/TimeNow.h>

// Connections/REST
#include <SocketReader.h>
#include <SocketWriter.h>
#include <interface/ICallData.h>

namespace catena {
namespace REST {

/**
 * @brief Controller class for handling UpdateSubscriptions requests
 */
class UpdateSubscriptions : public ICallData {
public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;

    /**
     * @brief Constructor for the UpdateSubscriptions controller
     * @param socket The TCP socket for the connection
     * @param context The socket reader context
     * @param dm The device model
     */
    UpdateSubscriptions(tcp::socket& socket, SocketReader& context, IDevice& dm);

    /**
     * @brief Processes the UpdateSubscriptions request
     */
    void proceed() override;

    /**
     * @brief Finishes the UpdateSubscriptions process
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
        return new UpdateSubscriptions(socket, context, dm);
    }

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
     * @brief Helper function to write status messages to the API console.
     * 
     * @param status The current state of the RPC (kCreate, kFinish, etc.)
     * @param ok The status of the RPC (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
        std::cout << "UpdateSubscriptions::proceed[" << objectId_ << "]: "
                  << catena::common::timeNow() << " status: "
                  << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok
                  << std::endl;
    }

    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;

    /**
     * @brief The SocketReader object.
     */
    SocketReader& context_;

    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SSEWriter writer_;

    /**
     * @brief The request payload
     */
    catena::UpdateSubscriptionsPayload req_;

    /**
     * @brief The response payload for a single response
     */
    catena::DeviceComponent_ComponentParam res_;

    /**
     * @brief Vector to store all responses to be sent
     */
    std::vector<catena::DeviceComponent_ComponentParam> responses_;

    /**
     * @brief Current response index being processed
     */
    uint32_t current_response_{0};

    /**
     * @brief The object's unique id counter
     */
    static int objectCounter_;

    /**
     * @brief The object's unique id
     */
    int objectId_;

    /**
     * @brief The mutex for the writer lock
     */
    std::mutex mtx_;

    /**
     * @brief The writer lock
     */
    std::unique_lock<std::mutex> writer_lock_{mtx_, std::defer_lock};

    /**
     * @brief The device model
     */
    IDevice& dm_;
};

} // namespace REST 
} // namespace catena
