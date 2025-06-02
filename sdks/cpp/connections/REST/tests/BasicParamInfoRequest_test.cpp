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
#include "../../common/tests/CommonTestHelpers.h"
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
        //oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Set default actions for common mock calls
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
        EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(empty_prefix));
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
        EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));

        request = BasicParamInfoRequest::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        //std::cout.rdbuf(oldCout); // Restoring cout
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
    std::string empty_prefix;
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

    // Override default behaviors for this test
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

    // Add isArrayType expectation
    EXPECT_CALL(*mockParam, isArrayType()).WillRepeatedly(::testing::Return(false));

    // Override default behaviors for this test
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(mockToken));
    
    EXPECT_CALL(context, fields("oid_prefix")).WillRepeatedly(::testing::ReturnRef(paramInfo.oid));
    EXPECT_CALL(dm, getParam("/" + paramInfo.oid, ::testing::_, ::testing::_))
       .WillRepeatedly(::testing::Invoke([&mockParam, &rc](const std::string&, catena::exception_with_status &status, catena::common::Authorizer &) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(mockParam);
        }));

    // Make a new request for this test
    auto authzRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);
    
    // Execute
    authzRequest->proceed();
    authzRequest->finish();

    delete authzRequest;

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
    
    // Add isArrayType expectations
    EXPECT_CALL(*param1, isArrayType()).WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*param2, isArrayType()).WillRepeatedly(::testing::Return(false));
    
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) -> std::vector<std::unique_ptr<IParam>> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(param1_info));
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(param2_info));
    std::string expected = expectedSSEResponse(rc, jsonBodies);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// Test 1.2: Get top-level parameters with error returned from getTopLevelParams
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsError) {
    catena::exception_with_status rc("Error getting top-level parameters", catena::StatusCode::INTERNAL);
    
    // Setup mock expectations 
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// Test 1.3: Get top-level parameters with empty list returned from getTopLevelParams
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getEmptyTopLevelParams) {
    catena::exception_with_status rc("No top-level parameters found", catena::StatusCode::NOT_FOUND);
    
    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK); 
            return std::vector<std::unique_ptr<IParam>>();  
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
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
    EXPECT_CALL(*arrayParam, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid("array_param");
            response.mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
            return catena::exception_with_status("", catena::StatusCode::OK);
        })); 
    
    top_level_params.push_back(std::move(arrayParam));

    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string jsonBody = catena::REST::test::createParamInfoJson(arrayParamInfo);
    std::string expected = expectedSSEResponse(rc, {jsonBody});
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// Test 1.5: Get top-level parameters with error status in returned parameters
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsProcessingError) {
    catena::exception_with_status rc("Error processing parameter", catena::StatusCode::INTERNAL);
    
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto errorParam = std::make_unique<MockParam>();
    
    // Set up mock parameter with error during processing
    catena::REST::test::ParamInfo errorParamInfo{
        .oid = "error_param",
        .type = catena::ParamType::STRING,
        .status = catena::StatusCode::INTERNAL 
    };
    catena::REST::test::setupMockParam(errorParam.get(), errorParamInfo);
        
    top_level_params.push_back(std::move(errorParam));

    // Setup mock expectations   
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
            return std::move(top_level_params);
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// Test 1.6: Get top-level parameters with exception thrown during parameter processing
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsThrow) {
    catena::exception_with_status rc("Error getting top-level parameters", catena::StatusCode::INTERNAL);
    
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
    
    // Set up param2 to throw during processing
    EXPECT_CALL(*param2, getOid())
        .WillRepeatedly(::testing::ReturnRef(param2_info.oid));
    EXPECT_CALL(*param2, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::BasicParamInfoResponse&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return catena::exception_with_status("", catena::StatusCode::OK);  // This line should never be reached
        }));

    // Add isArrayType expectation for param1
    EXPECT_CALL(*param1, isArrayType()).WillRepeatedly(::testing::Return(false));
    
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Execute
    request->proceed();
    request->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);
}

// == MODE 2 TESTS: Get all top-level parameters with recursion ==

// Test 2.1: Get all top-level parameters with recursion
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithRecursion) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto parentParam = std::make_unique<MockParam>();
    auto childParam = std::make_unique<MockParam>();
    
    // Set up mock parameters 
    catena::REST::test::ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo child_info{
        .oid = "child",
        .type = catena::ParamType::STRING
    };

    std::string parentOid = parent_info.oid;
    std::string childOid = child_info.oid;
    std::string nested_oid = "/" + parent_info.oid + "/" + child_info.oid;

    // Set up parameter hierarchy using ParamHierarchyBuilder
    auto parentDesc = ParamHierarchyBuilder::createDescriptor("/" + parent_info.oid);
    auto nestedDesc = ParamHierarchyBuilder::createDescriptor(nested_oid);
    ParamHierarchyBuilder::addChild(parentDesc, child_info.oid, nestedDesc);

    // Set up mock parameters with their descriptors
    setupMockParam(parentParam.get(), parent_info.oid, *parentDesc.descriptor);
    setupMockParam(childParam.get(), nested_oid, *nestedDesc.descriptor);

    // Set up mock expectations for getDescriptor
    EXPECT_CALL(*parentParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*parentDesc.descriptor));
    EXPECT_CALL(*childParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));

    // Add expectations for isArrayType
    EXPECT_CALL(*parentParam, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*childParam, isArrayType())
        .WillRepeatedly(::testing::Return(false));

    // Add expectations for toProto
    EXPECT_CALL(*parentParam, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([parentOid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(parentOid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    EXPECT_CALL(*childParam, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([childOid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(childOid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Add expectation for getAllSubParams to return the child
    std::unordered_map<std::string, IParamDescriptor*> subParams;
    subParams[child_info.oid] = nestedDesc.descriptor.get();
    EXPECT_CALL(*parentDesc.descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(subParams));

    // Add expectation for getSubParam to return the child descriptor
    EXPECT_CALL(*parentDesc.descriptor, getSubParam(childOid))
        .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));

    // Add expectation for getOid on parent descriptor
    EXPECT_CALL(*parentDesc.descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(parentOid));
    
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([&childParam, &nestedDesc, nested_oid, childOid](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
            catena::common::Path path(fqoid);
            if (path.fqoid() == nested_oid) {
                auto param = std::make_unique<MockParam>();
                setupMockParam(param.get(), fqoid, *nestedDesc.descriptor);
                
                // Set up all necessary expectations for the child parameter
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
                    .WillRepeatedly(::testing::Invoke([childOid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
                        response.mutable_info()->set_oid(childOid);
                        response.mutable_info()->set_type(catena::ParamType::STRING);
                        return catena::exception_with_status("", catena::StatusCode::OK);
                    }));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
            status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }));

    // Create a new request after setting up all expectations
    auto topRecursionRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    topRecursionRequest->proceed();
    topRecursionRequest->finish();

    // Get expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(parent_info));
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(child_info));
    std::string expected = expectedSSEResponse(rc, jsonBodies);
    std::string actual = readResponse();
    
    delete topRecursionRequest;

    EXPECT_EQ(actual, expected);
}

// Test 2.2: Get top-level parameters with recursion and array children
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithRecursionAndArrays) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto parentParam = std::make_unique<MockParam>();
    auto arrayChild = std::make_unique<MockParam>();
    
    // Set up mock parameters 
    catena::REST::test::ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo arrayChild_info{
        .oid = "array_child",
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 3
    };

    std::string parentOid = parent_info.oid;
    std::string childOid = arrayChild_info.oid;
    std::string nested_oid = "/" + parent_info.oid + "/" + arrayChild_info.oid;

    // Set up parameter hierarchy using ParamHierarchyBuilder
    auto parentDesc = ParamHierarchyBuilder::createDescriptor("/" + parent_info.oid);
    auto nestedDesc = ParamHierarchyBuilder::createDescriptor(nested_oid);
    ParamHierarchyBuilder::addChild(parentDesc, arrayChild_info.oid, nestedDesc);

    // Set up mock parameters with their descriptors
    setupMockParam(parentParam.get(), parent_info.oid, *parentDesc.descriptor);
    setupMockParam(arrayChild.get(), nested_oid, *nestedDesc.descriptor);

    // Set up mock expectations for getDescriptor
    EXPECT_CALL(*parentParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*parentDesc.descriptor));
    EXPECT_CALL(*arrayChild, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));

    // Add expectations for isArrayType
    EXPECT_CALL(*parentParam, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*arrayChild, isArrayType())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*arrayChild, size())
        .WillRepeatedly(::testing::Return(arrayChild_info.array_length));

    // Add expectations for toProto
    EXPECT_CALL(*parentParam, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([parentOid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(parentOid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    EXPECT_CALL(*arrayChild, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([childOid, arrayChild_info](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(childOid);
            response.mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
            response.set_array_length(arrayChild_info.array_length);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // // Add expectation for updateArrayLengths_ to be called
    // EXPECT_CALL(*arrayRequest, updateArrayLengths_(childOid, arrayChild_info.array_length))
    //     .Times(1);

    // // Add expectation for addParamToResponses_ to be called
    // EXPECT_CALL(*arrayRequest, addParamToResponses_(::testing::_, ::testing::_))
    //     .Times(2);

    // Add expectation for getAllSubParams to return the child
    std::unordered_map<std::string, IParamDescriptor*> subParams;
    subParams[arrayChild_info.oid] = nestedDesc.descriptor.get();
    EXPECT_CALL(*parentDesc.descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(subParams));

    // Add expectation for getSubParam to return the child descriptor
    EXPECT_CALL(*parentDesc.descriptor, getSubParam(childOid))
        .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));

    // Add expectation for getOid on parent descriptor
    EXPECT_CALL(*parentDesc.descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(parentOid));
    
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([&arrayChild, &nestedDesc, nested_oid, childOid, arrayChild_info](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
            catena::common::Path path(fqoid);
            if (path.fqoid() == nested_oid) {
                auto param = std::make_unique<MockParam>();
                setupMockParam(param.get(), fqoid, *nestedDesc.descriptor);
                
                // Set up all necessary expectations for the child parameter
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*param, size())
                    .WillRepeatedly(::testing::Return(arrayChild_info.array_length));
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
                    .WillRepeatedly(::testing::Invoke([childOid, arrayChild_info](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
                        response.mutable_info()->set_oid(childOid);
                        response.mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
                        response.set_array_length(arrayChild_info.array_length);
                        return catena::exception_with_status("", catena::StatusCode::OK);
                    }));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
            status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }));

    // Create a new request after setting up all expectations
    auto arrayRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    arrayRequest->proceed();
    arrayRequest->finish();

    // Get expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(parent_info));
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(arrayChild_info));
    std::string expected = expectedSSEResponse(rc, jsonBodies);
    std::string actual = readResponse();
    
    delete arrayRequest;

    EXPECT_EQ(actual, expected);
}

// Test 2.3: Get top-level parameters with recursion and error in child processing
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithRecursionError) {
    catena::exception_with_status rc("Error processing child parameter", catena::StatusCode::INTERNAL);

    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto parentParam = std::make_unique<MockParam>();
    auto errorChild = std::make_unique<MockParam>();
    
    // Set up mock parameters 
    catena::REST::test::ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo errorChild_info{
        .oid = "error_child",
        .type = catena::ParamType::STRING,
        .status = catena::StatusCode::INTERNAL
    };

    std::string parentOid = parent_info.oid;
    std::string childOid = errorChild_info.oid;
    std::string nested_oid = "/" + parent_info.oid + "/" + errorChild_info.oid;

    // Set up parameter hierarchy using ParamHierarchyBuilder
    auto parentDesc = ParamHierarchyBuilder::createDescriptor("/" + parent_info.oid);
    auto nestedDesc = ParamHierarchyBuilder::createDescriptor(nested_oid);
    ParamHierarchyBuilder::addChild(parentDesc, errorChild_info.oid, nestedDesc);

    // Set up mock parameters with their descriptors
    setupMockParam(parentParam.get(), parent_info.oid, *parentDesc.descriptor);
    setupMockParam(errorChild.get(), nested_oid, *nestedDesc.descriptor);

    // Set up mock expectations for getDescriptor
    EXPECT_CALL(*parentParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*parentDesc.descriptor));
    EXPECT_CALL(*errorChild, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));

    // Add expectations for isArrayType
    EXPECT_CALL(*parentParam, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*errorChild, isArrayType())
        .WillRepeatedly(::testing::Return(false));

    // Add expectations for toProto
    EXPECT_CALL(*parentParam, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([parentOid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(parentOid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    EXPECT_CALL(*errorChild, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::BasicParamInfoResponse&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error processing child parameter", catena::StatusCode::INTERNAL);
        }));

    // Add expectation for getAllSubParams to return the child
    std::unordered_map<std::string, IParamDescriptor*> subParams;
    subParams[errorChild_info.oid] = nestedDesc.descriptor.get();
    EXPECT_CALL(*parentDesc.descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(subParams));

    // Add expectation for getSubParam to return the child descriptor
    EXPECT_CALL(*parentDesc.descriptor, getSubParam(childOid))
        .WillRepeatedly(::testing::ReturnRef(*nestedDesc.descriptor));

    // Add expectation for getOid on parent descriptor
    EXPECT_CALL(*parentDesc.descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(parentOid));
    
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([&errorChild, &nestedDesc, nested_oid](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
            catena::common::Path path(fqoid);
            if (path.fqoid() == nested_oid) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(errorChild);
            }
            status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }));

    // Create a new request after setting up all expectations
    auto errorRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    errorRequest->proceed();
    errorRequest->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    
    delete errorRequest;

    EXPECT_EQ(actual, expected);
}

// Test 2.4: Get top-level parameters with recursion and deep nesting
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithDeepNesting) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    auto level1 = std::make_unique<MockParam>();
    auto level2 = std::make_unique<MockParam>();
    auto level3 = std::make_unique<MockParam>();
    
    // Set up mock parameters 
    catena::REST::test::ParamInfo level1_info{
        .oid = "level1",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo level2_info{
        .oid = "level2",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo level3_info{
        .oid = "level3",
        .type = catena::ParamType::STRING
    };

    std::string level1Oid = level1_info.oid;
    std::string level2Oid = level2_info.oid;
    std::string level3Oid = level3_info.oid;
    std::string level2Path = "/" + level1Oid + "/" + level2Oid;
    std::string level3Path = level2Path + "/" + level3Oid;

    // Set up parameter hierarchy using ParamHierarchyBuilder
    auto level1Desc = ParamHierarchyBuilder::createDescriptor("/" + level1Oid);
    auto level2Desc = ParamHierarchyBuilder::createDescriptor(level2Path);
    auto level3Desc = ParamHierarchyBuilder::createDescriptor(level3Path);
    
    ParamHierarchyBuilder::addChild(level1Desc, level2Oid, level2Desc);
    ParamHierarchyBuilder::addChild(level2Desc, level3Oid, level3Desc);

    // Set up mock parameters with their descriptors
    setupMockParam(level1.get(), level1Oid, *level1Desc.descriptor);
    setupMockParam(level2.get(), level2Path, *level2Desc.descriptor);
    setupMockParam(level3.get(), level3Path, *level3Desc.descriptor);

    // Set up mock expectations for getDescriptor
    EXPECT_CALL(*level1, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*level1Desc.descriptor));
    EXPECT_CALL(*level2, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*level2Desc.descriptor));
    EXPECT_CALL(*level3, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*level3Desc.descriptor));

    // Add expectations for isArrayType
    EXPECT_CALL(*level1, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*level2, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*level3, isArrayType())
        .WillRepeatedly(::testing::Return(false));

    // Add expectations for toProto
    EXPECT_CALL(*level1, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([level1Oid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(level1Oid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    EXPECT_CALL(*level2, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([level2Oid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(level2Oid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    EXPECT_CALL(*level3, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([level3Oid](catena::BasicParamInfoResponse& response, catena::common::Authorizer&) {
            response.mutable_info()->set_oid(level3Oid);
            response.mutable_info()->set_type(catena::ParamType::STRING);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Add expectations for getAllSubParams
    std::unordered_map<std::string, IParamDescriptor*> level1SubParams;
    level1SubParams[level2Oid] = level2Desc.descriptor.get();
    EXPECT_CALL(*level1Desc.descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(level1SubParams));

    std::unordered_map<std::string, IParamDescriptor*> level2SubParams;
    level2SubParams[level3Oid] = level3Desc.descriptor.get();
    EXPECT_CALL(*level2Desc.descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(level2SubParams));

    // Add expectations for getSubParam
    EXPECT_CALL(*level1Desc.descriptor, getSubParam(level2Oid))
        .WillRepeatedly(::testing::ReturnRef(*level2Desc.descriptor));
    EXPECT_CALL(*level2Desc.descriptor, getSubParam(level3Oid))
        .WillRepeatedly(::testing::ReturnRef(*level3Desc.descriptor));

    // Add expectations for getOid
    EXPECT_CALL(*level1Desc.descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(level1Oid));
    EXPECT_CALL(*level2Desc.descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(level2Path));
    EXPECT_CALL(*level3Desc.descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(level3Path));
    
    top_level_params.push_back(std::move(level1));

    // Enable recursion
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([&level2, &level3, &level2Desc, &level3Desc, level2Path, level3Path]
            (const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
            catena::common::Path path(fqoid);
            if (path.fqoid() == level2Path) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(level2);
            } else if (path.fqoid() == level3Path) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(level3);
            }
            status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
            return nullptr;
        }));

    // Create a new request after setting up all expectations
    auto deepNestingRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    deepNestingRequest->proceed();
    deepNestingRequest->finish();

    // Get expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(level1_info));
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(level2_info));
    jsonBodies.push_back(catena::REST::test::createParamInfoJson(level3_info));
    std::string expected = expectedSSEResponse(rc, jsonBodies);
    std::string actual = readResponse();
    
    delete deepNestingRequest;

    EXPECT_EQ(actual, expected);
}

