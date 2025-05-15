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
 * @author zuhayr.sarker@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include <Status.h>
#include <Enums.h>
#include <ISubscriptionManager.h>

// connections/REST
#include "interface/ISocketReader.h"

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
using boost::asio::ip::tcp;
using namespace boost::urls;

#include <string>
#include <iostream>

namespace catena {
namespace REST {

// Forward declaration
class CatenaServiceImpl;

/**
 * @brief Helper class used to read from a socket using boost.
 */
class SocketReader : public ISocketReader {
  public:
    /**
     * @brief Constructor for the SocketReader class.
     * @param subscriptionManager The subscription manager to use.
     * @param EOPath The path to external objects
     */
    SocketReader(catena::common::ISubscriptionManager& subscriptionManager, const std::string& EOPath = "");
    /**
     * @brief Populates variables using information read from the inputted
     * socket.
     * @param socket The socket to read from.
     * @param authz Flag to indicate if authorization is enabled.
     */
    void read(tcp::socket& socket, bool authz = false) override;
    /**
     * @brief Returns the HTTP method of the request.
     */
    const std::string& method() const override { return method_; }
    /**
     * @brief Returns the REST endpoint of the request (/v1/GetValue, etc.)
     */
    const std::string& endpoint() const override { return endpoint_; }
    /**
     * @brief Returns the slot of the device to make the API call on.
     */
    uint32_t slot() const override { return slot_; };
    /**
     * @brief Returns true if the field exists in the URL, regardless of its value.
     * 
     * @param key The name of the field to check.
     */
    bool hasField(const std::string& key) const override {
      return fields_.contains(key);
    }
    /**
     * @brief Returns the field "key" queried from the URL, or an empty sting
     * if it does not exist.
     * 
     * @param key The name of the field to retrieve.
     */
    const std::string& fields(const std::string& key) const override {
      if (fields_.contains(key)) {
        return fields_.at(key);
      } else {
        return fieldNotFound;
      }
    }
    /**
     * @brief Returns the client's jws token.
     */
    const std::string& jwsToken() const override { return jwsToken_; }
    /**
     * @brief Returns the origin of the request.
     */
    const std::string& origin() const override { return origin_; }
    /**
     * @brief Returns the language to return the resposne in.
     */
    const std::string& language() const override { return language_; };
    /**
     * @brief Returns the detail level to return the response in.
     */
    catena::Device_DetailLevel detailLevel() const override { return detailLevel_; };
    /**
     * @brief Returns the json body of the request, which may be empty.
     */
    const std::string& jsonBody() const override { return jsonBody_; }
    /**
     * @brief Returns a reference to the subscription manager
     */
    catena::common::ISubscriptionManager& getSubscriptionManager() override { return subscriptionManager_; }

    /**
     * @brief Returns true if authorization is enabled.
     */
    bool authorizationEnabled() const override { return authorizationEnabled_; };\

    /**
     * @brief Returns the path to the external object.
     */
    const std::string& EOPath() const override { return EOPath_; }  

  private:
    /**
     * @brief The method of the request (GET, PUT, etc.).
     */
    std::string method_ = "";
    /**
     * @brief The REST endpoint being accessed (/v1/GetValue, etc.)
     */
    std::string endpoint_ = "";
    /**
     * @brief The slot of the device to make the API call on.
     */
    uint32_t slot_ = 0;
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
     * @brief The language to return the response in.
     */
    std::string language_ = "";
    /**
     * @brief The subscription manager for handling parameter subscriptions
     */
    catena::common::ISubscriptionManager& subscriptionManager_;
    /**
     * @brief The detail level to return the response in.
     */
    catena::Device_DetailLevel detailLevel_;
    /**
     * @brief A map of fields queried from the URL.
     */
    std::unordered_map<std::string, std::string> fields_;
    /**
     * @brief An empty string var to return if the field is not found.
     * Exists to avoid scope issues.
     */
    std::string fieldNotFound = "";
    /**
     * @brief True if authorization is enabled.
     */
    bool authorizationEnabled_ = false;
    /**
     * @brief The path to the external object.
     */
    std::string EOPath_ = ""; 
};

}; // Namespace REST
}; // Namespace catena
