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
    }

    void TearDown() override {
        connect.reset();
    }

    MockDevice dm;
    MockSubscriptionManager subscriptionManager;
    std::unique_ptr<TestConnect> connect;
};

/*
 * ============================================================================
 *                               Connect Tests
 * ============================================================================
 */
 
// TEST 1 - Testing updateResponse_ with parameter update and FULL detail level.
TEST_F(ConnectTests, updateResponseWithParamFullDetail) {
    // Setup test data
    std::string oid = "/test/param";
    size_t idx = 0;
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup expectations
    EXPECT_CALL(dm, detail_level())
        .WillOnce(::testing::Return(catena::Device_DetailLevel_FULL));
    EXPECT_CALL(param, getDescriptor())
        .WillOnce(::testing::ReturnRef(descriptor));
    static std::set<std::string> empty_set;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
        .WillOnce(::testing::ReturnRef(empty_set));
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Return(catena::exception_with_status("", catena::StatusCode::OK)));

    // Call updateResponse_
    connect->updateResponse_(oid, idx, &param);

    // Verify update was triggered
    EXPECT_TRUE(connect->hasUpdate());
    EXPECT_EQ(connect->getResponse().value().oid(), oid);
    EXPECT_EQ(connect->getResponse().value().element_index(), idx);
}
// TEST 2 - Testing updateResponse_ with MINIMAL detail level.
TEST_F(ConnectTests, updateResponseWithParamMinimalDetail) {
    // Setup test data
    std::string oid = "/test/param";
    size_t idx = 0;
    MockParam param;
    MockParamDescriptor descriptor;
    
    // Setup expectations
    EXPECT_CALL(dm, detail_level())
        .WillOnce(::testing::Return(catena::Device_DetailLevel_MINIMAL));
    EXPECT_CALL(param, getDescriptor())
        .WillOnce(::testing::ReturnRef(descriptor));
    EXPECT_CALL(descriptor, minimalSet())
        .WillOnce(::testing::Return(true));
    static std::set<std::string> empty_set2;
    EXPECT_CALL(subscriptionManager, getAllSubscribedOids(::testing::Ref(dm)))
        .WillOnce(::testing::ReturnRef(empty_set2));
    EXPECT_CALL(param, toProto(::testing::An<catena::Value&>(), ::testing::An<catena::common::Authorizer&>()))
        .WillOnce(::testing::Return(catena::exception_with_status("", catena::StatusCode::OK)));

    // Call updateResponse_
    connect->updateResponse_(oid, idx, &param);

    // Verify update was triggered
    EXPECT_TRUE(connect->hasUpdate());
}

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