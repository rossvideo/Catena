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
 * @brief Implements the SocketReader class.
 * @author benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// suppress clang warning about comments that contain "/*"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomment"

// common
#include <Status.h>
#include <Enums.h>
#include <ISubscriptionManager.h>
#include <utils.h>

// connections/REST
#include "interface/ISocketReader.h"
#include "interface/IServiceImpl.h"

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
using boost::asio::ip::tcp;
using namespace boost::urls;

#include <string>

namespace catena {
namespace REST {

/**
 * @brief A helper class which reads from the client socket using boost and
 * extracts relevant information pertaining to a REST request.
 * 
 * This includes (if applicable):
 * 
 * - The HTTP method (GET, POST, etc.)
 * 
 * - The endpoint being accessed (param, etc.)
 * 
 * - The slot of the device to make the API call on.
 * 
 * - The fqoid of an asset to access.
 * 
 * - The fields queried from the URL.
 * 
 * - The client's jws token.
 * 
 * - The origin of the request.
 * 
 * - The detail level to return the response in.
 * 
 * - The json body of the request.
 * 
 * - A flag indicating whether to return the response as a stream.
 * 
 * It also contains links to several objects from the service, such as the
 * subscription manager, the external object path, and the connection queue.
 */
class SocketReader : public ISocketReader {
  public:
    /**
     * @brief Constructor for the SocketReader class.
     * @param service Pointer to the ServiceImpl.
     */
    SocketReader(IServiceImpl* service) : service_(service) {};
    /**
     * @brief Reads information from the inputted socket and extracts relevant
     * information pertaining to the REST request.
     * @param socket The socket to read from.
     */
    void read(tcp::socket& socket) override;
    /**
     * @brief Returns the HTTP method of the request.
     */
    RESTMethod method() const override { return method_; }
    /**
     * @brief Returns the endpoint of the request (parameter, etc.)
     */
    const std::string& endpoint() const override { return endpoint_; }
    /**
     * @brief Returns the slot of the device to make the API call on.
     */
    uint32_t slot() const override { return slot_; };
    /**
     * @brief Returns the fqoid of the asset to make the API call on.
     */
    const std::string& fqoid() const override { return fqoid_; };
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
     * @brief Returns the detail level to return the response in.
     */
    st2138::Device_DetailLevel detailLevel() const override { return detailLevel_; };
    /**
     * @brief Returns the json body of the request, which may be empty.
     */
    const std::string& jsonBody() const override { return jsonBody_; }
    /**
     * @brief Returns true if the client wants a stream response.
     */
    bool stream() const override { return stream_; } 

    /**
     * @brief Returns a pointer to the ServiceImpl
     */
    IServiceImpl* service() override { return service_; }
    /**
     * @brief Returns true if authorization is enabled for this service.
     */
    bool authorizationEnabled() const override { return service_->authorizationEnabled(); }
    /**
     * @brief Returns the path to the external object.
     */
    const std::string& EOPath() const override { return service_->EOPath();}
    /**
     * @brief Returns the ConnectionQueue object.
     */
    catena::common::IConnectionQueue& connectionQueue() override { return service_->connectionQueue(); }
    /**
     * @brief Returns a reference to the subscription manager
     */
    catena::common::ISubscriptionManager& subscriptionManager() override { return service_->subscriptionManager(); }


  private:
    /**
     * @brief The HTTP method of the request (GET, PUT, etc.).
     */
    RESTMethod method_;
    /**
     * @brief The slot of the device to make the API call on.
     */
    uint32_t slot_ = 0;
    /**
     * @brief The endpoint being accessed (/v1/GetValue, etc.)
     */
    std::string endpoint_ = "";
    /**
     * @brief The fqoid of the asset to make the API call on.
     */
    std::string fqoid_ = "";
    /**
     * @brief Flag indicating whether the client wants a stream response.
     */
    bool stream_ = false;
    /**
     * @brief The origin of the request. Required for CORS headers.
     */
    std::string origin_ = "";
    /**
     * @brief The detail level to return the response in.
     */
    st2138::Device_DetailLevel detailLevel_;
    /**
     * @brief The client's jws token (empty if authorization is disabled).
     */
    std::string jwsToken_ = "";
    /**
     * @brief The json body included with the request (empty if no body).
     */
    std::string jsonBody_ = "";
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
     * @brief Pointer to the ServiceImpl
     */
    IServiceImpl* service_ = nullptr;
};

}; // Namespace REST
}; // Namespace catena

template <>
inline const catena::REST::SocketReader::RESTMethodMap::FwdMap catena::REST::SocketReader::RESTMethodMap::fwdMap_ = {
  {catena::REST::Method_NONE,    "NONE"},
  {catena::REST::Method_GET,     "GET"},
  {catena::REST::Method_POST,    "POST"},
  {catena::REST::Method_PUT,     "PUT"},
  {catena::REST::Method_PATCH,   "PATCH"},
  {catena::REST::Method_DELETE,  "DELETE"},
  {catena::REST::Method_HEAD,    "HEAD"},
  {catena::REST::Method_OPTIONS, "OPTIONS"}
};

#pragma clang diagnostic pop
