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
 * @brief A collection of mock classes used across the gRPC tests.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/22
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>

#include "../../common/tests/CommonMockClasses.h"

// common
#include <ISubscriptionManager.h>

// gRPC
#include "interface/IServiceImpl.h"

using namespace catena::common;
using namespace catena::gRPC;

class MockServiceImpl : public ICatenaServiceImpl {
  public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, processEvents, (), (override));
    MOCK_METHOD(void, shutdownServer, (), (override));
    MOCK_METHOD(bool, authorizationEnabled, (), (const, override));
    MOCK_METHOD(ISubscriptionManager&, getSubscriptionManager, (), (override));
    MOCK_METHOD(grpc::ServerCompletionQueue*, cq, (), (override));
    MOCK_METHOD(const std::string&, EOPath, (), (override));
    MOCK_METHOD(void, registerItem, (ICallData* cd), (override));
    MOCK_METHOD(void, deregisterItem, (ICallData* cd), (override));
};

// When created, this calss mimics a gRPC server, allowing us to test easily
// test the various RPCs.
class MockServer {
  public:

    /*
     * Starts the gRPC server and client.
     */
    void start() {
        // Initializing service and device.
        service = new MockServiceImpl();
        dm = new MockDevice();

        // Creating the gRPC server.
        builder.AddListeningPort(serverAddr, grpc::InsecureServerCredentials());
        cq = builder.AddCompletionQueue();
        builder.RegisterService(service);
        server = builder.BuildAndStart();

        // Creating the gRPC client.
        channel = grpc::CreateChannel(serverAddr, grpc::InsecureChannelCredentials());
        client = catena::CatenaService::NewStub(channel);

        // Deploying cq handler on a thread.
        cqthread = std::make_unique<std::thread>([&]() {
            void* ignored_tag;
            bool ignored_ok;
            while (cq->Next(&ignored_tag, &ignored_ok)) {
                if (!testCall) {
                    testCall = asyncCall;
                    asyncCall = nullptr;
                }
                testCall->proceed(ok);
            }
        });
    }

    /*
     * Functions to expect when creating a new CallData object.
     */
    void expNew() {
        EXPECT_CALL(*service, registerItem(::testing::_)).Times(1)
            .WillOnce(::testing::Invoke([this](ICallData* cd) {
                asyncCall = cd;
            }));
        EXPECT_CALL(*service, cq()).Times(2).WillRepeatedly(::testing::Return(cq.get()));
    }

    /*
     * Shuts down the gRPC server and client.
     */
    void shutdown() {
        // Setting ok to false for still queued calls.
        ok = false;
        // Cleaning up the server.
        server->Shutdown();
        // Cleaning the cq
        cq->Shutdown();
        cqthread->join();
        // Make sure the calldata objects were destroyed.
        ASSERT_FALSE(testCall);
        ASSERT_FALSE(asyncCall);
        // Deleting device and service.
        delete dm;
        delete service;
    }

    // Address used for gRPC tests.
    std::string serverAddr = "0.0.0.0:50051";
    // Server and service variables.
    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server = nullptr;
    MockServiceImpl* service;
    std::mutex mtx;
    MockDevice* dm;
    // Completion queue variables.
    std::unique_ptr<grpc::ServerCompletionQueue> cq = nullptr;
    std::unique_ptr<std::thread> cqthread = nullptr;
    bool ok = true;
    // Client variables.
    std::shared_ptr<grpc::Channel> channel = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> client = nullptr;
    // gRPC test variables.
    ICallData* testCall = nullptr;
    ICallData* asyncCall = nullptr;
};
