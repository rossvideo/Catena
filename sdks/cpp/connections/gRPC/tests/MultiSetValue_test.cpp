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
 * @brief This file is for testing the MultiSetValue.cpp file.
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
#include "controllers/MultiSetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCMultiSetValueTests : public ::testing::Test {
  protected:
    // Sets up the inVal and expVal before each test.
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // inVal.set_oid("/test_oid");
        // expVal.set_string_value("Test string");
    }

    // Makes an async RPC to the MockServer and waits for a response.
    void makeRPC() {
        mockServer.client->async()->MultiSetValue(&clientContext, &inVal, &outVal, [this](grpc::Status status){
            outRc = status;
            done = true;
            cv.notify_one();
        });
        cv.wait(lock, [this] { return done; });
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
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
    // Client variables.
    grpc::ClientContext clientContext;
    bool done = false;
    std::condition_variable cv;
    std::mutex cv_mtx;
    std::unique_lock<std::mutex> lock{cv_mtx};
    catena::MultiSetValuePayload inVal;
    catena::Empty outVal;
    grpc::Status outRc;
    // Expected variables
    catena::Empty expVal;
    grpc::Status expRc;

    static MockServer mockServer;
};

MockServer gRPCMultiSetValueTests::mockServer;

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a MultiSetValue object.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_create) {
    // Making sure multiSetValue is created from the SetUp step.
    mockServer.start();
    // Creating multiSetValue object.
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1).WillOnce(::testing::Return());
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));

    // Creating multiSetValue object.
    mockServer.testCall = new MultiSetValue(mockServer.service, *mockServer.dm, true);
    EXPECT_TRUE(mockServer.testCall);
    EXPECT_FALSE(mockServer.asyncCall);
}
