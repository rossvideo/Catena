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
 * @brief This file is for testing the SetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/27
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
#include "controllers/SetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCSetValueTests : public ::testing::Test {
  protected:
    /*
     * Called at the start of all tests.
     * Starts the mockServer and initializes the static inVal.
     */
    static void SetUpTestSuite() {
        mockServer.start();
        // Setting up the inVal used across all tests.
        inVal.set_slot(1);
        auto value = inVal.mutable_value();
        value->set_oid("/test_oid");
        value->mutable_value()->set_string_value("test_value");
        // Converting above to multiSetValuePayload for input testing.
        expMultiVal.set_slot(inVal.slot());
        expMultiVal.add_values()->CopyFrom(*value);
    }

    /*
     * Sets up expectations for the creation of a new CallData obj.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());
        // We can always assume that a new MultiSetValuePayload is created.
        // Either from initialization or kProceed.
        mockServer.expNew();
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        mockServer.client->async()->SetValue(&clientContext, &inVal, &outVal, [this](grpc::Status status){
            outRc = status;
            done = true;
            cv.notify_one();
        });
        cv.wait(lock, [this] { return done; });
        // Comparing the results.
        EXPECT_EQ(outVal.SerializeAsString(), expVal.SerializeAsString());
        EXPECT_EQ(outRc.error_code(), expRc.error_code());
        EXPECT_EQ(outRc.error_message(), expRc.error_message());
    }

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
    // Client variables.
    grpc::ClientContext clientContext;
    bool done = false;
    std::condition_variable cv;
    std::mutex cv_mtx;
    std::unique_lock<std::mutex> lock{cv_mtx};
    static catena::SingleSetValuePayload inVal;
    static catena::MultiSetValuePayload expMultiVal;
    catena::Empty outVal;
    grpc::Status outRc;
    // Expected variables
    catena::Empty expVal;
    grpc::Status expRc;

    static MockServer mockServer;
};

MockServer gRPCSetValueTests::mockServer;
// Static as its only used to make sure the correct obj is passed into mocked
// functions.
catena::SingleSetValuePayload gRPCSetValueTests::inVal;
catena::MultiSetValuePayload gRPCSetValueTests::expMultiVal;

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a SetValue object. This tests request_().
 */
TEST_F(gRPCSetValueTests, SetValue_create) {
    // Creating multiSetValue object.
    new SetValue(mockServer.service, *mockServer.dm, true);
    EXPECT_FALSE(mockServer.testCall);
    EXPECT_TRUE(mockServer.asyncCall);
}

/*
 * TEST 2 - Normal case for SetValue proceed().
 * This tests both create_() and toMulti_().
 */
TEST_F(gRPCSetValueTests, SetValue_proceedNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs. Mostly care about
            // src here.
            EXPECT_EQ(src.SerializeAsString(), expMultiVal.SerializeAsString());
            EXPECT_EQ(ans.status, catena::StatusCode::OK);
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(rc.what(), rc.status);
            return true;
        }));
    EXPECT_CALL(*mockServer.dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), expMultiVal.SerializeAsString());
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            // Returning status.
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC and comparing the results.
    testRPC();
}
