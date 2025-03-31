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
 * @file SocketWriter.h
 * @brief Helper class used to write to a socket using boost.
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
 * @brief Helper class used to write to a socket using boost.
 */
class SocketWriter {
  public:
    /**
     * @brief Constructs a SocketWriter.
     * @param socket The socket to write to.
     */
    SocketWriter(tcp::socket& socket) : socket_{socket} {}
    /**
     * @brief Writes a protobuf message to socket in JSON format.
     * @param msg The protobuf message to write as JSON.
     */
    virtual void write(google::protobuf::Message& msg);
    /**
     * @brief Writes an error message to the socket.
     * @param err The catena::exception_with_status.
     */
    virtual void write(catena::exception_with_status& err);
    virtual ~SocketWriter() = default;
  protected:
    /**
     * @brief The socket to write to.
     */
    tcp::socket& socket_;
    /**
     * @brief Maps catena::StatusCode to HTTP status codes.
     */
    const std::map<catena::StatusCode, int> codeMap_ {
    {catena::StatusCode::OK,                  200},
    {catena::StatusCode::CANCELLED,           410},
    {catena::StatusCode::UNKNOWN,             404},
    {catena::StatusCode::INVALID_ARGUMENT,    406},
    {catena::StatusCode::DEADLINE_EXCEEDED,   408},
    {catena::StatusCode::NOT_FOUND,           410},
    {catena::StatusCode::ALREADY_EXISTS,      409},
    {catena::StatusCode::PERMISSION_DENIED,   401},
    {catena::StatusCode::UNAUTHENTICATED,     407},
    {catena::StatusCode::RESOURCE_EXHAUSTED,  8},   // TODO
    {catena::StatusCode::FAILED_PRECONDITION, 412},
    {catena::StatusCode::ABORTED,             10},  // TODO
    {catena::StatusCode::OUT_OF_RANGE,        416},
    {catena::StatusCode::UNIMPLEMENTED,       501},
    {catena::StatusCode::INTERNAL,            500},
    {catena::StatusCode::UNAVAILABLE,         503},
    {catena::StatusCode::DATA_LOSS,           15},  // TODO
    {catena::StatusCode::DO_NOT_USE,          -1},  // TODO
    };
};

/**
 * @brief Helper class used to write to a socket using boost.
 * 
 * Subclass for chunked encoding (Stream response).
 */
class ChunkedWriter : public SocketWriter {
  public:
    // Using parent constructor
    using SocketWriter::SocketWriter;
    /**
     * @brief Writes a protobuf message to socket in JSON format.
     * @param msg The protobuf message to write as JSON.
     */
    void write(google::protobuf::Message& msg) override;
    /**
     * @brief Writes an error message to the socket.
     * @param err The catena::exception_with_status.
     */
    void write(catena::exception_with_status& err) override;
    /**
     * @brief Finishes the chunked writing process.
     */
    void finish();

  private:
    /**
     * @brief Writes headers to the socket in chuncked encoding format.
     * @param status The catena::exception_with_status of the operation.
     */
    void writeHeaders(catena::exception_with_status& status);
    /**
     * @brief Indicates whether the writer has written headers or not. 
     */
    bool hasHeaders_ = false;
};

}; // Namespace REST
}; // Namespace catena
