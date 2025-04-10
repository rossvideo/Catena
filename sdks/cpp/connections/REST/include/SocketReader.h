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
 * @file SocketReader.h
 * @brief Helper class used to read from a socket using boost.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include <Status.h>

// connections/REST
#include "interface/ISocketReader.h"

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

#include <string>
#include <iostream>

namespace catena {
namespace REST {

/**
 * @brief Helper class used to read from a socket using boost.
 */
class SocketReader : public ISocketReader {
  public:
    /**
     * @brief Populates variables using information read from the inputted
     * socket.
     * @param socket The socket to read from.
     * @param authz Flag to indicate if authorization is enabled.
     */
    void read(tcp::socket& socket, bool authz = false) override;
    /**
     * @brief Returns the method of the request.
     */
    const std::string& method() const override { return method_; }
    /**
     * @brief Returns the rpc of the request.
     */
    const std::string& rpc() const override { return rpc_; }
    /**
     * @brief Parsed req_ for fields.
     * 
     * @param fields A map continaing the fields to parse and an empty
     * string to place the parsed field in.
     * fields is order dependent. The keys must be placed in the same order
     * in which they appear in the URL. Additionally, the last field is
     * assumed to be until the end of the request unless specified
     * otherwise.
     * 
     * @todo Update once URL format is finalized.
     */
    void fields(std::unordered_map<std::string, std::string>& fieldMap) const override;
    /**
     * @brief Returns the client's jws token.
     */
    const std::string& jwsToken() const override { return jwsToken_; }
    /**
     * @brief Returns the json body of the request, which may be empty.
     */
    const std::string& jsonBody() const override { return jsonBody_; }
    /**
     * @brief Returns the origin of the request.
     */
    const std::string& origin() const override { return origin_; }
    /**
     * @brief Returns the agent used to send the request.
     */
    const std::string& userAgent() const override { return userAgent_; }

    /**
     * @brief Returns true if authorization is enabled.
     */
    bool authorizationEnabled() const override { return authorizationEnabled_; };

  private:
    /**
     * @brief The method of the request (GET, PUT, etc.).
     */
    std::string method_ = "";
    /**
     * @brief The rpc of the request (/v1/GetValue, etc.)
     */
    std::string rpc_ = "";
    /**
     * @brief The request string (bit after "method .../rpc_").
     */
    std::string req_ = "";
    /**
     * @brief The client's jws token (empty if authorization is disabled).
     */
    std::string jwsToken_ = "";
    /**
     * @brief The json body included with the request (empty if no body).
     */
    std::string jsonBody_ = "";
    /**
     * @brief The origin of the request. Required for CORS headers.
     */
    std::string origin_ = "";
    /**
     * @brief The agent the request was sent from.
     */
    std::string userAgent_ = "";
    /**
     * @brief True if authorization is enabled.
     */
    bool authorizationEnabled_ = false;
};

}; // Namespace REST
}; // Namespace catena
