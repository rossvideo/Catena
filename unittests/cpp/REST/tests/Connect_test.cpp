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
 * @author benjamin.whitten@rossvideo.com
 * @date 2025-06-23
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"
#include "MockSubscriptionManager.h"
#include "MockParam.h"
#include "MockLanguagePack.h"
#include "MockServiceImpl.h"
#include "CommonTestHelpers.h"

// REST
#include "controllers/Connect.h"

using namespace catena::common;
using namespace catena::REST;

class RESTConnectTest : public RESTEndpointTest {
protected:

    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTConnectTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }

    RESTConnectTest() : RESTEndpointTest() {
        EXPECT_CALL(context_, detailLevel())
            .WillRepeatedly(testing::Return(catena::Device_DetailLevel::Device_DetailLevel_FULL));
        EXPECT_CALL(context_, subscriptionManager())
            .WillRepeatedly(testing::ReturnRef(subManager_));
        EXPECT_CALL(context_, fields("user_agent"))
            .WillRepeatedly(testing::ReturnRef(userAgent_));
        EXPECT_CALL(context_, hasField("force_connection"))
            .WillRepeatedly(testing::Return(false));
        // Connection registration and deregistration
        EXPECT_CALL(context_, service()).WillRepeatedly(testing::Return(&service_));
        EXPECT_CALL(service_, registerConnection(testing::_)).WillRepeatedly(testing::Return(true)); // Should always call
        EXPECT_CALL(service_, deregisterConnection(testing::_)).Times(1).WillOnce(testing::Return()); // Should always call
        // dm0_ signals
        EXPECT_CALL(dm0_, getValueSetByClient()).WillRepeatedly(testing::ReturnRef(valueSetByClient0));
        EXPECT_CALL(dm0_, getValueSetByServer()).WillRepeatedly(testing::ReturnRef(valueSetByServer0));
        EXPECT_CALL(dm0_, getLanguageAddedPushUpdate()).WillRepeatedly(testing::ReturnRef(languageAddedPushUpdate0));
        // dm1_ signals
        EXPECT_CALL(dm1_, getValueSetByClient()).WillRepeatedly(testing::ReturnRef(valueSetByClient1));
        EXPECT_CALL(dm1_, getValueSetByServer()).WillRepeatedly(testing::ReturnRef(valueSetByServer1));
        EXPECT_CALL(dm1_, getLanguageAddedPushUpdate()).WillRepeatedly(testing::ReturnRef(languageAddedPushUpdate1));
        
        // Set up default JWS token for tests
        jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor));
    }

    // This is required to disconnect signals before deletion
    ~RESTConnectTest() { endpoint_.reset(nullptr); }

    /*
     * Creates a Connect handler object.
     */
    ICallData* makeOne() override { return catena::REST::Connect::makeOne(serverSocket_, context_, dms_); }

    // Helper function to build slot response
    std::string buildSlotResponse() {
        catena::PushUpdates populatedSlots;
        for (auto& [slot, dm] : dms_) {
            if (dm) {
                populatedSlots.mutable_slots_added()->add_slots(slot);
            }
        }
        std::string slotJson;
        google::protobuf::util::JsonPrintOptions options;
        auto status = google::protobuf::util::MessageToJsonString(populatedSlots, &slotJson, options);
        return slotJson;
    }

    // Helper function to build parameter update response
    std::string buildParamUpdateResponse(uint32_t slot, const std::string& oid, const std::string& value) {
        catena::PushUpdates updateResponse;
        updateResponse.set_slot(slot);
        auto* paramValue = updateResponse.mutable_value();
        paramValue->set_oid(oid);
        auto* valueObj = paramValue->mutable_value();
        valueObj->set_string_value(value);
        std::string updateJson;
        google::protobuf::util::JsonPrintOptions options;
        auto status = google::protobuf::util::MessageToJsonString(updateResponse, &updateJson, options);
        return updateJson;
    }

    // Helper function to build language pack update response
    std::string buildLanguagePackUpdateResponse(uint32_t slot, const std::string& name, const std::map<std::string, std::string>& words) {
        catena::PushUpdates updateResponse;
        updateResponse.set_slot(slot);
        auto* deviceComponent = updateResponse.mutable_device_component();
        auto* languagePackComponent = deviceComponent->mutable_language_pack();
        auto* pack = languagePackComponent->mutable_language_pack();
        pack->set_name(name);
        for (const auto& [key, value] : words) {
            (*pack->mutable_words())[key] = value;
        }
        std::string updateJson;
        google::protobuf::util::JsonPrintOptions options;
        auto status = google::protobuf::util::MessageToJsonString(updateResponse, &updateJson, options);
        return updateJson;
    }

    MockSubscriptionManager subManager_;
    MockServiceImpl service_;
    std::string userAgent_ = "test_agent";
    std::string paramOid_ = "test_param";

    // dm0_ test signals.
    vdk::signal<void(const std::string&, const IParam*)> valueSetByClient0;
    vdk::signal<void(const ILanguagePack*)> languageAddedPushUpdate0;
    vdk::signal<void(const std::string&, const IParam*)> valueSetByServer0;
    // dm1_ test signals.
    vdk::signal<void(const std::string&, const IParam*)> valueSetByClient1;
    vdk::signal<void(const ILanguagePack*)> languageAddedPushUpdate1;
    vdk::signal<void(const std::string&, const IParam*)> valueSetByServer1;
};

// --- 0. INITIAL TESTS ---

// Test 0.1: Test constructor initialization
TEST_F(RESTConnectTest, Connect_Create) {
    ASSERT_TRUE(endpoint_);
}

// Test 0.2: Test unauthorized connection
TEST_F(RESTConnectTest, Connect_HandlesAuthzError) {
    jwsToken_ = "invalid_token";
    authzEnabled_ = true;
    expRc_ = catena::exception_with_status("", catena::StatusCode::UNAUTHENTICATED);
    
    endpoint_->proceed();
    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 0.3: Test authorized connection
TEST_F(RESTConnectTest, Connect_HandlesValidAuthz) {
    authzEnabled_ = true;

    std::string slotJson = buildSlotResponse();

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {slotJson}));
}

// --- 1. SIGNAL TESTS ---

// Test 1.1: Test value set by server signal
TEST_F(RESTConnectTest, Connect_HandlesValueSetByServer) {
    authzEnabled_ = true;

    auto param = std::make_unique<MockParam>();
    EXPECT_CALL(*param, getOid())
        .WillRepeatedly(testing::ReturnRef(paramOid_));
    EXPECT_CALL(*param, getScope())
        .WillRepeatedly(testing::ReturnRef(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    EXPECT_CALL(*param, toProto(testing::An<catena::Value&>(), testing::An<catena::common::Authorizer&>()))
        .WillOnce(testing::Invoke([](catena::Value& value, catena::common::Authorizer&) {
            value.set_string_value("test_value");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    std::string slotJson = buildSlotResponse();
    std::string updateJson = buildParamUpdateResponse(0, paramOid_, "test_value");

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    dm0_.getValueSetByServer().emit(paramOid_, param.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {slotJson, updateJson}));
}

// Test 1.2: Test value set by client signal
TEST_F(RESTConnectTest, Connect_HandlesValueSetByClient) {
    authzEnabled_ = true;

    auto param = std::make_unique<MockParam>();
    EXPECT_CALL(*param, getOid())
        .WillRepeatedly(testing::ReturnRef(paramOid_));
    EXPECT_CALL(*param, getScope())
        .WillRepeatedly(testing::ReturnRef(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    EXPECT_CALL(*param, toProto(testing::An<catena::Value&>(), testing::An<catena::common::Authorizer&>()))
        .WillOnce(testing::Invoke([](catena::Value& value, catena::common::Authorizer&) {
            value.set_string_value("test_value");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    std::string slotJson = buildSlotResponse();
    std::string updateJson = buildParamUpdateResponse(0, paramOid_, "test_value");

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    dm0_.getValueSetByClient().emit(paramOid_, param.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {slotJson, updateJson}));
}

// Test 1.3: Test language signal
TEST_F(RESTConnectTest, Connect_HandlesLanguage) {
    authzEnabled_ = true;

    auto languagePack = std::make_unique<MockLanguagePack>();
    EXPECT_CALL(*languagePack, toProto(testing::_))
        .WillOnce(testing::Invoke([](catena::LanguagePack& pack) {
            pack.set_name("English");
            (*pack.mutable_words())["greeting"] = "Hello";
        }));

    std::string slotJson = buildSlotResponse();
    std::string updateJson = buildLanguagePackUpdateResponse(0, "English", {{"greeting", "Hello"}});

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    dm0_.getLanguageAddedPushUpdate().emit(languagePack.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_, {slotJson, updateJson}));
}

// --- 3. EXCEPTION TESTS ---

// Test 3.1: Test registration failure
TEST_F(RESTConnectTest, Connect_RegisterConnectionFailure) {
    expRc_ = catena::exception_with_status("Too many connections to service", catena::StatusCode::RESOURCE_EXHAUSTED);
    EXPECT_CALL(service_, registerConnection(testing::_)).WillOnce(testing::Return(false));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 3.2: Test std::exception handling
TEST_F(RESTConnectTest, Connect_HandlesStdException) {
    expRc_ = catena::exception_with_status("Connection setup failed: Runtime error", catena::StatusCode::INTERNAL);
    authzEnabled_ = true;
    EXPECT_CALL(context_, jwsToken())
        .WillOnce(testing::Invoke([]() -> const std::string& {
            static const std::string empty;
            throw std::runtime_error("Runtime error");
            return empty;
        }));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 3.3: Test unknown exception handling
TEST_F(RESTConnectTest, Connect_HandlesUnknownException) {
    expRc_ = catena::exception_with_status("Unknown error during connection setup", catena::StatusCode::UNKNOWN);
    authzEnabled_ = true;
    EXPECT_CALL(context_, jwsToken())
        .WillOnce(testing::Throw(42));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    proceed_thread.join();

    EXPECT_EQ(readResponse(), expectedSSEResponse(expRc_));
}

// Test 3.4: Test socket close during response sending with writer failure
TEST_F(RESTConnectTest, Connect_HandlesWriterFailure) {
    authzEnabled_ = true;

    auto param = std::make_unique<MockParam>();
    EXPECT_CALL(*param, getOid())
        .WillRepeatedly(testing::ReturnRef(paramOid_));
    EXPECT_CALL(*param, getScope())
        .WillRepeatedly(testing::ReturnRef(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    EXPECT_CALL(*param, toProto(testing::An<catena::Value&>(), testing::An<catena::common::Authorizer&>()))
        .WillOnce(testing::Invoke([](catena::Value& value, catena::common::Authorizer&) {
            value.set_string_value("test_value");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    // Run proceed() in a separate thread since it blocks
    std::thread proceed_thread([this]() {
        endpoint_->proceed();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    clientSocket_.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    dm0_.getValueSetByServer().emit(paramOid_, param.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    EXPECT_FALSE(serverSocket_.is_open());

    catena::REST::Connect::shutdownSignal_.emit();
    proceed_thread.join();
}
