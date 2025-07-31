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
 * @date 25/07/03
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
#include <Authorizer.h>
#include <SubscriptionManager.h>
#include <Logger.h>

using namespace catena::common;

class SubscriptionManagerTest : public ::testing::Test {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("SubscriptionManagerTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    SubscriptionManagerTest() : authz_(jwsToken_) {}
    
    void SetUp() override {
        device = std::make_unique<MockDevice>();
        mockParam = std::make_unique<MockParam>();
        
        // Set up default behavior for calculateMaxSubscriptions
        EXPECT_CALL(*device, calculateMaxSubscriptions(::testing::_))
            .WillRepeatedly(::testing::Return(50));
        
        manager = std::make_unique<SubscriptionManager>(*device, authz_);
        
        // Set up default mock behavior for device
        EXPECT_CALL(*device, getValue(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Invoke([](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));
        static std::mutex test_mutex;
        EXPECT_CALL(*device, mutex())
            .WillRepeatedly(::testing::ReturnRef(test_mutex));
        EXPECT_CALL(*device, slot())
            .WillRepeatedly(::testing::Return(0));

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
                static const std::string scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
                EXPECT_CALL(*param, getScope())
                    .WillRepeatedly(::testing::ReturnRef(scope));
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

    // Set up default behavior for getAllSubParams to avoid uninteresting mock warnings
    static std::unordered_map<std::string, IParamDescriptor*> empty_sub_params;
    EXPECT_CALL(test_descriptor, getAllSubParams())
        .WillRepeatedly(::testing::ReturnRef(empty_sub_params));

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
        // Convert path to OID (remove leading slash)
        std::string oid = node.oid;
        if (oid.starts_with("/") && oid.length() > 1) {
            oid = oid.substr(1);
        }
        
        descriptors[node.oid] = ParamHierarchyBuilder::createDescriptor(oid);
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
                Node{"basic", "/test/basic", {
                    Node{"param2", "/test/basic/param2", {}},
                    Node{"deeper", "/test/basic/deeper", {
                        Node{"param3", "/test/basic/deeper/param3", {}}
                    }}                
                }},
                Node{"array", "/test/array", {
                    Node{"0", "/test/array/0", {
                        Node{"subparam", "/test/array/0/subparam", {}}
                    }},
                    Node{"1", "/test/array/1", {
                        Node{"subparam", "/test/array/1/subparam", {}}
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

    // Helper function to set up "all params" test hierarchy
    struct AllParamsTestSetup {
        std::map<std::string, ParamHierarchyBuilder::DescriptorInfo> descriptors;
        std::unique_ptr<MockParam> parentParam;
        std::unique_ptr<MockParam> subParam;
        std::string parentOid;
        std::string subOid;
    };

    AllParamsTestSetup setupAllParamsTestHierarchy() {
        AllParamsTestSetup setup;
        
        // Define a simple hierarchy: param with param/subparam as a sub-parameter
        setup.parentOid = "param";
        setup.subOid = "param/subparam";
        setup.descriptors[setup.parentOid] = ParamHierarchyBuilder::createDescriptor(setup.parentOid);
        setup.descriptors[setup.subOid] = ParamHierarchyBuilder::createDescriptor(setup.subOid);
        ParamHierarchyBuilder::addChild(setup.descriptors[setup.parentOid], "subparam", setup.descriptors[setup.subOid]);

        // Create mock parameters
        setup.parentParam = std::make_unique<MockParam>();
        setup.subParam = std::make_unique<MockParam>();
        setupMockParam(*setup.parentParam, setup.parentOid, *setup.descriptors[setup.parentOid].descriptor);
        setupMockParam(*setup.subParam, setup.subOid, *setup.descriptors[setup.subOid].descriptor);
        
        return setup;
    }

    std::unique_ptr<SubscriptionManager> manager;
    std::unique_ptr<MockDevice> device;
    std::unique_ptr<MockParam> mockParam;
    MockParamDescriptor test_descriptor;
    std::string jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));
    catena::common::Authorizer authz_;

};

// ======= 1. BASIC SUBSCRIPTION TESTS =======

// Test 1.1: Success case - Adding a new subscription
TEST_F(SubscriptionManagerTest, Subscription_AddNewSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    EXPECT_TRUE(manager->addSubscription("/test/param", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test 1.2: Error case - Adding a duplicate subscription
TEST_F(SubscriptionManagerTest, Subscription_AddDuplicateSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    manager->addSubscription("/test/param", *device, rc, authz_);
    EXPECT_FALSE(manager->addSubscription("/test/param", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::ALREADY_EXISTS);
}

// Test 1.3: Success case - Removing an existing subscription
TEST_F(SubscriptionManagerTest, Subscription_RemoveExistingSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    manager->addSubscription("/test/param", *device, rc, authz_);
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
    manager->addSubscription("/test/param1", *device, rc, authz_);
    manager->addSubscription("/test/param2", *device, rc, authz_);
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 2);
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/param2") != oids.end());
}

//Test 1.6: Success case - isSubscribed
TEST_F(SubscriptionManagerTest, Subscription_IsSubscribed) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    manager->addSubscription("/test/param1", *device, rc, authz_);
    EXPECT_TRUE(manager->isSubscribed("/test/param1", *device));
    EXPECT_FALSE(manager->isSubscribed("/test/param2", *device));
}

// ======= 2. WILDCARD SUBSCRIPTION TESTS =======   

// Test 2.1: Success case - Basic wildcard pattern validation
TEST_F(SubscriptionManagerTest, Wildcard_IsWildcard) {
    // Test valid wildcard patterns
    EXPECT_TRUE(manager->isWildcard("/test/*"));
    EXPECT_TRUE(manager->isWildcard("/test/basic/*"));
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
                    setupMockParam(*param, "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(*param, fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));

    // Test adding a wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, authz_));
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
                    setupMockParam(*param, "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(*param, fqoid, *wildcardDescriptors.at(fqoid).descriptor);
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
                setupMockParam(*param, "/test", *wildcardDescriptors.at("/test").descriptor);
                params.push_back(std::move(param));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // Add wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, authz_));

    // Verify all 9 parameters were subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 9);
    EXPECT_TRUE(oids.find("/test") != oids.end());
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/basic") != oids.end());
    EXPECT_TRUE(oids.find("/test/basic/param2") != oids.end());
    EXPECT_TRUE(oids.find("/test/basic/deeper") != oids.end());
    EXPECT_TRUE(oids.find("/test/basic/deeper/param3") != oids.end());
    EXPECT_TRUE(oids.find("/test/array") != oids.end());
    EXPECT_TRUE(oids.find("/test/array/0/subparam") != oids.end());
    EXPECT_TRUE(oids.find("/test/array/1/subparam") != oids.end());
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
                    setupMockParam(*param, "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(*param, fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else if (nonwildcardDescriptors.find(fqoid) != nonwildcardDescriptors.end()) {
                    setupMockParam(*param, fqoid, *nonwildcardDescriptors.at(fqoid).descriptor);
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
                setupMockParam(*test_param, "/test", *wildcardDescriptors.at("/test").descriptor);
                params.push_back(std::move(test_param));
                
                // Add nonwildcard parameter
                auto nonwildcard_param = std::make_unique<MockParam>();
                setupMockParam(*nonwildcard_param, "/nonwildcard", *nonwildcardDescriptors.at("/nonwildcard").descriptor);
                params.push_back(std::move(nonwildcard_param));
                
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));
    
    // Add both subscriptions
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc, authz_));
    EXPECT_TRUE(manager->addSubscription("/nonwildcard/param", *device, rc, authz_));
    
    // Remove wildcard subscription
    EXPECT_TRUE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify the state after removal
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/nonwildcard/param") != oids.end());
    EXPECT_TRUE(oids.find("/test") == oids.end()); // Wildcard subscriptions should be removed
}

// Test 2.5: Error case - Wildcard removal
TEST_F(SubscriptionManagerTest, Wildcard_RemoveNonExistentWildcard) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove a wildcard subscription that doesn't exist
    EXPECT_FALSE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// Test 2.6: Error case - Remove wildcard with invalid format
TEST_F(SubscriptionManagerTest, Wildcard_RemoveInvalidFormat) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove a subscription with invalid wildcard format (wildcard in middle)
    EXPECT_FALSE(manager->removeSubscription("/test/*/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// ======= 3. ALL PARAMS SUBSCRIPTION TESTS =======

// Test 3.1: Success case - Basic "all params" subscription addition and removal
TEST_F(SubscriptionManagerTest, AllParams_AddAllParamsSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up test hierarchy using helper function
    auto setup = setupAllParamsTestHierarchy();
    
    // Set up device to return all parameters 
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&setup](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                // Create new parameters with proper authorization setup
                auto parentParam = std::make_unique<MockParam>();
                setupMockParam(*parentParam, setup.parentOid, *setup.descriptors[setup.parentOid].descriptor);
                auto subParam = std::make_unique<MockParam>();
                setupMockParam(*subParam, setup.subOid, *setup.descriptors[setup.subOid].descriptor);
                // Return both parameters
                params.push_back(std::move(parentParam));
                params.push_back(std::move(subParam));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // Test adding "all params" subscription with authorization enabled
    EXPECT_TRUE(manager->addSubscription("/*", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify that both the parent and sub-parameter were subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 2);
    EXPECT_TRUE(oids.find("/param") != oids.end());
    EXPECT_TRUE(oids.find("/param/subparam") != oids.end());

    // Now remove the "all params" subscription
    EXPECT_TRUE(manager->removeSubscription("/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify that all subscriptions were removed
    oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 0);
}

// Test 3.2: Success case - "All params" with mixed authorization results
TEST_F(SubscriptionManagerTest, AllParams_MixedAuthorizationResults) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up test hierarchy using helper function
    auto setup = setupAllParamsTestHierarchy();
    
    // Set up different scopes for authorization testing
    const std::string authorized_scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    const std::string unauthorized_scope = Scopes().getForwardMap().at(Scopes_e::kUndefined);
    
    // Override getParam behavior for this test to return parameters with correct scopes
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&setup, &authorized_scope, &unauthorized_scope](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                // Setup parameters with correct scopes based on OID pattern
                if (fqoid.find(setup.subOid) != std::string::npos) {
                    setupMockParam(*param, fqoid, *setup.descriptors[setup.subOid].descriptor, false, 0, unauthorized_scope);
                } else if (fqoid.find(setup.parentOid) != std::string::npos) {
                    setupMockParam(*param, fqoid, *setup.descriptors[setup.parentOid].descriptor, false, 0, authorized_scope);
                } else {
                    status = catena::exception_with_status("Parameter not found", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));
    
    // Set up device to return all parameters
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&setup, &authorized_scope, &unauthorized_scope](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                auto parentParam = std::make_unique<MockParam>();
                setupMockParam(*parentParam, setup.parentOid, *setup.descriptors[setup.parentOid].descriptor);
                params.push_back(std::move(parentParam));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // Test adding "all params" subscription with mixed authorization
    EXPECT_TRUE(manager->addSubscription("/*", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify that only the authorized parameter was subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/param") != oids.end());
    EXPECT_TRUE(oids.find("/param/subparam") == oids.end()); // Should not be subscribed due to authorization
}

// Test 3.3: Error case - Remove non-existent "all params" subscription
TEST_F(SubscriptionManagerTest, AllParams_RemoveNonExistentAllParamsSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove an "all params" subscription that doesn't exist
    EXPECT_FALSE(manager->removeSubscription("/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    // Verify no subscriptions exist
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 0);
}

// Test 3.4: Error case - "All params" with getTopLevelParams error
TEST_F(SubscriptionManagerTest, AllParams_GetTopLevelParamsError) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return error from getTopLevelParams
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                status = catena::exception_with_status("Failed to get top level parameters", catena::StatusCode::INTERNAL);
                return std::vector<std::unique_ptr<IParam>>();
            }
        ));

    // Test adding "all params" subscription when getTopLevelParams fails
    EXPECT_FALSE(manager->addSubscription("/*", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::INTERNAL);
    
    // Verify no subscriptions were added
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 0);
}

// Test 3.5: Error case - "All params" with exception during parameter traversal
TEST_F(SubscriptionManagerTest, AllParams_ParameterTraversalException) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up test hierarchy using helper function
    auto setup = setupAllParamsTestHierarchy();
    
    // Set up getScope expectations (needed for authorization checks)
    const std::string param_scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    EXPECT_CALL(*setup.parentParam, getScope())
        .WillRepeatedly(::testing::ReturnRef(param_scope));
    EXPECT_CALL(*setup.subParam, getScope())
        .WillRepeatedly(::testing::ReturnRef(param_scope));
    
    // Set up getAllSubParams to throw an exception during traversal
    EXPECT_CALL(*setup.descriptors[setup.parentOid].descriptor, getAllSubParams())
        .WillRepeatedly(::testing::Throw(std::runtime_error("Traversal error")));
    
    // Set up device to return all parameters 
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&setup, &param_scope](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                // Return both the parent and sub-parameter
                params.push_back(std::move(setup.parentParam));
                params.push_back(std::move(setup.subParam));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));

    // Test adding "all params" subscription when parameter traversal throws an exception
    EXPECT_THROW(manager->addSubscription("/*", *device, rc, authz_), std::runtime_error);
}

// ======= 4. ARRAY SUBSCRIPTION TESTS =======

// Test 4.1: Success case - Basic array element subscription
TEST_F(SubscriptionManagerTest, Array_ElementSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up array element parameter with sub-parameter
    auto elementParam = std::make_unique<MockParam>();
    setupMockParam(*elementParam, "/test/array/0/subparam", test_descriptor, false);
    
    // Set up device to return array element parameter
    EXPECT_CALL(*device, getParam("/test/array/0/subparam", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&elementParam](const std::string&, catena::exception_with_status& status, Authorizer&) {
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return std::move(elementParam);
        }));

    // Test adding array element subscription with proper sub-parameter path
    EXPECT_TRUE(manager->addSubscription("/test/array/0/subparam", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify array element sub-parameter is subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/test/array/0/subparam") != oids.end());
}

// Test 4.2: Success case - Basic array subscription with nested elements
TEST_F(SubscriptionManagerTest, Array_BasicArraySubscriptionWithNestedElements) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up array parameter with nested structure
    auto arrayParam = std::make_unique<MockParam>();
    setupMockParam(*arrayParam, "/test/array", test_descriptor, true, 2);
    
    // Set up device to return array elements with nested parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            setupMockParam(*param, fqoid, test_descriptor, false);
            static const std::string scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
            EXPECT_CALL(*param, getScope())
                .WillRepeatedly(::testing::ReturnRef(scope));
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));

    // Test adding array subscription
    EXPECT_TRUE(manager->addSubscription("/test/array", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify array is subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/test/array") != oids.end());
    
    // Remove array subscription
    EXPECT_TRUE(manager->removeSubscription("/test/array", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify array subscription was removed
    oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 0);
}

// Test 4.3: Success case - Array wildcard subscription with nested elements
TEST_F(SubscriptionManagerTest, Array_WildcardSubscriptionWithNestedElements) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up array parameter with nested structure
    auto arrayParam = std::make_unique<MockParam>();
    setupMockParam(*arrayParam, "/test/array", test_descriptor, true, 2);
    auto subParam0 = std::make_unique<MockParam>();
    setupMockParam(*subParam0, "/test/array/0/subparam", test_descriptor, false);
    auto subParam1 = std::make_unique<MockParam>();
    setupMockParam(*subParam1, "/test/array/1/subparam", test_descriptor, false);
    
    // Set up device to return array and subparams
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                auto it = wildcardDescriptors.find(fqoid);
                if (it != wildcardDescriptors.end()) {
                    auto param = std::make_unique<MockParam>();
                    setupMockParam(*param, fqoid, *it->second.descriptor);
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return param;
                } else {
                    status = catena::exception_with_status("Not found", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
            }
        ));

    // Test adding array wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/array/*", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify array and its elements with complete parameter paths are subscribed
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 3); // Array + 2 elements with sub-parameters
    EXPECT_TRUE(oids.find("/test/array") != oids.end());
    EXPECT_TRUE(oids.find("/test/array/0/subparam") != oids.end());
    EXPECT_TRUE(oids.find("/test/array/1/subparam") != oids.end());
    
    // Remove array wildcard subscription
    EXPECT_TRUE(manager->removeSubscription("/test/array/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify all array subscriptions were removed
    oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 0);
}

// Test 4.4: Success case - Array element subscription isSubscribed check
TEST_F(SubscriptionManagerTest, Array_IsSubscribedCheck) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                auto it = wildcardDescriptors.find(fqoid);
                if (it != wildcardDescriptors.end()) {
                    auto param = std::make_unique<MockParam>();
                    setupMockParam(*param, fqoid, *it->second.descriptor);
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return param;
                } else {
                    status = catena::exception_with_status("Not found", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
            }
        ));

    // Add array element subscription
    EXPECT_TRUE(manager->addSubscription("/test/array/0/subparam", *device, rc, authz_));
    
    // Test isSubscribed for array element subscription
    EXPECT_TRUE(manager->isSubscribed("/test/array/0/subparam", *device));
    EXPECT_FALSE(manager->isSubscribed("/test/array/1/subparam", *device)); // Not subscribed to yet.
    EXPECT_FALSE(manager->isSubscribed("/test/other", *device)); // Doesn't exist.
    
    // Add another array element subscription
    EXPECT_TRUE(manager->addSubscription("/test/array/1/subparam", *device, rc, authz_));
    
    // Test isSubscribed for both array element subscriptions
    EXPECT_TRUE(manager->isSubscribed("/test/array/0/subparam", *device)); 
    EXPECT_TRUE(manager->isSubscribed("/test/array/1/subparam", *device));
    EXPECT_FALSE(manager->isSubscribed("/test/array/2/subparam", *device)); // Was never subscribed to.
}

// Test 4.5: Error case - Duplicate array element subscription
TEST_F(SubscriptionManagerTest, Array_DuplicateSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up device to return parameters using wildcardDescriptors
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer&) -> std::unique_ptr<IParam> {
                auto it = wildcardDescriptors.find(fqoid);
                if (it != wildcardDescriptors.end()) {
                    auto param = std::make_unique<MockParam>();
                    setupMockParam(*param, fqoid, *it->second.descriptor);
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return param;
                } else {
                    status = catena::exception_with_status("Not found", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
            }
        ));

    // Add array element subscription
    EXPECT_TRUE(manager->addSubscription("/test/array/0/subparam", *device, rc, authz_));
    
    // Try to add duplicate array element subscription
    EXPECT_FALSE(manager->addSubscription("/test/array/0/subparam", *device, rc, authz_));
    EXPECT_EQ(rc.status, catena::StatusCode::ALREADY_EXISTS);
    
    // Verify only one subscription exists
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/test/array/0/subparam") != oids.end());
}

// Test 4.6: Error case - Remove non-existent array subscription
TEST_F(SubscriptionManagerTest, Array_RemoveNonExistentSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Try to remove non-existent array subscription
    EXPECT_FALSE(manager->removeSubscription("/test/array", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    // Verify no subscriptions exist
    auto oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 0);
}

// Test 4.7: Error case - Resource exhaustion with non-wildcard subscription
TEST_F(SubscriptionManagerTest, ResourceExhaustion_NonWildcardSubscription) {
    // Override the subscription limit to be very low
    EXPECT_CALL(*device, calculateMaxSubscriptions(::testing::_))
        .WillRepeatedly(::testing::Return(1));
    
    // Recreate manager with the new limit
    manager = std::make_unique<SubscriptionManager>(*device, authz_);
    
    // First subscription should succeed
    catena::exception_with_status rc("", catena::StatusCode::OK);
    bool result = manager->addSubscription("/test1", *device, rc, authz_);
    EXPECT_TRUE(result);
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Second subscription should fail with resource exhaustion
    result = manager->addSubscription("/test2", *device, rc, authz_);
    EXPECT_FALSE(result);
    EXPECT_EQ(rc.status, catena::StatusCode::RESOURCE_EXHAUSTED);
}

// Test 4.8: Error case - Resource exhaustion with wildcard subscription
TEST_F(SubscriptionManagerTest, ResourceExhaustion_WildcardSubscription) {
    // Override the subscription limit to allow only 2 subscriptions
    EXPECT_CALL(*device, calculateMaxSubscriptions(::testing::_))
        .WillRepeatedly(::testing::Return(2));
    
    // Recreate manager with the new limit
    manager = std::make_unique<SubscriptionManager>(*device, authz_);
    
    // Use the existing wildcard hierarchy which creates 9 subscriptions (exceeding our limit of 2)
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [this](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
                auto param = std::make_unique<MockParam>();
                if (fqoid == "/test/*") {
                    setupMockParam(*param, "/test", *wildcardDescriptors.at("/test").descriptor);
                } else if (wildcardDescriptors.find(fqoid) != wildcardDescriptors.end()) {
                    setupMockParam(*param, fqoid, *wildcardDescriptors.at(fqoid).descriptor);
                } else {
                    status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                    return nullptr;
                }
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }
        ));
    
    catena::exception_with_status rc("", catena::StatusCode::OK);
    bool result = manager->addSubscription("/test/*", *device, rc, authz_);
    EXPECT_FALSE(result);
    EXPECT_EQ(rc.status, catena::StatusCode::RESOURCE_EXHAUSTED);
}

// Test 4.9: Error case - Resource exhaustion with all params subscription
TEST_F(SubscriptionManagerTest, ResourceExhaustion_AllParamsSubscription) {
    // Override the subscription limit to allow only 1 subscription
    EXPECT_CALL(*device, calculateMaxSubscriptions(::testing::_))
        .WillRepeatedly(::testing::Return(1));
    
    // Recreate manager with the new limit
    manager = std::make_unique<SubscriptionManager>(*device, authz_);
    
    // Set up test hierarchy using helper function (creates 2 parameters)
    auto setup = setupAllParamsTestHierarchy();
    
    // Mock getTopLevelParams to return multiple parameters that would exceed the limit
    EXPECT_CALL(*device, getTopLevelParams(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&setup](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
                std::vector<std::unique_ptr<IParam>> params;
                // Create new parameters with proper authorization setup
                auto parentParam = std::make_unique<MockParam>();
                setupMockParam(*parentParam, setup.parentOid, *setup.descriptors[setup.parentOid].descriptor);
                auto subParam = std::make_unique<MockParam>();
                setupMockParam(*subParam, setup.subOid, *setup.descriptors[setup.subOid].descriptor);
                // Return both parameters (exceeding our limit of 1)
                params.push_back(std::move(parentParam));
                params.push_back(std::move(subParam));
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return params;
            }
        ));
    
    catena::exception_with_status rc("", catena::StatusCode::OK);
    bool result = manager->addSubscription("/*", *device, rc, authz_);
    EXPECT_FALSE(result);
    EXPECT_EQ(rc.status, catena::StatusCode::RESOURCE_EXHAUSTED);
}
