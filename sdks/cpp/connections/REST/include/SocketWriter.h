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
 * @author zuhayr.sarker@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// connections/REST
#include "interface/ISocketWriter.h"

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// protobuf interface
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include <Status.h>
#include <utils.h>

namespace catena {
namespace REST {

/**
 * @brief Helper class used to write a unary response to a socket using boost.
 */
class SocketWriter : public ISocketWriter {
  public:
    /**
     * @brief Constructs a SocketWriter.
     * @param socket The socket to write to.
     * @param origin The origin of the request.
     * @param buffer Flag indicating whether to buffer a multi-message response.
     */
    SocketWriter(tcp::socket& socket, const std::string& origin = "*", bool buffer = false) : socket_{socket}, origin_{origin}, buffer_{buffer} {}

    /**
     * @brief Finishes writing the HTTP response.
     * @param msg The protobuf message to write (if status is OK). An empty
     * message signals the writer to flush the response if buffer is on.
     * @param err The error status to finish with. If status is OK, writes the
     * message, otherwise writes the error.
     */
    void sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg = catena::Empty()) override;

  private:
    /**
     * @brief The socket to write to.
     */
    tcp::socket& socket_;
    /**
     * @brief The origin of the request.
     */
    std::string origin_;

    /**
     * @brief flag to indicate whether to buffer a multi-message response.
     */
    bool buffer_;
    /**
     * @brief The json body of the response to write to the client.
     */
    std::string jsonBody_ = "";
};

/**
 * @brief Helper class to write Server Sent Events to a socket using boost.
 */
class SSEWriter : public ISocketWriter {
  public:
    /**
     * @brief Constructor for SSEWriter.
     * @param socket The socket to write to.
     * @param origin The origin of the request.
     */
    SSEWriter(tcp::socket& socket, const std::string& origin = "*") : socket_{socket}, origin_{origin}, headers_sent_{false} {}

    /**
     * @brief Sends a response as a Server-Sent Event.
     * @param msg The protobuf message to write as JSON.
     * @param err The error status to finish with.
     */
    void sendResponse(const catena::exception_with_status& err, const google::protobuf::Message& msg = catena::Empty()) override;

  private:
    /**
     * @brief The socket to write to.
     */
    tcp::socket& socket_;
    /**
     * @brief The origin of the request.
     */
    std::string origin_;
    /**
     * @brief Whether the headers have been sent.
     */
    bool headers_sent_;
};

using http_exception_with_status = std::pair<int, std::string>;
/**
 * @brief Maps catena::StatusCode to HTTP status codes and reasons.
 */
const std::map<catena::StatusCode, http_exception_with_status> codeMap_ {
    {catena::StatusCode::OK,                  {200, "OK"}},
    {catena::StatusCode::CREATED,             {201, "Created"}},
    {catena::StatusCode::ACCEPTED,            {202, "Accepted"}},
    {catena::StatusCode::NO_CONTENT,          {204, "No Content"}},
    {catena::StatusCode::ALREADY_EXISTS,      {208, "Already Reported"}},
    {catena::StatusCode::CANCELLED,           {400, "Cancelled"}},
    {catena::StatusCode::INVALID_ARGUMENT,    {400, "Bad Request"}},
    {catena::StatusCode::UNAUTHENTICATED,     {401, "Unauthorized"}},
    {catena::StatusCode::PERMISSION_DENIED,   {403, "Forbidden"}},
    {catena::StatusCode::NOT_FOUND,           {404, "Not Found"}},
    {catena::StatusCode::METHOD_NOT_ALLOWED,  {405, "Method Not Allowed"}},
    {catena::StatusCode::CONFLICT,            {409, "Conflict"}},
    {catena::StatusCode::ABORTED,             {409, "Conflict"}},
    {catena::StatusCode::FAILED_PRECONDITION, {412, "Precondition Failed"}},
    {catena::StatusCode::UNPROCESSABLE_ENTITY,{422, "Unprocessable Entity"}},
    {catena::StatusCode::RESOURCE_EXHAUSTED,  {429, "Too Many Requests"}},
    {catena::StatusCode::TOO_MANY_REQUESTS,   {429, "Too Many Requests"}},
    {catena::StatusCode::OUT_OF_RANGE,        {416, "Range Not Satisfiable"}},
    {catena::StatusCode::BAD_GATEWAY,         {502, "Bad Gateway"}},
    {catena::StatusCode::UNIMPLEMENTED,       {501, "Not Implemented"}},
    {catena::StatusCode::INTERNAL,            {500, "Internal Server Error"}},
    {catena::StatusCode::UNKNOWN,             {500, "Internal Server Error"}},
    {catena::StatusCode::DATA_LOSS,           {500, "Internal Server Error"}},
    {catena::StatusCode::DO_NOT_USE,          {500, "Internal Server Error"}},
    {catena::StatusCode::SERVICE_UNAVAILABLE, {503, "Service Unavailable"}},
    {catena::StatusCode::UNAVAILABLE,         {503, "Service Unavailable"}},
    {catena::StatusCode::GATEWAY_TIMEOUT,     {504, "Gateway Timeout"}},
};

}; // Namespace REST
}; // Namespace catena
