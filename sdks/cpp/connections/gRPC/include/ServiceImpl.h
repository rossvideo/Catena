#pragma once

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

// common
#include <Status.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <Device.h>
#include <Authorization.h>

// gRPC interface
#include <interface/service.grpc.pb.h>

#include <grpcpp/grpcpp.h>
#include <jwt-cpp/jwt.h>

#include <condition_variable>
#include <chrono>

using grpc::ServerContext;
using grpc::ServerAsyncWriter;
using grpc::ServerAsyncResponseWriter;
using grpc::Status;
using grpc::ServerCompletionQueue;

using catena::common::Device;
using catena::common::IParam;

class JWTAuthMetadataProcessor : public grpc::AuthMetadataProcessor {
public:
    grpc::Status Process(const InputMetadata& auth_metadata, grpc::AuthContext* context, 
                         OutputMetadata* consumed_auth_metadata, OutputMetadata* response_metadata) override;
};

class CatenaServiceImpl final : public catena::CatenaService::AsyncService {
  public:
    CatenaServiceImpl(ServerCompletionQueue* cq, Device &dm, std::string& EOPath, bool authz);

    void init();

    void processEvents();

    void shutdownServer();

    /**
     * Nested private classes
     */
    enum class CallStatus { kCreate, kProcess, kRead, kWrite, kPostWrite, kFinish };

  private:
    /**
     * @brief Abstract base class for the CallData classes
     * Provides a the proceed method
     */
    class CallData {
      public:
        virtual void proceed(CatenaServiceImpl *service, bool ok) = 0;
        virtual ~CallData() {}
    };

    std::vector<std::string> getScopes(grpc::ServerContext &context);

    using Registry = std::vector<std::unique_ptr<CatenaServiceImpl::CallData>>;
    using RegistryItem = std::unique_ptr<CatenaServiceImpl::CallData>;
    Registry registry_;
    std::mutex registryMutex_;

    ServerCompletionQueue* cq_;
    Device &dm_;
    std::string& EOPath_;
    bool authorizationEnabled_;

    static std::string timeNow();

  public:

    void registerItem(CallData *cd);

    void deregisterItem(CallData *cd);

    inline bool authorizationEnabled() const { return authorizationEnabled_; }

    //Forward declarations of CallData classes for their respective RPC
    class GetPopulatedSlots;
    class GetValue;
    class SetValue;
    class Connect;
    class DeviceRequest;
    class ExternalObjectRequest;
    // class GetParam;
    class ExecuteCommand;
};