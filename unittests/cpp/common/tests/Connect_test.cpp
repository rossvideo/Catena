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

#include "MockDevice.h"
#include "MockSubscriptionManager.h"
#include "MockParam.h"
#include "MockParamDescriptor.h"
#include "MockLanguagePack.h"
#include "CommonTestHelpers.h"
#include <rpc/Connect.h>
#include <iostream>
#include <Logger.h>

using namespace catena::common;

// Test class that implements Connect for testing both interface and implementation
class TestConnect : public Connect {
  public:
    TestConnect(SlotMap& dms, ISubscriptionManager& subscriptionManager) 
        : Connect(dms, subscriptionManager) {
            forceConnection_ = false;
        }
    
    bool isCancelled() override { return shutdown_; }
    void forceConnection(bool forceConnection) { forceConnection_ = forceConnection; }
    void objectId(uint32_t id) { objectId_ = id; }

    // Expose protected methods for testing
    using Connect::updateResponse_;
    using Connect::initAuthz_;
    using Connect::detailLevel_; 

    // Expose state for verification
    bool hasUpdate() const { return hasUpdate_; }
    const catena::PushUpdates& getResponse() const { return res_; }
};

// Fixture
class CommonConnectTest : public ::testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("ConnectTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    void SetUp() override {
        connect = std::make_unique<TestConnect>(dms_, subscriptionManager);
        // Set detail level to FULL
        connect->detailLevel_ = catena::Device_DetailLevel_FULL;
    }

    void TearDown() override {
        connect.reset();
    }

    // Common test data
    std::string monitorToken = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));
    std::string operatorToken = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate));
    std::string testOid = "/test/param";
    MockDevice dm0_;
    MockDevice dm1_;
    SlotMap dms_ = {{0, &dm0_}, {1, &dm1_}};
    MockSubscriptionManager subscriptionManager;
    std::unique_ptr<TestConnect> connect;

    // Helper method to setup common expectations
    void setupCommonExpectations(MockParam& param, const MockParamDescriptor& descriptor) {
        
        // Setup default behavior for getScope to avoid uninteresting mock warnings
        static const std::string scope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
        EXPECT_CALL(param, getScope())
            .WillRepeatedly(::testing::Invoke([]() {
                return std::ref(scope);
            }));

        // Setup detail level expectation
        ON_CALL(dm0_, detail_level())
            .WillByDefault(::testing::Invoke([]() {
                return catena::Device_DetailLevel_UNSET;
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
        EXPECT_CALL(subscriptionManager, isSubscribed(::testing::_, ::testing::Ref(dm0_)))
            .WillRepeatedly(::testing::Invoke([this](const std::string& oid, const IDevice&) {
                return oid == testOid;
            }));
    }

    // Helper method to setup mock param
    void setupMockParam(MockParam& param, const std::string& oid, const MockParamDescriptor& descriptor) {
        // Setup mock param expectations
        EXPECT_CALL(param, getOid())
            .WillRepeatedly(::testing::Invoke([&oid]() {
                return std::ref(oid);
            }));
        EXPECT_CALL(param, getDescriptor())
            .WillRepeatedly(::testing::Invoke([&descriptor]() {
                return std::ref(descriptor);
            }));
        EXPECT_CALL(param, isArrayType())
            .WillRepeatedly(::testing::Invoke([]() {
                return false;
            }));

        // Setup common expectations
        setupCommonExpectations(param, descriptor);
    }

    // Helper method to setup language pack test data
    std::unique_ptr<ILanguagePack> setupLanguagePack() {
        auto languagePack = std::make_unique<MockLanguagePack>();
        EXPECT_CALL(*languagePack, toProto(::testing::_))
            .WillRepeatedly(::testing::Invoke([](catena::LanguagePack& pack) {
                pack.set_name("English");
                (*pack.mutable_words())["greeting"] = "Hello";
            }));
        return languagePack;
    }
};

/*
 * ============================================================================
 *                               Connect Tests
 * ============================================================================
 */
// == 1. Authorization Tests ==

// Test 1.1: EXPECT FALSE - Parameter updateResponse readAuthz check fails
TEST_F(CommonConnectTest, updateResponseReadAuthzFails) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->initAuthz_(operatorToken, true);  // Using operator token which won't have the right scope
    
    // Setup param to require monitor scope
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    EXPECT_CALL(param, getScope())
        .WillRepeatedly(::testing::Invoke([]() {
            return std::ref(monitorScope);
        }));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since readAuthz will fail
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 1.2: EXPECT TRUE - Parameter updateResponse authorization check when disabled
TEST_F(CommonConnectTest, updateResponseAuthzOff) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);

    // Test authorization disabled - should allow update
    connect->initAuthz_("", false);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.3: EXPECT FALSE - Parameter updateResponse authorization check when enabled but fails
TEST_F(CommonConnectTest, updateResponseAuthzOnFails) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->initAuthz_(monitorToken, true);

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("Auth failed", catena::StatusCode::PERMISSION_DENIED);
        }));
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 1.4: EXPECT TRUE - Parameter updateResponse authorization check when enabled and succeeds
TEST_F(CommonConnectTest, updateResponseAuthzOnSucceeds) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->initAuthz_(monitorToken, true);

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.5: EXPECT TRUE - LanguagePack updateResponse authorization check when disabled
TEST_F(CommonConnectTest, updateResponseLanguagePackAuthzOff) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization disabled - should allow update
    connect->initAuthz_("", false);
    
    connect->updateResponse_(languagePack.get(), 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.6: EXPECT FALSE - LanguagePack updateResponse authorization check when enabled but fails
TEST_F(CommonConnectTest, updateResponseLanguagePackAuthzOnFails) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization enabled but fails - no monitor scope
    connect->initAuthz_(operatorToken, true);
    
    connect->updateResponse_(languagePack.get(), 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 1.7: EXPECT TRUE - LanguagePack updateResponse authorization check when enabled and succeeds
TEST_F(CommonConnectTest, updateResponseLanguagePackAuthzOnSucceeds) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization enabled and succeeds - with monitor scope
    connect->initAuthz_(monitorToken, true);
    
    connect->updateResponse_(languagePack.get(), 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.7: EXPECT EQ - Priority should be 0 if authz is off.
TEST_F(CommonConnectTest, PriorityAuthzOff) {
    // Test authorization disabled - priority should be 0
    connect->initAuthz_(monitorToken, false);
    EXPECT_EQ(connect->priority(), 0);
}

// Test 1.8: EXPECT EQ - Priority should not be 0 if authz is on.
TEST_F(CommonConnectTest, PriorityAuthzOn) {
    // No scopes
    connect->initAuthz_(getJwsToken(""), true);
    EXPECT_EQ(connect->priority(), 0);
    // Read/write scopes with and without force connection
    for (uint32_t i = 1; i <= static_cast<uint32_t>(Scopes_e::kAdmin); i += 1) {
        // Testing with force_connection = false
        connect->forceConnection(false);
        // Read
        connect->initAuthz_(getJwsToken(Scopes().getForwardMap().at(static_cast<Scopes_e>(i))), true);
        EXPECT_EQ(connect->priority(), 2 * i);
        // Write
        connect->initAuthz_(getJwsToken(Scopes().getForwardMap().at(static_cast<Scopes_e>(i)) + ":w"), true);
        EXPECT_EQ(connect->priority(), 2 * i + 1);

        // Testing with force_connection = true
        connect->forceConnection(true);
        // Read
        EXPECT_THROW(connect->initAuthz_(getJwsToken(Scopes().getForwardMap().at(static_cast<Scopes_e>(i))), true), catena::exception_with_status);
        // Write
        if (static_cast<Scopes_e>(i) != Scopes_e::kAdmin) {
            EXPECT_THROW(connect->initAuthz_(getJwsToken(Scopes().getForwardMap().at(static_cast<Scopes_e>(i)) + ":w"), true), catena::exception_with_status);
        } else {
            connect->initAuthz_(getJwsToken(Scopes().getForwardMap().at(static_cast<Scopes_e>(i)) + ":w"), true);
            EXPECT_EQ(connect->priority(), 2 * i + 2);  // Admin with force connection gets highest priority
        }
    }
}

// Test 1.9: Testing connection comparison based on priority and age
TEST_F(CommonConnectTest, Compare) {
    // Creating a second connect obj for comparison.
    connect->objectId(0);
    TestConnect otherConnect(dms_, subscriptionManager);
    otherConnect.objectId(1);  // Set a different object ID for comparison
    // Higher priority connection should be greater than lower priority connection
    connect->initAuthz_(getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor)), true);
    otherConnect.initAuthz_(getJwsToken(Scopes().getForwardMap().at(Scopes_e::kOperate)), true);
    EXPECT_TRUE(*connect < otherConnect) << "Connect with Monitor scope should be less than Connection with Operate scope";
    // If scopes are the same, older connection should be larger than newer connection
    otherConnect.initAuthz_(getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor)), true);
    EXPECT_FALSE(*connect < otherConnect) << "Older connection should have higher priority than newer connection";
}

// == 2. Cancellation Tests ==

// Test 2.1: EXPECT TRUE - Parameter updateResponse cancelled 
TEST_F(CommonConnectTest, updateResponseCancelled) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);

    // Set shutdown to true
    connect->shutdown();

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since we cancelled
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());  // Should be true even though toProto wasn't called
}

// Test 2.2: EXPECT TRUE - LanguagePack updateResponse cancelled
TEST_F(CommonConnectTest, updateResponseLanguagePackCancelled) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Set shutdown to true
    connect->shutdown();
    
    connect->updateResponse_(languagePack.get(), 0);
    EXPECT_TRUE(connect->hasUpdate());  // Should be true even though we didn't set language pack data
}

// == 3. Detail Level Tests ==

// Test 3.1: EXPECT TRUE - Test updateResponse_ on FULL detail level
TEST_F(CommonConnectTest, updateResponseLODFull) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_FULL;
    connect->initAuthz_(monitorToken, true);

    // Setup param toProto to succeed exactly twice
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(3)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    // Test FULL detail level lambda - should always update regardless of other conditions
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that FULL updates even with non-minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that FULL updates even when not subscribed
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm0_)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.2: EXPECT TRUE - Test updateResponse_ on MINIMAL detail level with minimal set
TEST_F(CommonConnectTest, updateResponseLODMinimalwMinimalSet) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_MINIMAL;
    connect->initAuthz_(monitorToken, true);

    // Test MINIMAL detail level lambda - should update when in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that MINIMAL updates even when not subscribed
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm0_)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.3: EXPECT FALSE - Test updateResponse_ on MINIMAL detail level without minimal set
TEST_F(CommonConnectTest, updateResponseLODMinimalNoMinimalSet) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_MINIMAL;
    connect->initAuthz_(monitorToken, true);

    // Test MINIMAL detail level lambda - should not update when not in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since not in minimal set
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that MINIMAL doesn't update even when subscribed
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.4: EXPECT TRUE - Test updateResponse_ on SUBSCRIPTIONS detail level with subscribed OID
TEST_F(CommonConnectTest, updateResponseLODSubscriptionsSubscribedOid) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_SUBSCRIPTIONS;
    connect->initAuthz_(monitorToken, true);

    // Test SUBSCRIPTIONS detail level lambda - should update when subscribed
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that SUBSCRIPTIONS updates when in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm0_)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.5: EXPECT FALSE - Test updateResponse_ on SUBSCRIPTIONS detail level with unsubscribed OID
TEST_F(CommonConnectTest, updateResponseLODSubscriptionsUnsubscribedOid) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_SUBSCRIPTIONS;
    connect->initAuthz_(monitorToken, true);

    // Test SUBSCRIPTIONS detail level lambda - should not update when not subscribed and not in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, isSubscribed(::testing::_, ::testing::Ref(dm0_)))
        .WillRepeatedly(::testing::Return(false));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since not subscribed and not in minimal set
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.6: EXPECT TRUE - Test updateResponse_ on COMMANDS detail level with command parameter
TEST_F(CommonConnectTest, updateResponseLODCommandsCommandParam) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_COMMANDS;
    connect->initAuthz_(monitorToken, true);

    // Test COMMANDS detail level lambda - should update when isCommand is true
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(true));

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, const IAuthorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that COMMANDS updates regardless of minimal set or subscription status
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm0_)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.7: EXPECT FALSE - Test updateResponse_ on COMMANDS detail level with non-command parameter
TEST_F(CommonConnectTest, updateResponseLODCommandsNonCommandParam) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_COMMANDS;
    connect->initAuthz_(monitorToken, true);

    // Test COMMANDS detail level lambda - should not update when isCommand is false
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(false));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since not a command
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that COMMANDS doesn't update even when in minimal set or subscribed
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));;
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.8: EXPECT FALSE - Test updateResponse_ on NONE detail level
TEST_F(CommonConnectTest, updateResponseLODNone) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_NONE;
    connect->initAuthz_(monitorToken, true);

    // Test NONE detail level lambda - should never update
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since detail level is NONE
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that NONE doesn't update even with all conditions met
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(true));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.9: EXPECT FALSE - Test updateResponse_ on UNSET detail level
TEST_F(CommonConnectTest, updateResponseLODUnset) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_UNSET;

    // Initialize authorization with monitor token
    connect->initAuthz_(monitorToken, true);

    // Test UNSET detail level lambda - should never update
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
        .Times(0);  // Should not be called since detail level is UNSET
    
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that UNSET doesn't update even with all conditions met
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(true));
    connect->updateResponse_(testOid, &param, 0);
    EXPECT_FALSE(connect->hasUpdate());
}

// // == 4. Exception Handling Tests ==
// these are bad tests, and need to me remade will add a ticket to fix them
// // Test 4.1: EXPECT FALSE - If toProto throws, no update is pushed to the client
// TEST_F(CommonConnectTest, updateResponseExceptionParamToProto) {
//     MockParam param;
//     MockParamDescriptor descriptor;
//     setupCommonExpectations(param, descriptor);
//     setupMockParam(param, testOid, descriptor);
//     connect->detailLevel_ = catena::Device_DetailLevel_FULL;
//     connect->initAuthz_(monitorToken, true);

//     // Make toProto throw an exception
//     EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<const IAuthorizer&>()))
//         .WillOnce(::testing::Invoke([](catena::Value&, const IAuthorizer&) -> catena::exception_with_status {
//             throw catena::exception_with_status("Test exception", catena::StatusCode::INTERNAL);
//         }));

//     connect->updateResponse_(testOid, &param, 0);
//     EXPECT_FALSE(connect->hasUpdate());
// }

// // Test 4.2: EXPECT FALSE - If any exception is thrown inside updateResponse_ (language pack), no update is pushed to the client
// TEST_F(CommonConnectTest, updateResponseLanguagePacktoProto) {
//     // Create a language pack that will throw when accessed
//     auto languagePack = std::make_unique<MockLanguagePack>();
    
//     // Make the language pack throw when its data is accessed
//     EXPECT_CALL(*languagePack, toProto(::testing::_))
//         .WillOnce(::testing::Invoke([](catena::LanguagePack&) {
//             throw catena::exception_with_status("Test exception", catena::StatusCode::INTERNAL);
//         }));
    
//     // Create a test connect instance
//     connect->initAuthz_(monitorToken, true);
    
//     connect->updateResponse_(languagePack.get(), 0);
//     EXPECT_FALSE(connect->hasUpdate());
// }

