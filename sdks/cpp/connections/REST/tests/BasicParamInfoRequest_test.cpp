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
        // oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Set default actions for common mock calls
        EXPECT_CALL(context, origin()).WillOnce(::testing::ReturnRef(origin));
        EXPECT_CALL(context, hasField("recursive")).Times(1).WillOnce(::testing::Return(false));
        EXPECT_CALL(context, fields("oid_prefix")).Times(1).WillOnce(::testing::ReturnRef(mockOid));

        request = BasicParamInfoRequest::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        // std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
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

// Test 1: Creating a BasicParamInfoRequest object
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_create) {
    // Making sure BasicParamInfoRequest object is created from the SetUp step.
    ASSERT_TRUE(request);
}

// Test 2: Get specific parameter without recursion
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_proceedSpecificParam) {
    // Setup mock parameter
    auto param = std::make_unique<MockParam>();
    catena::BasicParamInfoResponse returnVal;
    catena::exception_with_status rc("Invalid parameter", catena::StatusCode::INVALID_ARGUMENT);
    
    // Set up mock expectations for mode 2 (specific parameter)
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMtx));
    
    // First verify getParam is called correctly
    EXPECT_CALL(dm, getParam("/" + mockOid, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string&, catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Invalid parameter", catena::StatusCode::INVALID_ARGUMENT);
            return nullptr;
        }));
    
    // Execute
    request->proceed();
    request->finish();

    // Convert response to JSON
    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options;
    auto status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    
    // Debug output
    // std::string expectedResponse = "HTTP/1.1 400 Bad Request\r\n"
    //                              "Content-Type: text/event-stream\r\n"
    //                              "Cache-Control: no-cache\r\n"
    //                              "Connection: keep-alive\r\n"
    //                              "Access-Control-Allow-Origin: *\r\n"
    //                              "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
    //                              "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
    //                              "Access-Control-Allow-Credentials: true\r\n"
    //                              "\r\n"
    //                              "data: {}\r\n\r\n";
    
    //std::cerr << "Debug - Expected response:\n" << expectedResponse << "\n";
    std::cerr << "Debug - Expected response:\n" << expectedSSEResponse(rc, {jsonBody}) << "\n";
    std::cerr << "Debug - Actual response:\n" << readResponse() << "\n";
    
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, {jsonBody}));
    //EXPECT_EQ(readResponse(), expectedResponse);
}
 

// // Test 3: Get all top-level parameters without recursion
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParams) {
//     catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);

//     // Setup mock parameters
//     std::vector<std::unique_ptr<IParam>> top_level_params;
//     auto param1 = std::make_unique<MockParam>();
//     auto param2 = std::make_unique<MockParam>();
    
//     top_level_params.push_back(std::move(param1));
//     top_level_params.push_back(std::move(param2));

//     // Setup mock expectations
//     std::string empty_prefix;
//     EXPECT_CALL(context, hasField("recursive")).Times(1).WillOnce(::testing::Return(false));
//     EXPECT_CALL(context, fields("oid_prefix")).Times(1).WillOnce(::testing::ReturnRef(empty_prefix));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    
//     EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
//         .Times(1)
//         .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status&, Authorizer&) {
//             rc = catena::exception_with_status("", catena::StatusCode::OK);
//             return std::move(top_level_params);
//         }));

//     // Execute
//     request->proceed();
//     EXPECT_EQ(readResponse(), expectedSSEResponse(rc));
// }

// // Test 3: Get specific parameter with recursion
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getSpecificParamWithRecursion) {
//     // Setup mock parameter
//     auto param = std::make_unique<MockParam>();
//     std::string mockOid = "/test_param";

//     // Setup mock expectations
//     EXPECT_CALL(context, hasField("recursive")).Times(1).WillOnce(::testing::Return(true));
//     EXPECT_CALL(context, fields("oid_prefix")).Times(1).WillOnce(::testing::ReturnRef(mockOid));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    
//     EXPECT_CALL(dm, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
//         .WillOnce(::testing::Invoke([&param](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
//             status = catena::exception_with_status("", catena::StatusCode::OK);
//             return std::move(param);
//         }));

//     // Execute
//     request->proceed();

//     // Verify response using expectedSSEResponse
//     expectedSSEResponse(catena::exception_with_status("", catena::StatusCode::OK));
// }

// // Test 4: Error case - invalid parameter
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_invalidParam) {
//     // Setup mock expectations
//     std::string invalid_param = "/invalid_param";
//     EXPECT_CALL(context, hasField("recursive")).Times(1).WillOnce(::testing::Return(false));
//     EXPECT_CALL(context, fields("oid_prefix")).Times(1).WillOnce(::testing::ReturnRef(invalid_param));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    
//     EXPECT_CALL(dm, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
//         .WillOnce(::testing::Invoke([](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
//             status = catena::exception_with_status("Invalid parameter", catena::StatusCode::INVALID_ARGUMENT);
//             return nullptr;
//         }));

//     // Execute
//     request->proceed();

//     // Verify response using expectedSSEResponse
//     expectedSSEResponse(400, "Bad Request");
// }

// // Test 5: Authorization enabled with valid token
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_authzValid) {
//     // Setup mock parameter
//     auto param = std::make_unique<MockParam>();
//     std::string mockOid = "/test_param";
//     std::string mockToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MTUxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dTokrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UKeOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEmuIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s146M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";

//     // Setup mock expectations
//     std::string test_param = "/test_param";
//     EXPECT_CALL(context, hasField("recursive")).Times(1).WillOnce(::testing::Return(false));
//     EXPECT_CALL(context, fields("oid_prefix")).Times(1).WillOnce(::testing::ReturnRef(test_param));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
//     EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    
//     EXPECT_CALL(dm, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
//         .WillOnce(::testing::Invoke([&param](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
//             status = catena::exception_with_status("", catena::StatusCode::OK);
//             return std::move(param);
//         }));

//     // Execute
//     request->proceed();

//     // Verify response using expectedSSEResponse
//     expectedSSEResponse(200, "OK");
// }

// // Test 6: Authorization enabled with invalid token
// TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_authzInvalid) {
//     // Setup mock expectations
//     std::string test_param = "/test_param";
//     std::string invalid_token = "INVALID_TOKEN";
//     EXPECT_CALL(context, fields("oid_prefix")).Times(1).WillOnce(::testing::ReturnRef(test_param));
//     EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
//     EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(invalid_token));

//     // Execute
//     request->proceed();

//     // Verify response using expectedSSEResponse
//     expectedSSEResponse(401, "Unauthorized");
// }