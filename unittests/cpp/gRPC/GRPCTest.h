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
 * @brief A parent class for gRPC test fixtures.
 * @author christian.twarogn@rossvideo.com
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

#include "MockDevice.h"
#include "MockServiceImpl.h"

// protobuf
#include <interface/device.pb.h>
#include <interface/service.grpc.pb.h>
#include <google/protobuf/util/json_util.h>

// common
#include <Status.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

using namespace catena::gRPC;

/*
 * GRPCTest class inherited by test fixtures to provide functions for
 * writing, reading, and verifying requests and responses.
 */
class GRPCTest  : public ::testing::Test {
  protected:
    /*
     * Virtual function which creates a single CallData object for the test.
     */
    virtual void makeOne() = 0;
    
    /*
     * Creates a gRPC server and redirects cout before each test.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        
        // Creating the gRPC server.
        builder.AddListeningPort(serverAddr, grpc::InsecureServerCredentials());
        cq = builder.AddCompletionQueue();
        builder.RegisterService(&service);
        server = builder.BuildAndStart();

        // Creating the gRPC client.
        channel = grpc::CreateChannel(serverAddr, grpc::InsecureChannelCredentials());
        client = catena::CatenaService::NewStub(channel);

        // Setting common expected values for the mock service.
        EXPECT_CALL(service, registerItem(::testing::_)).WillRepeatedly(::testing::Invoke([this](ICallData* cd) {
            asyncCall.reset(cd);
        }));
        EXPECT_CALL(service, cq()).WillRepeatedly(::testing::Return(cq.get()));
        EXPECT_CALL(service, deregisterItem(::testing::_)).WillRepeatedly(::testing::Invoke([this](ICallData* cd) {
            testCall.reset(nullptr);
        }));
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mtx));
        EXPECT_CALL(service, authorizationEnabled()).WillRepeatedly(::testing::Invoke([this](){ return authzEnabled; }));

        // Deploying cq handler on a thread.
        cqthread = std::make_unique<std::thread>([&]() {
            void* ignored_tag;
            bool ok;
            while (cq->Next(&ignored_tag, &ok)) {
                if (!testCall) {
                    testCall.swap(asyncCall);
                }
                if (testCall) {
                    testCall->proceed(ok);
                }
            }
        });

        // Creating the CallData object for testing.
        makeOne();
    }

    /*
     * Shuts down the gRPC server and restores cout after each test.
     */
    void TearDown() override {
        // Cleaning up the server.
        server->Shutdown();
        // Cleaning the cq
        cq->Shutdown();
        cqthread->join();
        // Make sure the calldata objects were destroyed.
        ASSERT_FALSE(testCall) << "Failed to deregister handler";
        ASSERT_FALSE(asyncCall) << "Failed to deregister handler";
        // Restoring cout
        std::cout.rdbuf(oldCout);
    }

    // Cout variables
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    // Expected variables
    catena::exception_with_status expRc{"", catena::StatusCode::OK};

    // Address used for gRPC tests.
    std::string serverAddr = "0.0.0.0:50051";
    // Server and service variables.
    grpc::ServerBuilder builder;
    std::unique_ptr<grpc::Server> server = nullptr;
    MockServiceImpl service;
    std::mutex mtx;
    MockDevice dm;
    bool authzEnabled = false;
    // Completion queue variables.
    std::unique_ptr<grpc::ServerCompletionQueue> cq = nullptr;
    std::unique_ptr<std::thread> cqthread = nullptr;
    bool ok = true;
    // Client variables.
    std::shared_ptr<grpc::Channel> channel = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> client = nullptr;
    grpc::ClientContext clientContext;
    bool done = false;
    std::condition_variable cv;
    std::mutex cv_mtx;
    std::unique_lock<std::mutex> lock{cv_mtx};
    grpc::Status outRc;
    // gRPC test variables.
    std::unique_ptr<ICallData> testCall = nullptr;
    std::unique_ptr<ICallData> asyncCall = nullptr;
};
