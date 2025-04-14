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
 * @file ServiceImpl
 * @brief Implements REST API
 * @author Benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

// common
#include <Status.h>
#include <vdk/signals.h>
#include <patterns/GenericFactory.h>
#include <IParam.h>
#include <Device.h>
#include <Authorization.h>
#include <Enums.h>

// REST
#include <interface/IServiceImpl.h>
#include <interface/ICallData.h>
#include <SocketReader.h>
#include <SocketWriter.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// std
#include <string>
#include <iostream>
#include <regex>

using catena::REST::SocketReader;
using catena::REST::SocketWriter;
using catena::REST::SSEWriter;

namespace catena {
/**
 * @brief Namespace for classes relating to handling REST API requests.
 */
namespace REST {

/**
 * @brief Implements Catena REST API request handlers.
 */
class CatenaServiceImpl : public catena::REST::IServiceImpl {

  // Specifying which Device and IParam to use (defaults to catena::...)
  using Device = catena::common::Device;
  using IParam = catena::common::IParam;

  public:
    /**
     * @brief Constructor for the REST API.
     * 
     * @param dm The device to implement Catena services to.
     * @param EOPath The path to the external object.
     * @param authz Flag to enable authorization.
     * @param port The port to listen on. Default is 443.
     */
    explicit CatenaServiceImpl(Device &dm, std::string& EOPath, bool authz = false, uint16_t port = 443);

    /**
     * @brief Returns the API's version.
     */
    std::string version() const override { return version_; }
    /**
     * @brief Starts the API.
     */
    void run() override;
    /**
     * @brief Shuts down an running API. This should only be called sometime
     * after a call to run().
     */
    void Shutdown() override;
    /**
     * @brief Returns true if authorization is enabled.
     */
    bool authorizationEnabled() override { return authorizationEnabled_; };
    
  private:
    /**
     * @brief Writes a client's options to the socket.
     */
    void writeOptions(tcp::socket& socket, const std::string& origin);
    /**
     * @brief Returns true if port_ is already in use.
     * 
     * Currently unused.
     */
    bool is_port_in_use_() const override;

    /**
     * @brief Provides io functionality for tcp::sockets used in RPCs.
     */
    boost::asio::io_context io_context_;
    /**
     * @brief Accepts incoming connections from the specified port.
     */
    tcp::acceptor acceptor_;
    /**
     * @brief The API's version.
     */
    std::string version_;
    /**
     * @brief The port to listen to.
     */
    uint16_t port_;
    /**
     * @brief The device to implement Catena services to
     */
    Device& dm_;
    /**
     * @brief The path to the external object
     */
    std::string& EOPath_;
    /**
     * @brief Flag to enable authorization
     */
    bool authorizationEnabled_;
    /**
     * @brief Flag to indicate if shutdown() has been called.
     */
    bool shutdown_ = false;
    /**
     * @brief Counter used to track number of active RPCs. Run() does not
     * finish until this number is 0.
     */
    uint32_t activeRpcs_ = 0;
    /**
     * @brief Mutex for activeRpcs_ counter to avoid collisions.
     */
    std::mutex activeRpcMutex_;

    using Router = catena::patterns::GenericFactory<catena::REST::ICallData,
                                                    std::string,
                                                    tcp::socket&,
                                                    SocketReader&,
                                                    Device&>;
    /**
     * @brief Creating an ICallData factory for handling RPC routing.
     */
    Router& router_;
};

}; // namespace REST
}; // namespace catena

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
