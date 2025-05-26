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
 * @brief This file is for testing the GetValue.cpp file.
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
#include "controllers/GetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetValueTests : public ::testing::Test {
  protected:
    // Sets up the inVal and expVal before each test.
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        inVal.set_oid("/test_oid");
        expVal.set_string_value("Test string");
    }

    // Makes an async RPC to the MockServer and waits for a response.
    void makeRPC() {
        mockServer.client->async()->GetValue(&clientContext, &inVal, &outVal, [this](grpc::Status status){
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
    catena::GetValuePayload inVal;
    catena::Value outVal;
    grpc::Status outRc;
    // Expected variables
    catena::Value expVal;
    grpc::Status expRc;

    static MockServer mockServer;
};

MockServer gRPCGetValueTests::mockServer;

/*
 * ============================================================================
 *                               GetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetValue object.
 */
TEST_F(gRPCGetValueTests, GetValue_create) {
    // Making sure getValue is created from the SetUp step.
    mockServer.start();
    // Creating getValue object.
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1).WillOnce(::testing::Return());
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));

    // Creating getValue object.
    mockServer.testCall = new GetValue(mockServer.service, *mockServer.dm, true);
    EXPECT_TRUE(mockServer.testCall);
    EXPECT_FALSE(mockServer.asyncCall);
}

/*
 * TEST 2 - Normal case for GetValue proceed().
 */
TEST_F(gRPCGetValueTests, GetValue_proceed) {
    // Setting up an rcs.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getValue("/test_oid", ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            value.CopyFrom(expVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), expVal.string_value());
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}

/*
 * TEST 3 - GetValue with authz on and valid token.
 */
TEST_F(gRPCGetValueTests, GetValue_proceedAuthzValid) {
    // Setting up an rcs.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Adding authorization mockToken metadata. This it a random RSA token.
    std::string mockToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIi"
                            "OiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2Nvc"
                            "GUiOiJzdDIxMzg6bW9uOncgc3QyMTM4Om9wOncgc3QyMTM4Om"
                            "NmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MTUxNjIzOTAyMiw"
                            "ibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dTo"
                            "krEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko965"
                            "3v0khyUT4UKeOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKH"
                            "kWi4P3-CYWrwe-g6b4-a33Q0k6tSGI1hGf2bA9cRYr-VyQ_T3"
                            "RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEmuIwNOVM3EcVEaL"
                            "yISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg_w"
                            "bOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9M"
                            "dvJH-cx1s146M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    clientContext.AddMetadata("authorization", "Bearer " + mockToken);

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getValue("/test_oid", ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            value.CopyFrom(expVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), expVal.string_value());
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}

/*
 * TEST 3 - GetValue with authz on and invalid token.
 */
TEST_F(gRPCGetValueTests, GetValue_proceedAuthzInvalid) {
    // Setting up an rcs.
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Not a token so it should get rejected by the authorizer.
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), "");
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}

/*
 * TEST 4 - GetValue with authz on and invalid token.
 */
TEST_F(gRPCGetValueTests, GetValue_proceedAuthzJWSNotFound) {
    // Setting up an rcs.
    catena::exception_with_status rc("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Should not be able to find the bearer token.
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), "");
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}

/*
 * TEST 5 - dm.getValue() returns a catena::exception_with_status.
 */
TEST_F(gRPCGetValueTests, GetValue_proceedErrReturnCatena) {
    // Setting up an rcs.
    catena::exception_with_status rc("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getValue("/test_oid", ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            value.CopyFrom(expVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), "");
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}

/*
 * TEST 6 - dm.getValue() throws a catena::exception_with_status.
 */
TEST_F(gRPCGetValueTests, GetValue_proceedErrThrowCatena) {
    // Setting up an rcs.
    catena::exception_with_status rc("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getValue("/test_oid", ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            value.CopyFrom(expVal);
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), "");
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}

/*
 * TEST 7 - dm.getValue() throws a std::runtime_exception.
 */
TEST_F(gRPCGetValueTests, GetValue_proceedErrThrowUnknown) {
    // Setting up an rcs.
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());

    // Mocking functions.
    // new GetValue(service, dm, true);
    EXPECT_CALL(*mockServer.service, registerItem(::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](ICallData* cd) {
            mockServer.asyncCall = cd;
        }));
    EXPECT_CALL(*mockServer.service, cq()).Times(2).WillRepeatedly(::testing::Return(mockServer.cq.get()));
    // kProcess();
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getValue("/test_oid", ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](const std::string& jptr, catena::Value& value, Authorizer& authz) {
            value.CopyFrom(expVal);
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    makeRPC();

    // Comparing output.
    EXPECT_EQ(outVal.string_value(), "");
    EXPECT_EQ(outRc.error_code(), expRc.error_code());
    EXPECT_EQ(outRc.error_message(), expRc.error_message());
}
