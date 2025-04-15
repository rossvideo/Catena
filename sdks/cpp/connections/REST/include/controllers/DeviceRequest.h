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
 * @file DeviceRequest.h
 * @brief Implements REST DeviceRequest RPC.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include <rpc/TimeNow.h>
#include <Status.h>
#include <IParam.h>
#include <Device.h>
#include <utils.h>
#include <Authorization.h>

// Connections/REST
#include "SocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"
#include "ServiceImpl.h"

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the DeviceRequest REST RPC.
 */
class DeviceRequest : public ICallData {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using Device = catena::common::Device;
    using IParam = catena::common::IParam;

    /**
     * @brief Constructor for the DeviceRequest RPC.
     *
     * @param socket The socket to write the response stream to.
     * @param context The SocketReader object.
     * @param dm The device to get components from.
     */
    DeviceRequest(tcp::socket& socket, SocketReader& context, Device& dm);
    /**
     * @brief DeviceRequest's main process.
     */
    void proceed() override;
    /**
     * @brief Finishes the DeviceRequest process.
     */
    void finish() override;
    /**
     * @brief Creates a new rpc object for use with GenericFactory.
     * 
     * @param socket The socket to write the response stream to.
     * @param context The SocketReader object.
     * @param dm The device to connect to.
     */
    static ICallData* makeOne(tcp::socket& socket, SocketReader& context, Device& dm) {
      return new DeviceRequest(socket, context, dm);
    }
  private:
    /**
     * @brief Helper function to write status messages to the API console.
     * 
     * @param status The current state of the RPC (kCreate, kFinish, etc.)
     * @param ok The status of the RPC (open or closed).
     */
    inline void writeConsole(CallStatus status, bool ok) const override {
      std::cout << "DeviceRequest::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok
                << std::endl;
    }
    
    /**
     * @brief The socket to write the response stream to.
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
     * @brief The device to get components from.
     */
    Device& dm_;

    /**
     * @brief A list of the subscribed oids to return.
     */
    std::vector<std::string> subscribedOids_;

    /**
     * @brief A list of the subscriptions from the current request.
     */
    std::vector<std::string> requestSubscriptions_;

    /**
     * @brief Serializer for device.
     * Can't create serializer until we have client scopes
     */
    std::optional<Device::DeviceSerializer> serializer_ = std::nullopt;

    /**
     * @brief ID of the DeviceRequest object
     */
    int objectId_;
    /**
     * @brief The total # of DeviceRequest objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
