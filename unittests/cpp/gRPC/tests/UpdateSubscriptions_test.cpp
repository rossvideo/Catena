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
 * @file UpdateSubscriptions_test.cpp
 * @brief This file is for testing the gRPC UpdateSubscriptions.cpp file.
 * @author Zuhayr Sarker (zuhayr.sarker@rossvideo.com)
 * @date 2025-08-07
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// common
#include <Logger.h>

// Test helpers
#include "GRPCTest.h"
#include "GRPCTestHelpers.h"
#include "StreamReader.h"
#include "MockParam.h"
#include "MockSubscriptionManager.h"
#include "CommonTestHelpers.h"

// gRPC
#include <controllers/UpdateSubscriptions.h>

using namespace catena::common;
using namespace catena::gRPC;
using namespace catena::gRPC::test;

// Typedef for the stream reader
using UpdateSubscriptionsStreamReader = StreamReader<catena::DeviceComponent_ComponentParam, catena::UpdateSubscriptionsPayload, std::function<void(grpc::ClientContext*, const catena::UpdateSubscriptionsPayload*, grpc::ClientReadReactor<catena::DeviceComponent_ComponentParam>*)>>;

class gRPCUpdateSubscriptionsTests : public GRPCTest {
protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("gRPCUpdateSubscriptionsTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    /*
     * Creates an UpdateSubscriptions handler object.
     */
    void makeOne() override { new UpdateSubscriptions(&service_, dms_, true); }

    /*
     * Reset counters before each test.
     */
    void SetUp() override {
        GRPCTest::SetUp();
        addedOids_ = 0;
        removedOids_ = 0;
        
        // Set up default behavior for subscriptions to be enabled
        EXPECT_CALL(dm0_, subscriptions()).WillRepeatedly(testing::Return(true));
        EXPECT_CALL(dm1_, subscriptions()).WillRepeatedly(testing::Return(true));
        
        // Set up default behavior for subscription manager
        EXPECT_CALL(service_, getSubscriptionManager()).WillRepeatedly(testing::ReturnRef(subManager_));
        EXPECT_CALL(subManager_, getAllSubscribedOids(testing::Ref(dm0_))).WillRepeatedly(testing::Return(std::set<std::string>(testOids_.begin(), testOids_.end())));
        
        // Set up default expectations for subscription operations
        for (const auto& oid : testOids_) {
            EXPECT_CALL(subManager_, addSubscription(oid, testing::Ref(dm0_), testing::_, testing::_)).WillRepeatedly(testing::Invoke(
                [this](const std::string& oid, catena::common::IDevice& dm, catena::exception_with_status& rc, const IAuthorizer& authz) -> bool {
                    addedOids_++;
                    rc = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
            EXPECT_CALL(subManager_, removeSubscription(oid, testing::Ref(dm0_), testing::_)).WillRepeatedly(testing::Invoke(
                [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) -> bool {
                    removedOids_++;
                    rc = catena::exception_with_status("", catena::StatusCode::OK);
                    return true;
                }));
        }
        
        // Set up default expectations for test parameters
        for (size_t i = 0; i < testOids_.size(); ++i) {
            // Create mock param for this OID
            mockParams_.emplace_back(std::make_unique<MockParam>());
            
            // Set up getParam expectation
            EXPECT_CALL(dm0_, getParam(testOids_[i], testing::_, testing::_)).WillRepeatedly(testing::Invoke(
                [this, i](const std::string& oid, catena::exception_with_status& status, const IAuthorizer& authz) -> std::unique_ptr<IParam> {
                    status = catena::exception_with_status("", catena::StatusCode::OK);
                    return std::move(mockParams_[i]);
                }));
            
            // Set up getOid expectation
            EXPECT_CALL(*mockParams_[i], getOid()).WillRepeatedly(testing::ReturnRefOfCopy(testOids_[i]));
            
            // Set up toProto expectation
            EXPECT_CALL(*mockParams_[i], toProto(testing::An<catena::Param&>(), testing::_)).WillRepeatedly(testing::Invoke(
                [this, i](catena::Param &param, const IAuthorizer &authz) -> catena::exception_with_status {
                    param.set_type(catena::ParamType::STRING);
                    param.mutable_value()->set_string_value("value" + std::to_string(i + 1));
                    return catena::exception_with_status("", catena::StatusCode::OK);
                }));
        }
    }

    /*
     * Helper function which initializes an UpdateSubscriptionsPayload object.
     */
    void initPayload(uint32_t slot, const std::vector<std::string>& addOids = {}, const std::vector<std::string>& remOids = {}) {
        inVal_.set_slot(slot);
        for (const auto& oid : addOids) {
            inVal_.add_added_oids(oid);
        }
        for (const auto& oid : remOids) {
            inVal_.add_removed_oids(oid);
        }
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Creating the stream reader.
        UpdateSubscriptionsStreamReader streamReader(&outVals_, &outRc_);
        // Making the RPC call.
        streamReader.MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
            client_->async()->UpdateSubscriptions(ctx, payload, reactor);
        });
        // Waiting for the RPC to finish.
        streamReader.Await();
        // Comparing the results.
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another UpdateSubscriptions handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // Input/Output values
    catena::UpdateSubscriptionsPayload inVal_;
    std::vector<catena::DeviceComponent_ComponentParam> outVals_;
    
    // Trackers for calls to add/remove subscriptions
    uint32_t addedOids_ = 0;
    uint32_t removedOids_ = 0;
    
    // Mock subscription manager
    MockSubscriptionManager subManager_;
    
    // Test parameters
    std::vector<std::string> testOids_{"param1", "param2", "errParam"};
    std::vector<std::unique_ptr<MockParam>> mockParams_;

};

/*
 * ============================================================================
 *                              Preliminary Tests
 * ============================================================================
 *
 */

// Preliminary test: Creating an UpdateSubscriptions object with makeOne
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_create) {
    ASSERT_TRUE(asyncCall_);
}

// 0.1: Success Case - UpdateSubscriptions with a device which does not support subscriptions
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_NotSupported) {
    initPayload(0, {"param1"}, {"param2"});
    expRc_ = catena::exception_with_status("Subscriptions are not enabled for this device", catena::StatusCode::FAILED_PRECONDITION);
    
    // Setting expectations.
    EXPECT_CALL(dm0_, subscriptions()).WillOnce(testing::Return(false));
    EXPECT_CALL(service_, getSubscriptionManager()).Times(0); // Should not call.
    
    // Calling proceed and testing the output
    testRPC();
}

// 0.2: Error Case - UpdateSubscriptions with an invalid slot
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_InvalidSlot) {
    initPayload(dms_.size(), {"param1"}, {"param2"});
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);
    
    // Setting expectations.
    EXPECT_CALL(service_, getSubscriptionManager()).Times(0); // Should not call.
    
    // Calling proceed and testing the output
    testRPC();
}

/*
 * ============================================================================
 *                              Normal Operation Tests
 * ============================================================================
 * 
 */
 
// 1.1: Success Case - UpdateSubscriptions normal case - add only.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AddOnly) {
    initPayload(0, {"param1", "param2"}, {});
    
    // Setting expectations for add subscription operations
    EXPECT_CALL(subManager_, addSubscription("param1", testing::Ref(dm0_), testing::_, testing::_)).WillOnce(testing::Invoke(
        [this](const std::string& oid, catena::common::IDevice& dm, catena::exception_with_status& rc, const IAuthorizer& authz) -> bool {
            addedOids_++;
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    EXPECT_CALL(subManager_, addSubscription("param2", testing::Ref(dm0_), testing::_, testing::_)).WillOnce(testing::Invoke(
        [this](const std::string& oid, catena::common::IDevice& dm, catena::exception_with_status& rc, const IAuthorizer& authz) -> bool {
            addedOids_++;
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));

    // Calling proceed and testing the output
    testRPC();

    // Verify subscription operations were called
    EXPECT_EQ(addedOids_, 2);
    EXPECT_EQ(removedOids_, 0);
}

// 1.2: Success Case - UpdateSubscriptions normal case - remove only.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_RemoveOnly) {
    initPayload(0, {}, {"param1", "param2"});
    
    // Setting expectations for remove subscription operations
    EXPECT_CALL(subManager_, removeSubscription("param1", testing::Ref(dm0_), testing::_)).WillOnce(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) -> bool {
            removedOids_++;
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    EXPECT_CALL(subManager_, removeSubscription("param2", testing::Ref(dm0_), testing::_)).WillOnce(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) -> bool {
            removedOids_++;
            rc = catena::exception_with_status("", catena::StatusCode::OK);
            return true;
        }));
    
    // Calling proceed and testing the output
    testRPC();
    
    // Verify subscription operations were called
    EXPECT_EQ(addedOids_, 0);
    EXPECT_EQ(removedOids_, 2);
}

// 1.3: Success Case - UpdateSubscriptions normal case - simultaneous add and remove.
// This may not work due to a concurrency issue.
// TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AddAndRemove) {
//     initPayload(0, {"param1"}, {"param2"});
    
//     // Calling proceed and testing the output
//     testRPC();
    
//     // Verify subscription operations were called
//     EXPECT_EQ(addedOids_, 1);
//     EXPECT_EQ(removedOids_, 1);
// }

// 1.4: Success Case - UpdateSubscriptions with a valid token.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AuthzValid) {
    initPayload(0, {"param1", "param2"}, {"param1", "param2"});
    
    // Adding authorization mockToken metadata.
    authzEnabled_ = true;
    std::string mockToken = getJwsToken("st2138:mon:w st2138:op:w st2138:cfg:w st2138:adm:w");
    clientContext_.AddMetadata("authorization", "Bearer " + mockToken);
    
    // Calling proceed and testing the output
    testRPC();
}

/*
 * ============================================================================
 *                              Error Handling Tests
 * ============================================================================
 * 
 */

// 2.1: Error Case - UpdateSubscriptions add subscription returns error.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AddReturnErr) {
    initPayload(0, {"errParam", "param1"}, {});
    expRc_ = catena::exception_with_status("Failed to add subscription", catena::StatusCode::INVALID_ARGUMENT);
    
    // Override the default expectation for errParam to throw an error.
    EXPECT_CALL(subManager_, addSubscription("errParam", testing::_, testing::_, testing::_)).WillOnce(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, const IAuthorizer& authz) {
            // Simulating an error in adding subscription.
            throw catena::exception_with_status("Failed to add subscription", catena::StatusCode::INVALID_ARGUMENT);
            return false;
        }));
    
    // Calling proceed and testing the output
    testRPC();
}

// 2.2: Error Case - UpdateSubscriptions remove subscription returns error.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_RemoveReturnErr) {
    initPayload(0, {}, {"errParam", "param1"});
    expRc_ = catena::exception_with_status("Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT);
    
    // Override the default expectation for errParam to throw an error.
    EXPECT_CALL(subManager_, removeSubscription("errParam", testing::_, testing::_)).WillOnce(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) {
            // Simulating an error in removing subscription.
            throw catena::exception_with_status("Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT);
            return false;
        }));
    
    // Calling proceed and testing the output
    testRPC();
}

// 2.3: Error Case - UpdateSubscriptions add subscription throws catena::exception_with_status.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AddThrowCatena) {
    initPayload(0, {"errParam", "param1"}, {});
    expRc_ = catena::exception_with_status("Failed to add subscription", catena::StatusCode::INVALID_ARGUMENT);
    
    // Setting expectations for errParam specifically.
    EXPECT_CALL(subManager_, addSubscription("errParam", testing::_, testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc, const IAuthorizer& authz) -> bool {
            // Simulating an error in adding subscription.
            throw catena::exception_with_status("Failed to add subscription", catena::StatusCode::INVALID_ARGUMENT);
            return false;
        }));
    
    // Calling proceed and testing the output
    testRPC();
}

// 2.4: Error Case - UpdateSubscriptions remove subscription throws catena::exception_with_status.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_RemoveThrowCatena) {
    initPayload(0, {}, {"errParam", "param1"});
    expRc_ = catena::exception_with_status("Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT);
    
    // Setting expectations for errParam specifically.
    EXPECT_CALL(subManager_, removeSubscription("errParam", testing::_, testing::_)).WillRepeatedly(testing::Invoke(
        [this](const std::string& oid, const catena::common::IDevice& dm, catena::exception_with_status& rc) -> bool {
            // Simulating an error in removing subscription.
            throw catena::exception_with_status("Failed to remove subscription", catena::StatusCode::INVALID_ARGUMENT);
            return false;
        }));
    
    // Calling proceed and testing the output
    testRPC();
}

// 2.5: Error Case - UpdateSubscriptions add subscription throws std::runtime_error.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AddThrowUnknown) {
    initPayload(0, {"errParam", "param1"}, {});
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    
    // Setting expectations for errParam specifically.
    EXPECT_CALL(subManager_, addSubscription("errParam", testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Throw(std::runtime_error("Unknown error")));
    
    // Calling proceed and testing the output
    testRPC();
}

// 2.6: Error Case - UpdateSubscriptions remove subscription throws std::runtime_error.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_RemoveThrowUnknown) {
    initPayload(0, {}, {"errParam", "param1"});
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    
    // Setting expectations for errParam specifically.
    EXPECT_CALL(subManager_, removeSubscription("errParam", testing::_, testing::_))
        .WillRepeatedly(testing::Throw(std::runtime_error("Unknown error")));
    
    // Calling proceed and testing the output
    testRPC();
}

/*
 * ============================================================================
 *                              Authorization Tests
 * ============================================================================
 * 
 */

// 3.1: Success Case - UpdateSubscriptions with authorization disabled.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AuthzDisabled) {
    initPayload(0, {"param1", "param2"}, {"param1", "param2"});
    authzEnabled_ = false;
    
    // Calling proceed and testing the output
    testRPC();
    
    // Verify subscription operations were called
    EXPECT_EQ(addedOids_, 2);
    EXPECT_EQ(removedOids_, 2);
}

// 3.2: Error Case - UpdateSubscriptions with authorization enabled but invalid token.
TEST_F(gRPCUpdateSubscriptionsTests, UpdateSubscriptions_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    
    // Setting expectations - should not call any device methods
    EXPECT_CALL(dm0_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    EXPECT_CALL(dm1_, getParam(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    EXPECT_CALL(subManager_, addSubscription(testing::An<const std::string&>(), testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(subManager_, removeSubscription(testing::An<const std::string&>(), testing::_, testing::_)).Times(0);
    
    // Calling proceed and testing the output
    testRPC();
}