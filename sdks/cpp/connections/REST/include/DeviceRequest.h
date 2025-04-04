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
#include <utils.h>

// Connections/REST
#include "api.h"
#include "SocketReader.h"
#include "SocketWriter.h"
#include "ICallData.h"
using catena::API;
using catena::REST::CallStatus;

/**
 * @brief CallData class for the DeviceRequest REST RPC.
 */
class API::DeviceRequest : public catena::REST::ICallData {
  public:
    /**
     * @brief Constructor for the DeviceRequest RPC. Calls proceed() once
     * initialized.
     *
     * @param socket The socket to write the response stream to.
     * @param context The SocketReader object.
     * @param dm The device to get components from.
     */ 
    DeviceRequest(tcp::socket& socket, SocketReader& context, Device& dm);
  private:
    /**
     * @brief DeviceRequest's main process.
     */
    void proceed() override;
    /**
     * @brief Finishes the DeviceRequest process.
     */
    void finish() override;
    /**
     * @brief Helper function to write status messages to the API console.
     * 
     * @param status The current state of the RPC (kCreate, kFinish, etc.)
     * @param ok The status of the RPC (open or closed).
     */
    inline void writeConsole(CallStatus status, bool ok) const override {
      std::cout << "DeviceRequest::proceed[" << objectId_ << "]: "
                << timeNow() << " status: "<< static_cast<int>(status)
                <<", ok: "<< std::boolalpha << ok << std::endl;
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
    ChunkedWriter writer_;
    /**
     * @brief The device to get components from.
     */
    Device& dm_;

    /**
     * @brief The slot of the device to get the components from.
     */
    uint32_t slot_;
    /**
     * @brief The language to return the stream in.
     */
    std::string language_;
    /**
     * @brief The detail level to return the stream in.
     */
    int detailLevel_;
    /**
     * @brief A list of the subscribed oids to return.
     */
    std::vector<std::string> subscribedOids_;

    /**
     * @brief ID of the DeviceRequest object
     */
    int objectId_;
    /**
     * @brief The total # of DeviceRequest objects.
     */
    static int objectCounter_;
};
