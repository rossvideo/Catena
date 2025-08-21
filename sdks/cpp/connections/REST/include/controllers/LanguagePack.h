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
 * @brief Implements controller for the REST language-pack endpoint.
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
#include <Authorizer.h>
#include <Enums.h>

// Connections/REST
#include "interface/ISocketReader.h"
#include "SocketWriter.h"
#include "interface/ICallData.h"

#include <Logger.h>

namespace catena {
namespace REST {

/**
 * @brief Controller class for the language-pack REST endpoint.
 * 
 * This controller supports four methods:
 * 
 * - GET: Writes the specified language pack to the client.
 * 
 * - POST: Adds a new language pack to the specified device.
 * 
 * - PUT: Updates a language pack on the specified device.
 * 
 * - DELETE: Deletes a language pack from the specified device.
 */
class LanguagePack : public ICallData {
  public:
    // Specifying which Device to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using SlotMap = catena::common::SlotMap;

    /**
     * @brief Constructor for the language-pack endpoint controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object used to read the client's request.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    LanguagePack(tcp::socket& socket, ISocketReader& context, SlotMap& dms);
    /**
     * @brief The language-pack endpoint's main process.
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
      return new LanguagePack(socket, context, dms);
    }
    

  private:
    /**
     * @brief Writes the current state of the request to the console.
     * 
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      DEBUG_LOG << RESTMethodMap().getForwardMap().at(context_.method())
                << " LanguagePack::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "
                << static_cast<int>(status) << ", ok: " << std::boolalpha << ok;
    }
    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;
    /**
     * @brief The ISocketReader object used to read the client's request.
     * 
     * This is used to get five things from the client:
     * 
     * - A slot specifying the device to manage language packs of.
     * 
     * - The id of the language pack to manage (e.g. "es" Global spanish).
     * 
     * - The language pack to add/overwrite (if applicable).
     */
    ISocketReader& context_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SocketWriter writer_;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;

    /**
     * @brief The object's unique id.
     */
    int objectId_;
    /**
     * @brief The total # of language-pack endpoint controller objects.
     */
    static int objectCounter_;
};

}; // namespace REST
}; // namespace catena
