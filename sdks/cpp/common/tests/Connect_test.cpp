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

#include "CommonMockClasses.h"
#include <rpc/Connect.h>
#include <iostream>

using namespace catena::common;

// Test class that implements Connect for testing both interface and implementation
class TestConnect : public Connect {
public:
    TestConnect(IDevice& dm, ISubscriptionManager& subscriptionManager) 
        : Connect(dm, subscriptionManager) {}
    
    bool isCancelled() override { return cancelled_; }
    void setCancelled(bool cancelled) { cancelled_ = cancelled; }

    // Expose protected methods for testing
    using Connect::updateResponse_;
    using Connect::initAuthz_;
    using Connect::detailLevel_; 

    // Expose state for verification
    bool hasUpdate() const { return hasUpdate_; }
    const catena::PushUpdates& getResponse() const { return res_; }

private:
    bool cancelled_ = false;
};

// Fixture
class ConnectTests : public ::testing::Test {
protected:
    void SetUp() override {
        connect = std::make_unique<TestConnect>(dm, subscriptionManager);
        
        // Common test data
        mockToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIi"
                   "OiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2Nvc"
                   "GUiOiJzdDIxMzg6bW9uOncgc3QyMTM4Om9wOncgc3QyMTM4Om"
                   "NmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MTUxNjIzOTAyMiw"
                   "ibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dTo"
                   "krEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko965"
                   "3v0khyUT4UKeOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKH"
                   "kWi4P3-CYWrwe-g6b4-a33Q0k6tSGI1hGf2bA9cRYr-VyQ_T3"
                   "RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEmuIwNOVM3EcVEaL"
                   "yISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg_w"
                   "bOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9M"
                   "dvJH-cx1s146M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
        testOid = "/test/param";
        testIdx = 0;

        // Set detail level to FULL
        connect->detailLevel_ = catena::Device_DetailLevel_FULL;
    }

    void TearDown() override {
        connect.reset();
    }

    // Common test data
    std::string mockToken;
    std::string testOid;
    size_t testIdx;
    MockDevice dm;
    MockSubscriptionManager subscriptionManager;
    std::unique_ptr<TestConnect> connect;

    // Helper method to setup common expectations
    void setupCommonExpectations(MockParam& param, MockParamDescriptor& descriptor) {
        
        // Setup default behavior for getScope to avoid uninteresting mock warnings
        static const std::string scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
        EXPECT_CALL(param, getScope())
            .WillRepeatedly(::testing::Invoke([]() {
                return std::ref(scope);
            }));

        // Setup detail level expectation
        EXPECT_CALL(dm, detail_level())
            .WillRepeatedly(::testing::Invoke([]() {
                return catena::Device_DetailLevel_FULL;
            }));

        // Setup descriptor expectations
        EXPECT_CALL(param, getDescriptor())
            .WillRepeatedly(::testing::Invoke([&descriptor]() {
                return std::ref(descriptor);
            }));
        EXPECT_CALL(descriptor, minimalSet())
            .WillRepeatedly(::testing::Invoke([]() {
                return true;
            }));

        // Setup subscription manager expectations
        static std::set<std::string> subscribed_set{testOid};
        EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
            .WillRepeatedly(::testing::Invoke([](const IDevice&) -> const std::set<std::string>& {
                return subscribed_set;
            }));
    }

    void setupMockParam(MockParam* param, const std::string& oid, const MockParamDescriptor& descriptor) {
        // Setup mock param expectations
        EXPECT_CALL(*param, getOid())
            .WillRepeatedly(::testing::Invoke([&oid]() {
                return std::ref(oid);
            }));
        EXPECT_CALL(*param, getDescriptor())
            .WillRepeatedly(::testing::Invoke([&descriptor]() {
                return std::ref(descriptor);
            }));
        EXPECT_CALL(*param, isArrayType())
            .WillRepeatedly(::testing::Invoke([]() {
                return false;
            }));
    }
};

/*
 * ============================================================================
 *                               Connect Tests
 * ============================================================================
 */
 

// Test 0.1 - updateResponse authorization check when disabled
TEST_F(ConnectTests, updateResponseAuthorizationCheckDisabled) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup common expectations
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Test authorization disabled - should allow update
    connect->initAuthz_("", false);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, testIdx, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 0.2 - updateResponse authorization check when enabled but fails
TEST_F(ConnectTests, updateResponseAuthorizationCheckEnabledFails) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup common expectations
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Test authorization enabled but fails
    connect->initAuthz_(mockToken, true);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("Auth failed", catena::StatusCode::PERMISSION_DENIED);
        }));
    
    connect->updateResponse_(testOid, testIdx, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 0.3 - updateResponse authorization check when enabled and succeeds
TEST_F(ConnectTests, updateResponseAuthorizationCheckEnabledSucceeds) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup common expectations
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Test authorization enabled and succeeds
    connect->initAuthz_(mockToken, true);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, testIdx, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// TEST 1 - Testing updateResponse_ with parameter update and FULL detail level.
// TEST_F(ConnectTests, updateResponseWithParamFullDetail) {
//     // Setup test data
//     std::string oid = "/test/param";
//     size_t idx = 0;
//     MockParam param;
//     MockParamDescriptor descriptor;
    
//     // Setup expectations
//     EXPECT_CALL(dm, detail_level())
//         .WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
//     EXPECT_CALL(param, getDescriptor())
//         .WillOnce(::testing::ReturnRef(descriptor));
//     static std::set<std::string> empty_set;
//     EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
//         .WillOnce(::testing::ReturnRef(empty_set));
//     EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
//         .WillOnce(::testing::Return(catena::exception_with_status("", catena::StatusCode::OK)));

//     // Call updateResponse_
//     connect->updateResponse_(oid, idx, &param);

//     // Verify update was triggered
//     EXPECT_TRUE(connect->hasUpdate());
//     EXPECT_EQ(connect->getResponse().value().oid(), oid);
//     EXPECT_EQ(connect->getResponse().value().element_index(), idx);
// }
// TEST 2 - Testing updateResponse_ with MINIMAL detail level.
// TEST_F(ConnectTests, updateResponseWithParamMinimalDetail) {
//     // Setup test data
//     std::string oid = "/test/param";
//     size_t idx = 0;
//     MockParam param;
//     MockParamDescriptor descriptor;
    
//     // Setup expectations
//     EXPECT_CALL(dm, detail_level())
//         .WillOnce(::testing::Return(catena::Device_DetailLevel_MINIMAL));
//     EXPECT_CALL(param, getDescriptor())
//         .WillOnce(::testing::ReturnRef(descriptor));
//     EXPECT_CALL(descriptor, minimalSet())
//         .WillOnce(::testing::Return(true));
//     static std::set<std::string> empty_set2;
//     EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
//         .WillOnce(::testing::ReturnRef(empty_set2));
//     EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
//         .WillOnce(::testing::Return(catena::exception_with_status("", catena::StatusCode::OK)));

//     // Call updateResponse_
//     connect->updateResponse_(oid, idx, &param);

//     // Verify update was triggered
//     EXPECT_TRUE(connect->hasUpdate());
// }

// TEST 3 - Testing updateResponse_ with SUBSCRIPTIONS detail level.
// TEST_F(ConnectTests, updateResponseWithParamSubscriptionsDetail) {
//     // Setup test data
//     std::string oid = "/test/param";
//     size_t idx = 0;
//     MockParam param;
//     MockParamDescriptor descriptor;
    
//     //Need Ben's PR to go through to fix descriptors ! Currently segfaults!

//     // Setup expectations
//     EXPECT_CALL(dm, detail_level())
//         .WillOnce(::testing::Return(catena::Device_DetailLevel_SUBSCRIPTIONS));
//     EXPECT_CALL(param, getDescriptor())
//         .WillOnce(::testing::ReturnRef(descriptor));
//     EXPECT_CALL(descriptor, minimalSet())
//         .WillOnce(::testing::Return(false));
//     static std::set<std::string> subscribed_set{oid};
//     EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
//         .WillOnce(::testing::ReturnRef(subscribed_set));
//     EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
//         .WillOnce(::testing::Return(catena::exception_with_status("", catena::StatusCode::OK)));

//     // Call updateResponse_
//     connect->updateResponse_(oid, idx, &param);

//     // Verify update was triggered
//     EXPECT_TRUE(connect->hasUpdate());
// }

// // TEST 4 - Testing updateResponse_ with language pack update.
// TEST_F(ConnectTests, updateResponseWithLanguagePack) {
//     // Setup test data
//     catena::DeviceComponent_ComponentLanguagePack languagePack;
//     languagePack.set_language("en");
    
//     // Call updateResponse_
//     connect->updateResponse_(languagePack);

//     // Verify update was triggered and language pack was set
//     EXPECT_TRUE(connect->hasUpdate());
//     EXPECT_EQ(connect->getResponse().device_component().language_pack().language(), "en");
// }

// // TEST 5 - Testing initAuthz_ with authorization enabled.
// TEST_F(ConnectTests, initAuthzEnabled) {
//     // Call initAuthz_ with authz enabled
//     connect->initAuthz_("test_token", true);

//     // Verify authorization was initialized
//     // Note: We can't directly verify the internal state, but we can verify
//     // that subsequent operations work with the authorization
//     std::string oid = "/test/param";
//     size_t idx = 0;
//     MockParam param;
//     MockParamDescriptor descriptor;
    
//     EXPECT_CALL(dm, detail_level())
//         .WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
//     EXPECT_CALL(param, getDescriptor())
//         .WillOnce(::testing::ReturnRef(descriptor));
//     static std::set<std::string> empty_set3;
//     EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
//         .WillOnce(::testing::ReturnRef(empty_set3));
//     EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
//         .WillOnce(::testing::Return(catena::exception_with_status("", catena::StatusCode::OK)));

//     connect->updateResponse_(oid, idx, &param);
//     EXPECT_TRUE(connect->hasUpdate());
// }

// /* 
//  * TEST 6 - Testing updateResponse_ with cancelled connection.
//  */
// TEST_F(ConnectTests, updateResponseCancelled) {
//     // Setup test data
//     std::string oid = "/test/param";
//     size_t idx = 0;
//     MockParam param;
    
//     // Set cancelled state
//     connect->setCancelled(true);

//     // Call updateResponse_
//     connect->updateResponse_(oid, idx, &param);

//     // Verify update was triggered immediately due to cancellation
//     EXPECT_TRUE(connect->hasUpdate());
// }

// // TEST 7 - Testing updateResponse_ with authorization failure.
// TEST_F(ConnectTests, updateResponseAuthzFailure) {
//     // Setup test data
//     std::string oid = "/test/param";
//     size_t idx = 0;
//     MockParam param;
//     MockParamDescriptor descriptor;
    
//     // Setup expectations
//     EXPECT_CALL(dm, detail_level())
//         .WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
//     EXPECT_CALL(param, getDescriptor())
//         .WillOnce(::testing::ReturnRef(descriptor));
//     static std::set<std::string> empty_set4;
//     EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
//         .WillOnce(::testing::ReturnRef(empty_set4));
//     EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
//         .WillOnce(::testing::Return(catena::exception_with_status("Auth failed", catena::StatusCode::PERMISSION_DENIED)));

//     // Call updateResponse_
//     connect->updateResponse_(oid, idx, &param);

//     // Verify no update was triggered due to auth failure
//     EXPECT_FALSE(connect->hasUpdate());
// } 