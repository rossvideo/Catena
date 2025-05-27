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
        // Using monitor token from Authorization_test.cpp 
        monitorToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uIiwiaWF0IjoxNTE2MjM5MDIyfQ.YkqS7hCxstpXulFnR98q0m088pUj6Cnf5vW6xPX8aBQ";
        // Using operator token from Authorization_test.cpp (no monitor permissions)
        operatorToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6b3AiLCJpYXQiOjE1MTYyMzkwMjJ9.lduNvr6tEaLFeIYR4bH5tC55WUSDBEe5PFz9rvGRD3o";
        testOid = "/test/param";
        testIdx = 0;

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

    // Helper method to setup language pack test data
    IDevice::ComponentLanguagePack setupLanguagePack() {
        IDevice::ComponentLanguagePack languagePack;
        languagePack.set_language("en");
        auto* pack = languagePack.mutable_language_pack();
        pack->set_name("English");
        (*pack->mutable_words())["greeting"] = "Hello";
        (*pack->mutable_words())["parting"] = "Goodbye";
        return languagePack;
    }

    // Helper method to verify language pack response
    void verifyLanguagePackResponse(const catena::PushUpdates& response) {
        EXPECT_EQ(response.device_component().language_pack().language(), "en");
        EXPECT_EQ(response.device_component().language_pack().language_pack().name(), "English");
        EXPECT_EQ(response.device_component().language_pack().language_pack().words().at("greeting"), "Hello");
        EXPECT_EQ(response.device_component().language_pack().language_pack().words().at("parting"), "Goodbye");
    }
};

/*
 * ============================================================================
 *                               Connect Tests
 * ============================================================================
 */
 
// == Authorization Tests ==

// Test 0.1 - Parameter updateResponse authorization check when disabled
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

// Test 0.2 - Parameter updateResponse authorization check when enabled but fails
TEST_F(ConnectTests, updateResponseAuthorizationCheckEnabledFails) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup common expectations
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Test authorization enabled but fails
    connect->initAuthz_(monitorToken, true);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("Auth failed", catena::StatusCode::PERMISSION_DENIED);
        }));
    
    connect->updateResponse_(testOid, testIdx, &param);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 0.3 - Parameter updateResponse authorization check when enabled and succeeds
TEST_F(ConnectTests, updateResponseAuthorizationCheckEnabledSucceeds) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup common expectations
    setupCommonExpectations(param, descriptor);
    setupMockParam(&param, testOid, descriptor);

    // Test authorization enabled and succeeds
    connect->initAuthz_(monitorToken, true);
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillRepeatedly(::testing::Invoke([this](catena::Value& value, catena::common::Authorizer& authz) -> catena::exception_with_status {
            value.set_string_value(testOid);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    
    connect->updateResponse_(testOid, testIdx, &param);
    EXPECT_TRUE(connect->hasUpdate());
}

// Test 0.4 - LanguagePack updateResponse authorization check when disabled
TEST_F(ConnectTests, updateResponseLanguagePackAuthorizationCheckDisabled) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization disabled - should allow update
    connect->initAuthz_("", false);
    
    connect->updateResponse_(languagePack);
    EXPECT_TRUE(connect->hasUpdate());
    verifyLanguagePackResponse(connect->getResponse());
}

// Test 0.5 - LanguagePack updateResponse authorization check when enabled but fails
TEST_F(ConnectTests, updateResponseLanguagePackAuthorizationCheckEnabledFails) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization enabled but fails - no monitor scope
    connect->initAuthz_(operatorToken, true);
    
    connect->updateResponse_(languagePack);
    EXPECT_FALSE(connect->hasUpdate());
}

// Test 0.6 - LanguagePack updateResponse authorization check when enabled and succeeds
TEST_F(ConnectTests, updateResponseLanguagePackAuthorizationCheckEnabledSucceeds) {
    // Setup test data
    auto languagePack = setupLanguagePack();
    
    // Test authorization enabled and succeeds - with monitor scope
    connect->initAuthz_(monitorToken, true);
    
    connect->updateResponse_(languagePack);
    EXPECT_TRUE(connect->hasUpdate());
    verifyLanguagePackResponse(connect->getResponse());
}

// == Cancellation Tests ==

// Test 1.1 - Parameter updateResponse cancelled 
TEST_F(ConnectTests, updateResponseCancelled) {
    // Setup test data
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup common expectations
}

// Test 1.2 - LanguagePack updateResponse cancelled




// == Detail Level Tests ==

// Test 3.1 - Test updateResponse_ on FULL detail level.

// Test 3.2 - Test updateResponse_ on MINIMAL detail level.

// Test 3.3 - Test updateResponse_ on SUBSCRIPTIONS detail level.

// Test 3.4 - Test updateResponse_ on COMMANDS detail level.

// Test 3.5 - Test updateResponse_ on NONE detail level.

