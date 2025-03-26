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
#include <IParam.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>

namespace catena {

/**
 * @brief REST API
 */
class API {

  using Device = catena::common::Device;
  using IParam = catena::common::IParam;
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

    // idk why this is public I'll fix at some point.
    Device &dm_;

  private:
    boost::asio::io_context io_context_;
    Tcp::acceptor acceptor_;

    std::string version_;
    uint16_t port_;
    crow::SimpleApp app_;
    bool authorizationEnabled_;

    /**
     * @brief Helper class used to write to a socket using boost.
     */
    class SocketWriter {
      public:
        /**
         * @brief Constructs a SocketWriter.
         * @param socket The socket to write to.
         */
        SocketWriter(Tcp::socket& socket) : socket_{socket} {}
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
        Tcp::socket& socket_;
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
     */
    class ChunkedWriter : public SocketWriter {
      public:
      // Using parent constructor
        using SocketWriter::SocketWriter;
        /**
         * @brief Writes headers to the socket in chuncked encoding format.
         * @param status The catena::exception_with_status of the operation.
         */
        void writeHeaders(catena::exception_with_status& status);
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
        /**
         * @brief Getter for hasHeaders_.
         */
        bool hasHeaders() const { return hasHeaders_; }
      private:
        /**
         * @brief Indicates whether the writer has written headers or not. 
         */
        bool hasHeaders_ = false;
    };

    /**
     * @brief Parses fields from the request URL.
     * @param request The request URL to extract fields from.
     * @param fields An unordered map of fields to populate. The keys are the
     * field names, and their values will become whatever is between any key
     * and the next key in the map. As such, the order of these keys must match
     * their order in the URL.
     */
    void parseFields(std::string& request, std::unordered_map<std::string, std::string>& fields) const;
    /**
     * @returns The slots that are populated by dm_.
     */
    void getPopulatedSlots(Tcp::socket& socket);
    /**
     * @brief The deviceRequest REST call.
     * @param request The request URL to extract fields from.
     * @param socket The socket to communicate with the client with.
     * @param authz The authorizer object containing client's scopes.
     * 
     * @todo Maybe also pass in JSON body for subscribed oids. Kind of a pain
     * to put them in the URL.
     */
    void deviceRequest(std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz);
    /**
     * @brief The getValue REST call.
     * @param request The request URL to extract fields from.
     * @param socket The socket to communicate with the client with.
     * @param authz The authorizer object containing client's scopes.
     */
    void getValue(std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz);
    /**
     * @brief The setValue REST call.
     * @param jsonPayload The JSON payload containing the oid and value to set.
     * @param socket The socket to communicate with the client with.
     * @param authz The authorizer object containing client's scopes.
     * 
     * Right now it is identical to multiSetValue save for one line. Having two
     * functions will be more justified once MultiSetValuePayload is fixed.
     */
    void setValue(std::string& jsonPayload, Tcp::socket& socket, catena::common::Authorizer* authz);
    /**
     * @brief The multiSet REST call.
     * @param jsonPayload The JSON payload containing the oids and values to set.
     * @param socket The socket to communicate with the client with.
     * @param authz The authorizer object containing client's scopes.
     */
    void multiSetValue(std::string& jsonPayload, Tcp::socket& socket, catena::common::Authorizer* authz);
    /**
     * @brief Helper function for setValue calls. Tries/sets value and writes
     * to socket.
     * @param payload The MultiSetValuePayload.
     * @param writer The SockerWriter created above.
     * @param authz The authorizer object containing client's scopes.
     */
    void multiSetValue(catena::MultiSetValuePayload& payload, SocketWriter& writer, catena::common::Authorizer* authz);
    void connect(std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz);
    void updateResponse(bool& hasUpdate, catena::PushUpdates& res, catena::common::Authorizer* authz, const std::string& oid, const int32_t idx, const IParam* p);
    /**
     * @brief Routes a request to the appropriate controller.
     * @param method The HTTP method extracted from the URL (GET, POST, PUT).
     * @param request The request extracted from the URL (/v1/DeviceRequest).
     * @param socket The socket to communicate with the client with.
     * @param authz The authorizer object containing client's scopes.
     * @returns Nothing, errors are thrown or communicated through the socket.
     */
    void route(std::string& method, std::string& request, std::string& jsonPayload, Tcp::socket& socket, catena::common::Authorizer* authz);

  private:
  bool is_port_in_use_() const;
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
