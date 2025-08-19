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
 * @brief Implements controll for the REST "DeviceReqeust" endpoint.
 * @author benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
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
#include <IDevice.h>
#include <utils.h>
#include <Authorizer.h>

// Connections/REST
#include "interface/ISocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"

#include <Logger.h>

namespace catena {
namespace REST {

/**
 * @brief Controller class for the "DeviceRequest" REST endpoint.
 * 
 * This controller supports one method:
 * 
 * - GET: Writes the device as a collection of components to the client.
 * Supports both stream and unary responses.
 */
class DeviceRequest : public ICallData {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;
    using SlotMap = catena::common::SlotMap;

    /**
     * @brief Constructor for the "DeviceRequest" endpoint controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object used to read the client's request.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    DeviceRequest(tcp::socket& socket, ISocketReader& context, SlotMap& dms);
    /**
     * @brief The "DeviceRequest" endpoint's main process.
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
      return new DeviceRequest(socket, context, dms);
    }
    

  private:
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      DEBUG_LOG << "DeviceRequest::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok;
    }
    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;
    /**
     * @brief The ISocketReader object used to read the client's request.
     * 
     * This is used to get two things from the client:
     * 
     * - A slot specifying the device to request the components of.
     * 
     * - The detail level to return this request in.
     */
    ISocketReader& context_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    std::unique_ptr<ISocketWriter> writer_ = nullptr;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;

    /**
     * @brief A list of the subscribed oids to return.
     */
    std::set<std::string> subscribedOids_;

    /**
     * @brief The device serializer coroutine recieved from a call to
     * Device.getComponentSerializer().
     */
    std::unique_ptr<IDevice::IDeviceSerializer> serializer_ = nullptr;

    /**
     * @brief The object's unique id.
     */
    int objectId_;
    /**
     * @brief The total # of "DeviceRequest" endpoint controller objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
