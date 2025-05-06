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
 * @file ServiceImpl.h
 * @brief Implements Catena gRPC request handlers
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-06-08
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

#include "interface/IServiceImpl.h"

// gRPC
#include <controllers/GetPopulatedSlots.h>
#include <controllers/GetValue.h>
#include <controllers/SetValue.h>
#include <controllers/MultiSetValue.h>
#include <controllers/Connect.h>
#include <controllers/DeviceRequest.h>
#include <controllers/GetParam.h>
#include <controllers/ExternalObjectRequest.h>
#include <controllers/BasicParamInfoRequest.h>
#include <controllers/ExecuteCommand.h>
#include <controllers/AddLanguage.h>
#include <controllers/ListLanguages.h>
#include <controllers/LanguagePackRequest.h>
#include <controllers/UpdateSubscriptions.h>

// common
#include <Status.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <IDevice.h>
#include <Authorization.h>

// gRPC interface
#include <interface/service.grpc.pb.h>
#include <SubscriptionManager.h>
#include <ISubscriptionManager.h>

#include <grpcpp/grpcpp.h>
// #include <jwt-cpp/jwt.h>

#include <condition_variable>
#include <chrono>

using grpc::ServerContext;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::Status;
using grpc::ServerCompletionQueue;

using catena::common::IDevice;
using catena::common::IParam;

namespace catena {
namespace gRPC {

/**
 * @brief Implements Catena gRPC request handlers.
 */
class CatenaServiceImpl : public ICatenaServiceImpl {
  public:
    /**
     * @brief Constructor for the CatenaServiceImpl class.
     * @param cq The completion queue for the server.
     * @param dm The device to implement Catena services to.
     * @param EOPath The path to the external object.
     * @param authz Flag to enable authorization.
     */
    CatenaServiceImpl(ServerCompletionQueue* cq, IDevice& dm, std::string& EOPath, bool authz);  
    /**
     * @brief Creates the CallData objects for each gRPC command.
     */
    void init() override;
    /**
     * @brief Processes events in the server's completion queue.
     */
    void processEvents() override;
    /**
     * @brief Not implemented
     */
    void shutdownServer() override {};
    /**
     * @brief Flag to set authorization as enabled or disabled
     */
    inline bool authorizationEnabled() const override { return authorizationEnabled_; }
    /**
     * @brief Get the subscription manager
     * @return Reference to the subscription manager
     */
    inline catena::common::ISubscriptionManager& getSubscriptionManager() override { return *subscriptionManager_; }
    /**
     * @brief Returns a pointer to the server's completion queue.
     */
    grpc::ServerCompletionQueue* cq() override { return cq_; };
    /**
     * @brief Returns the EOPath.
     */
    const std::string& EOPath() override { return EOPath_; };
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

  private:
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
    /**
     * @brief The completion queue for the server for event processing
     */
    ServerCompletionQueue* cq_; 
    /**
     * @brief The device to implement Catena services to
     */
    IDevice& dm_;
    /**
     * @brief The path to the external object
     */
    std::string& EOPath_; 
    /**
     * @brief Flag to enable authorization
     */
    bool authorizationEnabled_;
    /**
     * @brief The subscription manager for handling parameter subscriptions
     */
    std::unique_ptr<catena::common::ISubscriptionManager> subscriptionManager_;
};

}; // namespace gRPC
}; // namespace catena
