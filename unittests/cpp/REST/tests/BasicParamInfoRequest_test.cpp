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
#include "MockSocketReader.h"
#include "MockLanguagePack.h"
#include "MockParam.h"
#include "MockDevice.h"
#include "RESTTest.h"
#include "RESTTestHelpers.h"
#include "CommonTestHelpers.h"

// REST
#include <controllers/BasicParamInfoRequest.h>
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTBasicParamInfoRequestTests : public ::testing::Test, public RESTTest {
protected:
    RESTBasicParamInfoRequestTests() : RESTTest(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Set default actions for common mock calls
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
        EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(false));
        EXPECT_CALL(context, fqoid()).WillRepeatedly(::testing::ReturnRef(empty_prefix));
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMtx));
        EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(false));

        request = BasicParamInfoRequest::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (request) {
            delete request;
        }
    }

    // Helper method to setup parameter hierarchy
    struct ParamHierarchy {
        ParamHierarchyBuilder::DescriptorInfo parent;
        ParamHierarchyBuilder::DescriptorInfo child;
        std::string nested_oid;
    };

    ParamHierarchy createParamHierarchy(const std::string& parent_oid, const std::string& child_oid) {
        auto parentDesc = ParamHierarchyBuilder::createDescriptor("/" + parent_oid);
        std::string nested_oid = "/" + parent_oid + "/" + child_oid;
        auto childDesc = ParamHierarchyBuilder::createDescriptor(nested_oid);
        ParamHierarchyBuilder::addChild(parentDesc, child_oid, childDesc);

        // Setup common expectations for the hierarchy
        std::unordered_map<std::string, IParamDescriptor*> subParams;
        subParams[child_oid] = childDesc.descriptor.get();
        EXPECT_CALL(*parentDesc.descriptor, getAllSubParams())
            .WillRepeatedly(::testing::ReturnRef(subParams));
        EXPECT_CALL(*parentDesc.descriptor, getSubParam(child_oid))
            .WillRepeatedly(::testing::ReturnRef(*childDesc.descriptor));
        EXPECT_CALL(*parentDesc.descriptor, getOid())
            .WillRepeatedly(::testing::ReturnRef(parent_oid));

        return {std::move(parentDesc), std::move(childDesc), nested_oid};
    }

    std::stringstream MockConsole;
    std::streambuf* oldCout;
    std::mutex mockMtx;
    
    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* request = nullptr;
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
    auto param = std::make_unique<MockParam>();
    catena::REST::test::ParamInfo param_info{
        .oid = "test_param",
        .type = catena::ParamType::STRING
    };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + param_info.oid);
    catena::REST::test::setupMockParam(param.get(), param_info, desc.descriptor.get());

    // Add isArrayType expectation
    EXPECT_CALL(*param, isArrayType()).WillRepeatedly(::testing::Return(false));

    // Override default behaviors for this test
    EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(mockToken));
    
    EXPECT_CALL(context, fqoid()).WillRepeatedly(::testing::ReturnRef(param_info.oid));
    EXPECT_CALL(dm, getParam(param_info.oid, ::testing::_, ::testing::_))
       .WillRepeatedly(::testing::Invoke([&param, &rc](const std::string&, catena::exception_with_status &status, catena::common::Authorizer &) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(param);
        }));

    // Make a new request for this test
    auto authzRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);
    
    // Execute
    authzRequest->proceed();
    authzRequest->finish();

    delete authzRequest;

    // Get expected and actual responses
    std::string jsonBody = catena::REST::test::createParamInfoJson(param_info);
    std::string expected = expectedSSEResponse(rc, {jsonBody});
    std::string actual = readResponse();
    EXPECT_EQ(actual, expected);

}

// == MODE 1 TESTS: Get all top-level parameters without recursion ==

// Test 1.1: Get all top-level parameters without recursion
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParams) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Setup mock parameters
    catena::REST::test::ParamInfo param1_info{ .oid = "param1", .type = catena::ParamType::STRING };
    catena::REST::test::ParamInfo param2_info{ .oid = "param2", .type = catena::ParamType::STRING };
    auto desc1 = ParamHierarchyBuilder::createDescriptor("/" + param1_info.oid);
    auto desc2 = ParamHierarchyBuilder::createDescriptor("/" + param2_info.oid);
    
    // Create ParamInfo structs
    catena::REST::test::ParamInfo param1_info_struct{param1_info.oid, param1_info.type};
    catena::REST::test::ParamInfo param2_info_struct{param2_info.oid, param2_info.type};

    auto param1 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(param1.get(), param1_info_struct, desc1.descriptor.get());

    auto param2 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(param2.get(), param2_info_struct, desc2.descriptor.get());

    std::vector<std::unique_ptr<IParam>> top_level_params;
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
    catena::REST::test::ParamInfo arrayParamInfo{ .oid = "array_param", .type = catena::ParamType::STRING_ARRAY, .array_length = 5 };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + arrayParamInfo.oid);
    auto arrayParam = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(arrayParam.get(), arrayParamInfo, desc.descriptor.get());
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
    catena::REST::test::ParamInfo errorParamInfo{ .oid = "error_param", .type = catena::ParamType::STRING, .status = catena::StatusCode::INTERNAL };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + errorParamInfo.oid);
    auto errorParam = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(errorParam.get(), errorParamInfo, desc.descriptor.get());
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
    catena::REST::test::ParamInfo param1_info{ .oid = "param1", .type = catena::ParamType::STRING };
    catena::REST::test::ParamInfo param2_info{ .oid = "param2", .type = catena::ParamType::STRING };
    auto desc1 = ParamHierarchyBuilder::createDescriptor("/" + param1_info.oid);
    auto desc2 = ParamHierarchyBuilder::createDescriptor("/" + param2_info.oid);
    auto param1 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(param1.get(), param1_info, desc1.descriptor.get());
    auto param2 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(param2.get(), param2_info, desc2.descriptor.get());
    
    // Set up param2 to throw during processing
    EXPECT_CALL(*param2, getOid())
        .WillRepeatedly(::testing::ReturnRef(param2_info.oid));
    EXPECT_CALL(*param2, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::BasicParamInfoResponse&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return catena::exception_with_status("", catena::StatusCode::OK);  // This line should never be reached
        }));

    std::vector<std::unique_ptr<IParam>> top_level_params;
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

// Test 2.1: Get top-level parameters with recursion and deep nesting
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithDeepNesting) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Define parameter info structs first
    catena::REST::test::ParamInfo level1_info{.oid = "level1", .type = catena::ParamType::STRING};
    catena::REST::test::ParamInfo level2_info{.oid = "level2", .type = catena::ParamType::STRING};
    catena::REST::test::ParamInfo level3_info{.oid = "level3", .type = catena::ParamType::STRING};

    // Create parameter hierarchy using ParamHierarchyBuilder
    auto level1Desc = ParamHierarchyBuilder::createDescriptor("/" + level1_info.oid);
    auto level2Desc = ParamHierarchyBuilder::createDescriptor("/" + level1_info.oid + "/" + level2_info.oid);
    auto level3Desc = ParamHierarchyBuilder::createDescriptor("/" + level1_info.oid + "/" + level2_info.oid + "/" + level3_info.oid);

    // Add children to hierarchy
    ParamHierarchyBuilder::addChild(level1Desc, level2_info.oid, level2Desc);
    ParamHierarchyBuilder::addChild(level2Desc, level3_info.oid, level3Desc);

    // Set up OID expectations for each descriptor
    std::string level1Oid = "/" + level1_info.oid;
    std::string level2Oid = level1Oid + "/" + level2_info.oid;
    std::string level3Oid = level2Oid + "/" + level3_info.oid;
    EXPECT_CALL(*level1Desc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(level1Oid));
    EXPECT_CALL(*level2Desc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(level2Oid));
    EXPECT_CALL(*level3Desc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(level3Oid));

    // Create ParamInfo structs
    catena::REST::test::ParamInfo level1_info_struct{level1_info.oid, level1_info.type};
    catena::REST::test::ParamInfo level2_info_struct{level2_info.oid, level2_info.type};
    catena::REST::test::ParamInfo level3_info_struct{level3_info.oid, level3_info.type};

    auto level1 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(level1.get(), level1_info_struct, level1Desc.descriptor.get());

    auto level2 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(level2.get(), level2_info_struct, level2Desc.descriptor.get());

    auto level3 = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(level3.get(), level3_info_struct, level3Desc.descriptor.get());

    // Setup top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(level1));

    // Enable recursion and stream
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke(
            [&level2, &level3, level2Desc, level3Desc]
            (const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                std::string level2Oid = level2Desc.descriptor->getOid();
                std::string level3Oid = level3Desc.descriptor->getOid();
                if (fqoid == level2Oid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(level2);
                } else if (fqoid == level3Oid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(level3);
                }
                status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }
        ));

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

// Test 2.2: Get top-level parameters with recursion and arrays
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithRecursionAndArrays) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Define parameter info
    catena::REST::test::ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 5
    };
    catena::REST::test::ParamInfo arrayChild_info{
        .oid = "array_child",
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 3
    };

    // Build hierarchy using ParamHierarchyBuilder
    std::string parentOid = "/" + parent_info.oid;
    std::string childOid = parentOid + "/" + arrayChild_info.oid;
    auto parentDesc = ParamHierarchyBuilder::createDescriptor(parentOid);
    auto childDesc = ParamHierarchyBuilder::createDescriptor(childOid);
    ParamHierarchyBuilder::addChild(parentDesc, arrayChild_info.oid, childDesc);

    // Set up OID expectations for descriptors
    EXPECT_CALL(*parentDesc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(parentOid));
    EXPECT_CALL(*childDesc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(childOid));

    // Create mock params using helpers
    auto parentParam = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(parentParam.get(), parent_info, parentDesc.descriptor.get());

    auto arrayChild = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(arrayChild.get(), arrayChild_info, childDesc.descriptor.get());

    // Set up top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion and stream
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params, &rc](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this, &arrayChild, childOid, arrayChild_info, childDesc]
            (const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                if (fqoid == childOid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(arrayChild);
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

    // Define parameter info
    catena::REST::test::ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING
    };
    catena::REST::test::ParamInfo errorChild_info{
        .oid = "error_child",
        .type = catena::ParamType::STRING,
        .status = catena::StatusCode::INTERNAL
    };

    // Build hierarchy using ParamHierarchyBuilder
    std::string parentOid = "/" + parent_info.oid;
    std::string childOid = parentOid + "/" + errorChild_info.oid;
    auto parentDesc = ParamHierarchyBuilder::createDescriptor(parentOid);
    auto childDesc = ParamHierarchyBuilder::createDescriptor(childOid);
    ParamHierarchyBuilder::addChild(parentDesc, errorChild_info.oid, childDesc);

    // Set up OID expectations for descriptors
    EXPECT_CALL(*parentDesc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(parentOid));
    EXPECT_CALL(*childDesc.descriptor, getOid()).WillRepeatedly(::testing::ReturnRef(childOid));

    // Create ParamInfo structs
    catena::REST::test::ParamInfo parent_info_struct{parent_info.oid, parent_info.type};
    catena::REST::test::ParamInfo error_child_info_struct{errorChild_info.oid, errorChild_info.type};

    auto parentParam = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(parentParam.get(), parent_info_struct, parentDesc.descriptor.get());

    // For the error child, set up a param that throws in toProto
    auto errorChild = std::make_unique<MockParam>();
    catena::REST::test::setupMockParam(errorChild.get(), error_child_info_struct, childDesc.descriptor.get());
    EXPECT_CALL(*errorChild, toProto(::testing::An<catena::BasicParamInfoResponse&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::BasicParamInfoResponse&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error processing child parameter", catena::StatusCode::INTERNAL);
        }));

    // Set up top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion and stream
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(true));

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm, getParam(::testing::An<const std::string&>(), ::testing::An<catena::exception_with_status&>(), ::testing::An<Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([&errorChild, childOid]
            (const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                if (fqoid == childOid) {
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

// Test 2.4: Get top-level parameters with error status from getTopLevelParams
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithErrorStatus) {
    catena::exception_with_status rc("Error getting parameters", catena::StatusCode::INTERNAL);

    // Setup mock expectations
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error getting parameters", catena::StatusCode::INTERNAL);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    // Create a new request after setting up all expectations
    auto errorStatusRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    errorStatusRequest->proceed();
    errorStatusRequest->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    
    delete errorStatusRequest;

    EXPECT_EQ(actual, expected);
}

// Test 2.5: Get top-level parameters with empty list and recursion
TEST_F(RESTBasicParamInfoRequestTests, BasicParamInfoRequest_getTopLevelParamsWithEmptyListAndRecursion) {
    catena::exception_with_status rc("No top-level parameters found", catena::StatusCode::NOT_FOUND);

    // Setup mock expectations
    EXPECT_CALL(context, hasField("recursive")).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(context, stream()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(dm, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    // Create a new request after setting up all expectations
    auto emptyListRequest = BasicParamInfoRequest::makeOne(serverSocket, context, dm);

    // Execute
    emptyListRequest->proceed();
    emptyListRequest->finish();

    // Get expected and actual responses
    std::string expected = expectedSSEResponse(rc);
    std::string actual = readResponse();
    
    delete emptyListRequest;

    EXPECT_EQ(actual, expected);
}

