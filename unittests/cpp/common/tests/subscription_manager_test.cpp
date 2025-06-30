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
 * @date 25/05/16
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "MockDevice.h"
#include "CommonTestHelpers.h"
#include <IDevice.h>
#include <IParam.h>
#include <Status.h>
#include <Authorization.h>
#include <SubscriptionManager.h>

using namespace catena::common;

class SubscriptionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<SubscriptionManager>();
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

        // Set up default behavior for subscriptions
        EXPECT_CALL(*device, subscriptions())
            .WillRepeatedly(::testing::Return(true));

        // Set up default behavior for mockParam
        static std::string test_oid = "/test/param";
        EXPECT_CALL(*mockParam, getOid())
            .WillRepeatedly(::testing::ReturnRef(test_oid));

        // Set up default behavior for test_descriptor
        EXPECT_CALL(test_descriptor, getOid())
            .WillRepeatedly(::testing::ReturnRef(test_oid));

        // Set up default behavior for getTopLevelParams
        EXPECT_CALL(*device, getTopLevelParams(::testing::Matcher<catena::exception_with_status&>(::testing::_), ::testing::Matcher<Authorizer&>(::testing::_)))
            .WillRepeatedly(::testing::Invoke([](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }));
    }

    std::unique_ptr<SubscriptionManager> manager;
    std::unique_ptr<MockDevice> device;
    std::unique_ptr<MockParam> mockParam;
    MockParamDescriptor test_descriptor;
};

// Test adding a new subscription
TEST_F(SubscriptionManagerTest, AddNewSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Test adding a new subscription
    EXPECT_TRUE(manager->addSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test adding a duplicate subscription
TEST_F(SubscriptionManagerTest, AddDuplicateSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Add initial subscription
    manager->addSubscription("/test/param", *device, rc);
    
    // Try to add the same subscription again
    EXPECT_FALSE(manager->addSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::ALREADY_EXISTS);
}

// Test removing an existing subscription
TEST_F(SubscriptionManagerTest, RemoveExistingSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Add a subscription first
    manager->addSubscription("/test/param", *device, rc);
    
    // Remove the subscription
    EXPECT_TRUE(manager->removeSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test removing a non-existent subscription
TEST_F(SubscriptionManagerTest, RemoveNonExistentSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove a subscription that doesn't exist
    EXPECT_FALSE(manager->removeSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// Test getting all subscribed OIDs
TEST_F(SubscriptionManagerTest, GetAllSubscribedOids) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Add some subscriptions
    manager->addSubscription("/test/param1", *device, rc);
    manager->addSubscription("/test/param2", *device, rc);
    
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 2);
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/param2") != oids.end());
}


// ======= BASIC WILDCARD TESTS =======   

// Test adding a wildcard subscription
TEST_F(SubscriptionManagerTest, AddWildcardSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up mock behavior for getParam
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            EXPECT_CALL(*param, getDescriptor())
                .WillRepeatedly(::testing::ReturnRef(test_descriptor));
            EXPECT_CALL(*param, isArrayType())
                .WillRepeatedly(::testing::Return(false));
            EXPECT_CALL(*param, getOid())
                .WillRepeatedly(::testing::ReturnRef(fqoid));
            EXPECT_CALL(test_descriptor, getAllSubParams())
                .WillRepeatedly(::testing::ReturnRefOfCopy(std::unordered_map<std::string, IParamDescriptor*>()));
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));
    
    // Test adding a wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test isWildcard function
TEST_F(SubscriptionManagerTest, IsWildcard) {
    // Test valid wildcard patterns
    EXPECT_TRUE(manager->isWildcard("/test/*"));
    EXPECT_TRUE(manager->isWildcard("/test/nested/*"));
    EXPECT_TRUE(manager->isWildcard("/*"));
    
    // Test invalid patterns
    EXPECT_FALSE(manager->isWildcard("/test/param"));
    EXPECT_FALSE(manager->isWildcard("/test/*/param"));
    EXPECT_FALSE(manager->isWildcard("/test/"));
    EXPECT_FALSE(manager->isWildcard(""));

    //Test array wildcard patterns
    EXPECT_TRUE(manager->isWildcard("/test/array/*"));
    EXPECT_TRUE(manager->isWildcard("/test/array/0/*"));
    EXPECT_TRUE(manager->isWildcard("/test/array/1/*"));
    EXPECT_FALSE(manager->isWildcard("/test/array/0"));
    EXPECT_FALSE(manager->isWildcard("/test/array/1"));
    
}

// Test basic wildcard subscription expansion
TEST_F(SubscriptionManagerTest, WildcardSubscriptionExpansion) {
    std::cout << "\n=== WildcardSubscriptionExpansion test started ===\n" << std::endl;
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Define the parameter hierarchy for wildcard expansion using a tree structure
    struct Node {
        std::string name;  
        std::string oid;  
        std::vector<Node> children;
    };

    Node root{
        "root",
        "/test",
        {
            Node{"param1", "/test/param1", {}},
            Node{"nested", "/test/nested", {
                Node{"param2", "/test/nested/param2", {}},
                Node{"deeper", "/test/nested/deeper", {
                    Node{"param3", "/test/nested/deeper/param3", {}}
                }}
            }}
        }
    };

    std::map<std::string, ParamHierarchyBuilder::DescriptorInfo> descriptors;

    // Recursive function to build descriptors and establish parent-child relationships
    std::function<void(const Node&)> buildTree;
    buildTree = [&](const Node& node) {
        descriptors[node.oid] = ParamHierarchyBuilder::createDescriptor(node.oid);
        for (const auto& child : node.children) {
            buildTree(child);
            ParamHierarchyBuilder::addChild(
                descriptors[node.oid],
                child.name,
                descriptors[child.oid]
            );
        }
    };

    buildTree(root);

    // Set up device to return our mock parameters
    EXPECT_CALL(*device, getParam(
        ::testing::Matcher<const std::string&>(::testing::_), 
        ::testing::Matcher<catena::exception_with_status&>(::testing::_), 
        ::testing::Matcher<Authorizer&>(::testing::_)))
        .WillRepeatedly(::testing::Invoke(
            [descriptors = descriptors](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {

                auto param = std::make_unique<MockParam>();

                // Handle wildcard requests (e.g., "/test/*" should return the root "/test" parameter)
                if (fqoid == "/test/*") {
                    setupMockParam(param.get(), "/test", *descriptors.at("/test").descriptor);
                } else if (descriptors.find(fqoid) != descriptors.end()) {
                    // Handle specific parameter requests by looking up in our descriptor map
                    setupMockParam(param.get(), fqoid, *descriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }

                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Set up device to return all mock parameters for getTopLevelParams
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [descriptors = descriptors](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                auto param = std::make_unique<MockParam>();
                setupMockParam(param.get(), "/test", *descriptors.at("/test").descriptor);
                params.push_back(std::move(param));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // === TEST SCENARIO: Add wildcard subscription ===
    // This should trigger the expansion process and add all 6 parameters under /test/*
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc));

    // === VERIFICATION: Check that all expected parameters were subscribed ===
    auto oids = manager->getAllSubscribedOids(*device);

    // Should have exactly 6 parameters: root + 5 children
    EXPECT_EQ(oids.size(), 6);
    
    // Verify each expected parameter is present
    EXPECT_TRUE(oids.find("/test") != oids.end());                    // Root parameter
    EXPECT_TRUE(oids.find("/test/nested") != oids.end());             // Intermediate parameter
    EXPECT_TRUE(oids.find("/test/nested/deeper") != oids.end());      // Nested intermediate parameter
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());             // Leaf parameter
    EXPECT_TRUE(oids.find("/test/nested/param2") != oids.end());      // Nested leaf parameter
    EXPECT_TRUE(oids.find("/test/nested/deeper/param3") != oids.end()); // Deep nested leaf parameter

    std::cout << "\n=== WildcardSubscriptionExpansion test completed ===\n" << std::endl;
}

//Test basic wildcard removal
TEST_F(SubscriptionManagerTest, BasicWildcardRemoval) {
    std::cout << "\n=== BasicWildcardRemoval test started ===\n" << std::endl;
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Define the parameter hierarchy for wildcard removal using a tree structure
    struct Node {
        std::string name;  
        std::string oid;  
        std::vector<Node> children;
    };

    // Define the main test hierarchy that will be covered by wildcard "/test/*"
    Node root{
        "root",
        "/test",
        {
            Node{"param1", "/test/param1", {}},
            Node{"nested", "/test/nested", {
                Node{"param2", "/test/nested/param2", {}},
                Node{"deeper", "/test/nested/deeper", {
                    Node{"param3", "/test/nested/deeper/param3", {}}
                }}
            }}
        }
    };

    // Define a separate hierarchy that should NOT be affected by wildcard removal
    Node nonwildcard_node{
        "nonwildcard",
        "/nonwildcard",
        {
            Node{"param", "/nonwildcard/param", {}}
        }
    };

    std::map<std::string, ParamHierarchyBuilder::DescriptorInfo> descriptors;

    // Recursive function to build descriptors and establish parent-child relationships
    std::function<void(const Node&)> buildTree;
    buildTree = [&](const Node& node) {
        descriptors[node.oid] = ParamHierarchyBuilder::createDescriptor(node.oid);
        for (const auto& child : node.children) {
            buildTree(child);
            ParamHierarchyBuilder::addChild(
                descriptors[node.oid],
                child.name,
                descriptors[child.oid]
            );
        }
    };

    buildTree(root);
    buildTree(nonwildcard_node);

    // Set up device to return our mock parameters
    EXPECT_CALL(*device, getParam(
        ::testing::Matcher<const std::string&>(::testing::_), 
        ::testing::Matcher<catena::exception_with_status&>(::testing::_), 
        ::testing::Matcher<Authorizer&>(::testing::_)))
        .WillRepeatedly(::testing::Invoke(
            [descriptors = descriptors](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {

                auto param = std::make_unique<MockParam>();

                // Handle wildcard requests (e.g., "/test/*" should return the root "/test" parameter)
                if (fqoid == "/test/*") {
                    setupMockParam(param.get(), "/test", *descriptors.at("/test").descriptor);
                } else if (descriptors.find(fqoid) != descriptors.end()) {
                    // Handle specific parameter requests by looking up in our descriptor map
                    setupMockParam(param.get(), fqoid, *descriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }

                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Set up device to return all mock parameters for getTopLevelParams
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [descriptors = descriptors](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                
                // Add test parameter
                auto test_param = std::make_unique<MockParam>();
                setupMockParam(test_param.get(), "/test", *descriptors.at("/test").descriptor);
                params.push_back(std::move(test_param));
                
                // Add the nonwildcard parameter (should not be affected by wildcard operations)
                auto nonwildcard_param = std::make_unique<MockParam>();
                setupMockParam(nonwildcard_param.get(), "/nonwildcard", *descriptors.at("/nonwildcard").descriptor);
                params.push_back(std::move(nonwildcard_param));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));
    
    // === TEST SCENARIO 1: Add wildcard subscription ===
    // This should add all parameters under /test/* (6 total: /test, /test/param1, /test/nested, etc.)
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // === TEST SCENARIO 2: Add non-wildcard subscription ===
    // This should add only the specific parameter /nonwildcard/param
    EXPECT_TRUE(manager->addSubscription("/nonwildcard/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // === TEST SCENARIO 3: Remove wildcard subscription ===
    // This should remove only the parameters that match /test/* pattern
    // The /nonwildcard/param should remain unaffected
    EXPECT_TRUE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // === VERIFICATION: Check that only non-wildcard subscription remains ===
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);  // Should only have /nonwildcard/param left
    EXPECT_TRUE(oids.find("/nonwildcard/param") != oids.end());  // Verify the correct one remains
    
    // === TEST SCENARIO 4: Test error cases ===
    
    // Try to remove a wildcard subscription that doesn't exist
    EXPECT_FALSE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    // Try to remove a wildcard subscription with an invalid path
    EXPECT_FALSE(manager->removeSubscription("/invalid/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    // Try to remove a subscription with invalid wildcard format (wildcard in middle)
    EXPECT_FALSE(manager->removeSubscription("/test/*/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    std::cout << "\n=== BasicWildcardRemoval test completed ===\n" << std::endl;
}


// === WILDCARD ARRAY TESTS ===

//Test array wildcard subscription expansion
TEST_F(SubscriptionManagerTest, ArrayWildcardSubscriptionExpansion) {
    std::cout << "\n=== ArrayWildcardSubscriptionExpansion test started ===\n" << std::endl;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // @todo
    
}

//Test array wildcard removal
TEST_F(SubscriptionManagerTest, ArrayWildcardRemoval) {
    std::cout << "\n=== ArrayWildcardRemoval test started ===\n" << std::endl;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // @todo
}

