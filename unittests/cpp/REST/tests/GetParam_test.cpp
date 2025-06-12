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
 * @date 25/05/21
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
#include "RESTTest.h"
#include "MockSocketReader.h"
#include "MockParam.h"
#include "MockDevice.h"

// REST
#include "controllers/GetParam.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTGetParamTests : public ::testing::Test, public RESTTest {
  protected:
    RESTGetParamTests() : RESTTest(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating getParam object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        getParam = GetParam::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (getParam) {
            delete getParam;
        }
    }
    
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    std::mutex mockMtx;
    MockDevice dm;
    catena::REST::ICallData* getParam = nullptr;
};

/*
 * ============================================================================
 *                               GetParam tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetParam object with makeOne.
 */
TEST_F(RESTGetParamTests, GetParam_create) {
    // Making sure getParam is created from the SetUp step.
    ASSERT_TRUE(getParam);
}

/*
 * TEST 2 - Normal case for GetParam proceed().
 */
TEST_F(RESTGetParamTests, GetParam_proceedNormal) {
    // Setting up the returnVal to test with.
    catena::DeviceComponent_ComponentParam returnVal;
    std::string jsonBody = "{\"oid\":\"/test_oid\",\"param\":{}}";
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(jsonBody), &returnVal);
    // Other variables.
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(returnVal.oid()));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(returnVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    // Defining param->toProto()
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(returnVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&returnVal, &rc](catena::Param &param, catena::common::Authorizer &authz)
        -> catena::exception_with_status {
            param.CopyFrom(returnVal.param());
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    jsonBody = "";
    status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/*
 * TEST 3 - GetParam with authz on and valid token.
 */
TEST_F(RESTGetParamTests, GetParam_proceedAuthzValid) {
    // Setting up the returnVal to test with.
    catena::DeviceComponent_ComponentParam returnVal;
    std::string jsonBody = "{\"oid\":\"/test_oid\",\"param\":{}}";
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(jsonBody), &returnVal);
    // Other variables.
    /* Authz just tests for a properly encrypted token, proxy handles authz.
     * This is a random RSA token I made jwt.io it is not a security risk I
     * swear. */
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
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(returnVal.oid()));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(returnVal.oid(), ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    // Defining param->toProto()
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(returnVal.oid()));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&returnVal, &rc](catena::Param &param, catena::common::Authorizer &authz)
        -> catena::exception_with_status {
            param.CopyFrom(returnVal.param());
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    jsonBody = "";
    status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/*
 * TEST 4 - GetParam with authz on and invalid token.
 */
TEST_F(RESTGetParamTests, GetParam_proceedAuthzInvalid) {
    // Not a token so it should get rejected by the authorizer.
    std::string mockToken = "THIS SHOULD NOT PARSE";
    // Test status
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 5 - dm.getParam() returns a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_getParamErrReturnCatena) {
    // Test status
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(mockOid, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            status = catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 6 - dm.getParam() throws a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_getParamErrThrowCatena) {
    // Test status
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(mockOid, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            throw catena::exception_with_status(rc.what(), rc.status);
            return nullptr;
        }));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 7 - dm.getParam() throws an std::runtime_error.
 */
TEST_F(RESTGetParamTests, GetParam_getParamErrThrowUnknown) {
    // Test status
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(mockOid, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Throw(std::runtime_error("Unknown error")));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 8 - param->toProto() returns a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_toProtoErrReturnCatena) {
    // Other variables.
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(mockOid, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    // Defining param->toProto()
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Return(catena::exception_with_status(rc.what(), rc.status)));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 9 - param->toProto() throws a catena::exception_with_status.
 */
TEST_F(RESTGetParamTests, GetParam_toProtoErrThrowCatena) {
    // Other variables.
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(mockOid, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    // Defining param->toProto()
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::Param &param, catena::common::Authorizer &authz)
        -> catena::exception_with_status {
            throw catena::exception_with_status(rc.what(), rc.status);
        }));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 10 - param->toProto() throws an std::runtime_error.
 */
TEST_F(RESTGetParamTests, GetParam_toProtoErrThrowUnknown) {
    // Other variables.
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    std::string mockOid = "/test_oid";

    // Defining mock functions
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    EXPECT_CALL(context, fqoid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    // Defining Device.getParam()
    EXPECT_CALL(dm, getParam(mockOid, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&mockParam, &rc](const std::string &fqoid, catena::exception_with_status &status, catena::common::Authorizer &authz)
        -> std::unique_ptr<IParam> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));
    // Defining param->toProto()
    EXPECT_CALL(*mockParam, getOid()).Times(1).WillOnce(::testing::ReturnRef(mockOid));
    EXPECT_CALL(*mockParam, toProto(::testing::An<catena::Param&>(), ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error("Unknown error")));
    
    // Calling proceed() and checking written response.
    getParam->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * TEST 11 - Writing to console with GetParam finish().
 */
TEST_F(RESTGetParamTests, GetParam_finish) {
    // Calling finish and expecting the console output.
    getParam->finish();
    ASSERT_TRUE(MockConsole.str().find("GetParam[10] finished\n") != std::string::npos);
}
