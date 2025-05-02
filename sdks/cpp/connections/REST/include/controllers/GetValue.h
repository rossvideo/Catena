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
 * @file GetValue.h
 * @brief Implements REST GetValue controller.
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
#include <IDevice.h>
#include <utils.h>
#include <Authorization.h>
#include <Enums.h>

// Connections/REST
#include "interface/ISocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the GetValue REST controller.
 */
class GetValue : public ICallData {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;

    /**
     * @brief Constructor for the GetValue controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dm The device to get the value from.
     */ 
    GetValue(tcp::socket& socket, ISocketReader& context, IDevice& dm);
    /**
     * @brief GetValue's main process.
     */
    void proceed() override;
    
    /**
     * @brief Finishes the GetValue process.
     */
    void finish() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     * 
     * @param socket The socket to write the response stream to.
     * @param context The ISocketReader object.
     * @param dm The device to connect to.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, IDevice& dm) {
      return new GetValue(socket, context, dm);
    }
    
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      std::cout << "GetValue::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok
                << std::endl;
    }
  private:
    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;
    /**
     * @brief The ISocketReader object.
     */
    ISocketReader& context_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SocketWriter writer_;
    /**
     * @brief The device to set values of.
     */
    IDevice& dm_;

    /**
     * @brief ID of the GetValue object
     */
    int objectId_;
    /**
     * @brief The total # of GetValue objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
