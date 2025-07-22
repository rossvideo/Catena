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
#include <Authorization.h>
#include <Enums.h>
#include <SubscriptionManager.h>
#include <SharedFlags.h>

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
class CatenaServiceImpl : public catena::REST::ICatenaServiceImpl {
  public:
    /**
     * @brief Constructor for the REST API.
     * 
     * @param dm The device to implement Catena services to.
     * @param EOPath The path to the external object.
     * @param authz Flag to enable authorization.
     * @param port The port to listen on. Default is 443.
     */
    explicit CatenaServiceImpl(std::vector<IDevice*> dms, std::string& EOPath, bool authz = false, uint16_t port = 443, uint32_t maxConnections = 16);

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
     * @brief Flag to set authorization as enabled or disabled
     */
    inline bool authorizationEnabled() const override { return authorizationEnabled_; }
    /**
     * @brief Get the subscription manager
     * @return Reference to the subscription manager
     */
    inline catena::common::ISubscriptionManager& subscriptionManager() override { return subscriptionManager_; }
    /**
     * @brief Returns the EOPath.
     */
    const std::string& EOPath() override { return EOPath_; }
    /**
     * @brief Regesters a Connect CallData object into the Connection priority queue.
     * @param cd The Connect CallData object to register.
     * @return TRUE if successfully registered, FALSE otherwise
     */
    bool registerConnection(catena::common::IConnect* cd) override;
    /**
     * @brief Deregisters a Connect CallData object into the Connection priority queue.
     * @param cd The Connect CallData object to deregister.
     */
    void deregisterConnection(catena::common::IConnect* cd) override;

  private:
    /**
     * @brief Returns true if port_ is already in use.
     * 
     * Currently unused.
     */
    bool is_port_in_use_() const override;

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
     * @brief Mutex to protect the connectionQueue 
     */
    std::mutex connectionMutex_;
    /**
     * @brief The priority queue for Connect CallData objects.
     * 
     * Not an actual priority queue object since individual access is required
     * for deregistering old connections.
     */
    std::vector<catena::common::IConnect*> connectionQueue_;
    /**
     * @brief The maximum number of connections allowed to the service.
     */
    uint32_t maxConnections_;

    uint32_t activeRequests_ = 0;

    std::mutex activeRequestMutex_;
     /**
     * @brief Returns the size of the registry.
     */
    uint32_t registrySize() const override { return registry_.size(); }
    /**
     * @brief Registers a CallData object into the registry
     * @param cd The CallData object to register
     */
    void registerItem(ICallData *cd) override;
    /**
     * @brief Deregisters a CallData object from registry
     * @param cd The CallData object to deregister
     */
    void deregisterItem(ICallData *cd) override;
    // Aliases for special vectors and unique_ptrs.
    using Registry = std::vector<std::unique_ptr<ICallData>>;
    using RegistryItem = std::unique_ptr<ICallData>;
    /**
     * @brief The registry of CallData objects
     */
    Registry registry_;
    /**
     * @brief Mutex to protect the registry 
     */
    std::mutex registryMutex_;

    using Router = catena::patterns::GenericFactory<catena::REST::ICallData,
                                                    std::string,
                                                    ICatenaServiceImpl*,
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
