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
 * @brief This file is for testing the ServiceImpl.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/24
 * @copyright Copyright © 2025 Ross Video Ltd
 */

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
#include <grpcpp/grpcpp.h>

#include "ServiceImpl.h"

// common
#include <Status.h>

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCServiceImplTests : public testing::Test {
  protected:
    /*
     * Redirects cout and creates a grpc server around our service for testing.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout_ = std::cout.rdbuf(MockConsole_.rdbuf());
        EXPECT_CALL(dm_, slot()).WillRepeatedly(testing::Return(0));
        // Creating the gRPC server.
        builder_.AddListeningPort(serverAddr_, grpc::InsecureServerCredentials());
        cq_ = builder_.AddCompletionQueue();
        service_.reset(new CatenaServiceImpl(cq_.get(), {&dm_}, EOPath_, authzEnabled_));
        builder_.RegisterService(service_.get());
        server_ = builder_.BuildAndStart();
        service_->init();
        ASSERT_EQ(service_->registrySize(), 14) << "ServiceImpl failed to register " << 14 - service_->registrySize() << " CallData objects";
        cqthread_ = std::make_unique<std::thread>([&]() { service_->processEvents(); });
        // Creating the gRPC client.
        channel_ = grpc::CreateChannel(serverAddr_, grpc::InsecureChannelCredentials());
        client_ = catena::CatenaService::NewStub(channel_);
    }

    /*
     * Restored cout and cleans up the server.
     */
    void TearDown() override {
        // Cleaning up the server.
        server_->Shutdown();
        // Cleaning the cq
        cq_->Shutdown();
        cqthread_->join();
        // Make sure all items are derigistered before destroying the service.
        EXPECT_EQ(service_->registrySize(), 0) << "ServiceImpl failed to deregister " << service_->registrySize() << " CallData objects"; 
        // Restoring cout
        std::cout.rdbuf(oldCout_);
    }

    // Cout variables
    std::stringstream MockConsole_;
    std::streambuf* oldCout_;

    // Address used for gRPC tests.
    std::string serverAddr_ = "0.0.0.0:50051";
    // Server and service variables.
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_ = nullptr;
    std::unique_ptr<CatenaServiceImpl> service_ = nullptr;
    MockDevice dm_;
    // Completion queue variables.
    std::unique_ptr<grpc::ServerCompletionQueue> cq_ = nullptr;
    std::unique_ptr<std::thread> cqthread_ = nullptr;
    // Client variables.
    std::shared_ptr<grpc::Channel> channel_ = nullptr;
    std::unique_ptr<catena::CatenaService::Stub> client_ = nullptr;
    grpc::ClientContext clientContext_;
    bool done_ = false;
    std::condition_variable cv_;
    std::mutex cv_mtx_;
    std::unique_lock<std::mutex> lock_{cv_mtx_};
    // Random serviceImpl variables.
    std::string EOPath_ = "/Test/EO/Path";
    bool authzEnabled_ = false;
};

/*
 * TEST 1 - Test creation and destruction of the service implementation.
 */
TEST_F(gRPCServiceImplTests, ServiceImpl_CreateDestroy) {
    ASSERT_TRUE(service_);
    EXPECT_EQ(service_->authorizationEnabled(), authzEnabled_);
    EXPECT_EQ(service_->EOPath(), EOPath_);
    // Give it time to set up and timeout once.
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    service_->shutdownServer(); // Does nothing.
}
