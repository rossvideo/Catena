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
#include <Authorizer.h>

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

namespace {
static inline bool valid_start_time(grpc::string_ref s) {
    // Empty or larger than 308 chars cannot be converted to a double
    if (s.empty() || s.length() > 308) return false;
    const unsigned char* ps = reinterpret_cast<const unsigned char*>(s.data());
    const std::size_t n = s.size();
    bool hasPeriod = false;

    for(std::size_t i = 0; i < n; ++i) {
        unsigned char cs = ps[i];

        if (!isdigit(cs)) {
            if (cs == '.' && i == 0) {
                return false; // Reject leading period
            }
            else if (cs == '.' && !hasPeriod) {
                hasPeriod = true; // Decimal point found
            } else {
                return false; // cs is 2nd period or invalid character
            }
        }
    }
    // No invalid characters found, if hasPeriod then format is valid.
    return hasPeriod;
}
}

namespace catena {
namespace gRPC {
 
/**
 * @brief CallData states.
 */
enum class CallStatus { kCreate, kProcess, kRead, kWrite, kPostWrite, kFinish };

const double DEFAULT_REQUEST_START = 0.0;
const double DEFAULT_REQUEST_RECEIVED = 0.0;

/**
 * @brief Abstract base class for inherited by CallData child classes defining
 * the jwsToken_() method.
 */
class CallData : public ICallData {
  public:
  
    /**
     * @brief Getter for requestStart_ 
     */
    double getRequestStart_() { return requestStart_; }
    /**
     * @brief Getter for requestReceived_ 
     */
    double getRequestReceieved_() { return requestReceived_; }

  protected:
    /**
     * @brief CallData constructor which sets service_.
     * 
     * @param service Pointer to the ServiceImpl.
     */
    CallData(IServiceImpl *service): service_(service) {}
    /**
     * @brief Extracts the JWS Bearer token from the client's metadata.
     * 
     * Client metadata is included in the ServerContext.
     * The bearer token can then be found under the "authorization" key.
     * 
     * @return The JWS Bearer token with the "Bearer " prefix removed.
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
     * @brief Reads requestStart from metadata and records current time for requestReceived.
     */
    void processTimesatmps_() {
        // Getting request receival time formatted as,
        // <number of seconds since start of epoch>.<number of milliseconds since start of current second>
        const auto epoch_time = std::chrono::system_clock::now().time_since_epoch();
        double receival_time = std::chrono::duration_cast<std::chrono::milliseconds>(epoch_time).count() / 1000.0;
        requestReceived_ = receival_time;

        auto clientMeta = &context_.client_metadata();
        // Getting client metadata from context.
        if (clientMeta != nullptr) {
            auto kv = clientMeta->find("request-start");
            if (kv != clientMeta->end() && valid_start_time(kv->second)) {
                try {
                    char* end = NULL;
                    requestStart_ = strtod(kv->second.data(), &end);
                } catch (...) {
                    throw catena::exception_with_status("Invalid Request Start", catena::StatusCode::INVALID_ARGUMENT);
                }
            }
        } else {
            throw catena::exception_with_status("Client metadata not found", catena::StatusCode::NOT_FOUND);
        }
    }

    /**
     * @brief The context of the RPC. This is retrieved once a call to
     * RequestRPC has been made to register the RPC with the server.
     */
    ServerContext context_;
    /**
     * @brief Pointer to ServiceImpl.
     */
    IServiceImpl* service_;
    /**
     * @brief The time at which the request was sent formatted as,
     * <number of seconds since start of epoch>.<number of milliseconds since start of current second>
     */
    double requestStart_ = DEFAULT_REQUEST_START;
    /**
     * @brief The time at which the request was sent formatted as,
     * <number of seconds since start of epoch>.<number of milliseconds since start of current second>
     */
    double requestReceived_ = DEFAULT_REQUEST_RECEIVED;
};

};
};
