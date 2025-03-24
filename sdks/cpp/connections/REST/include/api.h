/*
 * Copyright 2024 Ross Video Ltd
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @file api.h
 * @brief Implements REST API
 * @author unknown
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

#define CROW_ENABLE_SSL
#include <crow.h>

// common
#include <Device.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>

namespace catena {

/**
 * @brief REST API
 */
class API {

  using Device = catena::common::Device;
  using Tcp = boost::asio::ip::tcp;

  public:
    // explicit API(uint16_t port = 443) : version_{"1.0.0"}, port_{port} {}
    explicit API(Device &dm, uint16_t port = 443);
    virtual ~API() = default;
    API(const API&) = delete;
    API& operator=(const API&) = delete;
    API(API&&) = delete;
    API& operator=(API&&) = delete;

    /**
     * @brief Get the API Version
     *
     * @return std::string
     */
    std::string version() const;

    /**
     * @brief run the API
     */
    void run();

    Device &dm_;

  private:
    boost::asio::io_context io_context_;
    Tcp::acceptor acceptor_;

    std::string version_;
    uint16_t port_;
    crow::SimpleApp app_;
    bool authorizationEnabled_;

    // TODO Remove getJWSToken.
    std::string getJWSToken(const crow::request& req) const;
    std::string getJWSToken(Tcp::socket& socket) const;

    /**
     * @brief Writes a protobuf message to socket in JSON format.
     * @param socket The socket to write to.
     * @param msg The protobuf message to write as JSON.
     */
    void write(Tcp::socket& socket, google::protobuf::Message& msg) const;
    // Deprecated.
    crow::response finish(google::protobuf::Message& msg) const;
    /**
     * @brief Extracts a field from the request string extracted from the URL.
     * @param request The request extracted from the URL (/v1/DeviceRequest).
     * @param field The field to extract (slot, oid, etc).
     */
    std::string getField(std::string& request, std::string field) const;

    /**
     * @returns The slots that are populated by dm_.
     */
    crow::response getPopulatedSlots();
    void deviceRequest(std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz);
    /**
     * @brief The getValue REST call.
     * @param req A crow::request which can be converted into JSON.
     * JSON object should contain keys "slot" and "oid".
     * @returns A crow::response containing the value at the end of oid or an
     * error message if something goes wrong.
     */
    crow::response getValue(const crow::request& req, int slot, std::string& oid);

    crow::response setValue(const crow::request& req);
    crow::response multiSetValue(const crow::request& req);
    crow::response multiSetValue(catena::MultiSetValuePayload& payload, const crow::request& req);
    /**
     * @brief Routes a request to the appropriate controller.
     * @param method The HTTP method extracted from the URL (GET, POST, PUT).
     * @param request The request extracted from the URL (/v1/DeviceRequest).
     * @param socket The socket to communicate with the client with.
     * @param authz The authorizer object containing client's scopes.
     * @returns Nothing, errors are thrown or communicated through the socket.
     */
    void route(std::string& method, std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz);

  private:
  bool is_port_in_use_() const;

  /**
   * @todo clean this up.
   */
  const std::map<catena::StatusCode, int> toCrowStatus_ {
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
}  // namespace catena

// flags for the API
// flags.h
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

// Declare flags for the API
ABSL_DECLARE_FLAG(std::string, certs);
ABSL_DECLARE_FLAG(uint16_t, port);
ABSL_DECLARE_FLAG(bool, mutual_authc);
ABSL_DECLARE_FLAG(bool, authz);
ABSL_DECLARE_FLAG(std::string, static_root);
