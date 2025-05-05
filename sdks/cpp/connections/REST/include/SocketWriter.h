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
 * 
 * This writer buffers the response until a call to finish() is made.
 */
class SocketWriter : public ISocketWriter {
  public:
    /**
     * @brief Constructs a SocketWriter.
     * @param socket The socket to write to.
     * @param origin The origin of the request.
     */
    SocketWriter(tcp::socket& socket, const std::string& origin = "*") : socket_{socket} {
      CORS_ = "Access-Control-Allow-Origin: " + origin + "\r\n"
              "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
              "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
              "Access-Control-Allow-Credentials: true\r\n";
    }

    /**
     * @brief Adds the protobuf message to the end of response in JSON format.
     * @param msg The protobuf message to add as JSON.
     */
    void write(const google::protobuf::Message& msg) override;

    /**
     * @brief Finishes writing the HTTP response.
     * @param msg The protobuf message to write (if status is OK)
     * @param err The error status to finish with. If status is OK, writes the message, otherwise writes the error.
     */
    void finish(const google::protobuf::Message& msg = catena::Empty(), const catena::exception_with_status& err = catena::exception_with_status("", catena::StatusCode::OK)) override;

  private:
    /**
     * @brief The socket to write to.
     */
    tcp::socket& socket_;
    /**
     * @brief CORS headers used for all responses.
     * Access-Control-Allow-Origin,
     * Access-Control-Allow-Methods,
     * Access-Control-Allow-Headers
     * Access-Control-Allow-Credentials
     */
    std::string CORS_;
    /**
     * @brief The response to write to the socket.
     */
    std::string response_ = "";
    /**
     * @brief Flag indicating whether the response is a multi-part response.
     * 
     * Used to determine formatting when finish() is called.
     */
    bool multi_ = false;
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
     * @param err The catena::exception_with_status for initialization.
     */
    SSEWriter(tcp::socket& socket, const std::string& origin = "*", const catena::exception_with_status& err = catena::exception_with_status("", catena::StatusCode::OK));

    /**
     * @brief Writes a protobuf message to the socket as an SSE.
     * @param msg The protobuf message to write as JSON.
     */
    void write(const google::protobuf::Message& msg) override;

    /**
     * @brief Finishes the SSE stream.
     * @param msg The protobuf message to write (if status is OK)
     * @param err The error status to finish with. If status is OK, writes the message, otherwise writes the error.
     */
    void finish(const google::protobuf::Message& msg = catena::Empty(), const catena::exception_with_status& err = catena::exception_with_status("", catena::StatusCode::OK)) override;

  private:
    /**
     * @brief The socket to write to.
     */
    tcp::socket& socket_;
};

/**
 * @brief Maps catena::StatusCode to HTTP status codes and reasons.
 */
const std::map<catena::StatusCode, std::pair<int, std::string>> codeMap_ {
  {catena::StatusCode::OK,                  {200, "OK"}},
  {catena::StatusCode::CANCELLED,           {400, "Cancelled"}},
  {catena::StatusCode::UNKNOWN,             {500, "Internal Server Error"}},
  {catena::StatusCode::INVALID_ARGUMENT,    {400, "Bad Request"}},
  {catena::StatusCode::DEADLINE_EXCEEDED,   {408, "Request Timeout"}},
  {catena::StatusCode::NOT_FOUND,           {404, "Not Found"}},
  {catena::StatusCode::ALREADY_EXISTS,      {409, "Conflict"}},
  {catena::StatusCode::PERMISSION_DENIED,   {403, "Forbidden"}},
  {catena::StatusCode::UNAUTHENTICATED,     {401, "Unauthorized"}},
  {catena::StatusCode::RESOURCE_EXHAUSTED,  {429, "Too Many Requests"}},
  {catena::StatusCode::FAILED_PRECONDITION, {412, "Precondition Failed"}},
  {catena::StatusCode::ABORTED,             {409, "Conflict"}},
  {catena::StatusCode::OUT_OF_RANGE,        {416, "Range Not Satisfiable"}},
  {catena::StatusCode::UNIMPLEMENTED,       {501, "Not Implemented"}},
  {catena::StatusCode::INTERNAL,            {500, "Internal Server Error"}},
  {catena::StatusCode::UNAVAILABLE,         {503, "Service Unavailable"}},
  {catena::StatusCode::DATA_LOSS,           {500, "Internal Server Error"}},
  {catena::StatusCode::DO_NOT_USE,          {500, "Internal Server Error"}},
  {catena::StatusCode::CREATED,             {201, "Created"}},
  {catena::StatusCode::ACCEPTED,            {202, "Accepted"}},
  {catena::StatusCode::NO_CONTENT,          {204, "No Content"}},
  {catena::StatusCode::UNPROCESSABLE_ENTITY,{422, "Unprocessable Entity"}},
  {catena::StatusCode::METHOD_NOT_ALLOWED,  {405, "Method Not Allowed"}},
  {catena::StatusCode::CONFLICT,            {409, "Conflict"}},
  {catena::StatusCode::TOO_MANY_REQUESTS,   {429, "Too Many Requests"}},
  {catena::StatusCode::BAD_GATEWAY,         {502, "Bad Gateway"}},
  {catena::StatusCode::SERVICE_UNAVAILABLE, {503, "Service Unavailable"}},
  {catena::StatusCode::GATEWAY_TIMEOUT,     {504, "Gateway Timeout"}},
};

}; // Namespace REST
}; // Namespace catena
