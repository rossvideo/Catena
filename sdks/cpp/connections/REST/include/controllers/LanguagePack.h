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
 * @file LanguagePack.h
 * @brief Implements REST LanguagePack controller.
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
 * @brief ICallData class for the LanguagePack REST controller.
 */
class LanguagePack : public ICallData {
  public:
    // Specifying which Device to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;

    /**
     * @brief Constructor for the LanguagePack controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dm The device to get the language pack from.
     */ 
    LanguagePack(tcp::socket& socket, ISocketReader& context, IDevice& dm);
    /**
     * @brief LanguagePack's main processes.
     */
    void proceed() override;
    
    /**
     * @brief Finishes the LanguagePack process.
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
      return new LanguagePack(socket, context, dm);
    }
    

  private:
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      std::cout << context_.method() << " LanguagePack::proceed[" << objectId_
                << "]: " << catena::common::timeNow() << " status: "
                << static_cast<int>(status) <<", ok: "<< std::boolalpha << ok
                << std::endl;
    }
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
     * @brief The device to get language pack from.
     */
    IDevice& dm_;

    /**
     * @brief ID of the LanguagePack object
     */
    int objectId_;
    /**
     * @brief The total # of LanguagePack objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
