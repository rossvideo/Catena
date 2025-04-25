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
 * @brief Implements the ExecuteCommand REST API
 * @author zuhayr.sarker@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/REST
#include <interface/ICallData.h> 
#include <SocketReader.h>
#include <SocketWriter.h>

// common
#include <Device.h>
#include <IParam.h>
#include <rpc/TimeNow.h>

// boost
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

namespace catena {
namespace REST {

using catena::common::IParam;
using catena::common::IDevice;

class ExecuteCommand : public ICallData {
  public:
    static ICallData* makeOne(tcp::socket& socket, SocketReader& context, IDevice& dm) {
        return new ExecuteCommand(socket, context, dm);
    }

    ExecuteCommand(tcp::socket& socket, SocketReader& context, IDevice& dm);
    void proceed() override;
    void finish() override;

  private:
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
    SocketWriter writer_;
    /**
     * @brief The SocketReader object.
     */
    SocketReader& context_;
    /**
     * @brief The device to connect to.
     */
    IDevice& dm_;
    /**
     * @brief The payload of the request.
     */
    catena::ExecuteCommandPayload req_;
    /**
     * @brief The OID of the parameter to connect to.
     */
    std::string oid_;

    /**
     * @brief Helper function to write status messages to the API console.
     * 
     * @param status The current state of the RPC (kCreate, kFinish, etc.)
     * @param ok The status of the RPC (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
        std::cout << "ExecuteCommand::proceed[" << objectId_ << "]: "
                  << catena::common::timeNow() << " status: "
                  << static_cast<int>(status) << ", ok: " << std::boolalpha << ok
                  << std::endl;
    }
};

} // namespace REST
} // namespace catena 