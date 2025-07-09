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
#include "interface/ICallData.h"

// protobuf
#include <interface/device.pb.h>
#include <interface/service.grpc.pb.h>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

// common
#include <Status.h>

namespace catena {
namespace gRPC {

/*
 * GRPCTest class inherited by test fixtures to provide functions for
 * writing, reading, and verifying requests and responses.
 */
class GRPCTest : public ::testing::Test {
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
        // oldCout_ = std::cout.rdbuf(MockConsole_.rdbuf());
        
        // Creating the gRPC server.
        builder_.AddListeningPort(serverAddr_, grpc::InsecureServerCredentials());
        cq_ = builder_.AddCompletionQueue();
        builder_.RegisterService(&service_);
        server_ = builder_.BuildAndStart();

        // Creating the gRPC client.
        channel_ = grpc::CreateChannel(serverAddr_, grpc::InsecureChannelCredentials());
        client_ = catena::CatenaService::NewStub(channel_);

        // Setting common expected values for the mock service.
        EXPECT_CALL(service_, registerItem(::testing::_)).WillRepeatedly(::testing::Invoke([this](ICallData* cd) {
            asyncCall_.reset(cd);
        }));
        EXPECT_CALL(service_, cq()).WillRepeatedly(::testing::Return(cq_.get()));
        EXPECT_CALL(service_, deregisterItem(::testing::_)).WillRepeatedly(::testing::Invoke([this](ICallData* cd) {
            testCall_.reset(nullptr);
        }));
        EXPECT_CALL(dm0_, mutex()).WillRepeatedly(::testing::ReturnRef(mtx0_));
        EXPECT_CALL(dm0_, slot()).WillRepeatedly(::testing::Return(0));
        EXPECT_CALL(dm1_, mutex()).WillRepeatedly(::testing::ReturnRef(mtx1_));
        EXPECT_CALL(dm1_, slot()).WillRepeatedly(::testing::Return(1));
        EXPECT_CALL(service_, authorizationEnabled()).WillRepeatedly(::testing::Invoke([this](){ return authzEnabled_; }));

        // Deploying cq handler on a thread.
        cqthread_ = std::make_unique<std::thread>([&]() {
            void* ignored_tag;
            bool ok;
            while (cq_->Next(&ignored_tag, &ok)) {
                if (!testCall_) {
                    testCall_.swap(asyncCall_);
                }
                if (testCall_) {
                    testCall_->proceed(ok);
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
        server_->Shutdown();
        // Cleaning the cq
        cq_->Shutdown();
        cqthread_->join();
        // Make sure the calldata objects were destroyed.
        ASSERT_FALSE(testCall_) << "Failed to deregister handler";
        ASSERT_FALSE(asyncCall_) << "Failed to deregister handler";
        // Restoring cout
        // std::cout.rdbuf(oldCout_);
    }

    // Cout variables
    std::stringstream MockConsole_;
    std::streambuf* oldCout_;

    // Expected variables
    catena::exception_with_status expRc_{"", catena::StatusCode::OK};

    // Address used for gRPC tests.
    std::string serverAddr_ = "0.0.0.0:50051";
    // Server and service variables.
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_ = nullptr;
    MockServiceImpl service_;
    std::mutex mtx0_;
    std::mutex mtx1_;
    MockDevice dm0_;
    MockDevice dm1_;
    SlotMap dms_ = {{0, &dm0_}, {1, &dm1_}};
    bool authzEnabled_ = false;
    // Completion queue variables.
    std::unique_ptr<grpc::ServerCompletionQueue> cq_ = nullptr;
    std::unique_ptr<std::thread> cqthread_ = nullptr;
    bool ok_ = true;
    // Client variables.
    std::shared_ptr<grpc::Channel> channel_ = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> client_ = nullptr;
    grpc::ClientContext clientContext_;
    bool done_ = false;
    std::condition_variable cv_;
    std::mutex cv_mtx_;
    std::unique_lock<std::mutex> lock_{cv_mtx_};
    grpc::Status outRc_;
    // gRPC test variables.
    std::unique_ptr<ICallData> testCall_ = nullptr;
    std::unique_ptr<ICallData> asyncCall_ = nullptr;
};

} // namespace gRPC
} // namespace catena
