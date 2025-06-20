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

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>
#include <vector>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// Test helpers
#include "RESTTest.h"
#include "MockSocketReader.h"
#include "MockDevice.h"
#include "MockSubscriptionManager.h"
#include "CommonTestHelpers.h"

// REST
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::REST;
using namespace testing;

// Fixture
class RESTDeviceRequestTests : public ::testing::Test, public RESTTest {
protected:
    RESTDeviceRequestTests() : RESTTest(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Set up common expectations
        setupCommonExpectations();
        
        // Create DeviceRequest instance
        device_request_ = catena::REST::DeviceRequest::makeOne(serverSocket, socket_reader_, device_);
    }

    void TearDown() override {
        if (device_request_) {
            delete device_request_;
        }
    }

    // Helper function to set up common mock expectations
    void setupCommonExpectations() {
        EXPECT_CALL(socket_reader_, detailLevel())
            .WillRepeatedly(Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));
        EXPECT_CALL(socket_reader_, authorizationEnabled())
            .WillRepeatedly(Return(false));
        EXPECT_CALL(socket_reader_, stream())
            .WillRepeatedly(Return(false));
        EXPECT_CALL(socket_reader_, origin())
            .WillRepeatedly(ReturnRef(origin));
        EXPECT_CALL(device_, slot())
            .WillRepeatedly(Return(1));
        EXPECT_CALL(device_, mutex())
            .WillRepeatedly(ReturnRef(mutex_));
    }

    MockSocketReader socket_reader_;
    MockDevice device_;
    catena::REST::ICallData* device_request_ = nullptr;
    MockSubscriptionManager subscription_manager_;
    std::string origin = "*";
    std::mutex mutex_;
};

// --- 0. INITIAL TESTS ---

// Test 0.1: Test constructor initialization
TEST_F(RESTDeviceRequestTests, DeviceRequest_create) {
    // Verify that the DeviceRequest object was created successfully in SetUp()
    ASSERT_NE(device_request_, nullptr);
}

// Test 0.2: Test writer type selection based on streaming
TEST_F(RESTDeviceRequestTests, DeviceRequest_writer_type_selection) {
    // Test unary (non-streaming) writer
    {
        // Set up expectations for unary mode
        EXPECT_CALL(socket_reader_, stream())
            .WillOnce(Return(false));
        EXPECT_CALL(socket_reader_, origin())
            .WillOnce(ReturnRef(origin));
        
        // Create a new DeviceRequest for unary mode
        auto* unary_request = new DeviceRequest(serverSocket, socket_reader_, device_);
        EXPECT_NE(unary_request, nullptr);
        delete unary_request;
    }
    
    // Test streaming writer
    {
        // Set up expectations for streaming mode
        EXPECT_CALL(socket_reader_, stream())
            .WillOnce(Return(true));
        EXPECT_CALL(socket_reader_, origin())
            .WillOnce(ReturnRef(origin));
        
        // Create a new DeviceRequest for streaming mode
        auto* stream_request = new DeviceRequest(serverSocket, socket_reader_, device_);
        EXPECT_NE(stream_request, nullptr);
        delete stream_request;
    }
}

// Test 0.3: Test unauthorized request
TEST_F(RESTDeviceRequestTests, DeviceRequest_authz_invalid_token) {
    std::string mockToken = "invalid_token";
    catena::exception_with_status rc("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);

    EXPECT_CALL(socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

// Test 0.4: Test authorized request with valid token
TEST_F(RESTDeviceRequestTests, DeviceRequest_authz_valid_token) {
    std::string mockToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIi"
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
    catena::exception_with_status rc("", catena::StatusCode::OK);

    EXPECT_CALL(socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(socket_reader_, jwsToken())
        .WillOnce(ReturnRef(mockToken));
    
    // Set up expectation for getComponentSerializer to return a working serializer
    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            auto mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(Return(false));
            return mockSerializer;
        }));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

// Test 0.5: Test authorization with std::exception
TEST_F(RESTDeviceRequestTests, DeviceRequest_authz_std_exception) {
    catena::exception_with_status rc("Device request failed: Test auth setup failure", catena::StatusCode::INTERNAL);

    EXPECT_CALL(socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(socket_reader_, jwsToken())
        .WillOnce(Throw(std::runtime_error("Test auth setup failure")));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

// Test 0.6: Test subscribed OIDs handling
TEST_F(RESTDeviceRequestTests, DeviceRequest_subscribed_oids) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::set<std::string> expectedSubscribedOids = {"param1", "param2", "param3"};

    // Set up expectations for subscription mode
    EXPECT_CALL(socket_reader_, detailLevel())
        .WillOnce(Return(catena::Device_DetailLevel_SUBSCRIPTIONS));
    EXPECT_CALL(socket_reader_, getSubscriptionManager())
        .WillOnce(ReturnRef(subscription_manager_));
    EXPECT_CALL(subscription_manager_, getAllSubscribedOids(::testing::Ref(device_)))
        .WillOnce(Return(expectedSubscribedOids));
    
    // Set up expectation for getComponentSerializer to verify subscribed OIDs are passed
    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&expectedSubscribedOids](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            EXPECT_EQ(subscribedOids, expectedSubscribedOids);
            EXPECT_EQ(dl, catena::Device_DetailLevel_SUBSCRIPTIONS);
            auto mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(Return(false));
            return mockSerializer;
        }));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}
