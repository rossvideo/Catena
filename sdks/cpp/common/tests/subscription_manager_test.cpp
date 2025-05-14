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
 * @date 25/05/13
 * @copyright Copyright © 2025 Ross Video Ltd
 */


#include <gtest/gtest.h>
#include "CommonMockClasses.h"
#include <IDevice.h>
#include <IParam.h>
#include <Status.h>
#include <Authorization.h>

using namespace catena::common;

class SubscriptionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<MockSubscriptionManager>();
        device = std::make_unique<MockDevice>();
        
        // Set up default mock behavior for device
        ON_CALL(*device, getValue(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke([](const std::string& jptr, catena::Value& value, Authorizer& authz) -> catena::exception_with_status {
                return catena::exception_with_status("", catena::StatusCode::OK);
            }));

        // Set up default mock behavior for subscription manager
        ON_CALL(*manager, addSubscription(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
                rc = catena::exception_with_status("", catena::StatusCode::OK);
                return true;
            }));
        ON_CALL(*manager, removeSubscription(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
                rc = catena::exception_with_status("", catena::StatusCode::OK);
                return true;
            }));
        static std::set<std::string> empty_set;
        ON_CALL(*manager, getAllSubscribedOids(::testing::_))
            .WillByDefault(::testing::ReturnRef(empty_set));
    }

    std::unique_ptr<MockSubscriptionManager> manager;
    std::unique_ptr<MockDevice> device;
};

// Test adding a new subscription
TEST_F(SubscriptionManagerTest, AddNewSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up expectations
    EXPECT_CALL(*manager, addSubscription("/test/param", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    
    // Test adding a new subscription
    EXPECT_TRUE(manager->addSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test adding a duplicate subscription
TEST_F(SubscriptionManagerTest, AddDuplicateSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up expectations
    EXPECT_CALL(*manager, addSubscription("/test/param", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::ALREADY_EXISTS);
            return false;
        }));
    
    // Add initial subscription
    manager->addSubscription("/test/param", *device, rc);
    
    // Try to add the same subscription again
    EXPECT_FALSE(manager->addSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::ALREADY_EXISTS);
}

// Test removing an existing subscription
TEST_F(SubscriptionManagerTest, RemoveExistingSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up expectations
    EXPECT_CALL(*manager, addSubscription("/test/param", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    EXPECT_CALL(*manager, removeSubscription("/test/param", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    
    // Add a subscription first
    manager->addSubscription("/test/param", *device, rc);
    
    // Remove the subscription
    EXPECT_TRUE(manager->removeSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::OK);
}

// Test removing a non-existent subscription
TEST_F(SubscriptionManagerTest, RemoveNonExistentSubscription) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up expectations
    EXPECT_CALL(*manager, removeSubscription("/test/param", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::NOT_FOUND);
            return false;
        }));
    
    // Try to remove a subscription that doesn't exist
    EXPECT_FALSE(manager->removeSubscription("/test/param", *device, rc));
    EXPECT_EQ(rc.status, catena::StatusCode::NOT_FOUND);
}

// Test getting all subscribed OIDs
TEST_F(SubscriptionManagerTest, GetAllSubscribedOids) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    
    // Set up expectations
    EXPECT_CALL(*manager, addSubscription("/test/param1", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    EXPECT_CALL(*manager, addSubscription("/test/param2", ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](const std::string& oid, IDevice& dm, catena::exception_with_status& rc) -> bool {
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    
    static std::set<std::string> expected_oids = {"/test/param1", "/test/param2"};
    EXPECT_CALL(*manager, getAllSubscribedOids(::testing::_))
        .WillOnce(::testing::ReturnRef(expected_oids));
    
    // Add some subscriptions
    manager->addSubscription("/test/param1", *device, rc);
    manager->addSubscription("/test/param2", *device, rc);
    
    const auto& oids = manager->getAllSubscribedOids(*device);
    EXPECT_EQ(oids.size(), 2);
    EXPECT_TRUE(oids.find("/test/param1") != oids.end());
    EXPECT_TRUE(oids.find("/test/param2") != oids.end());
}
