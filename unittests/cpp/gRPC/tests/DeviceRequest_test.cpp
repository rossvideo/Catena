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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// gtest
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// std
#include <string>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// Test helpers
#include "GRPCTest.h"

// gRPC
#include "MockSubscriptionManager.h"
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCDeviceRequestTests : public GRPCTest {
  protected:
    /*
     * Creates a DeviceRequest handler object.
     */
    void makeOne() override { new DeviceRequest(&service, dms, true); }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and returns the streamed-back response.
     */
    class StreamReader : public grpc::ClientReadReactor<catena::DeviceComponent> {
        public:
            StreamReader(std::vector<catena::DeviceComponent>* outVals, grpc::Status* outRc)
                : outVals_(outVals), outRc_(outRc) {}
            /*
             * This function makes an async RPC to the MockServer.
             */
            void MakeCall(catena::CatenaService::Stub* client, grpc::ClientContext* clientContext, const catena::DeviceRequestPayload* inVal) {
                // Sending async RPC.
                client->async()->DeviceRequest(clientContext, inVal, this);
                StartRead(&outVal_);
                StartCall();
            }
            /*
             * Triggers when a read is done and adds output to outVals_.
             */
            void OnReadDone(bool ok) override {
                if (ok) {
                    outVals_->emplace_back(outVal_);
                    StartRead(&outVal_);
                }
            }
            /*
             * Triggers when the RPC is finished and notifies Await().
             */
            void OnDone(const grpc::Status& status) override {
                *outRc_ = status;
                done_ = true;
                cv_.notify_one();
            }
            /*
             * Blocks until the RPC is finished. 
             */
            inline void Await() { cv_.wait(lock_, [this] { return done_; }); }
        
          private:
            // Pointers to the output variables.
            grpc::Status* outRc_;
            std::vector<catena::DeviceComponent>* outVals_;

            catena::DeviceComponent outVal_;
            bool done_ = false;
            std::condition_variable cv_;
            std::mutex cv_mtx_;
            std::unique_lock<std::mutex> lock_{cv_mtx_};
    };

    /*
     * Helper function which initializes an DeviceRequestPayload object.
     */
    void initPayload(uint32_t slot, const catena::Device_DetailLevel& dl, const std::set<std::string>& subbedOids) {
        inVal.set_detail_level(dl);
        inVal.set_slot(slot);
        for (const auto& oid : subbedOids) {
            inVal.add_subscribed_oids(oid);
        }
    }

    /*
     * Helper function which populates expNum with up to 6 items.
     */
    void initExpVal(uint32_t expNum = 0) {
        if (expNum > 6 ) { expNum = 6; }
        switch (expNum) {
            case 6:
                expVals.emplace(expVals.begin(), catena::DeviceComponent());
                expVals.begin()->mutable_command()->set_oid("command_test");
            case 5:
                expVals.emplace(expVals.begin(), catena::DeviceComponent());
                expVals.begin()->mutable_param()->set_oid("param_test");
            case 4:
                expVals.emplace(expVals.begin(), catena::DeviceComponent());
                expVals.begin()->mutable_shared_constraint()->set_oid("constraint_test");
            case 3:
                expVals.emplace(expVals.begin(), catena::DeviceComponent());
                expVals.begin()->mutable_language_pack()->set_language("language_test");
            case 2:
                expVals.emplace(expVals.begin(), catena::DeviceComponent());
                expVals.begin()->mutable_menu()->set_oid("menu_test");
            case 1:
                expVals.emplace(expVals.begin(), catena::DeviceComponent());
                expVals.begin()->mutable_device()->set_slot(inVal.slot());
        }
    }

    /* 
    * Makes an async RPC to the MockServer and waits for a response before
    * comparing output.
    */
    void testRPC() {
        // Sending async RPC.
        StreamReader streamReader(&outVals, &outRc);
        streamReader.MakeCall(client.get(), &clientContext, &inVal);
        streamReader.Await();
        // Comparing the results.
        ASSERT_EQ(outVals.size(), expVals.size()) << "Output missing >= 1 DeviceComponents";
        for (uint32_t i = 0; i < outVals.size(); i++) {
            EXPECT_EQ(outVals[i].SerializeAsString(), expVals[i].SerializeAsString());
        }
        EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
        EXPECT_EQ(outRc.error_message(), expRc.what());
        // Make sure another DeviceRequest handler was created.
        EXPECT_TRUE(asyncCall) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::DeviceRequestPayload inVal;
    std::vector<catena::DeviceComponent> outVals;
    // Expected variables
    std::vector<catena::DeviceComponent> expVals;

    std::unique_ptr<MockDevice::MockDeviceSerializer> mockSerializer = std::make_unique<MockDevice::MockDeviceSerializer>();
};

/*
 * ============================================================================
 *                               DeviceRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating a DeviceRequest object.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_Create) {
    EXPECT_TRUE(asyncCall);
}

/*
 * TEST 2 - Normal case for DeviceRequest proceed().
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_Normal) {
    initPayload(0, catena::Device_DetailLevel::Device_DetailLevel_FULL, {});
    initExpVal(6);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, inVal.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_TRUE(subscribedOids.empty());
            return std::move(mockSerializer);
        }));
    EXPECT_CALL(*mockSerializer, getNext()).Times(6)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]))
        .WillOnce(::testing::Return(expVals[2]))
        .WillOnce(::testing::Return(expVals[3]))
        .WillOnce(::testing::Return(expVals[4]))
        .WillOnce(::testing::Return(expVals[5]));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(6)
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(true))
        .WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 3 - DeviceRequest proceed() with detail_level subscriptions.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_Subscriptions) {
    std::set<std::string> subscribedTestOids{"oid_test_1", "oid_test_2", "oid_test_3"};
    initPayload(0, catena::Device_DetailLevel::Device_DetailLevel_SUBSCRIPTIONS, subscribedTestOids);
    initExpVal(1);
    MockSubscriptionManager mockSubManager;
    // Setting expectations
    EXPECT_CALL(service, getSubscriptionManager()).WillRepeatedly(::testing::ReturnRef(mockSubManager));
    for (auto oid : subscribedTestOids) {
        EXPECT_CALL(mockSubManager, addSubscription(oid, ::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [this](const std::string &oid, catena::common::IDevice &dm, catena::exception_with_status &rc, catena::common::Authorizer &authz){
            // Making sure the correct values were passed in
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(&dm, &this->dm);
            return true;
        }));
    }
    EXPECT_CALL(mockSubManager, getAllSubscribedOids(::testing::_)).Times(1).WillOnce(::testing::Return(subscribedTestOids));
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, inVal.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([this, &subscribedTestOids](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(subscribedOids, subscribedTestOids);
            return std::move(mockSerializer);
        }));
    EXPECT_CALL(*mockSerializer, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 4 - DeviceRequest with authz on and valid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_AuthzValid) {
    initPayload(0, catena::Device_DetailLevel::Device_DetailLevel_MINIMAL, {});
    initExpVal(1);
    // Adding authorization mockToken metadata. This it a random RSA token
    authzEnabled = true;
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
    clientContext.AddMetadata("authorization", "Bearer " + mockToken);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, inVal.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_TRUE(subscribedOids.empty());
            return std::move(mockSerializer);
        }));
    EXPECT_CALL(*mockSerializer, getNext()).Times(1).WillOnce(::testing::Return(expVals[0]));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 5 - DeviceRequest with authz on and invalid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_AuthzInvalid) {
    expRc = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - DeviceRequest with authz on and invalid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_AuthzJWSNotFound) {
    expRc = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 7 - dm.getComponentSerializer() returns nullptr.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetSerializerIllegalState) {
    expRc = catena::exception_with_status("Illegal state", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(nullptr));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 8 - dm.getComponentSerializer() throws a catena::exception_with_status.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetSerializerThrowCatena) {
    expRc = catena::exception_with_status("Component not found", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::common::Authorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return nullptr;
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 9 - dm.getComponentSerializer() throws a std::runtime_exception.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetSerializerThrowUnknown) {
    expRc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 10 - serializer.getNext() throws a catena::exception_with_status.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetNextThrowCatena) {
    expRc = catena::exception_with_status("Component not found", catena::StatusCode::INVALID_ARGUMENT);
    initExpVal(2);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockSerializer); }));
    // Reading 2 components successfully before throwing an exception.
    EXPECT_CALL(*mockSerializer, getNext()).Times(3)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]))
        .WillOnce(::testing::Invoke([this](){
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return expVals[1]; // Should not recieve
        }));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(2).WillRepeatedly(::testing::Return(true));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 11 - serializer.getNext() throws a std::runtime_exception.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetNextThrowUnknown) {
    expRc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initExpVal(2);
    // Setting expectations
    EXPECT_CALL(dm, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockSerializer); }));
    // Reading 2 components successfully before throwing an exception.
    EXPECT_CALL(*mockSerializer, getNext()).Times(3)
        .WillOnce(::testing::Return(expVals[0]))
        .WillOnce(::testing::Return(expVals[1]))
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    EXPECT_CALL(*mockSerializer, hasMore()).Times(2).WillRepeatedly(::testing::Return(true));
    // Sending the RPC
    testRPC();
}
