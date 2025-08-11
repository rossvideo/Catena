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
 * @brief This file is for testing the gRPC ParamInfoRequest.cpp file.
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-07-24
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"
#include "GRPCTestHelpers.h"
#include "StreamReader.h"
#include "MockParam.h"
#include "CommonTestHelpers.h"

// gRPC
#include <controllers/ParamInfoRequest.h>

using namespace catena::common;
using namespace catena::gRPC;
using namespace catena::gRPC::test;

class gRPCParamInfoRequestTests : public GRPCTest {
protected:
    
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("gRPCParamInfoRequestTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    gRPCParamInfoRequestTests() : GRPCTest() {
        // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm1_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
        EXPECT_CALL(dm1_, getTopLevelParams(testing::_, testing::_)).Times(0);
    }

    /*
     * Creates a ParamInfoRequest handler object.
     */
    void makeOne() override { new ParamInfoRequest(&service_, dms_, true); }

    void initPayload(uint32_t slot, const std::string& oid_prefix = "", bool recursive = false) {
        inVal_.set_slot(slot);
        inVal_.set_oid_prefix(oid_prefix);
        inVal_.set_recursive(recursive);
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and returns the streamed-back response.
     */
    using StreamReader = catena::gRPC::test::StreamReader<catena::ParamInfoResponse, catena::ParamInfoRequestPayload, 
        std::function<void(grpc::ClientContext*, const catena::ParamInfoRequestPayload*, grpc::ClientReadReactor<catena::ParamInfoResponse>*)>>;

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Creating the stream reader.
        StreamReader reader(&outVals_, &outRc_);
        // Making the RPC call.
        reader.MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
            client_->async()->ParamInfoRequest(ctx, payload, reactor);
        });
        // Waiting for the RPC to finish.
        reader.Await();
        // Comparing the results.
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another ParamInfoRequest handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
        
        // Validate response content if we have expected responses
        if (!expVals_.empty()) {
            ASSERT_EQ(outVals_.size(), expVals_.size()) << "Expected " << expVals_.size() << " responses, got " << outVals_.size();
            for (size_t i = 0; i < outVals_.size(); i++) {
                EXPECT_EQ(outVals_[i].SerializeAsString(), expVals_[i].SerializeAsString()) 
                    << "Response " << i << " does not match expected";
            }
        }
    }

    // In/out val
    catena::ParamInfoRequestPayload inVal_;
    std::vector<catena::ParamInfoResponse> outVals_;
    std::vector<catena::ParamInfoResponse> expVals_;
};

// == SECTION 0: Preliminary tests ==

// 0.0: Preliminary test: Creating a ParamInfoRequest object
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_Create) {
    ASSERT_TRUE(asyncCall_);
}

// 0.1: Success Case - Authorization test with valid token
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_AuthzValid) {
    initPayload(0, "/mockOid", true);
    // Use a valid JWS token with monitor scope
    authzEnabled_ = true;
    std::string mockToken = getJwsToken("st2138:mon:w st2138:op:w st2138:cfg:w st2138:adm:w");
    clientContext_.AddMetadata("authorization", "Bearer " + mockToken);

    // Setup mock parameter
    auto param = std::make_unique<MockParam>();
    ParamInfo param_info{
        .oid = "mockOid",
        .type = catena::ParamType::STRING
    };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + param_info.oid);
    setupMockParamInfo(*param, param_info, *desc.descriptor);

    // Set up expected response
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("mockOid");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);

    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::_, testing::_))
       .WillRepeatedly(testing::Invoke([&param](const std::string&, catena::exception_with_status &status, const IAuthorizer &) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(param);
        }));
    
    testRPC();
}

// 0.2: Error Case - Authorization test with invalid token
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    initPayload(0); // Initialize with valid slot

    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    EXPECT_CALL(dm1_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);

    testRPC();
}

// 0.3: Error Case - Authorization test with invalid slot
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_InvalidSlot) {
    initPayload(dms_.size()); // Use invalid slot
    expRc_ = catena::exception_with_status("Device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);

    testRPC();
}

// == SECTION/MODE 1: Get all top-level parameters without recursion == //

// 1.1: Success Case - Get all top-level parameters without recursion
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParams) {
    // Setup mock parameters
    ParamInfo param1_info{ .oid = "param1", .type = catena::ParamType::STRING };
    ParamInfo param2_info{ .oid = "param2", .type = catena::ParamType::STRING };
    auto desc1 = ParamHierarchyBuilder::createDescriptor("/" + param1_info.oid);
    auto desc2 = ParamHierarchyBuilder::createDescriptor("/" + param2_info.oid);

    // Create ParamInfo structs
    ParamInfo param1_info_struct{param1_info.oid, param1_info.type};
    ParamInfo param2_info_struct{param2_info.oid, param2_info.type};

    auto param1 = std::make_unique<MockParam>();
    setupMockParamInfo(*param1, param1_info_struct, *desc1.descriptor);
    auto param2 = std::make_unique<MockParam>();
    setupMockParamInfo(*param2, param2_info_struct, *desc2.descriptor);

    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Set up expected responses
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("param1");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("param2");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) -> std::vector<std::unique_ptr<IParam>> {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Verify the call completed successfully
    testRPC();
}

// 1.2: Success Case - Get top-level parameters with array type
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithArray) {
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    ParamInfo arrayParamInfo{ .oid = "array_param", .type = catena::ParamType::STRING_ARRAY, .array_length = 5 };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + arrayParamInfo.oid);
    auto arrayParam = std::make_unique<MockParam>();
    setupMockParamInfo(*arrayParam, arrayParamInfo, *desc.descriptor);
    top_level_params.push_back(std::move(arrayParam));

    // Set up expected responses
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("array_param");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
    expVals_.back().set_array_length(5);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    testRPC();
}

// 1.3: Error Case - Get top-level parameters with empty list returned from getTopLevelParams
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getEmptyTopLevelParams) {
    initPayload(0);
    expRc_ = catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);
    
    // Setup mock expectations - return OK status but empty list
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK); 
            return std::vector<std::unique_ptr<IParam>>();  
        }));

    testRPC();
}

// 1.4: Error Case - Get top-level parameters with error status in returned parameters (catena::exception_with_status)
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsProcessingError) {
    expRc_ = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
    
    // Setup mock parameters
    std::vector<std::unique_ptr<IParam>> top_level_params;
    ParamInfo errorParamInfo{ .oid = "error_param", .type = catena::ParamType::STRING, .status = catena::StatusCode::INTERNAL };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + errorParamInfo.oid);
    auto errorParam = std::make_unique<MockParam>();
    setupMockParamInfo(*errorParam, errorParamInfo, *desc.descriptor);
    top_level_params.push_back(std::move(errorParam));

    // Setup mock expectations   
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
            return std::move(top_level_params);
        }));

    testRPC();
}

// 1.5: Error Case - Get top-level parameters with exception thrown during parameter processing
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsThrow) {
    expRc_ = catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
    
    // Setup mock parameters
    ParamInfo param1_info{ .oid = "param1", .type = catena::ParamType::STRING };
    ParamInfo param2_info{ .oid = "param2", .type = catena::ParamType::STRING };
    auto desc1 = ParamHierarchyBuilder::createDescriptor("/" + param1_info.oid);
    auto desc2 = ParamHierarchyBuilder::createDescriptor("/" + param2_info.oid);
    auto param1 = std::make_unique<MockParam>();
    setupMockParamInfo(*param1, param1_info, *desc1.descriptor);
    auto param2 = std::make_unique<MockParam>();
    setupMockParamInfo(*param2, param2_info, *desc2.descriptor);
    
    // Set up param2 to throw during processing
    EXPECT_CALL(*param2, getOid())
        .WillRepeatedly(testing::ReturnRef(param2_info.oid));
    EXPECT_CALL(*param2, toProto(testing::An<catena::ParamInfoResponse&>(), testing::An<const IAuthorizer&>()))
        .WillOnce(testing::Invoke([](catena::ParamInfoResponse&, const IAuthorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error getting top-level parameters", catena::StatusCode::INTERNAL);
            return catena::exception_with_status("", catena::StatusCode::OK);  // This line should never be reached
        }));

    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(param1));
    top_level_params.push_back(std::move(param2));

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    testRPC();
}

// == SECTION/MODE 2: Get all top-level parameters with recursion == //

// 2.1: Success Case - Get top-level parameters with recursion and deep nesting
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithDeepNesting) {
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
    setupMockParamInfo(*level1, level1_info_struct, *level1Desc.descriptor);

    auto level2 = std::make_unique<MockParam>();
    setupMockParamInfo(*level2, level2_info_struct, *level2Desc.descriptor);

    auto level3 = std::make_unique<MockParam>();
    setupMockParamInfo(*level3, level3_info_struct, *level3Desc.descriptor);

    // Setup top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(level1));

    // Enable recursion by setting it in the request payload
    initPayload(0, "", true);

    // Set up expected responses
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("level1");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("level2");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("level3");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::An<catena::exception_with_status&>(), testing::An<const IAuthorizer&>()))
        .WillRepeatedly(testing::Invoke(
            [&level2, &level3, level2Desc, level3Desc]
            (const std::string& fqoid, catena::exception_with_status& status, const IAuthorizer&) -> std::unique_ptr<IParam> {
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

    testRPC();
}

// 2.2: Success Case - Get top-level parameters with recursion and arrays
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithRecursionAndArrays) {
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
    setupMockParamInfo(*parentParam, parent_info, *parentDesc.descriptor);

    auto arrayChild = std::make_unique<MockParam>();
    setupMockParamInfo(*arrayChild, arrayChild_info, *childDesc.descriptor);

    // Set up top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion by setting it in the request payload
    initPayload(0, "", true);

    // Set up expected responses
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("parent");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
    expVals_.back().set_array_length(5);
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("array_child");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
    expVals_.back().set_array_length(3);

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::An<catena::exception_with_status&>(), testing::An<const IAuthorizer&>()))
        .WillRepeatedly(testing::Invoke([this, &arrayChild, childOid, arrayChild_info, childDesc]
            (const std::string& fqoid, catena::exception_with_status& status, const IAuthorizer&) -> std::unique_ptr<IParam> {
                if (fqoid == childOid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(arrayChild);
                }
                status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }));

    testRPC();
}

// 2.3: Error Case - Get top-level parameters with recursion and error in child processing
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithRecursionError) {
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
    setupMockParamInfo(*parentParam, parent_info_struct, *parentDesc.descriptor);

    // For the error child, set up a param that throws in toProto
    auto errorChild = std::make_unique<MockParam>();
    setupMockParamInfo(*errorChild, error_child_info_struct, *childDesc.descriptor);
    EXPECT_CALL(*errorChild, toProto(testing::An<catena::ParamInfoResponse&>(), testing::An<const IAuthorizer&>()))
        .WillOnce(testing::Invoke([](catena::ParamInfoResponse&, const IAuthorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Error processing child parameter", catena::StatusCode::INTERNAL);
        }));

    // Set up top-level params
    std::vector<std::unique_ptr<IParam>> top_level_params;
    top_level_params.push_back(std::move(parentParam));

    // Enable recursion by setting it in the request payload
    initPayload(0, "", true);

    // Setup mock expectations for getTopLevelParams
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([&top_level_params](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(top_level_params);
        }));

    // Setup mock expectations for getParam to handle child traversal
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::An<catena::exception_with_status&>(), testing::An<const IAuthorizer&>()))
        .WillRepeatedly(testing::Invoke([&errorChild, childOid]
            (const std::string& fqoid, catena::exception_with_status& status, const IAuthorizer&) -> std::unique_ptr<IParam> {
                if (fqoid == childOid) {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(errorChild);
                }
                status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }));

    testRPC();
}

// 2.4: Error Case - Get top-level parameters with empty list and recursion
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getTopLevelParamsWithEmptyListAndRecursion) {
    expRc_ = catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);

    // Enable recursion by setting it in the request payload
    initPayload(0, "", true);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getTopLevelParams(testing::_, testing::_))
        .WillOnce(testing::Invoke([](catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::vector<std::unique_ptr<IParam>>();
        }));

    testRPC();
}

// == SECTION/MODE 3: Get a specific parameter and its children if recursive == //

// 3.1: Success Case - Get specific parameter without recursion
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_proceedSpecificParam) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    std::string fqoid = "mockOid";

    // Setup mock parameter with our helper
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    ParamInfo paramInfo{
        .oid = fqoid,
        .type = catena::ParamType::STRING_ARRAY,
        .array_length = 5
    };
    auto desc = ParamHierarchyBuilder::createDescriptor("/" + paramInfo.oid);
    setupMockParamInfo(*mockParam, paramInfo, *desc.descriptor);

    // Add expectations for array type and size to trigger array length update logic
    EXPECT_CALL(*mockParam, isArrayType()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockParam, size()).WillRepeatedly(testing::Return(5));

    // Setup mock expectations for mode 2 (specific parameter)
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke(
            [&mockParam](const std::string&, catena::exception_with_status& status, const IAuthorizer&) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(mockParam);
            }
        ));
    
    // Request specific parameter
    initPayload(0, fqoid, false);
    
    // Set up expected responses
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("mockOid");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING_ARRAY);
    expVals_.back().set_array_length(5);
    
    testRPC();
}

// 3.2: Success Case - Get specific parameter with recursion
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_getSpecificParamWithRecursion) {
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    std::string fqoid = "mockOid";
    
    // Create parameter hierarchy using ParamHierarchyBuilder
    auto mockDesc = ParamHierarchyBuilder::createDescriptor("/" + fqoid);
    std::string mockOidWSlash = "/" + fqoid;
    EXPECT_CALL(*mockDesc.descriptor, getOid()).WillRepeatedly(testing::ReturnRef(mockOidWSlash));
    
    // Setup mock parameter with our helper
    std::unique_ptr<MockParam> mockParam = std::make_unique<MockParam>();
    ParamInfo paramInfo{
        .oid = fqoid,
        .type = catena::ParamType::STRING
    };
    setupMockParamInfo(*mockParam, paramInfo, *mockDesc.descriptor);

    // Request specific parameter with recursion
    initPayload(0, fqoid, true);

    // Set up expected responses
    expVals_.clear();
    expVals_.emplace_back();
    expVals_.back().mutable_info()->set_oid("mockOid");
    expVals_.back().mutable_info()->set_type(catena::ParamType::STRING);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke(
            [&mockParam](const std::string&, catena::exception_with_status& status, const IAuthorizer&) {
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return std::move(mockParam);
            }
        ));

    testRPC();
}

// 3.3: Error Case - parameter not found
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_parameterNotFound) {
    expRc_ = catena::exception_with_status("Parameter not found: missing_param", catena::StatusCode::NOT_FOUND);
    std::string fqoid = "missing_param";

    // Request specific parameter
    initPayload(0, fqoid, false);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return nullptr;
        }));

    testRPC();
}

// 3.4: Error Case - catena exception in getParam
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_catenaExceptionInGetParam) {
    expRc_ = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
    std::string fqoid = "test_param";

    // Request specific parameter
    initPayload(0, fqoid, false);

    // Setup mock expectations
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status& status, const IAuthorizer&) {
            status = catena::exception_with_status("Error processing parameter", catena::StatusCode::INTERNAL);
            return nullptr;
        }));

    testRPC();
}

// == SECTION 4: Additional error cases at end of proceed() == //

// 4.1: Error Case - catena exception in proceed()
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_catchCatenaException) {
    expRc_ = catena::exception_with_status("Test catena exception", catena::StatusCode::INTERNAL);
    std::string fqoid = "test_param";
    
    // Request specific parameter
    initPayload(0, fqoid, false);

    // Setup mock expectations to trigger an exception during proceed()
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status&, const IAuthorizer&) -> std::unique_ptr<IParam> {
            throw catena::exception_with_status("Test catena exception", catena::StatusCode::INTERNAL);
        }));

    testRPC();
}

// 4.2: Error Case - std::exception in proceed()
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_catchStdException) {
    expRc_ = catena::exception_with_status("Failed due to unknown error in ParamInfoRequest: Test std exception", catena::StatusCode::UNKNOWN);
    std::string fqoid = "test_param";
    
    // Request specific parameter
    initPayload(0, fqoid, false);

    // Setup mock expectations to trigger a std::exception during proceed()
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status&, const IAuthorizer&) -> std::unique_ptr<IParam> {
            throw std::runtime_error("Test std exception");
        }));

    testRPC();
}

// 4.3: Error Case - unknown exception in proceed()
TEST_F(gRPCParamInfoRequestTests, ParamInfoRequest_catchUnknownException) {
    expRc_ = catena::exception_with_status("Failed due to unknown error in ParamInfoRequest", catena::StatusCode::UNKNOWN);
    std::string fqoid = "test_param";
    
    // Request specific parameter
    initPayload(0, fqoid, false);

    // Setup mock expectations to trigger an unknown exception during proceed()
    EXPECT_CALL(dm0_, getParam(fqoid, testing::_, testing::_))
        .WillOnce(testing::Invoke([](const std::string&, catena::exception_with_status&, const IAuthorizer&) -> std::unique_ptr<IParam> {
            throw 42; // Throw an int
        }));

    testRPC();
}