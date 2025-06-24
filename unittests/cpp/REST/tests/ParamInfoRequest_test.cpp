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
 * @file ParamInfoRequest_test.cpp
 * @brief This file is for testing the ParamInfoRequest.cpp file.
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-05-20
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"
#include "MockParam.h"
#include "CommonTestHelpers.h"

// REST
#include <controllers/ParamInfoRequest.h>

using namespace catena::common;
using namespace catena::REST;

/*
 * ============================================================================
 *                        ParamInfoRequest Helpers
 * ============================================================================
 */

struct ParamInfo {
    std::string oid;
    // std::string name; // Might be irrelevant
    catena::ParamType type;
    // std::string template_oid = ""; // Might be irrelevant
    uint32_t array_length = 0;
    catena::StatusCode status = catena::StatusCode::OK;  // Default to OK
};

/**
 * @brief Creates or populates a ParamInfoResponse with the specified parameters
 */
inline void setupParamInfo(catena::ParamInfoResponse& response, const ParamInfo& info) {
    response.mutable_info()->set_oid(info.oid);
    // response.mutable_info()->mutable_name()->mutable_display_strings()->insert({"en", info.name}); // Might be irrelevant
    response.mutable_info()->set_type(info.type);
    // response.mutable_info()->set_template_oid(info.template_oid); // Might be irrelevant
    response.set_array_length(info.array_length);
}

/**
 * @brief Sets up a mock parameter to return a ParamInfoResponse
 *        Also sets up getDescriptor, isArrayType, and size for array types if descriptor is provided.
 */
inline void setupMockParam(catena::common::MockParam* mockParam, const ParamInfo& info, const catena::common::IParamDescriptor* descriptor = nullptr) {
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(info.oid));

    // Set up getDescriptor if provided
    if (descriptor) {
        EXPECT_CALL(*mockParam, getDescriptor())
            .WillRepeatedly(::testing::ReturnRef(*descriptor));
    }

    // Set up isArrayType and size if array_length > 0
    if (info.array_length > 0) {
        EXPECT_CALL(*mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(*mockParam, size())
            .WillRepeatedly(::testing::Return(info.array_length));
    } else {
        EXPECT_CALL(*mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(false));
    }

    // Only expect toProto if status indicates success (HTTP status < 300)
    if (catena::REST::codeMap_.at(info.status).first < 300) {
        EXPECT_CALL(*mockParam, toProto(::testing::An<catena::ParamInfoResponse&>(), ::testing::_))
            .WillRepeatedly(::testing::Invoke([info](catena::ParamInfoResponse& response, catena::common::Authorizer&) {
                setupParamInfo(response, info);
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));
    }
}

/**
 * @brief Creates and serializes a ParamInfoResponse to JSON
 * @return The serialized JSON string
 */
inline std::string createParamInfoJson(const ParamInfo& info) {
    catena::ParamInfoResponse response;
    setupParamInfo(response, info);
    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options;
    auto status = google::protobuf::util::MessageToJsonString(response, &jsonBody, options);
    return jsonBody;
}

/*
 * ============================================================================
 *                        ParamInfoRequest Tests
 * ============================================================================
 */

class RESTParamInfoRequestTests : public RESTEndpointTest {
protected:
    RESTParamInfoRequestTests() : RESTEndpointTest() {
        EXPECT_CALL(context_, hasField("recursive")).WillRepeatedly(testing::Return(false));
    }

    /*
     * Creates a ParamInfoRequest handler object.
     */
    ICallData* makeOne() override { return ParamInfoRequest::makeOne(serverSocket_, context_, dm0_); }

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
            .WillRepeatedly(testing::ReturnRef(subParams));
        EXPECT_CALL(*parentDesc.descriptor, getSubParam(child_oid))
            .WillRepeatedly(testing::ReturnRef(*childDesc.descriptor));
        EXPECT_CALL(*parentDesc.descriptor, getOid())
            .WillRepeatedly(testing::ReturnRef(parent_oid));

        return {std::move(parentDesc), std::move(childDesc), nested_oid};
    }
};

// Preliminary test: Creating a ParamInfoRequest object
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_Create) {
    ASSERT_TRUE(endpoint_);
}

// Test 0.1: Authorization test with std::exception
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_AuthzStdException) {
    expRc_ = catena::exception_with_status("Authorization setup failed: Test auth setup failure", catena::StatusCode::INTERNAL);
    authzEnabled_ = true;

    // Setup mock expectations
    EXPECT_CALL(context_, jwsToken()).WillRepeatedly(testing::Throw(std::runtime_error("Test auth setup failure")));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 0.2: Authorization test with invalid token
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    jwsToken_ = "test_token";
    authzEnabled_ = true;

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 0.3: Authorization test with valid token
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_AuthzValid) {
    // Use a valid JWT token that was borrowed from GetValue_test.cpp
    jwsToken_ = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    authzEnabled_ = true;
    
    // Setup mock parameter
    auto param = std::make_unique<MockParam>();
    ParamInfo param_info{
        .oid = "test_param",
        .type = catena::ParamType::STRING
    };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + param_info.oid);
    setupMockParam(param.get(), param_info, desc.descriptor.get());
    fqoid_ = param_info.oid;

    // Add isArrayType expectation
    EXPECT_CALL(*param, isArrayType()).WillRepeatedly(testing::Return(false));

    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
       .WillRepeatedly(testing::Invoke([&param](const std::string&, catena::exception_with_status &status, catena::common::Authorizer &) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(param);
        }));
    
    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::string jsonBody = createParamInfoJson(param_info);
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {jsonBody}));
}

// == MODE 1 TESTS: Get all top-level parameters without recursion ==

// Test 1.1: Get all top-level parameters without recursion
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParams) {
    // Setup mock parameters
    ParamInfo param1_info{ .oid = "param1", .type = catena::ParamType::STRING };
    ParamInfo param2_info{ .oid = "param2", .type = catena::ParamType::STRING };
    auto desc1 = ParamHierarchyBuilder::createDescriptor("/" + param1_info.oid);
    auto desc2 = ParamHierarchyBuilder::createDescriptor("/" + param2_info.oid);
    
    // Create ParamInfo structs
    ParamInfo param1_info_struct{param1_info.oid, param1_info.type};
    ParamInfo param2_info_struct{param2_info.oid, param2_info.type};

    auto param1 = std::make_unique<MockParam>();
    setupMockParam(param1.get(), param1_info_struct, desc1.descriptor.get());

    auto param2 = std::make_unique<MockParam>();
    setupMockParam(param2.get(), param2_info_struct, desc2.descriptor.get());

    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) -> std::vector<std::unique_ptr<IParam>> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(createParamInfoJson(param1_info));
    jsonBodies.push_back(createParamInfoJson(param2_info));
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, jsonBodies));
}

// Test 1.2: Get top-level parameters with error returned from getTopLevelParams
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsError) {
    expRc_ = catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
    
    // Setup mock expectations 
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 1.3: Get top-level parameters with empty list returned from getTopLevelParams
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getEmptyTopLevelParams) {
    expRc_ = catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);
    
    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK); 
            return std::vector<std::unique_ptr<IParam>>();  
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 1.4: Get top-level parameters with array type
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithArray) {
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    ParamInfo arrayParamInfo{ .oid = "array_param", .type = catena::ParamType::STRING_ARRAY, .array_length = 5 };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + arrayParamInfo.oid);
    auto arrayParam = std::make_unique<MockParam>();
    setupMockParam(arrayParam.get(), arrayParamInfo, desc.descriptor.get());
    top_level_params.push_back(std::move(arrayParam));

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::string jsonBody = createParamInfoJson(arrayParamInfo);
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {jsonBody}));
}

// Test 1.5: Get top-level parameters with error status in returned parameters
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsProcessingError) {
    expRc_ = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
    
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    ParamInfo errorParamInfo{ .oid = "error_param", .type = catena::ParamType::STRING, .status = catena::StatusCode::INTERNAL };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + errorParamInfo.oid);
    auto errorParam = std::make_unique<MockParam>();
    setupMockParam(errorParam.get(), errorParamInfo, desc.descriptor.get());
    top_level_params.push_back(std::move(errorParam));

    // Setup mock expectations   
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
            return std::move(top_level_params);
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 1.6: Get top-level parameters with exception thrown during parameter processing
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsThrow) {
    expRc_ = catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
    
    // Setup mock parameters
    ParamInfo param1_info{ .oid = "param1", .type = catena::ParamType::STRING };
    ParamInfo param2_info{ .oid = "param2", .type = catena::ParamType::STRING };
    auto desc1 = ParamHierarchyBuilder::createDescriptor("/" + param1_info.oid);
    auto desc2 = ParamHierarchyBuilder::createDescriptor("/" + param2_info.oid);
    auto param1 = std::make_unique<MockParam>();
    setupMockParam(param1.get(), param1_info, desc1.descriptor.get());
    auto param2 = std::make_unique<MockParam>();
    setupMockParam(param2.get(), param2_info, desc2.descriptor.get());
    
    // Set up param2 to throw during processing
    EXPECT_CALL(*param2, getOid())
        .WillRepeatedly(testing::ReturnRef(param2_info.oid));
    EXPECT_CALL(*param2, toProto(testing::An<catena::ParamInfoResponse&>(), testing::An<catena::common::Authorizer&>()))
        .WillOnce(testing::Invoke([](catena::ParamInfoResponse&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return catena::exception_with_status("", catena::StatusCode::OK);  // This line should never be reached
        }));

    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// == MODE 2 TESTS: Get all top-level parameters with recursion ==

// Test 2.1: Get top-level parameters with recursion and deep nesting
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithDeepNesting) {
    // Define parameter info structs first
    ParamInfo level1_info{.oid = "level1", .type = catena::ParamType::STRING};
    ParamInfo level2_info{.oid = "level2", .type = catena::ParamType::STRING};
    ParamInfo level3_info{.oid = "level3", .type = catena::ParamType::STRING};

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
    EXPECT_CALL(*level1Desc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(level1Oid));
    EXPECT_CALL(*level2Desc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(level2Oid));
    EXPECT_CALL(*level3Desc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(level3Oid));

    // Create ParamInfo structs
    ParamInfo level1_info_struct{level1_info.oid, level1_info.type};
    ParamInfo level2_info_struct{level2_info.oid, level2_info.type};
    ParamInfo level3_info_struct{level3_info.oid, level3_info.type};

    auto level1 = std::make_unique<MockParam>();
    setupMockParam(level1.get(), level1_info_struct, level1Desc.descriptor.get());

    auto level2 = std::make_unique<MockParam>();
    setupMockParam(level2.get(), level2_info_struct, level2Desc.descriptor.get());

    auto level3 = std::make_unique<MockParam>();
    setupMockParam(level3.get(), level3_info_struct, level3Desc.descriptor.get());

    // Setup top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(level1));

    // Enable recursion and stream
    EXPECT_CALL(context_, hasField("recursive")).WillOnce(testing::Return(true));
    stream_ = true;

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::An<catena::exception_with_status&>(), testing::An<Authorizer&>()))
        .WillRepeatedly(testing::Invoke(
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

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(createParamInfoJson(level1_info));
    jsonBodies.push_back(createParamInfoJson(level2_info));
    jsonBodies.push_back(createParamInfoJson(level3_info));
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, jsonBodies));
}

// Test 2.2: Get top-level parameters with recursion and arrays
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithRecursionAndArrays) {
    // Define parameter info
    ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 5
    };
    ParamInfo arrayChild_info{
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
    EXPECT_CALL(*parentDesc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(parentOid));
    EXPECT_CALL(*childDesc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(childOid));

    // Create mock params using helpers
    auto parentParam = std::make_unique<MockParam>();
    setupMockParam(parentParam.get(), parent_info, parentDesc.descriptor.get());

    auto arrayChild = std::make_unique<MockParam>();
    setupMockParam(arrayChild.get(), arrayChild_info, childDesc.descriptor.get());

    // Set up top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion and stream
    EXPECT_CALL(context_, hasField("recursive")).WillOnce(testing::Return(true));
    stream_ = true;

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::An<catena::exception_with_status&>(), testing::An<Authorizer&>()))
        .WillRepeatedly(testing::Invoke([this, &arrayChild, childOid, arrayChild_info, childDesc]
            (const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                if (fqoid == childOid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(arrayChild);
                }
                status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::vector<std::string> jsonBodies;
    jsonBodies.push_back(createParamInfoJson(parent_info));
    jsonBodies.push_back(createParamInfoJson(arrayChild_info));
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, jsonBodies));
}

// Test 2.3: Get top-level parameters with recursion and error in child processing
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithRecursionError) {
    expRc_ = catena::exception_with_status("Error processing child parameter", catena::StatusCode::INTERNAL);

    // Define parameter info
    ParamInfo parent_info{
        .oid = "parent",
        .type = catena::ParamType::STRING
    };
    ParamInfo errorChild_info{
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
    EXPECT_CALL(*parentDesc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(parentOid));
    EXPECT_CALL(*childDesc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(childOid));

    // Create ParamInfo structs
    ParamInfo parent_info_struct{parent_info.oid, parent_info.type};
    ParamInfo error_child_info_struct{errorChild_info.oid, errorChild_info.type};

    auto parentParam = std::make_unique<MockParam>();
    setupMockParam(parentParam.get(), parent_info_struct, parentDesc.descriptor.get());

    // For the error child, set up a param that throws in toProto
    auto errorChild = std::make_unique<MockParam>();
    setupMockParam(errorChild.get(), error_child_info_struct, childDesc.descriptor.get());
    EXPECT_CALL(*errorChild, toProto(testing::An<catena::ParamInfoResponse&>(), testing::An<catena::common::Authorizer&>()))
        .WillOnce(testing::Invoke([](catena::ParamInfoResponse&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error processing child parameter", catena::StatusCode::INTERNAL);
        }));

    // Set up top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion and stream
    EXPECT_CALL(context_, hasField("recursive")).WillOnce(testing::Return(true));
    stream_ = true;

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::An<catena::exception_with_status&>(), testing::An<Authorizer&>()))
        .WillRepeatedly(testing::Invoke([&errorChild, childOid]
            (const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                if (fqoid == childOid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(errorChild);
                }
                status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 2.4: Get top-level parameters with error status from getTopLevelParams
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithErrorStatus) {
    expRc_ = catena::exception_with_status("Error getting parameters", catena::StatusCode::INTERNAL);

    // Setup mock expectations
    EXPECT_CALL(context_, hasField("recursive")).WillOnce(testing::Return(true));
    EXPECT_CALL(context_, stream()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error getting parameters", catena::StatusCode::INTERNAL);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 2.5: Get top-level parameters with empty list and recursion
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithEmptyListAndRecursion) {
    expRc_ = catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);

    // Setup mock expectations
    EXPECT_CALL(context_, hasField("recursive")).WillOnce(testing::Return(true));
    stream_ = true;
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([](catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// == MODE 3 TESTS: Get a specific parameter and its children if recursive ==

// Test 3.1: Get specific parameter without recursion
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_proceedSpecificParam) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    fqoid_ = "mockOid";

    // Setup mock parameter with our helper
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    ParamInfo paramInfo{
        .oid = fqoid_,
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 5
    };
    setupMockParam(mockParam.get(), paramInfo);

    // Add expectations for array type and size to trigger array length update logic
    EXPECT_CALL(*mockParam, isArrayType()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockParam, size()).WillRepeatedly(testing::Return(5));

    // Setup mock expectations for mode 2 (specific parameter)
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke(
            [&mockParam](const std::string&, catena::exception_with_status& status, Authorizer&) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(mockParam);
            }
        ));
    
    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::string jsonBody = createParamInfoJson(paramInfo);
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {jsonBody}));
}

// Test 3.2: Get specific parameter with recursion
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_getSpecificParamWithRecursion) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    fqoid_ = "mockOid";
    
    // Create parameter hierarchy using ParamHierarchyBuilder
    auto mockDesc = ParamHierarchyBuilder::createDescriptor("/" + fqoid_);
    std::string mockOidWSlash = "/" + fqoid_;
    EXPECT_CALL(*mockDesc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(mockOidWSlash));
    
    // Setup mock parameter with our helper
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    ParamInfo paramInfo{
        .oid = fqoid_,
        .type = catena::ParamType::STRING
    };
    setupMockParam(mockParam.get(), paramInfo, mockDesc.descriptor.get());

    // Setup mock expectations
    EXPECT_CALL(context_, hasField("recursive")).WillOnce(testing::Return(true));
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke(
            [&mockParam](const std::string&, catena::exception_with_status& status, Authorizer&) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(mockParam);
            }
        ));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    std::string jsonBody = createParamInfoJson(paramInfo);
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {jsonBody}));
}

// Test 3.3: Error case - parameter not found
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_parameterNotFound) {
    expRc_ = catena::exception_with_status("Parameter not found: missing_param", catena::StatusCode::NOT_FOUND);
    fqoid_ = "missing_param";

    // Setup mock expectations
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return nullptr;
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 3.4: Error case - catena exception in getParam
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_catenaExceptionInGetParam) {
    expRc_ = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
    fqoid_ = "test_param";

    // Setup mock expectations
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
            return nullptr;
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// == SECTION 4 TESTS: Catch blocks at the end of proceed() ==

// Test 4.1: Error case - catena exception in proceed()
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_catchCatenaException) {
    expRc_ = catena::exception_with_status("Test catena exception", catena::StatusCode::INTERNAL);
    fqoid_ = "test_param";
    
    // Setup mock expectations to trigger an exception during proceed()
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status&, Authorizer&) -> std::unique_ptr<IParam> {
            throw catena::exception_with_status("Test catena exception", catena::StatusCode::INTERNAL);
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 4.2: Error case - std::exception in proceed()
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_catchStdException) {
    expRc_ = catena::exception_with_status("Unknown error in ParamInfoRequest: Test std exception", catena::StatusCode::UNKNOWN);
    fqoid_ = "test_param";
    
    // Setup mock expectations to trigger a std::exception during proceed()
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status&, Authorizer&) -> std::unique_ptr<IParam> {
            throw std::runtime_error("Test std exception");
        }));

    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 4.3: Error case - unknown exception in proceed()
TEST_F(RESTParamInfoRequestTests, ParamInfoRequest_catchUnknownException) {
    expRc_ = catena::exception_with_status("Unknown error in ParamInfoRequest", catena::StatusCode::UNKNOWN);
    fqoid_ = "test_param";
    
    // Setup mock expectations to trigger an unknown exception during proceed()
    EXPECT_CALL(dm0_, getParam(fqoid_, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status&, Authorizer&) -> std::unique_ptr<IParam> {
            throw 42; // Throw an int
        }));

    // Create a new endpoint_ for this test
    endpoint_->proceed();
    endpoint_->finish();

    // Match expected and actual responses
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}