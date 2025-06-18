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
 * @file ISocketWriter.h
 * @brief Interface for the SocketWriter class.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;
 
// protobuf interface
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
 
// common
#include <Status.h>
 
namespace catena {
namespace REST {
 
/**
 * @brief Interface for the SocketWriter classes.
 */
class ISocketWriter {
  public:
    /**
     * @brief Constructor should save the socket as member variable.
     */
    ISocketWriter() = default;
    ISocketWriter(const ISocketWriter&) = default;
    ISocketWriter& operator=(const ISocketWriter&) = default;
    ISocketWriter(ISocketWriter&&) = default;
    ISocketWriter& operator=(ISocketWriter&&) = default;
    virtual ~ISocketWriter() = default;

    /**
     * @brief Finishes writing the HTTP response.
     * @param msg The protobuf message to write as JSON.
     * @param err The error status to finish with.
     */
    virtual void sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg = catena::Empty()) = 0;
};
 
}; // Namespace REST
}; // Namespace catena
 