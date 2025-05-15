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
#include <IDevice.h>
#include <IParam.h>
#include <ParamVisitor.h>
#include <Status.h>

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
    
    // Create descriptors as class members to ensure they live throughout the test
    auto nested_descriptor = std::make_shared<MockParamDescriptor>();
    nested_descriptor->setOid(full_nested_oid); 
    EXPECT_CALL(*nested_descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(full_nested_oid));

    auto nested2_descriptor = std::make_shared<MockParamDescriptor>();
    nested2_descriptor->setOid(full_nested2_oid);
    EXPECT_CALL(*nested2_descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(full_nested2_oid));
    
    // Set up parent param and descriptor
    test_descriptor.setOid(parent_oid);  // Set the OID for test_descriptor
    test_descriptor.addSubParam(nested_oid, nested_descriptor.get());  // Add nested as subparam to parent
    nested_descriptor->addSubParam(nested2_oid, nested2_descriptor.get());  // Add nested2 as subparam to nested

    EXPECT_CALL(*mockParam, isArrayType())
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(test_descriptor));
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(parent_oid));

    // Set up the subParams maps as static to ensure they live throughout the test
    static std::unordered_map<std::string, ParamDescriptor*> parent_subParams;
    static std::unordered_map<std::string, ParamDescriptor*> nested_subParams;
    
    // Set up the hierarchy
    parent_subParams[nested_oid] = nested_descriptor.get();
    nested_subParams[nested2_oid] = nested2_descriptor.get();

    // Set up test_descriptor to return parent_subParams
    EXPECT_CALL(test_descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(parent_subParams));

    // Set up nested_descriptor to return nested_subParams
    EXPECT_CALL(*nested_descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(nested_subParams));

    // Set up nested2_descriptor to return empty subParams
    EXPECT_CALL(*nested2_descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(nested_subParams));  // Empty map for leaf node

    // Set up device to return different params based on the path
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this, nested_descriptor, nested2_descriptor, full_nested_oid, full_nested2_oid](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            
            if (fqoid == full_nested2_oid) {
                // Return nested2 param
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(full_nested2_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*nested2_descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            } else if (fqoid == full_nested_oid) {
                // Return nested param
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(full_nested_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*nested_descriptor));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            } else {
                // Return parent param
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(parent_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(test_descriptor));
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
    test_descriptor.setOid(array_oid);  // Set the OID for test_descriptor

    std::cout << "\n=== Starting VisitArrayElements test ===" << std::endl;
    std::cout << "Array OID: " << array_oid << std::endl;
    std::cout << "Element param name: " << element_param << std::endl;

    // Create descriptor for first array element parameter
    auto element_param_descriptor0 = std::make_shared<MockParamDescriptor>();
    std::string element_param_oid0 = array_oid + "/0/" + element_param;
    element_param_descriptor0->setOid(element_param_oid0);
    EXPECT_CALL(*element_param_descriptor0, getOid())
        .WillRepeatedly(::testing::ReturnRef(element_param_oid0));
    std::unordered_map<std::string, ParamDescriptor*> empty_subParams0;;
    EXPECT_CALL(*element_param_descriptor0, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(empty_subParams0));

    // Create descriptor for second array element parameter
    auto element_param_descriptor1 = std::make_shared<MockParamDescriptor>();
    std::string element_param_oid1 = array_oid + "/1/" + element_param;
    element_param_descriptor1->setOid(element_param_oid1);
    EXPECT_CALL(*element_param_descriptor1, getOid())
        .WillRepeatedly(::testing::ReturnRef(element_param_oid1));
    std::unordered_map<std::string, ParamDescriptor*> empty_subParams1;
    EXPECT_CALL(*element_param_descriptor1, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(empty_subParams1));

    // Descriptor for first array element
    auto array_element_descriptor0 = std::make_shared<MockParamDescriptor>();
    std::string array_element_oid0 = array_oid + "/0";
    array_element_descriptor0->setOid(array_element_oid0);
    EXPECT_CALL(*array_element_descriptor0, getOid())
        .WillRepeatedly(::testing::ReturnRef(array_element_oid0));
    
    // Set up subparams map for first array element
    std::unordered_map<std::string, ParamDescriptor*> element_subParams0;
    element_subParams0[element_param] = element_param_descriptor0.get();
    array_element_descriptor0->addSubParam(element_param, element_param_descriptor0.get());
    EXPECT_CALL(*array_element_descriptor0, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(element_subParams0));

    // Descriptor for second array element
    auto array_element_descriptor1 = std::make_shared<MockParamDescriptor>();
    std::string array_element_oid1 = array_oid + "/1";
    array_element_descriptor1->setOid(array_element_oid1);
    EXPECT_CALL(*array_element_descriptor1, getOid())
        .WillRepeatedly(::testing::ReturnRef(array_element_oid1));
    
    // Set up subparams map for second array element
    std::unordered_map<std::string, ParamDescriptor*> element_subParams1;
    element_subParams1[element_param] = element_param_descriptor1.get();
    array_element_descriptor1->addSubParam(element_param, element_param_descriptor1.get());
    EXPECT_CALL(*array_element_descriptor1, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(element_subParams1));

    // Set up test_descriptor to have no subparams
    std::unordered_map<std::string, ParamDescriptor*> array_root_subParams;
    test_descriptor.setOid(array_oid);
    EXPECT_CALL(test_descriptor, getOid())
        .WillRepeatedly(::testing::ReturnRef(array_oid));
    EXPECT_CALL(test_descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(array_root_subParams));

    // Set up mock param to be an array type
    EXPECT_CALL(*mockParam, isArrayType())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*mockParam, size())
        .WillRepeatedly(::testing::Return(2));
    EXPECT_CALL(*mockParam, getDescriptor())
        .WillRepeatedly(::testing::ReturnRef(test_descriptor));
    EXPECT_CALL(*mockParam, getOid())
        .WillRepeatedly(::testing::ReturnRef(array_oid));

    // Set up device to return array elements with proper descriptors
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this, array_oid, element_param, array_element_descriptor0, array_element_descriptor1, element_param_descriptor0, element_param_descriptor1, &array_root_subParams](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            
            // Check if this is an array element parameter (e.g., /test/array/0/param)
            if (fqoid == array_oid + "/0/" + element_param) {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*element_param_descriptor0));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            else if (fqoid == array_oid + "/1/" + element_param) {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*element_param_descriptor1));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            // Check if this is the first array element (e.g., /test/array/0)
            else if (fqoid == array_oid + "/0") {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*array_element_descriptor0));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            // Check if this is the second array element (e.g., /test/array/1)
            else if (fqoid == array_oid + "/1") {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(fqoid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(*array_element_descriptor1));
                EXPECT_CALL(*param, isArrayType())
                    .WillRepeatedly(::testing::Return(false));
            }
            // Check if this is the array root (e.g., /test/array)
            else if (fqoid == array_oid) {
                EXPECT_CALL(*param, getOid())
                    .WillRepeatedly(::testing::ReturnRef(array_oid));
                EXPECT_CALL(*param, getDescriptor())
                    .WillRepeatedly(::testing::ReturnRef(test_descriptor));
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