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
#include "CommonMockClasses.h"
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
    
    const auto& oids = manager->getAllSubscribedOids(*device);
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
    
    // Store OIDs as class members to return references
    std::string test_oid = "/test";
    std::string nested_oid = "/test/nested";
    std::string deeper_oid = "/test/nested/deeper";
    std::string param1_oid = "/test/param1";
    std::string param2_oid = "/test/nested/param2";
    std::string param3_oid = "/test/nested/deeper/param3";
    
    // Create descriptors using the helper
    auto root = ParamHierarchyBuilder::createDescriptor(test_oid);
    auto nested = ParamHierarchyBuilder::createDescriptor(nested_oid);
    auto deeper = ParamHierarchyBuilder::createDescriptor(deeper_oid);
    auto param1 = ParamHierarchyBuilder::createDescriptor(param1_oid);
    auto param2 = ParamHierarchyBuilder::createDescriptor(param2_oid);
    auto param3 = ParamHierarchyBuilder::createDescriptor(param3_oid);

    // Set up the parameter hierarchy
    ParamHierarchyBuilder::addChild(root, "param1", param1);
    ParamHierarchyBuilder::addChild(root, "nested", nested);
    ParamHierarchyBuilder::addChild(nested, "param2", param2);
    ParamHierarchyBuilder::addChild(nested, "deeper", deeper);
    ParamHierarchyBuilder::addChild(deeper, "param3", param3);

    // Set up device to return our mock parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this, root, nested, deeper, param1, param2, param3, test_oid, nested_oid, deeper_oid, param1_oid, param2_oid, param3_oid](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            
            // For wildcard paths, return the base path parameter
            if (fqoid == "/test/*") {
                auto param = std::make_unique<MockParam>();
                setupMockParam(param.get(), test_oid, *root.descriptor);
                status = catena::exception_with_status("", catena::StatusCode::OK);
                return param;
            }

            auto param = std::make_unique<MockParam>();
            
            if (fqoid == test_oid) {
                setupMockParam(param.get(), test_oid, *root.descriptor);
            } else if (fqoid == nested_oid) {
                setupMockParam(param.get(), nested_oid, *nested.descriptor);
            } else if (fqoid == deeper_oid) {
                setupMockParam(param.get(), deeper_oid, *deeper.descriptor);
            } else if (fqoid == param1_oid) {
                setupMockParam(param.get(), param1_oid, *param1.descriptor);
            } else if (fqoid == param2_oid) {
                setupMockParam(param.get(), param2_oid, *param2.descriptor);
            } else if (fqoid == param3_oid) {
                setupMockParam(param.get(), param3_oid, *param3.descriptor);
            } else {
                std::cout << "DEBUG: Invalid path: " << fqoid << std::endl;
                status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));

    // Set up device to return all mock parameters for getTopLevelParams
    EXPECT_CALL(*device, getTopLevelParams(::testing::Matcher<catena::exception_with_status&>(::testing::_), ::testing::Matcher<Authorizer&>(::testing::_)))
        .WillRepeatedly(::testing::Invoke([root, test_oid](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
            std::vector<std::unique_ptr<IParam>> params;
            auto param = std::make_unique<MockParam>();
            EXPECT_CALL(*param, getOid())
                .WillRepeatedly(::testing::ReturnRef(test_oid));
            EXPECT_CALL(*param, getDescriptor())
                .WillRepeatedly(::testing::ReturnRef(*root.descriptor));
            EXPECT_CALL(*param, isArrayType())
                .WillRepeatedly(::testing::Return(false));
            params.push_back(std::move(param));
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return params;
        }));
    
    // Add wildcard subscription
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify all matching parameters are subscribed, including deeply nested ones
    const auto& oids = manager->getAllSubscribedOids(*device);
    std::cout << "DEBUG: Found " << oids.size() << " subscribed OIDs" << std::endl;
    for (const auto& oid : oids) {
        std::cout << "DEBUG: Subscribed OID: " << oid << std::endl;
    }
    EXPECT_EQ(oids.size(), 6);
    EXPECT_TRUE(oids.find("/test") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested/deeper") != oids.end());
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested/param2") != oids.end());
    EXPECT_TRUE(oids.find("/test/nested/deeper/param3") != oids.end());

    std::cout << "\n=== WildcardSubscriptionExpansion test completed ===\n" << std::endl;
}

//Test basic wildcard removal
TEST_F(SubscriptionManagerTest, BasicWildcardRemoval) {
    std::cout << "\n=== BasicWildcardRemoval test started ===\n" << std::endl;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Store OIDs as class members to return references
    std::string test_oid = "/test";
    std::string nested_oid = "/test/nested";
    std::string deeper_oid = "/test/nested/deeper";
    std::string param1_oid = "/test/param1";
    std::string param2_oid = "/test/nested/param2";
    std::string param3_oid = "/test/nested/deeper/param3";
    std::string nonwildcard_oid = "/nonwildcard/param";
    
    // Create descriptors using the helper
    auto root = ParamHierarchyBuilder::createDescriptor(test_oid);
    auto nested = ParamHierarchyBuilder::createDescriptor(nested_oid);
    auto deeper = ParamHierarchyBuilder::createDescriptor(deeper_oid);
    auto param1 = ParamHierarchyBuilder::createDescriptor(param1_oid);
    auto param2 = ParamHierarchyBuilder::createDescriptor(param2_oid);
    auto param3 = ParamHierarchyBuilder::createDescriptor(param3_oid);
    auto nonwildcard = ParamHierarchyBuilder::createDescriptor(nonwildcard_oid);

    // Set up the parameter hierarchy
    ParamHierarchyBuilder::addChild(root, "param1", param1);
    ParamHierarchyBuilder::addChild(root, "nested", nested);
    ParamHierarchyBuilder::addChild(nested, "param2", param2);
    ParamHierarchyBuilder::addChild(nested, "deeper", deeper);
    ParamHierarchyBuilder::addChild(deeper, "param3", param3);

    // Set up device to return our mock parameters
    EXPECT_CALL(*device, getParam(::testing::Matcher<const std::string&>(::testing::_), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke([this, root, nested, deeper, param1, param2, param3, nonwildcard, test_oid, nested_oid, deeper_oid, param1_oid, param2_oid, param3_oid, nonwildcard_oid](const std::string& fqoid, catena::exception_with_status& status, Authorizer& authz) -> std::unique_ptr<IParam> {
            auto param = std::make_unique<MockParam>();
            
            if (fqoid == "/test/*") {
                setupMockParam(param.get(), test_oid, *root.descriptor);
            } else if (fqoid == test_oid) {
                setupMockParam(param.get(), test_oid, *root.descriptor);
            } else if (fqoid == nested_oid) {
                setupMockParam(param.get(), nested_oid, *nested.descriptor);
            } else if (fqoid == deeper_oid) {
                setupMockParam(param.get(), deeper_oid, *deeper.descriptor);
            } else if (fqoid == param1_oid) {
                setupMockParam(param.get(), param1_oid, *param1.descriptor);
            } else if (fqoid == param2_oid) {
                setupMockParam(param.get(), param2_oid, *param2.descriptor);
            } else if (fqoid == param3_oid) {
                setupMockParam(param.get(), param3_oid, *param3.descriptor);
            } else if (fqoid == nonwildcard_oid) {
                setupMockParam(param.get(), nonwildcard_oid, *nonwildcard.descriptor);
            } else {
                status = catena::exception_with_status("Invalid path", catena::StatusCode::NOT_FOUND);
                return nullptr;
            }
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return param;
        }));

    // Set up device to return all mock parameters for getTopLevelParams
    EXPECT_CALL(*device, getTopLevelParams(::testing::Matcher<catena::exception_with_status&>(::testing::_), ::testing::Matcher<Authorizer&>(::testing::_)))
        .WillRepeatedly(::testing::Invoke([root, test_oid](catena::exception_with_status& status, Authorizer& authz) -> std::vector<std::unique_ptr<IParam>> {
            std::vector<std::unique_ptr<IParam>> params;
            auto param = std::make_unique<MockParam>();
            setupMockParam(param.get(), test_oid, *root.descriptor);
            params.push_back(std::move(param));
            status = catena::exception_with_status("", catena::StatusCode::OK);
            return params;
        }));
    
    // First add a wildcard subscription so we can test successful removal
    EXPECT_TRUE(manager->addSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Add another subscription that won't match the wildcard
    EXPECT_TRUE(manager->addSubscription("/nonwildcard/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Test successful removal of existing wildcard subscription
    EXPECT_TRUE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
    
    // Verify only the wildcard subscription was removed
    const auto& oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 1);
    EXPECT_TRUE(oids.find("/nonwildcard/param") != oids.end());
    
    // Test removing non-existent wildcard subscription
    EXPECT_FALSE(manager->removeSubscription("/test/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    // Test removing wildcard subscription with invalid path
    EXPECT_FALSE(manager->removeSubscription("/invalid/*", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
    
    // Test removing wildcard subscription with invalid wildcard format
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

