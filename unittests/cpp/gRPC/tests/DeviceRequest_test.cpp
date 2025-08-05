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

// Test helpers
#include "GRPCTest.h"
#include "StreamReader.h"
#include "CommonTestHelpers.h"

// gRPC
#include "MockSubscriptionManager.h"
#include "MockDeviceSerializer.h"
#include "controllers/DeviceRequest.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCDeviceRequestTests : public GRPCTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("gRPCDeviceRequestTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    /*
     * Creates a DeviceRequest handler object.
     */
    void makeOne() override { new DeviceRequest(&service_, dms_, true); }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and returns the streamed-back response.
     */
    using StreamReader = catena::gRPC::test::StreamReader<catena::DeviceComponent, catena::DeviceRequestPayload, 
        std::function<void(grpc::ClientContext*, const catena::DeviceRequestPayload*, grpc::ClientReadReactor<catena::DeviceComponent>*)>>;

    /*
     * Helper function which initializes an DeviceRequestPayload object.
     */
    void initPayload(uint32_t slot, const catena::Device_DetailLevel& dl, const std::set<std::string>& subbedOids) {
        inVal_.set_detail_level(dl);
        inVal_.set_slot(slot);
        for (const auto& oid : subbedOids) {
            inVal_.add_subscribed_oids(oid);
        }
    }

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
                expVals_.begin()->mutable_device()->set_slot(inVal_.slot());
        }
    }

    /* 
    * Makes an async RPC to the MockServer and waits for a response before
    * comparing output.
    */
    void testRPC() {
        // Sending async RPC.
        StreamReader streamReader(&outVals_, &outRc_);
        streamReader.MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
            client_->async()->DeviceRequest(ctx, payload, reactor);
        });
        streamReader.Await();
        // Comparing the results.
        ASSERT_EQ(outVals_.size(), expVals_.size()) << "Output missing >= 1 DeviceComponents";
        for (uint32_t i = 0; i < outVals_.size(); i++) {
            EXPECT_EQ(outVals_[i].SerializeAsString(), expVals_[i].SerializeAsString());
        }
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another DeviceRequest handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::DeviceRequestPayload inVal_;
    std::vector<catena::DeviceComponent> outVals_;
    // Expected variables
    std::vector<catena::DeviceComponent> expVals_;

    std::unique_ptr<MockDeviceSerializer> mockSerializer_ = std::make_unique<MockDeviceSerializer>();
};

/*
 * ============================================================================
 *                               DeviceRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating a DeviceRequest object.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_Create) {
    EXPECT_TRUE(asyncCall_);
}

/*
 * TEST 2 - Normal case for DeviceRequest proceed().
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_Normal) {
    initPayload(0, catena::Device_DetailLevel::Device_DetailLevel_FULL, {});
    initExpVal(6);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, inVal_.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([this](const IAuthorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            EXPECT_TRUE(subscribedOids.empty());
            return std::move(mockSerializer_);
        }));
    EXPECT_CALL(*mockSerializer_, getNext()).Times(6)
        .WillOnce(::testing::Return(expVals_[0]))
        .WillOnce(::testing::Return(expVals_[1]))
        .WillOnce(::testing::Return(expVals_[2]))
        .WillOnce(::testing::Return(expVals_[3]))
        .WillOnce(::testing::Return(expVals_[4]))
        .WillOnce(::testing::Return(expVals_[5]));
    EXPECT_CALL(*mockSerializer_, hasMore()).Times(6)
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
    EXPECT_CALL(service_, getSubscriptionManager()).WillRepeatedly(::testing::ReturnRef(mockSubManager));
    for (auto oid : subscribedTestOids) {
        EXPECT_CALL(mockSubManager, addSubscription(oid, ::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Invoke(
        [this](const std::string &oid, catena::common::IDevice &dm, catena::exception_with_status &rc, const IAuthorizer &authz){
            // Making sure the correct values were passed in
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            EXPECT_EQ(&dm, &dm0_);
            return true;
        }));
    }
    EXPECT_CALL(mockSubManager, getAllSubscribedOids(::testing::_)).Times(1).WillOnce(::testing::Return(subscribedTestOids));
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, inVal_.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([this, &subscribedTestOids](const IAuthorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            EXPECT_EQ(subscribedOids, subscribedTestOids);
            return std::move(mockSerializer_);
        }));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockSerializer_, getNext()).Times(1).WillOnce(::testing::Return(expVals_[0]));
    EXPECT_CALL(*mockSerializer_, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 4 - DeviceRequest with authz on and valid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_AuthzValid) {
    initPayload(0, catena::Device_DetailLevel::Device_DetailLevel_MINIMAL, {});
    initExpVal(1);
    // Adding authorization mockToken metadata.
    authzEnabled_ = true;
    std::string mockToken = getJwsToken("st2138:mon:w st2138:op:w st2138:cfg:w st2138:adm:w");
    clientContext_.AddMetadata("authorization", "Bearer " + mockToken);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, inVal_.detail_level(), true)).Times(1)
        .WillOnce(::testing::Invoke([this](const IAuthorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            // Making sure the correct values were passed in.
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            EXPECT_TRUE(subscribedOids.empty());
            return std::move(mockSerializer_);
        }));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*mockSerializer_, getNext()).Times(1).WillOnce(::testing::Return(expVals_[0]));
    EXPECT_CALL(*mockSerializer_, hasMore()).Times(1).WillOnce(::testing::Return(false));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 5 - DeviceRequest with authz on and invalid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - DeviceRequest with authz on and invalid token.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_AuthzJWSNotFound) {
    expRc_ = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 7 - No device in the specified slot.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrInvalidSlot) {
    initPayload(dms_.size(), catena::Device_DetailLevel_FULL, {});
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 8 - dm.getComponentSerializer() returns nullptr.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetSerializerIllegalState) {
    expRc_ = catena::exception_with_status("Illegal state", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(nullptr));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 9 - dm.getComponentSerializer() throws a catena::exception_with_status.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetSerializerThrowCatena) {
    expRc_ = catena::exception_with_status("Component not found", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const IAuthorizer &authz, const std::set<std::string> &subscribedOids, catena::Device_DetailLevel dl, bool shallow){
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return nullptr;
        }));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 10 - dm.getComponentSerializer() throws a std::runtime_exception.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetSerializerThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 11 - serializer.getNext() throws a catena::exception_with_status.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetNextThrowCatena) {
    expRc_ = catena::exception_with_status("Component not found", catena::StatusCode::INVALID_ARGUMENT);
    initExpVal(2);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockSerializer_); }));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Reading 2 components successfully before throwing an exception.
    EXPECT_CALL(*mockSerializer_, getNext()).Times(3)
        .WillOnce(::testing::Return(expVals_[0]))
        .WillOnce(::testing::Return(expVals_[1]))
        .WillOnce(::testing::Invoke([this](){
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return expVals_[1]; // Should not recieve
        }));
    EXPECT_CALL(*mockSerializer_, hasMore()).Times(2).WillRepeatedly(::testing::Return(true));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 12 - serializer.getNext() throws a std::runtime_exception.
 */
TEST_F(gRPCDeviceRequestTests, DeviceRequest_ErrGetNextThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    initExpVal(2);
    // Setting expectations
    EXPECT_CALL(dm0_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](){ return std::move(mockSerializer_); }));
    EXPECT_CALL(dm1_, getComponentSerializer(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Reading 2 components successfully before throwing an exception.
    EXPECT_CALL(*mockSerializer_, getNext()).Times(3)
        .WillOnce(::testing::Return(expVals_[0]))
        .WillOnce(::testing::Return(expVals_[1]))
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    EXPECT_CALL(*mockSerializer_, hasMore()).Times(2).WillRepeatedly(::testing::Return(true));
    // Sending the RPC
    testRPC();
}
