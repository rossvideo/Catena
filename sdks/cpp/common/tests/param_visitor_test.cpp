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
 * @brief This file is for testing the SubscriptionManager.cpp file.
 * @author zuhayr.sarker@rossvideo.com
 * @date 25/05/15
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include "CommonMockClasses.h"
#include "CommonTestHelpers.h"
#include <IDevice.h>
#include <IParam.h>
#include <Status.h>
#include <Authorization.h>
#include <ParamVisitor.h>

using namespace catena::common;

// Mock visitor that records visited parameters and arrays
class MockParamVisitor : public IParamVisitor {
public:
    std::vector<std::string> visitedPaths;
    std::vector<std::pair<std::string, uint32_t>> visitedArrays;

    void visit(IParam* param, const std::string& path) override {
        visitedPaths.push_back(path);
    }

    void visitArray(IParam* param, const std::string& path, uint32_t length) override {
        visitedArrays.push_back({path, length});
    }
};

class ParamVisitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        device = std::make_unique<MockDevice>();
        mockParam = std::make_unique<MockParam>();
        
        // Set up default mock behavior for device
        EXPECT_CALL(*device, getValue(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Invoke([](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));

        // Set up default behavior for mutex
        static std::mutex test_mutex;
        EXPECT_CALL(*device, mutex())
            .WillRepeatedly(::testing::ReturnRef(test_mutex));

        // Set up default behavior for getParam
        EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Invoke([this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(test_descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }));

        // Set up default behavior for mockParam
        EXPECT_CALL(*mockParam, getOid())
            .WillRepeatedly(::testing::ReturnRef(test_oid));
        EXPECT_CALL(*mockParam, getDescriptor())
            .WillRepeatedly(::testing::ReturnRef(test_descriptor));
        EXPECT_CALL(*mockParam, isArrayType())
            .WillRepeatedly(::testing::Return(false));
    }

    std::unique_ptr<MockDevice> device;
    std::unique_ptr<MockParam> mockParam;
    MockParamDescriptor test_descriptor;
    std::string test_oid = "/test/param";
    std::string array_oid = "/test/array";
};

// Test visiting a single parameter
TEST_F(ParamVisitorTest, VisitSingleParam) {
    MockParamVisitor visitor;
    ParamVisitor::traverseParams(mockParam.get(), "/test/param", *device, visitor);
    
    EXPECT_EQ(visitor.visitedPaths.size(), 1);
    EXPECT_EQ(visitor.visitedPaths[0], "/test/param");
    EXPECT_EQ(visitor.visitedArrays.size(), 0);
}

// Test visiting a parameter with array type
TEST_F(ParamVisitorTest, VisitArrayParam) {
    // Set up mock param to be an array type
    EXPECT_CALL(*mockParam, isArrayType())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockParam, size())
        .WillRepeatedly(::testing::Return(3));
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(test_descriptor));
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(array_oid));

    // Set up device to return array elements with proper descriptor
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            EXPECT_CALL(*param, getDescriptor())
                .WillRepeatedly(::testing::ReturnRef(test_descriptor));
            EXPECT_CALL(*param, isArrayType())
                .WillRepeatedly(::testing::Return(false));
            EXPECT_CALL(*param, getOid())
                .WillRepeatedly(::testing::ReturnRef(fqoid));
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));

    MockParamVisitor visitor;
    ParamVisitor::traverseParams(mockParam.get(), array_oid, *device, visitor);
    
    EXPECT_EQ(visitor.visitedPaths.size(), 4);  // Array + 3 elements
    EXPECT_EQ(visitor.visitedPaths[0], array_oid);
    EXPECT_EQ(visitor.visitedPaths[1], array_oid + "/0");
    EXPECT_EQ(visitor.visitedPaths[2], array_oid + "/1");
    EXPECT_EQ(visitor.visitedPaths[3], array_oid + "/2");
    EXPECT_EQ(visitor.visitedArrays.size(), 1);
    EXPECT_EQ(visitor.visitedArrays[0].first, array_oid);
    EXPECT_EQ(visitor.visitedArrays[0].second, 3);
}

// Test visiting a parameter with nested parameters
TEST_F(ParamVisitorTest, VisitNestedParams) {
    // Set up mock param to have nested parameters
    static std::string parent_oid = "/testparam";
    static std::string nested_oid = "nested";  // Just the relative path
    static std::string nested2_oid = "nested2";  // Second level nested path
    std::string full_nested_oid = parent_oid + "/" + nested_oid;
    std::string full_nested2_oid = full_nested_oid + "/" + nested2_oid;
    
    // Create descriptors using the helper
    auto parent = ParamHierarchyBuilder::createDescriptor(parent_oid);
    auto nested = ParamHierarchyBuilder::createDescriptor(full_nested_oid);
    auto nested2 = ParamHierarchyBuilder::createDescriptor(full_nested2_oid);
    
    // Set up the parameter hierarchy
    ParamHierarchyBuilder::addChild(parent, nested_oid, nested);
    ParamHierarchyBuilder::addChild(nested, nested2_oid, nested2);

    EXPECT_CALL(*mockParam, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*parent.descriptor));
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(parent_oid));

    // Set up device to return different params based on the path
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([parent, nested, nested2, full_nested_oid, full_nested2_oid](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            
            if (fqoid == full_nested2_oid) {
                // Return nested2 param
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(full_nested2_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*nested2.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            } else if (fqoid == full_nested_oid) {
                // Return nested param
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(full_nested_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*nested.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            } else {
                // Return parent param
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(parent_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*parent.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));

    MockParamVisitor visitor;
    ParamVisitor::traverseParams(mockParam.get(), parent_oid, *device, visitor);
    
    EXPECT_EQ(visitor.visitedPaths.size(), 3);
    EXPECT_EQ(visitor.visitedPaths[0], parent_oid);  // First path should be parent
    EXPECT_EQ(visitor.visitedPaths[1], full_nested_oid);  // Second path should be nested
    EXPECT_EQ(visitor.visitedPaths[2], full_nested2_oid);  // Third path should be nested2
    EXPECT_EQ(visitor.visitedArrays.size(), 0);
}

//Test visiting a parameter with array elements
TEST_F(ParamVisitorTest, VisitArrayElements) {
    // Set up mock param to be an array type
    std::string array_oid = "/test/array";
    std::string element_param = "param";  // Parameter name for array elements

    // Create descriptors using the helper
    auto array_root = ParamHierarchyBuilder::createDescriptor(array_oid);
    auto element0 = ParamHierarchyBuilder::createDescriptor(array_oid + "/0");
    auto element1 = ParamHierarchyBuilder::createDescriptor(array_oid + "/1");
    auto element_param0 = ParamHierarchyBuilder::createDescriptor(array_oid + "/0/" + element_param);
    auto element_param1 = ParamHierarchyBuilder::createDescriptor(array_oid + "/1/" + element_param);

    // Set up the parameter hierarchy
    ParamHierarchyBuilder::addChild(element0, element_param, element_param0);
    ParamHierarchyBuilder::addChild(element1, element_param, element_param1);

    // Set up mock param to be an array type
    EXPECT_CALL(*mockParam, isArrayType())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockParam, size())
        .WillRepeatedly(::testing::Return(2));
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(*array_root.descriptor));
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(array_oid));

    // Set up device to return array elements with proper descriptors
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([array_root, element0, element1, element_param0, element_param1, array_oid, element_param](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            
            if (fqoid == array_oid + "/0/" + element_param) {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*element_param0.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            else if (fqoid == array_oid + "/1/" + element_param) {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*element_param1.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            else if (fqoid == array_oid + "/0") {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*element0.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            // Check if this is the second array element (e.g., /test/array/1)
            else if (fqoid == array_oid + "/1") {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*element1.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            // Check if this is the array root (e.g., /test/array)
            else if (fqoid == array_oid) {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(array_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*array_root.descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(true));
                EXPECT_CALL(*param, size())
                    .WillRepeatedly(::testing::Return(2));
            }
            // Any other path should be rejected
            else {
                std::cout << "DEBUG TEST: Rejecting invalid path: " << fqoid << std::endl;
                status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));

    MockParamVisitor visitor;
    ParamVisitor::traverseParams(mockParam.get(), array_oid, *device, visitor);
    
    std::cout << "\nVisited paths:" << std::endl;
    for (size_t i = 0; i < visitor.visitedPaths.size(); ++i) {
        std::cout << "  " << i << ": " << visitor.visitedPaths[i] << std::endl;
    }
    
    std::cout << "\nVisited arrays:" << std::endl;
    for (size_t i = 0; i < visitor.visitedArrays.size(); ++i) {
        std::cout << "  " << i << ": path=" << visitor.visitedArrays[i].first 
                  << ", length=" << visitor.visitedArrays[i].second << std::endl;
    }
    
    EXPECT_EQ(visitor.visitedPaths.size(), 5);  // Root + 2 array elements + 2 element params
    EXPECT_EQ(visitor.visitedPaths[0], array_oid);  // First path should be array root
    EXPECT_EQ(visitor.visitedPaths[1], array_oid + "/0");  // Second path should be first element
    EXPECT_EQ(visitor.visitedPaths[2], array_oid + "/0/" + element_param);  // Third path should be first element's param
    EXPECT_EQ(visitor.visitedPaths[3], array_oid + "/1");  // Fourth path should be second element
    EXPECT_EQ(visitor.visitedPaths[4], array_oid + "/1/" + element_param);  // Fifth path should be second element's param
    EXPECT_EQ(visitor.visitedArrays.size(), 1);  // Should have one array visit
    EXPECT_EQ(visitor.visitedArrays[0].first, array_oid);  // Array path
    EXPECT_EQ(visitor.visitedArrays[0].second, 2);  // Array length
    std::cout << "\n=== VisitArrayElements test completed ===" << std::endl;
} 