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
 * @brief This file is for testing the DeviceRequest.cpp file.
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-06-20
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"
#include "MockDevice.h"
#include "MockDeviceSerializer.h"
#include "MockSubscriptionManager.h"

// REST
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTDeviceRequestTests : public RESTEndpointTest {
  protected:
    /*
     * Sets default expectations for detailLevel().
     */
    RESTDeviceRequestTests() : RESTEndpointTest() {
        EXPECT_CALL(context_, detailLevel()).WillRepeatedly(testing::Return(catena::Device_DetailLevel_FULL));
    }

    /*
     * Creates a DeviceRequest handler object.
     */
    ICallData* makeOne() override { return DeviceRequest::makeOne(serverSocket_, context_, dm0_); }

    /*
     * Helper function which populates expNum with up to 6 items.
     */
    void initExpVal(uint32_t expNum = 0) {
        if (expNum > 6 ) { expNum = 6; }
        switch (expNum) {
            case 6:
                expVals_.emplace(expVals_.begin(), catena::DeviceComponent());
                expVals_.begin()->mutable_command()->set_oid("command_test");
            case 5:
                expVals_.emplace(expVals_.begin(), catena::DeviceComponent());
                expVals_.begin()->mutable_param()->set_oid("param_test");
            case 4:
                expVals_.emplace(expVals_.begin(), catena::DeviceComponent());
                expVals_.begin()->mutable_shared_constraint()->set_oid("constraint_test");
            case 3:
                expVals_.emplace(expVals_.begin(), catena::DeviceComponent());
                expVals_.begin()->mutable_language_pack()->set_language("language_test");
            case 2:
                expVals_.emplace(expVals_.begin(), catena::DeviceComponent());
                expVals_.begin()->mutable_menu()->set_oid("menu_test");
            case 1:
                expVals_.emplace(expVals_.begin(), catena::DeviceComponent());
                expVals_.begin()->mutable_device()->set_slot(slot_);
        }
    }
    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        std::vector<std::string> jsonBodies;
        for (const auto& expVal : expVals_) {
            jsonBodies.push_back("");
            auto status = google::protobuf::util::MessageToJsonString(expVal, &jsonBodies.back());
            ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
        }
        if (stream_) {
            EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, jsonBodies));
        } else {
            EXPECT_EQ(readResponse(), expectedResponse(expRc_, jsonBodies));
        }
    }

    // Expected variables
    std::vector<catena::DeviceComponent> expVals_;
};

// --- 0. INITIAL TESTS ---
// Test 0.1: Test constructor initialization
TEST_F(RESTDeviceRequestTests, DeviceRequest_Create) {
    ASSERT_TRUE(endpoint_);
}

// Test 0.2: Test finish method functionality
TEST_F(RESTDeviceRequestTests, DeviceRequest_Finish) {
    // Test that finish method does not throw
    EXPECT_NO_THROW(endpoint_->finish());
    ASSERT_TRUE(MockConsole_.str().find("DeviceRequest[1] finished\n") != std::string::npos);
}

// --- 1. PROCEED TESTS ---
// Test 1.1: Test proceed unary response with multiple components
TEST_F(RESTDeviceRequestTests, DeviceRequest_Normal) {
    initExpVal(3);
    // Set up expectation for getComponentSerializer to return a working serializer
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([this](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(dl, catena::Device_DetailLevel_FULL);
            EXPECT_TRUE(subscribedOids.empty());
            auto mockSerializer = std::make_unique<MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(testing::Return(true))
                .WillOnce(testing::Return(true))
                .WillOnce(testing::Return(true))
                .WillOnce(testing::Return(false));
            EXPECT_CALL(*mockSerializer, getNext())
                .WillOnce(testing::Return(expVals_[0]))
                .WillOnce(testing::Return(expVals_[1]))
                .WillOnce(testing::Return(expVals_[2]));
            return mockSerializer;
        }));
    // Calling proceed and testing the output
    testCall();
}

// Test 1.2: Test proceed stream response with multiple components
TEST_F(RESTDeviceRequestTests, DeviceRequest_Stream) {
    // Remaking with stream enabled.
    stream_ = true;
    endpoint_.reset(makeOne());
    // Initializing expected values
    initExpVal(3);
    // Set up expectation for getComponentSerializer to return a working serializer
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([this](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(dl, catena::Device_DetailLevel_FULL);
            EXPECT_TRUE(subscribedOids.empty());
            auto mockSerializer = std::make_unique<MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore()).Times(4)
                .WillOnce(testing::Return(true))
                .WillOnce(testing::Return(true))
                .WillOnce(testing::Return(true))
                .WillOnce(testing::Return(false));
            EXPECT_CALL(*mockSerializer, getNext()).Times(3)
                .WillOnce(testing::Return(expVals_[0]))
                .WillOnce(testing::Return(expVals_[1]))
                .WillOnce(testing::Return(expVals_[2]));
            return mockSerializer;
        }));
    // Calling proceed and testing the output
    testCall();
}

// Test 1.3: Test proceed with authz enabled and a valid token.
TEST_F(RESTDeviceRequestTests, DeviceRequest_AuthzValid) {
    jwsToken_ = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    authzEnabled_ = true;
    // Set up expectation for getComponentSerializer to return a working serializer
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(dl, catena::Device_DetailLevel_FULL);
            EXPECT_TRUE(subscribedOids.empty());
            auto mockSerializer = std::make_unique<MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore()).WillOnce(testing::Return(false));
            EXPECT_CALL(*mockSerializer, getNext()).Times(0);
            return mockSerializer;
        }));
    // Calling proceed and testing the output
    testCall();
}

// Test 1.4: Test proceed with Subscriptions
TEST_F(RESTDeviceRequestTests, DeviceRequest_Subscriptions) {
    std::set<std::string> expectedSubscribedOids = {"param1", "param2", "param3"};
    MockSubscriptionManager mockSubManager;
    // Set up expectations for subscription mode
    EXPECT_CALL(context_, detailLevel()).WillOnce(testing::Return(catena::Device_DetailLevel_SUBSCRIPTIONS));
    EXPECT_CALL(context_, getSubscriptionManager()).Times(1)
        .WillOnce(testing::ReturnRef(mockSubManager));
    EXPECT_CALL(mockSubManager, getAllSubscribedOids(::testing::Ref(dm0_))).Times(1)
        .WillOnce(testing::Return(expectedSubscribedOids));
    // Set up expectation for getComponentSerializer to verify subscribed OIDs are passed
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&expectedSubscribedOids](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            EXPECT_EQ(subscribedOids, expectedSubscribedOids);
            EXPECT_EQ(dl, catena::Device_DetailLevel_SUBSCRIPTIONS);
            auto mockSerializer = std::make_unique<MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore()).WillOnce(testing::Return(false));
            EXPECT_CALL(*mockSerializer, getNext()).Times(0);
            return mockSerializer;
        }));
    // Calling proceed and testing the output
    testCall();
}

// --- 3. EXCEPTION TESTS ---
// Test 3.1: Test proceed with authz enabled and an invalid token.
TEST_F(RESTDeviceRequestTests, DeviceRequest_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    authzEnabled_ = true;
    jwsToken_ = "invalid_token";
    // Setting expectations.
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

// Test 3.2: Testing dm.getComponentSerializer() returning a nullptr.
TEST_F(RESTDeviceRequestTests, DeviceRequest_ErrGetSerializerIllegalState) {
    expRc_ = catena::exception_with_status("Illegal state", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1).WillOnce(::testing::Return(nullptr));
    // Calling proceed and testing the output
    testCall();
}

// Test 3.3: Test catch(std::exception) handling
TEST_F(RESTDeviceRequestTests, DeviceRequest_GetSerializerThrowStd) {
    expRc_ = catena::exception_with_status("Device request failed: std error", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1).WillOnce(testing::Throw(std::runtime_error("std error")));
    // Calling proceed and testing the output
    testCall();
}

// Test 3.4: Test catch (...) exception handling
TEST_F(RESTDeviceRequestTests, DeviceRequest_GetSerializerThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(1).WillOnce(testing::Throw(42));
    // Calling proceed and testing the output
    testCall();
}

