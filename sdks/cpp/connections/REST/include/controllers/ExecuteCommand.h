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
 * @file ExecuteCommand.h
 * @brief Implements REST ExecuteCommand controller.
 * @author zuhayr.sarker@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/REST
#include "interface/ISocketReader.h"
#include <SocketWriter.h>
#include "interface/ICallData.h"

// common
#include <Device.h>
#include <IParam.h>
#include <rpc/TimeNow.h>

// boost
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the ExecuteCommand REST controller.
 */
class ExecuteCommand : public ICallData {
  public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;
    using SlotMap = catena::common::SlotMap;

    /**
     * @brief Constructor for the ExecuteCommand controller.
     *
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    ExecuteCommand(tcp::socket& socket, ISocketReader& context, SlotMap& dms);
    
    /**
     * @brief ExecuteCommand's main process.
     */
    void proceed() override;
    
    /**
     * @brief Finishes the ExecuteCommand process
     */
    void finish() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     * 
     * @param socket The socket to write the response stream to.
     * @param context The SocketReader object.
     * @param dms A map of slots to ptrs to their corresponding device.
     */
    static ICallData* makeOne(tcp::socket& socket, ISocketReader& context, SlotMap& dms) {
      return new ExecuteCommand(socket, context, dms);
    }

  private:
      /**
     * @brief Writes the current state of the request to the console.
     * @param status The current state of the request (kCreate, kFinish, etc.)
     * @param ok The status of the request (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
      std::cout << "ExecuteCommand::proceed[" << objectId_ << "]: "
                << catena::common::timeNow() << " status: "<< static_cast<int>(status)
                <<", ok: "<< std::boolalpha << ok << std::endl;
    }
    /**
     * @brief The number of ExecuteCommand objects created.
     */
    static int objectCounter_;
    /**
     * @brief The ID of the current ExecuteCommand object.
     */
    int objectId_;
    /**
     * @brief The socket to write the response stream to.
     */
    tcp::socket& socket_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    SSEWriter writer_;
    /**
     * @brief The ISocketReader object.
     */
    ISocketReader& context_;
    /**
     * @brief Map of slot numbers to device pointers.
     */
    SlotMap& dms_;
};

} // namespace REST
} // namespace catena 
