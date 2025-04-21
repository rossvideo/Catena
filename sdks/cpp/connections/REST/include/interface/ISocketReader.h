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
 * @file ISocketReader.h
 * @brief Interface for the SocketReader class.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include <SubscriptionManager.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

#include <string>
#include <unordered_map>
 
namespace catena {
namespace REST {
 
/**
 * @brief Interface for the SocketReader class.
 * 
 * Overriden constructor should be passed in a socket, which should then be
 * read to populate any necessary fields.
 */
class ISocketReader {
  public:
    ISocketReader() = default;
    ISocketReader(const ISocketReader&) = default;
    ISocketReader& operator=(const ISocketReader&) = default;
    ISocketReader(ISocketReader&&) = default;
    ISocketReader& operator=(ISocketReader&&) = default;
    virtual ~ISocketReader() = default;

    /**
     * @brief Populates variables using information read from the inputted
     * socket.
     * @param socket The socket to read from.
     * @param authz Flag to indicate if authorization is enabled.
     */
    virtual void read(tcp::socket& socket, bool authz = false) = 0;
    /**
     * @brief Returns the method of the request.
     */
    virtual const std::string& method() const = 0;
    /**
     * @brief Returns the rpc of the request.
     */
    virtual const std::string& service() const = 0;
    /**
     * @brief Returns the slot of the device to make the API call on.
     */
    virtual uint32_t slot() const = 0;
    /**
     * @brief Returns the field "key" queried from the URL, or an empty sting
     * if it does not exist.
     * 
     * @param key The name of the field to retrieve.
     */
    virtual const std::string& fields(const std::string& key) const = 0;
    /**
     * @brief Returns the client's jws token.
     */
    virtual const std::string& jwsToken() const = 0;
    /**
     * @brief Returns the origin of the request.
     */
    virtual const std::string& origin() const = 0;
    /**
     * @brief Returns the language to return the resposne in.
     */
    virtual const std::string& language() const = 0;
    /**
     * @brief Returns the detail level to return the response in.
     */
    virtual catena::Device_DetailLevel detailLevel() const = 0;
    /**
     * @brief Returns the json body of the request, which may be empty.
     */
    virtual const std::string& jsonBody() const = 0;

    /**
     * @brief Returns true if authorization is enabled.
     */
    virtual bool authorizationEnabled() const = 0;

    /**
     * @brief Returns a reference to the subscription manager
     */
    virtual catena::common::SubscriptionManager& getSubscriptionManager() = 0;
};
 
}; // Namespace REST
}; // Namespace catena
 