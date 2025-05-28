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
 * @brief This file is for testing the DeviceRequest.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// Test helpers
#include "gRPCMockClasses.h"

// gRPC
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCDeviceRequestTests : public ::testing::Test {
  protected:
    /*
     * Called at the start of all tests.
     * Starts the mockServer and initializes the static inVal.
     */
    static void SetUpTestSuite() {
        mockServer.start();
    }

    /*
     * Sets up expectations for the creation of a new CallData obj.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // We can always assume that a new CallData obj is created.
        // Either from initialization or kProceed.
        mockServer.expNew();
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and compares the streamed-back response.
     */
    class TestRPC : public grpc::ClientReadReactor<catena::DeviceComponent> {
        public:
            /*
             * This function makes an async RPC to the MockServer.
             */
            void MakeCall(catena::CatenaService::Stub* client, const catena::DeviceRequestPayload& inVal) {
                client->async()->DeviceRequest(&context, &inVal, this);
                StartRead(&outVal);
                StartCall();
            }
            /*
             * This function triggers when a read is done and compares the
             * returned outVal with the expected response.
             */
            void OnReadDone(bool ok) override {
                if (ok) {
                    // Test value.
                    StartRead(&outVal);
                }
            }
            /*
             * This function triggers when the RPC is finished and notifies
             * Await().
             */
            void OnDone(const grpc::Status& status) override {
                outRc = status;
                done = true;
                cv.notify_one();
            }
            /*
             * This function is used to wait for the RPC to finish and compares
             * the rc with what was expected.
             */
            void Await() {
                // std::unique_lock<std::mutex> lock{cv_mtx};
                cv.wait(lock, [this] { return done; });
                // Comparing the results.
                EXPECT_EQ(outRc.error_code(), expRc.error_code());
                EXPECT_EQ(outRc.error_message(), expRc.error_message());
            }
        
            grpc::ClientContext context;
            catena::DeviceComponent expVal;
            catena::DeviceComponent outVal;
            grpc::Status expRc;
            grpc::Status outRc;

            bool done = false;
            std::condition_variable cv;
            std::mutex cv_mtx;
            std::unique_lock<std::mutex> lock{cv_mtx};
    };

    /*
     * Restores cout after each test.
     */
    void TearDown() override {
        std::cout.rdbuf(oldCout);
    }

    /*
     * Called at the end of all tests, shuts down the server and cleans up.
     */
    static void TearDownTestSuite() {
        // Redirecting cout to a stringstream for testing.
        std::stringstream MockConsole;
        std::streambuf* oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // Destroying the server.
        EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
            delete mockServer.testCall;
            mockServer.testCall = nullptr;
        }));
        mockServer.shutdown();
        // Restoring cout
        std::cout.rdbuf(oldCout);
    }

    // Console variables
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    TestRPC testRPC;

    static MockServer mockServer;
};

MockServer gRPCDeviceRequestTests::mockServer;

/*
 * ============================================================================
 *                               DeviceRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating a DeviceRequest object.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_create) {
    // Creating deviceRequest object.
    new DeviceRequest(mockServer.service, *mockServer.dm, true);
    EXPECT_FALSE(mockServer.testCall);
    EXPECT_TRUE(mockServer.asyncCall);
}

/*
 * TEST 2 - Normal case for GetParam proceed().
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_proceedNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testRPC.expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    catena::DeviceRequestPayload inVal;
    std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
    std::cerr <<"MockSerialer: "<<mockSerializer.get() << std::endl;
    // testRPC.expVal.set

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockSerializer](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            return std::move(mockSerializer);
        }));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(3).WillRepeatedly(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockSerializer, getNext()).Times(3)
        .WillOnce(::testing::Return(catena::DeviceComponent()))
        .WillOnce(::testing::Return(catena::DeviceComponent()))
        .WillOnce(::testing::Return(catena::DeviceComponent()));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(3)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    testRPC.MakeCall(mockServer.client.get(), inVal);
    testRPC.Await();
}

/*
 * TEST 3 - Normal case for GetParam proceed().
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_end) {
   
}
