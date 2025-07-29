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
 * @file CallData.h
 * @brief Base class for gRPC CallData classes which defines getJWSToken_().
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// gRPC/interface
#include "interface/IServiceImpl.h"
#include "interface/ICallData.h"

// common
#include <rpc/TimeNow.h>

// gRPC
#include <grpcpp/grpcpp.h>
using grpc::ServerContext;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::Status;
using grpc::ServerCompletionQueue;

// std
#include <vector>
#include <mutex>

namespace catena {
namespace gRPC {
 
/**
 * @brief CallData states.
 */
enum class CallStatus { kCreate, kProcess, kRead, kWrite, kPostWrite, kFinish };

/**
 * @brief Abstract base class for the CallData classes which defines the
 * jwsToken_ class.
 */
class CallData : public ICallData {
  protected:
    /**
     * @brief CallData constructor which initializes service_.
     */
    CallData(IServiceImpl *service): service_(service) {}
    /**
     * @brief Extracts the JWS Bearer token from the server context's
     * client metadata.
     * @return The JWS Bearer token as a string.
     * @throw catena::exception_with_status if the token is not found.
     */
    std::string jwsToken_() const override {
        std::string jwsToken = "";  
        if (service_->authorizationEnabled()) {
            // Getting client metadata from context.
            auto clientMeta = &context_.client_metadata();
            if (clientMeta != nullptr) {
                // Getting authorization data (JWS bearer token) from client metadata.
                auto authData = clientMeta->find("authorization");
                if (authData != clientMeta->end() && authData->second.starts_with("Bearer ")) {
                    // Getting token (after "bearer") and returning as an std::string.
                    auto tokenSubStr = authData->second.substr(std::string("Bearer ").length());
                    jwsToken = std::string(tokenSubStr.begin(), tokenSubStr.end());
                } else {
                    throw catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
                }
            } else {
                throw catena::exception_with_status("Client metadata not found", catena::StatusCode::UNAUTHENTICATED);
            }
        }
        return jwsToken;
    }

    /**
     * @brief The context of the gRPC command.
     */
    ServerContext context_;
    /**
     * @brief Pointer to ServiceImpl
     */
    IServiceImpl* service_;
};

};
};
