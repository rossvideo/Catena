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
 * @file IServiceImpl.h
 * @brief Interface class for the gRPC API Implementation.
 * @author Benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include <SubscriptionManager.h>

// gRPC/interface
#include "ICallData.h"

// gRPC interface
#include <interface/service.grpc.pb.h>

using namespace catena::common;

/**
 * @brief Namespace containing classes relating to the gRPC API.
 */
namespace catena {
namespace gRPC {

/**
 * @brief Interface class for the gRPC API Implementation.
 */
class ICatenaServiceImpl : public catena::CatenaService::AsyncService {
  public:
    /**
     * @brief Creates the CallData objects for each gRPC command.
     */
    virtual void init() = 0;
    /**
     * @brief Processes events in the server's completion queue.
     */
    virtual void processEvents() = 0;
    /**
     * @brief Shuts down the server.
     */
    virtual void shutdownServer() = 0;
    /**
     * @brief Flag to set authorization as enabled or disabled
     */
    virtual inline bool authorizationEnabled() const = 0;
    /**
     * @brief Get the subscription manager
     * @return Reference to the subscription manager
     */
    virtual inline ISubscriptionManager& getSubscriptionManager() = 0;
    /**
     * @brief Returns a pointer to the server's completion queue.
     */
    virtual grpc::ServerCompletionQueue* cq() = 0;
    /**
     * @brief Returns the EOPath.
     */
    virtual const std::string& EOPath() = 0;
    /**
     * @brief Registers a CallData object into the registry
     * @param cd The CallData object to register
     */
    virtual void registerItem(ICallData *cd) = 0;
    /**
     * @brief Deregisters a CallData object from registry
     * @param cd The CallData object to deregister
     */
    virtual void deregisterItem(ICallData *cd) = 0;
};

};
};
