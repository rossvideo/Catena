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

    // Set up default behavior for getScope to avoid uninteresting mock warnings
    static const std::string scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    EXPECT_CALL(test_descriptor, getScope())
        .WillRepeatedly(::testing::Invoke([]() {
            return std::ref(scope);
        }));

    // Set up default behavior for getTopLevelParams
    EXPECT_CALL(*device, getTopLevelParams(::testing::Matcher<catena::exception_with_status&>(::testing::_), ::testing::Matcher<Authorizer&>(::testing::_)))
        .WillRepeatedly(::testing::Invoke([](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
            std::vector<std::unique_ptr<IParam>> params;
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return params;
        }));

        // Build the test parameter hierarchies
        buildTestHierarchies();
    }

    // Define the parameter hierarchy using a tree structure for clarity
    struct Node {
        std::string name;  
        std::string oid;  
        std::vector<Node> children;
    };

    // Test hierarchies as fixture members
    Node wildcardRoot;
    Node nonwildcardRoot;
    std::map<std::string, ParamHierarchyBuilder::DescriptorInfo> wildcardDescriptors;
    std::map<std::string, ParamHierarchyBuilder::DescriptorInfo> nonwildcardDescriptors;

    // Recursive function to build descriptors and establish parent-child relationships
    void buildTree(const Node& node, std::map<std::string, ParamHierarchyBuilder::DescriptorInfo>& descriptors) {
        descriptors[node.oid] = ParamHierarchyBuilder::createDescriptor(node.oid);
        for (const auto& child : node.children) {
            buildTree(child, descriptors);
            ParamHierarchyBuilder::addChild(
                descriptors[node.oid],
                child.name,
                descriptors[child.oid]
            );
        }
    }

    void buildTestHierarchies() {
        // Define the wildcard hierarchy that will be expanded by "/test/*"
        wildcardRoot = {
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

        // Define the non-wildcard hierarchy that should NOT be affected by wildcard operations
        nonwildcardRoot = {
            "nonwildcard",
            "/nonwildcard",
            {
                Node{"param", "/nonwildcard/param", {}}
            }
        };

        // Build descriptors for both hierarchies
        buildTree(wildcardRoot, wildcardDescriptors);
        buildTree(nonwildcardRoot, nonwildcardDescriptors);
    }

    std::unique_ptr<SubscriptionManager> manager;
    std::unique_ptr<MockDevice> device;
    std::unique_ptr<MockParam> mockParam;
    MockParamDescriptor test_descriptor;
};

// ======= BASIC SUBSCRIPTION TESTS =======

// Test 1.1: Success case - Adding a new subscription
TEST_F(SubscriptionManagerTest, Subscription_AddNewSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    EXPECT_TRUE(manager->addSubscription("/test/param", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test 1.2: Error case - Adding a duplicate subscription
TEST_F(SubscriptionManagerTest, Subscription_AddDuplicateSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    manager->addSubscription("/test/param", *device, rc, catena::common::Authorizer::kAuthzDisabled);
    EXPECT_FALSE(manager->addSubscription("/test/param", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    EXPECT_EQ(rc.status, catena::StatusCode::ALREADY_EXISTS);
}

// Test 1.3: Success case - Removing an existing subscription
TEST_F(SubscriptionManagerTest, Subscription_RemoveExistingSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    manager->addSubscription("/test/param", *device, rc, catena::common::Authorizer::kAuthzDisabled);
    EXPECT_TRUE(manager->removeSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test 1.4: Error case - Removing a non-existent subscription
TEST_F(SubscriptionManagerTest, Subscription_RemoveNonExistentSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    EXPECT_FALSE(manager->removeSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// Test 1.5: Success case - Getting all subscribed OIDs
TEST_F(SubscriptionManagerTest, Subscription_GetAllSubscribedOids) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    manager->addSubscription("/test/param1", *device, rc, catena::common::Authorizer::kAuthzDisabled);
    manager->addSubscription("/test/param2", *device, rc, catena::common::Authorizer::kAuthzDisabled);
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 2);
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/param2") != oids.end());
}

// ======= WILDCARD SUBSCRIPTION TESTS =======   

// Test 2.1: Success case - Basic wildcard pattern validation
TEST_F(SubscriptionManagerTest, Wildcard_IsWildcard) {
    // Test valid wildcard patterns
    EXPECT_TRUE(manager->isWildcard("/test/*"));
    EXPECT_TRUE(manager->isWildcard("/test/nested/*"));
    EXPECT_TRUE(manager->isWildcard("/*"));
    
    // Test invalid patterns
    EXPECT_FALSE(manager->isWildcard("/test/param"));
    EXPECT_FALSE(manager->isWildcard("/test/*/param"));
    EXPECT_FALSE(manager->isWildcard("/test/"));
    EXPECT_FALSE(manager->isWildcard(""));

    // Test array wildcard patterns
    EXPECT_TRUE(manager->isWildcard("/test/array/*"));
    EXPECT_TRUE(manager->isWildcard("/test/array/0/*"));
    EXPECT_TRUE(manager->isWildcard("/test/array/1/*"));
    EXPECT_FALSE(manager->isWildcard("/test/array/0"));
    EXPECT_FALSE(manager->isWildcard("/test/array/1"));
}

// Test 2.2: Success case - Basic wildcard subscription addition
TEST_F(SubscriptionManagerTest, Wildcard_AddWildcardSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return wildcard root parameter
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                if (fqoid == "/test/*") {
                    setupMockParam(param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(param.get(), fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Test adding a wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test 2.3: Success case - Wildcard subscription expansion verification
TEST_F(SubscriptionManagerTest, Wildcard_ExpansionVerification) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return wildcard parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                if (fqoid == "/test/*") {
                    setupMockParam(param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(param.get(), fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Set up device to return top-level parameters
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                auto param = std::make_unique<MockParam>();
                setupMockParam(param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                params.push_back(std::move(param));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // Add wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, catena::common::Authorizer::kAuthzDisabled));

    // Verify all 6 parameters were subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 6);
    EXPECT_TRUE(oids.find("/test") != oids.end());
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested/param2") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested/deeper") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested/deeper/param3") != oids.end());
}

// Test 2.4: Success case - Wildcard subscription removal
TEST_F(SubscriptionManagerTest, Wildcard_RemoveWildcardSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return both wildcard and non-wildcard parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                if (fqoid == "/test/*") {
                    setupMockParam(param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(param.get(), fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else if (nonwildcardDescriptors.find(fqoid) != nonwildcardDescriptors.end()) {
                    setupMockParam(param.get(), fqoid, *nonwildcardDescriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Set up device to return top-level parameters
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                
                // Add test parameter
                auto test_param = std::make_unique<MockParam>();
                setupMockParam(test_param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                params.push_back(std::move(test_param));
                
                // Add nonwildcard parameter
                auto nonwildcard_param = std::make_unique<MockParam>();
                setupMockParam(nonwildcard_param.get(), "/nonwildcard", *nonwildcardDescriptors.at("/nonwildcard").descriptor);
                params.push_back(std::move(nonwildcard_param));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));
    
    // Add wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    
    // Add non-wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/nonwildcard/param", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    
    // Remove wildcard subscription
    EXPECT_TRUE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test 2.5: Success case - Wildcard removal verification
TEST_F(SubscriptionManagerTest, Wildcard_RemovalVerification) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return both wildcard and non-wildcard parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                if (fqoid == "/test/*") {
                    setupMockParam(param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(param.get(), fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else if (nonwildcardDescriptors.find(fqoid) != nonwildcardDescriptors.end()) {
                    setupMockParam(param.get(), fqoid, *nonwildcardDescriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Set up device to return top-level parameters
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                
                // Add test parameter
                auto test_param = std::make_unique<MockParam>();
                setupMockParam(test_param.get(), "/test", *wildcardDescriptors.at("/test").descriptor);
                params.push_back(std::move(test_param));
                
                // Add nonwildcard parameter
                auto nonwildcard_param = std::make_unique<MockParam>();
                setupMockParam(nonwildcard_param.get(), "/nonwildcard", *nonwildcardDescriptors.at("/nonwildcard").descriptor);
                params.push_back(std::move(nonwildcard_param));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));
    
    // Add both subscriptions
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    EXPECT_TRUE(manager->addSubscription("/nonwildcard/param", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    
    // Remove wildcard subscription
    EXPECT_TRUE(manager->removeSubscription("/test/*", *device, rc));
    
    // Verify only non-wildcard subscription remains
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/nonwildcard/param") != oids.end());
}

// Test 2.6: Error case - Wildcard removal
TEST_F(SubscriptionManagerTest, Wildcard_RemoveNonExistentWildcard) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove a wildcard subscription that doesn't exist
    EXPECT_FALSE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// Test 2.7: Error case - Remove wildcard with invalid path
TEST_F(SubscriptionManagerTest, Wildcard_RemoveInvalidPath) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove a wildcard subscription with an invalid path
    EXPECT_FALSE(manager->removeSubscription("/invalid/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// Test 2.8: Error case - Remove wildcard with invalid format
TEST_F(SubscriptionManagerTest, Wildcard_RemoveInvalidFormat) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove a subscription with invalid wildcard format (wildcard in middle)
    EXPECT_FALSE(manager->removeSubscription("/test/*/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// ======= ALL PARAMS SUBSCRIPTION TESTS =======

// Test 3.1: Success case - Basic "all params" subscription addition
TEST_F(SubscriptionManagerTest, AllParams_AddAllParamsSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return multiple top-level parameters
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                
                // Add test parameter 
                auto test_param = std::make_unique<MockParam>();
                setupMockParam(test_param.get(), "/test", test_descriptor);
                // Use the same pattern as Authorization_test.cpp
                const std::string& scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
                EXPECT_CALL(*test_param, getScope())
                    .WillRepeatedly(::testing::ReturnRef(scope));
                params.push_back(std::move(test_param));
                
                // Add nonwildcard parameter
                auto nonwildcard_param = std::make_unique<MockParam>();
                setupMockParam(nonwildcard_param.get(), "/nonwildcard", test_descriptor);
                EXPECT_CALL(*nonwildcard_param, getScope())
                    .WillRepeatedly(::testing::ReturnRef(scope));
                params.push_back(std::move(nonwildcard_param));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // Test adding "all params" subscription
    EXPECT_TRUE(manager->addSubscription("/*", *device, rc, catena::common::Authorizer::kAuthzDisabled));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test 3.2: Success case - "All params" subscription expansion verification

// Test 3.3: Success case - "All params" subscription removal

// Test 3.4: Success case - "All params" removal verification

// Test 3.5: Error case - Remove non-existent "all params" subscription

// Test 3.6: Error case - "All params" with getTopLevelParams error

// Test 3.7: Success case - "All params" with empty top-level parameters

// Test 3.8: Success case - "All params" with authorization filtering

// Test 3.9: Success case - "All params" with mixed authorization results 
// This is new, I've never mixed up authorization before

// Test 3.10: Error case - "All params" with exception during parameter traversal
