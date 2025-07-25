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
 * @file ServiceImpl.h
 * @brief Implements REST API
 * @author Benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include <Status.h>
#include <vdk/signals.h>
#include <patterns/GenericFactory.h>
#include <IParam.h>
#include <Authorizer.h>
#include <Enums.h>
#include <SubscriptionManager.h>
#include <SharedFlags.h>
#include <rpc/ConnectionQueue.h>

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

using namespace catena::common;

namespace catena {
/**
 * @brief Namespace for classes relating to handling REST API requests.
 */
namespace REST {

/**
 * @brief Implements Catena REST API request handlers.
 */
class ServiceImpl : public catena::REST::IServiceImpl {
  public:
    /**
     * @brief Constructor for the REST API.
     * 
     * @param dm The device to implement Catena services to.
     * @param EOPath The path to the external object.
     * @param authz Flag to enable authorization.
     * @param port The port to listen on. Default is 443.
     * @param maxConnections The maximum # of connections the service allows.
     */
    explicit ServiceImpl(std::vector<IDevice*> dms, std::string& EOPath, bool authz = false, uint16_t port = 443, uint32_t maxConnections = 16);
    /**
     * @brief Returns the API's version.
     */
    const std::string& version() const override { return version_; }
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
    inline bool authorizationEnabled() const override { return authorizationEnabled_; }
    /**
     * @brief Get the subscription manager
     * @return Reference to the subscription manager
     */
    inline ISubscriptionManager& subscriptionManager() override { return subscriptionManager_; }
    /**
     * @brief Returns the EOPath.
     */
    const std::string& EOPath() override { return EOPath_; }
    /**
     * @brief Returns the ConnectionQueue object.
     */
    IConnectionQueue& connectionQueue() override { return connectionQueue_; };

  private:
    /**
     * @brief Returns true if port_ is already in use.
     * 
     * Currently unused.
     */
    bool is_port_in_use_() const;

    /**
     * @brief Provides io functionality for tcp::sockets used in requests.
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
     * @brief A map of slots to ptrs to their corresponding device.
     * 
     * Devices are global objects so raw ptrs should be safe.
     */
    SlotMap dms_;
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
     * @brief The subscription manager for handling parameter subscriptions
     */
    catena::common::SubscriptionManager subscriptionManager_;
    /**
     * @brief The # of active requests. Increments after a socket is recieved
     * and decrements once that request is finished.
     */
    uint32_t activeRequests_ = 0;
    /**
     * @brief Mutex to protect the activeRequests_ variable.
     */
    std::mutex activeRequestMutex_;
    /**
     * @brief The connectionQueue object for managing connections to the service
     */
    ConnectionQueue connectionQueue_;

    using Router = catena::patterns::GenericFactory<catena::REST::ICallData,
                                                    std::string,
                                                    tcp::socket&,
                                                    ISocketReader&,
                                                    SlotMap&>;
    /**
     * @brief Creating an ICallData factory for handling request routing.
     */
    Router& router_;
};

}; // namespace REST
}; // namespace catena
