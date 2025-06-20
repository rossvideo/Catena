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
class CommonConnectTests : public ::testing::Test {
protected:
    void SetUp() override {
        connect = std::make_unique<TestConnect>(dm, subscriptionManager);
        
        // Common test data
        // Using monitor token from Authorization_test.cpp 
        monitorToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxstpXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";
        // Using operator token from Authorization_test.cpp (no monitor permissions)
        operatorToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6b3AiLCJpYXQiOjE1MTYyMzkwMjJ9.lduNvr6tEaLFeIYR4bH5tC55WUSDBEe5PFz9rvGRD3o";
        testOid = "/test/param";

        // Set detail level to FULL
        connect->detailLevel_ = catena::Device_DetailLevel_FULL;
    }

    void TearDown() override {
        connect.reset();
    }

    // Common test data
    std::string monitorToken;
    std::string operatorToken;
    std::string testOid;
    MockDevice dm;
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
        ON_CALL(dm, detail_level())
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
        EXPECT_CALL(subscriptionManager, isSubscribed(::testing::_, ::testing::Ref(dm)))
            .WillRepeatedly(::testing::Invoke([this](const std::string& oid, const IDevice&) {
                return oid == testOid;
            }));
    }

    // Helper method to setup mock param
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

        // Setup common expectations
        setupCommonExpectations(*param, descriptor);
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
TEST_F(CommonConnectTests, updateResponseReadAuthzFails) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->initAuthz_(operatorToken, true);  // Using operator token which won't have the right scope
    
    // Setup param to require monitor scope
    static const std::string monitorScope = Scopes().getForwardMap().at(Scopes_e::kMonitor);
    EXPECT_CALL(param, getScope())
        .WillRepeatedly(::testing::Invoke([]() {
            return std::ref(monitorScope);
        }));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since readAuthz will fail
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 1.2: EXPECT TRUE - Parameter updateResponse authorization check when disabled
TEST_F(CommonConnectTests, updateResponseAuthzOff) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Test authorization disabled - should allow update
    connect->initAuthz_("", false);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.3: EXPECT FALSE - Parameter updateResponse authorization check when enabled but fails
TEST_F(CommonConnectTests, updateResponseAuthzOnFails) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->initAuthz_(monitorToken, true);

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("Auth failed", catena::StatusCode::PERMISSION_DENIED);
        }));
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 1.4: EXPECT TRUE - Parameter updateResponse authorization check when enabled and succeeds
TEST_F(CommonConnectTests, updateResponseAuthzOnSucceeds) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->initAuthz_(monitorToken, true);

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.5: EXPECT TRUE - LanguagePack updateResponse authorization check when disabled
TEST_F(CommonConnectTests, updateResponseLanguagePackAuthzOff) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization disabled - should allow update
    connect->initAuthz_("", false);
    
    connect->updateResponse_(languagePack.get());
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 1.6: EXPECT FALSE - LanguagePack updateResponse authorization check when enabled but fails
TEST_F(CommonConnectTests, updateResponseLanguagePackAuthzOnFails) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization enabled but fails - no monitor scope
    connect->initAuthz_(operatorToken, true);
    
    connect->updateResponse_(languagePack.get());
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 1.7: EXPECT TRUE - LanguagePack updateResponse authorization check when enabled and succeeds
TEST_F(CommonConnectTests, updateResponseLanguagePackAuthzOnSucceeds) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization enabled and succeeds - with monitor scope
    connect->initAuthz_(monitorToken, true);
    
    connect->updateResponse_(languagePack.get());
    EXPECT_TRUE(connect->hasUpdate());
}


// == 2. Cancellation Tests ==

// Test 2.1: EXPECT TRUE - Parameter updateResponse cancelled 
TEST_F(CommonConnectTests, updateResponseCancelled) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Set cancelled to true
    connect->setCancelled(true);

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since we cancelled
    
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());  // Should be true even though toProto wasn't called
}

// Test 2.2: EXPECT TRUE - LanguagePack updateResponse cancelled
TEST_F(CommonConnectTests, updateResponseLanguagePackCancelled) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Set cancelled to true
    connect->setCancelled(true);
    
    connect->updateResponse_(languagePack.get());
    EXPECT_TRUE(connect->hasUpdate());  // Should be true even though we didn't set language pack data
}

// == 3. Detail Level Tests ==

// Test 3.1: EXPECT TRUE - Test updateResponse_ on FULL detail level
TEST_F(CommonConnectTests, updateResponseLODFull) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_FULL;
    connect->initAuthz_(monitorToken, true);

    // Setup param toProto to succeed exactly twice
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(3)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    // Test FULL detail level lambda - should always update regardless of other conditions
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that FULL updates even with non-minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that FULL updates even when not subscribed
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.2: EXPECT TRUE - Test updateResponse_ on MINIMAL detail level with minimal set
TEST_F(CommonConnectTests, updateResponseLODMinimalwMinimalSet) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_MINIMAL;
    connect->initAuthz_(monitorToken, true);

    // Test MINIMAL detail level lambda - should update when in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that MINIMAL updates even when not subscribed
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.3: EXPECT FALSE - Test updateResponse_ on MINIMAL detail level without minimal set
TEST_F(CommonConnectTests, updateResponseLODMinimalNoMinimalSet) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_MINIMAL;
    connect->initAuthz_(monitorToken, true);

    // Test MINIMAL detail level lambda - should not update when not in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since not in minimal set
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that MINIMAL doesn't update even when subscribed
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.4: EXPECT TRUE - Test updateResponse_ on SUBSCRIPTIONS detail level with subscribed OID
TEST_F(CommonConnectTests, updateResponseLODSubscriptionsSubscribedOid) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_SUBSCRIPTIONS;
    connect->initAuthz_(monitorToken, true);

    // Test SUBSCRIPTIONS detail level lambda - should update when subscribed
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that SUBSCRIPTIONS updates when in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.5: EXPECT FALSE - Test updateResponse_ on SUBSCRIPTIONS detail level with unsubscribed OID
TEST_F(CommonConnectTests, updateResponseLODSubscriptionsUnsubscribedOid) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_SUBSCRIPTIONS;
    connect->initAuthz_(monitorToken, true);

    // Test SUBSCRIPTIONS detail level lambda - should not update when not subscribed and not in minimal set
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, isSubscribed(::testing::_, ::testing::Ref(dm)))
        .WillRepeatedly(::testing::Return(false));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since not subscribed and not in minimal set
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.6: EXPECT TRUE - Test updateResponse_ on COMMANDS detail level with command parameter
TEST_F(CommonConnectTests, updateResponseLODCommandsCommandParam) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_COMMANDS;
    connect->initAuthz_(monitorToken, true);

    // Test COMMANDS detail level lambda - should update when isCommand is true
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(true));

    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(2)
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());

    // Verify that COMMANDS updates regardless of minimal set or subscription status
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(false));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
        .WillRepeatedly(::testing::Return(empty_set));
    connect->updateResponse_(testOid, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 3.7: EXPECT FALSE - Test updateResponse_ on COMMANDS detail level with non-command parameter
TEST_F(CommonConnectTests, updateResponseLODCommandsNonCommandParam) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_COMMANDS;
    connect->initAuthz_(monitorToken, true);

    // Test COMMANDS detail level lambda - should not update when isCommand is false
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(false));

    // Setup param toProto to succeed (but it shouldn't be called)
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since not a command
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that COMMANDS doesn't update even when in minimal set or subscribed
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));;
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.8: EXPECT FALSE - Test updateResponse_ on NONE detail level
TEST_F(CommonConnectTests, updateResponseLODNone) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_NONE;
    connect->initAuthz_(monitorToken, true);

    // Test NONE detail level lambda - should never update
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since detail level is NONE
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that NONE doesn't update even with all conditions met
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(true));
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 3.9: EXPECT FALSE - Test updateResponse_ on UNSET detail level
TEST_F(CommonConnectTests, updateResponseLODUnset) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_UNSET;

    // Initialize authorization with monitor token
    connect->initAuthz_(monitorToken, true);

    // Test UNSET detail level lambda - should never update
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .Times(0);  // Should not be called since detail level is UNSET
    
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());

    // Verify that UNSET doesn't update even with all conditions met
    EXPECT_CALL(descriptor, minimalSet())
        .WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(descriptor, isCommand())
        .WillRepeatedly(::testing::Return(true));
    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// == 4. Exception Handling Tests ==

// Test 4.1: EXPECT FALSE - If toProto throws, no update is pushed to the client
TEST_F(CommonConnectTests, updateResponseExceptionParamToProto) {
    MockParam param;
    MockParamDescriptor descriptor;
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);
    connect->detailLevel_ = catena::Device_DetailLevel_FULL;
    connect->initAuthz_(monitorToken, true);

    // Make toProto throw an exception
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Invoke([](catena::Value&, catena::common::Authorizer&) -> catena::exception_with_status {
            throw catena::exception_with_status("Test exception", catena::StatusCode::INTERNAL);
        }));

    connect->updateResponse_(testOid, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 4.2: EXPECT FALSE - If any exception is thrown inside updateResponse_ (language pack), no update is pushed to the client
TEST_F(CommonConnectTests, updateResponseLanguagePacktoProto) {
    // Create a language pack that will throw when accessed
    auto languagePack = std::make_unique<MockLanguagePack>();
    
    // Make the language pack throw when its data is accessed
    EXPECT_CALL(*languagePack, toProto(::testing::_))
        .WillOnce(::testing::Invoke([](catena::LanguagePack&) {
            throw catena::exception_with_status("Test exception", catena::StatusCode::INTERNAL);
        }));
    
    // Create a test connect instance
    auto testConnect = std::make_unique<TestConnect>(dm, subscriptionManager);
    testConnect->initAuthz_(monitorToken, true);
    
    testConnect->updateResponse_(languagePack.get());
    EXPECT_FALSE(testConnect->hasUpdate());
}

