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
 * @brief This file is for testing the GetParam.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/28
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
#include "GRPCTest.h"
#include "MockParam.h"

// gRPC
#include "controllers/GetParam.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCGetParamTests : public GRPCTest {
  protected:
    gRPCGetParamTests() : GRPCTest() {
        // Setting up the inVal used across all tests.
        inVal.set_slot(1);
        inVal.set_oid("/test_oid");
        // Setting up the test param.
        // Just enough to tell its the same object.
        testParam.set_type(catena::ParamType::STRING);
        testParam.mutable_value()->set_string_value("test_value");
        testParam.add_oid_aliases("test_alias");
        (*testParam.mutable_name()->mutable_display_strings())["en"] = "Test Param";
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        mockServer.client->async()->GetParam(&clientContext, &inVal, &outVal, [this](grpc::Status status){
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

    catena::GetParamPayload inVal;
    catena::DeviceComponent_ComponentParam outVal;
    grpc::Status outRc;
    // Expected variables
    catena::Param testParam;
    catena::DeviceComponent_ComponentParam expVal;
    grpc::Status expRc;
};

/*
 * ============================================================================
 *                               GetParam tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetParam object.
 */
TEST_F(gRPCGetParamTests, GetParam_create) {
    // Creating getParam object.
    new GetParam(mockServer.service, *mockServer.dm, true);
    EXPECT_FALSE(mockServer.testCall);
    EXPECT_TRUE(mockServer.asyncCall);
}

/*
 * TEST 2 - Normal case for GetParam proceed().
 */
TEST_F(gRPCGetParamTests, GetParam_proceed) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    expVal.set_oid(inVal.oid());
    expVal.mutable_param()->CopyFrom(testParam);
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockParam);
        }));
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::Param &param, catena::common::Authorizer &authz)  {
            // Checking that function gets correct inputs.
            EXPECT_EQ(&authz, &Authorizer::kAuthzDisabled);
            param.CopyFrom(testParam);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 3 - GetParam with authz on and valid token.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedAuthzValid) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("", catena::StatusCode::OK);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    expVal.set_oid(inVal.oid());
    expVal.mutable_param()->CopyFrom(testParam);
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
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

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_FALSE(&authz == &Authorizer::kAuthzDisabled);
            status = catena::exception_with_status(rc.what(), rc.status);
            return std::move(mockParam);
        }));
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::Param &param, catena::common::Authorizer &authz)  {
            // Checking that function gets correct inputs.
            EXPECT_FALSE(&authz == &Authorizer::kAuthzDisabled);
            param.CopyFrom(testParam);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 4 - GetParam with authz on and invalid token.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedAuthzInvalid) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Not a token so it should get rejected by the authorizer.
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 5 - GetParam with authz on and invalid token.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedAuthzJWSNotFound) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Should not be able to find the bearer token.
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(2).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 6 - dm.getParam() returns a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedErrGetParamReturnCatena) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            status = catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 7 - dm.getParam() throws a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedErrGetParamThrowCatena) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 8 - dm.getParam() throws a std::runtime_exception.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedErrGetParamThrowUnknown) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return nullptr;
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 9 - param->toProto() returns a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedErrToProtoReturnCatena) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Return(catena::exception_with_status(rc.what(), rc.status)));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 10 - param->toProto() throws a catena::exception_with_status.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedErrToProtoThrowCatena) {
    new GetParam(mockServer.service, *mockServer.dm, true);

    catena::exception_with_status rc("Oid does not exist", catena::StatusCode::INVALID_ARGUMENT);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::Param &param, catena::common::Authorizer &authz)  {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}

/*
 * TEST 11 - param->toProto() throws a std::runtime_exception.
 */
TEST_F(gRPCGetParamTests, GetParam_proceedErrToProtoThrowUnknown) {
    new GetParam(mockServer.service, *mockServer.dm, true);
    
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    expRc = grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();

    // Mocking kProcess and kFinish functions
    EXPECT_CALL(*mockServer.service, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(*mockServer.dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockServer.mtx));
    EXPECT_CALL(*mockServer.dm, getParam(inVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(expVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this, &rc](catena::Param &param, catena::common::Authorizer &authz)  {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(*mockServer.service, deregisterItem(::testing::_)).Times(1).WillOnce(::testing::Invoke([this]() {
        delete mockServer.testCall;
        mockServer.testCall = nullptr;
    }));

    // Sending the RPC.
    testRPC();
}
