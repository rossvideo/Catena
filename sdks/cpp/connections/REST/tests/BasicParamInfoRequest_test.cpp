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
 * @brief This file is for testing the BasicParamInfoRequest.cpp file.
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-05-20
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>
#include <memory>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// Test helpers
#include "SocketHelper.h"
#include "RESTMockClasses.h"
#include "../../common/tests/CommonMockClasses.h"
#include "RESTTestHelpers.h"

// REST
#include <controllers/BasicParamInfoRequest.h>
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTBasicParamInfoRequestTests : public ::testing::Test, public SocketHelper {
protected:
    RESTBasicParamInfoRequestTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Set default actions for common mock calls
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
        EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(mockOid));
        EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));

        request = BasicParamInfoRequest::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here6
        if (request) {
            delete request;
        }
    }

    std::stringstream MockConsole;
    std::streambuf* oldCout;
    std::mutex mockMtx;
    
    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* request = nullptr;
    std::string mockOid = "test_param";
};

/*
 * ============================================================================
 *                        BasicParamInfoRequest tests
 * ============================================================================
 */

// Preliminary test: Creating a BasicParamInfoRequest object
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_create) {
    // Making sure BasicParamInfoRequest object is created from the SetUp step.
    ASSERT_TRUE(request);
}

// Test 0.1: Authorization test with std::exceptionh
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_authz_std_exception) {
    catena::exception_with_status rc("Authorization setup failed: Test auth setup failure", catena::StatusCode::UNAUTHENTICATED);

    // Setup mock expectations
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::Throw(std::runtime_error("Test auth setup failure")));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// Test 0.2: Authorization test with invalid token
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_authz_invalid_token) {
    std::string mockToken = "test_token";
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);

    // Setup mock expectations
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(mockToken));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// Test 0.3: Authorization test with valid token
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_authz_valid_token) {
    // Use a valid JWT token that was borrowed from GetValue_test.cpp
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
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Setup mock parameter
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    catena::REST::test::ParamInfo paramInfo{
        .oid = mockOid,
        .type = catena::ParamType::STRING
    };
    catena::REST::test::setupMockParam(mockParam.get(), paramInfo);

    // Setup mock expectations
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(mockToken));
    EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
    EXPECT_CALL(dm, getParam("/" + mockOid, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([&mockParam, &rc](const std::string&, catena::exception_with_status &status, catena::common::Authorizer &) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string jsonBody = catena::REST::test::createParamInfoJson(paramInfo);
    std::string expected = expectedSSEResponse(rc, {jsonBody});
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// == MODE 1 TESTS: Get all top-level parameters without recursion ==

// Test 1.1: Get all top-level parameters without recursion
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParams) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto param1 = std::make_unique<MockParam>();
    auto param2 = std::make_unique<MockParam>();
    
    // Set up mock parameters 
    catena::REST::test::ParamInfo param1_info{
        .oid = "param1",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo param2_info{
        .oid = "param2",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::setupMockParam(param1.get(), param1_info);
    catena::REST::test::setupMockParam(param2.get(), param2_info);
    
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Setup mock expectations
    std::string empty_prefix;
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(empty_prefix));
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Create a new request for this test
    auto topLevelRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    topLevelRequest->proceed();
    topLevelRequest->finish();

    // Get expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(param1_info));
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(param2_info));
    std::string expected = expectedSSEResponse(rc, jsonBodies);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);

    // Cleanup
    delete topLevelRequest;
}

// Test 1.2: Get top-level parameters with error
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsError) {
    catena::exception_with_status rc("Error getting top-level parameters", catena::StatusCode::INTERNAL);
    
    // Setup mock expectations
    std::string empty_prefix;
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(empty_prefix));
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    // Create a new request for this test
    auto topLevelRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    topLevelRequest->proceed();
    topLevelRequest->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);

    // Cleanup
    delete topLevelRequest;
}

// Test 1.3: Get empty top-level parameters
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getEmptyTopLevelParams) {
    catena::exception_with_status rc("No top-level parameters found", catena::StatusCode::NOT_FOUND);
    
    // Setup mock expectations
    std::string empty_prefix;
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(empty_prefix));
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::NOT_FOUND);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    // Create a new request for this test
    auto topLevelRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    topLevelRequest->proceed();
    topLevelRequest->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);

    // Cleanup
    delete topLevelRequest;
}

// Test 1.4: Get top-level parameters with array type
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithArray) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto arrayParam = std::make_unique<MockParam>();
    
    // Set up mock array parameter
    catena::REST::test::ParamInfo arrayParamInfo{
        .oid = "array_param",
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 5
    };
    catena::REST::test::setupMockParam(arrayParam.get(), arrayParamInfo);
    
    // Set up array-specific expectations
    EXPECT_CALL(*arrayParam, isArrayType()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*arrayParam, size()).WillRepeatedly(::testing::Return(5));
    
    top_level_params.push_back(std::move(arrayParam));

    // Setup mock expectations
    std::string empty_prefix;
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(empty_prefix));
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Create a new request for this test
    auto topLevelRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    topLevelRequest->proceed();
    topLevelRequest->finish();

    // Get expected and actual responses
    std::string jsonBody = catena::REST::test::createParamInfoJson(arrayParamInfo);
    std::string expected = expectedSSEResponse(rc, {jsonBody});
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);

    // Cleanup
    delete topLevelRequest;
}

// Test 1.5: Get top-level parameters with error during processing of a parameter
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsProcessingError) {
    catena::exception_with_status rc("Error processing parameter", catena::StatusCode::INTERNAL);
    
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto errorParam = std::make_unique<MockParam>();
    
    // Set up mock parameter with error during processing
    catena::REST::test::ParamInfo errorParamInfo{
        .oid = "error_param",
        .type = catena::ParamType::STRING,
        .status = 500  // 500 maps to INTERNAL
    };
    catena::REST::test::setupMockParam(errorParam.get(), errorParamInfo);
        
    top_level_params.push_back(std::move(errorParam));

    // Setup mock expectations
    std::string empty_prefix;
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(empty_prefix));
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
            return std::move(top_level_params);
        }));

    // Create a new request for this test
    auto topLevelRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    topLevelRequest->proceed();
    topLevelRequest->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);

    // Cleanup
    delete topLevelRequest;
}

// The following work, but are not the focus of this branch!

// == MODE 3 TESTS: Get a specific parameter and its children if recursive ==

// Test 3.1: Get specific parameter without recursion
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_proceedSpecificParam) {
//     catena::exception_with_status rc("", catena::StatusCode::OK);
    
//     // Setup mock parameter with our helper
//     std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
//     catena::REST::test::ParamInfo paramInfo{
//         .oid = mockOid,
//         .type = catena::ParamType::STRING
//     };
//     catena::REST::test::setupMockParam(mockParam.get(), paramInfo);

//     // Setup mock expectations for mode 2 (specific parameter)
//     EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
//     EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(mockOid));
//     EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
//     EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
//     EXPECT_CALL(dm, getParam("/" + mockOid, ::testing::_, ::testing::_))
//         .WillOnce(::testing::Invoke(
//             [&mockParam](const std::string&, catena::exception_with_status& status, Authorizer&) {
//                 status = catena::exception_with_status("", catena::StatusCode::OK);
//                 return std::move(mockParam);
//             }
//         ));
    
//     // Execute
//     request->proceed();
//     request->finish();

//     // Get expected and actual responses
//     std::string jsonBody = catena::REST::test::createParamInfoJson(paramInfo);
//     std::string expected = expectedSSEResponse(rc, {jsonBody});
//     std::string actual = readResponse();
//     EXPECT_EQ(actual, expected);
// }

// // Test 3.2: Get specific parameter with recursion
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getSpecificParamWithRecursion) {
//     catena::exception_with_status rc("", catena::StatusCode::OK);
    
//     // Setup mock parameter with our helper
//     std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
//     catena::REST::test::ParamInfo paramInfo{
//         .oid = mockOid,
//         .type = catena::ParamType::STRING
//     };
//     catena::REST::test::setupMockParam(mockParam.get(), paramInfo);

//     // Setup mock expectations
//     EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));
//     EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(mockOid));
//     EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
//     EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
//     EXPECT_CALL(dm, getParam("/" + mockOid, ::testing::_, ::testing::_))
//         .WillOnce(::testing::Invoke(
//             [&mockParam](const std::string&, catena::exception_with_status& status, Authorizer&) {
//                 status = catena::exception_with_status("", catena::StatusCode::OK);
//                 return std::move(mockParam);
//             }
//         ));

//     // Execute
//     request->proceed();
//     request->finish();

//     // Get expected and actual responses
//     std::string jsonBody = catena::REST::test::createParamInfoJson(paramInfo);
//     std::string expected = expectedSSEResponse(rc, {jsonBody});
//     std::string actual = readResponse();
//     EXPECT_EQ(actual, expected);
// }

// // Test 3.3: Error case - invalid parameter
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_invalidParam) {
//     catena::exception_with_status rc("Invalid parameter", catena::StatusCode::NOT_FOUND);
    
//     std::string invalid_param = "invalid_param";

//     // Setup mock expectations
//     EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
//     EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(invalid_param));
//     EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));
//     EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
    
//     EXPECT_CALL(dm, getParam("/" + invalid_param, ::testing::_, ::testing::_))
//         .WillOnce(::testing::Invoke([](const std::string&, catena::exception_with_status& status, Authorizer&) {
//             status = catena::exception_with_status("Invalid parameter", catena::StatusCode::NOT_FOUND);
//             return nullptr;
//         }));

//     // Create a new request for this test
//     auto invalidRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

//     // Execute
//     invalidRequest->proceed();
//     invalidRequest->finish();

//     // Get expected and actual responses
//     std::string expected = expectedSSEResponse(rc);
//     std::string actual = readResponse();
//     EXPECT_EQ(actual, expected);

//     // Cleanup
//     delete invalidRequest;
// }

