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
#include "RESTTestHelpers.h"

// REST
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::REST;
using namespace testing;

/**
 * @brief Class to provide expected DeviceComponent objects for testing
 */
class ExpectedComponents {
    public:
        ExpectedComponents() {
            // Create expected values with one of everything, similar to gRPC test
            expVals.push_back(catena::DeviceComponent()); // [0] Device
            expVals.push_back(catena::DeviceComponent()); // [1] Menu
            expVals.push_back(catena::DeviceComponent()); // [2] Language pack
            expVals.push_back(catena::DeviceComponent()); // [3] Constraint
            expVals.push_back(catena::DeviceComponent()); // [4] Param
            expVals.push_back(catena::DeviceComponent()); // [5] Command
            
            expVals[0].mutable_device()->set_slot(1);
            expVals[1].mutable_menu()->set_oid("menu_test");
            expVals[2].mutable_language_pack()->set_language("language_test");
            expVals[3].mutable_shared_constraint()->set_oid("constraint_test");
            expVals[4].mutable_param()->set_oid("param_test");
            expVals[5].mutable_command()->set_oid("command_test");
        }
        
        /**
        * @brief Serializes a DeviceComponent to JSON string
        * @param component The DeviceComponent to serialize
        * @return The serialized JSON string
        */
        std::string serializeToJson(const catena::DeviceComponent& component) {
            std::string jsonString;
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = false;
            auto status = google::protobuf::util::MessageToJsonString(component, &jsonString, options);
            if (!status.ok()) {
                return "{}"; // Return empty object if serialization fails
            }
            return jsonString;
        }
        
        std::vector<catena::DeviceComponent> expVals;
};

// Fixture
class RESTDeviceRequestTests : public ::testing::Test, public RESTTest {
    protected:
        RESTDeviceRequestTests() : RESTTest(&serverSocket, &clientSocket) {}

        void SetUp() override {
            // Redirecting cout to a stringstream for testing.
            oldCout = std::cout.rdbuf(MockConsole.rdbuf());

            // Set up common expectations
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
            
            // Create DeviceRequest instance
            device_request_ = catena::REST::DeviceRequest::makeOne(serverSocket, socket_reader_, device_);
        }

        void TearDown() override {
            std::cout.rdbuf(oldCout); // Restoring cout
            
            if (device_request_) {
                delete device_request_;
            }
        }


        MockSocketReader socket_reader_;
        MockDevice device_;
        catena::REST::ICallData* device_request_ = nullptr;
        MockSubscriptionManager subscription_manager_;
        std::string origin = "*";
        std::mutex mutex_;
        ExpectedComponents expectedComponents_;
        
        // Cout variables.
        std::stringstream MockConsole;
        std::streambuf* oldCout;
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
        auto* unary_request = DeviceRequest::makeOne(serverSocket, socket_reader_, device_);
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
        auto* stream_request = DeviceRequest::makeOne(serverSocket, socket_reader_, device_);
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

// Test 0.5: Test subscribed OIDs handling
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

// --- 1. SERIALIZER TESTS ---

// Test 1.1: Test normal serializer operation with single component
TEST_F(RESTDeviceRequestTests, DeviceRequest_serializer_normal_single) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    const catena::DeviceComponent& component = expectedComponents_.expVals[0]; // Device with slot=1

    // Set up expectation for getComponentSerializer to return a working serializer
    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&component](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            auto mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(Return(true))
                .WillOnce(Return(false));
            EXPECT_CALL(*mockSerializer, getNext())
                .WillOnce(Return(component));
            return mockSerializer;
        }));
    
    device_request_->proceed();
    std::vector<std::string> components = {expectedComponents_.serializeToJson(component)};
    EXPECT_EQ(readResponse(), expectedResponse(rc, components));
}

// Test 1.2: Test serializer with multiple components
TEST_F(RESTDeviceRequestTests, DeviceRequest_serializer_multiple_components) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    const catena::DeviceComponent& component1 = expectedComponents_.expVals[0]; // Device
    const catena::DeviceComponent& component2 = expectedComponents_.expVals[1]; // Menu
    const catena::DeviceComponent& component3 = expectedComponents_.expVals[4]; // Param

    // Set up expectation for getComponentSerializer to return a working serializer
    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&component1, &component2, &component3](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            auto mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(Return(true))
                .WillOnce(Return(true))
                .WillOnce(Return(true))
                .WillOnce(Return(false));
            EXPECT_CALL(*mockSerializer, getNext())
                .WillOnce(Return(component1))
                .WillOnce(Return(component2))
                .WillOnce(Return(component3));
            return mockSerializer;
        }));
    
    device_request_->proceed();
    std::vector<std::string> components = {
        expectedComponents_.serializeToJson(component1),
        expectedComponents_.serializeToJson(component2),
        expectedComponents_.serializeToJson(component3)
    };
    EXPECT_EQ(readResponse(), expectedResponse(rc, components));
}

// Test 1.3: Test serializer with no components
TEST_F(RESTDeviceRequestTests, DeviceRequest_serializer_no_components) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Set up expectation for getComponentSerializer to return a serializer with no components
    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            auto mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(Return(false));
            return mockSerializer;
        }));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

// --- 2. FINISH TESTS ---

// Test 2.1: Test finish method functionality
TEST_F(RESTDeviceRequestTests, DeviceRequest_finish) {
    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            auto mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
            EXPECT_CALL(*mockSerializer, hasMore())
                .WillOnce(Return(false));
            return mockSerializer;
        }));
    
    device_request_->proceed();

    // Test that finish method does not throw
    EXPECT_NO_THROW(device_request_->finish());
    
    // Verify the object is still valid after finish
    ASSERT_NE(device_request_, nullptr);
}

// --- 3. EXCEPTION TESTS ---

// Test 3.1: Test std::exception by throwing in authorization setup
TEST_F(RESTDeviceRequestTests, DeviceRequest_authz_std_exception) {
    catena::exception_with_status rc("Device request failed: Test auth setup failure", catena::StatusCode::INTERNAL);

    EXPECT_CALL(socket_reader_, authorizationEnabled())
        .WillOnce(Return(true));
    EXPECT_CALL(socket_reader_, jwsToken())
        .WillOnce(Throw(std::runtime_error("Test auth setup failure")));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

// Test 3.2: Test catch (...) exception handling
// Note: catena and std exceptions covered in authorization tests
TEST_F(RESTDeviceRequestTests, DeviceRequest_catch_unknown_exception) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    EXPECT_CALL(device_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            throw 42; // Throw an int
            return nullptr; // Only exists to avoid lambda error
        }));
    
    device_request_->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

