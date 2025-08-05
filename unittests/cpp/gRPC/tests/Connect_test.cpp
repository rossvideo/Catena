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
 * @brief This file is for testing the Connect.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/07/22
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "MockParam.h"
#include "MockSubscriptionManager.h"
#include "MockLanguagePack.h"
#include "MockConnectionQueue.h"
#include "GRPCTest.h"
#include "StreamReader.h"

// gRPC
#include "controllers/Connect.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCConnectTests : public GRPCTest {
  protected:
    gRPCConnectTests() {
        expRc_ = catena::exception_with_status("", catena::StatusCode::CANCELLED);
        EXPECT_CALL(service_, connectionQueue()).WillRepeatedly(testing::ReturnRef(connectionQueue_));
        EXPECT_CALL(connectionQueue_, registerConnection(testing::_)).WillRepeatedly(testing::Return(true));
        EXPECT_CALL(connectionQueue_, deregisterConnection(testing::_)).WillRepeatedly(testing::Return());
        // dm0_ signals
        EXPECT_CALL(dm0_, getValueSetByClient()).WillRepeatedly(testing::ReturnRef(valueSetByClient0));
        EXPECT_CALL(dm0_, getValueSetByServer()).WillRepeatedly(testing::ReturnRef(valueSetByServer0));
        EXPECT_CALL(dm0_, getLanguageAddedPushUpdate()).WillRepeatedly(testing::ReturnRef(languageAddedPushUpdate0));
        // dm1_ signals
        EXPECT_CALL(dm1_, getValueSetByClient()).WillRepeatedly(testing::ReturnRef(valueSetByClient1));
        EXPECT_CALL(dm1_, getValueSetByServer()).WillRepeatedly(testing::ReturnRef(valueSetByServer1));
        EXPECT_CALL(dm1_, getLanguageAddedPushUpdate()).WillRepeatedly(testing::ReturnRef(languageAddedPushUpdate1));
    }
    
    /*
     * Creates an Connect handler object.
     */
    void makeOne() override {
        EXPECT_CALL(service_, getSubscriptionManager()).WillRepeatedly(testing::ReturnRef(subManager_));
        new catena::gRPC::Connect(&service_, dms_, true);
    }
    /*
     * Overriding processEvents to allow for asyncCall_ to emit the shutdown
     * signal at tear down.
     */
    void processEvents() override {
        std::unique_ptr<std::thread> testThread = nullptr;
        void* ignored_tag;
        bool ok;
        while (cq_->Next(&ignored_tag, &ok)) {
            if (!testCall_) {
                testCall_.swap(asyncCall_);
            }
            // asyncCall_ emits the shutdown signal.
            if (asyncCall_ && !ok) {
                EXPECT_CALL(service_, deregisterItem(asyncCall_.get()))
                    .WillOnce(::testing::Invoke([this] { asyncCall_.reset(nullptr); }));
                asyncCall_->proceed(ok);
            // testCall_ proceed assigned to thread to avoid blocking asyncCall_.
            } else if (testCall_) {
                if (testThread) { testThread->join(); }
                testThread = std::make_unique<std::thread>([&]{ testCall_->proceed(ok); });
            }
        }
        // Make sure the testCall is completely finished before continuing.
        if (testThread) { testThread->join(); }
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and compares the streamed-back response.
     */
    using StreamReader = catena::gRPC::test::StreamReader<catena::PushUpdates, catena::ConnectPayload, 
        std::function<void(grpc::ClientContext*, const catena::ConnectPayload*, grpc::ClientReadReactor<catena::PushUpdates>*)>>;

    /*
     * Streamlines the creation of executeCommandPayloads. 
     */
    void initPayload(const std::string& language, const catena::Device_DetailLevel& dl, const std::string& userAgent, bool forceConnection) {
        inVal_.set_language(language);
        inVal_.set_detail_level(dl);
        inVal_.set_user_agent(userAgent);
        inVal_.set_force_connection(forceConnection);
    }
    /*
     * Adds a PushValue to the expected values.
     */
    void expPushValue(uint32_t slot, const std::string& oid, const std::string& stringVal) {
        catena::PushUpdates pushUpdate;
        pushUpdate.set_slot(slot);
        // Set value.
        pushUpdate.mutable_value()->set_oid(oid);
        pushUpdate.mutable_value()->mutable_value()->set_string_value(stringVal);
        // Add to back
        expVals_.push_back(pushUpdate);
    }
    /*
     * Adds a LanguagePack to the expected values.
     */
    void expLanguage(uint32_t slot, const std::string& language, const std::unordered_map<std::string, std::string>& words) {
        catena::PushUpdates pushUpdate;
        pushUpdate.set_slot(slot);
        // Set language pack
        auto langComponent = pushUpdate.mutable_device_component()->mutable_language_pack();
        langComponent->mutable_language_pack()->set_name(language);
        for (auto& [key, value] : words) {
            (*langComponent->mutable_language_pack()->mutable_words())[key] = value;
        }
        // Add to back
        expVals_.push_back(pushUpdate);
    }
    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Adding slotList
        expVals_.insert(expVals_.begin(), catena::PushUpdates());
        auto slotList = expVals_.front().mutable_slots_added();
        for (auto& [slot, dm] : dms_) {
            slotList->add_slots(slot);
        }
        // Comparing the results.
        ASSERT_EQ(outVals_.size(), expVals_.size()) << "Output missing >= 1 PushUpdate";
        for (uint32_t i = 0; i < outVals_.size(); i++) {
            EXPECT_EQ(outVals_[i].SerializeAsString(), expVals_[i].SerializeAsString());
        }
        // Make sure another Command handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    ~gRPCConnectTests() override {
        // Make sure the stream reader is destroyed.
        if (streamReader_) {
            streamReader_->Await();
            // Compare the output status.
            EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        }
    }

    // in/out val
    catena::ConnectPayload inVal_;
    std::vector<catena::PushUpdates> outVals_;
    // Expected variables
    std::vector<catena::PushUpdates> expVals_;
    bool respond_ = false;

    MockSubscriptionManager subManager_;
    MockConnectionQueue connectionQueue_;
    std::unique_ptr<StreamReader> streamReader_ = nullptr;

    // Test signals.
    vdk::signal<void(const std::string&, const IParam*)> valueSetByClient0, valueSetByClient1;
    vdk::signal<void(const ILanguagePack*)> languageAddedPushUpdate0, languageAddedPushUpdate1;
    vdk::signal<void(const std::string&, const IParam*)> valueSetByServer0, valueSetByServer1;
};

/*
 * ============================================================================
 *                               Connect tests
 * ============================================================================
 * 
 * TEST 1 - Creating a Connect object.
 */
TEST_F(gRPCConnectTests, Connect_Create) {
    EXPECT_TRUE(asyncCall_);
}
/*
 * TEST 2 - Testing Connect's ability to connect and disconnect to the
 *          mockDevice signals.
 */
TEST_F(gRPCConnectTests, Connect_ConnectDisconnect) {
    // Setting expectations.
    EXPECT_CALL(connectionQueue_, registerConnection(testing::_)).Times(1).WillOnce(testing::Return(true));
    // Once for main call, once for the async call.
    EXPECT_CALL(connectionQueue_, deregisterConnection(testing::_)).Times(2).WillOnce(testing::Return());
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    testRPC();
}
/*
 * TEST 3 - Testing Connect recieving ValueSetByClient signals.
 */
TEST_F(gRPCConnectTests, Connect_ValueSetByClient) {
    MockParam param0, param1;
    initPayload("en", catena::Device_DetailLevel::Device_DetailLevel_FULL, "", false);
    expPushValue(0, "oid0", "value0");
    expPushValue(1, "oid1", "value1");
    // Setting expectations
    EXPECT_CALL(param0, getScope()).WillRepeatedly(testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    EXPECT_CALL(param0, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, const IAuthorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            dst.set_string_value("value0");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(param1, getScope()).WillRepeatedly(testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    EXPECT_CALL(param1, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, const IAuthorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            dst.set_string_value("value1");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    dm0_.getValueSetByClient().emit("oid0", &param0);
    streamReader_->Await();
    dm1_.getValueSetByClient().emit("oid1", &param1);
    streamReader_->Await();
    testRPC();
}
/*
 * TEST 4 - Testing Connect recieving ValueSetByServer signals.
 */
TEST_F(gRPCConnectTests, Connect_ValueSetByServer) {
    MockParam param0, param1;
    initPayload("en", catena::Device_DetailLevel::Device_DetailLevel_FULL, "", false);
    expPushValue(0, "oid0", "value0");
    expPushValue(1, "oid1", "value1");
    // Setting expectations
    EXPECT_CALL(param0, getScope()).WillRepeatedly(
        testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    EXPECT_CALL(param0, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, const IAuthorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            dst.set_string_value("value0");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(param1, getScope()).WillRepeatedly(
        testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kUndefined)));
    EXPECT_CALL(param1, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, const IAuthorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            dst.set_string_value("value1");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    dm0_.getValueSetByServer().emit("oid0", &param0);
    streamReader_->Await();
    dm1_.getValueSetByServer().emit("oid1", &param1);
    streamReader_->Await();
    testRPC();
}
/*
 * TEST 4 - Testing Connect recieving LanguageAddedPushUpdate signals.
 */
TEST_F(gRPCConnectTests, Connect_LanguageAddedPushUpdate) {
    MockLanguagePack languagePack0, languagePack1;
    initPayload("en", catena::Device_DetailLevel::Device_DetailLevel_FULL, "", false);
    expLanguage(0, "language0", {{"key0", "value0"}});
    expLanguage(1, "language1", {{"key1", "value1"}});
    // Setting expectations
    EXPECT_CALL(languagePack0, toProto(testing::_)).Times(1)
        .WillOnce(testing::Invoke([](catena::LanguagePack &pack) {
            pack.set_name("language0");
            (*pack.mutable_words())["key0"] = "value0";
        }));
    EXPECT_CALL(languagePack1, toProto(testing::_)).Times(1)
        .WillOnce(testing::Invoke([](catena::LanguagePack &pack) {
            pack.set_name("language1");
            (*pack.mutable_words())["key1"] = "value1";
        }));
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    dm0_.getLanguageAddedPushUpdate().emit(&languagePack0);
    streamReader_->Await();
    dm1_.getLanguageAddedPushUpdate().emit(&languagePack1);
    streamReader_->Await();
    testRPC();
}
/*
 * TEST 5 - Testing Connect with valid authz token.
 */
TEST_F(gRPCConnectTests, Connect_AuthzValid) {
    MockParam param0, param1;
    MockLanguagePack languagePack0, languagePack1;
    initPayload("en", catena::Device_DetailLevel::Device_DetailLevel_FULL, "", false);
    expPushValue(0, "oid0", "value0");
    expPushValue(0, "oid0", "value0");
    expLanguage(0, "language0", {{"key0", "value0"}});
    // Adding authorization mockToken metadata. This it a random RSA token
    authzEnabled_ = true;
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
    clientContext_.AddMetadata("authorization", "Bearer " + mockToken);
    // Setting expectations
    EXPECT_CALL(param0, getScope()).WillRepeatedly(
        testing::ReturnRefOfCopy(Scopes().getForwardMap().at(Scopes_e::kMonitor)));
    EXPECT_CALL(param0, toProto(testing::An<catena::Value&>(), testing::_)).Times(2)
        .WillRepeatedly(testing::Invoke([this](catena::Value& dst, const IAuthorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            dst.set_string_value("value0");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(languagePack0, toProto(testing::_)).Times(1)
        .WillOnce(testing::Invoke([](catena::LanguagePack &pack) {
            pack.set_name("language0");
            (*pack.mutable_words())["key0"] = "value0";
        }));
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    dm0_.getValueSetByClient().emit("oid0", &param0);
    streamReader_->Await();
    dm0_.getValueSetByServer().emit("oid0", &param0);
    streamReader_->Await();
    dm0_.getLanguageAddedPushUpdate().emit(&languagePack0);
    streamReader_->Await();
    testRPC();
}
/*
 * TEST 6 - Testing Connect with invalid authz token.
 */
TEST_F(gRPCConnectTests, Connect_AuthzInvalid) {
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    streamReader_.reset(nullptr);
}
/*
 * TEST 7 - Testing Connect with no authz token.
 */
TEST_F(gRPCConnectTests, Connect_AuthzJWSNotFound) {
    expRc_ = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token
    authzEnabled_ = true;
    clientContext_.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    streamReader_.reset(nullptr);
}

/*
 * TEST 8 - Testing Connect failing to register with service.
 */
TEST_F(gRPCConnectTests, Connect_RegisterConnectionFailure) {
    expRc_ = catena::exception_with_status("Too many connections to service", catena::StatusCode::RESOURCE_EXHAUSTED);
    // Setting expectations
    EXPECT_CALL(connectionQueue_, registerConnection(testing::_)).WillOnce(testing::Return(false));
    // Making call
    streamReader_ = std::make_unique<StreamReader>(&outVals_, &outRc_, true);
    streamReader_->MakeCall(&clientContext_, &inVal_, [this](auto ctx, auto payload, auto reactor) {
        client_->async()->Connect(ctx, payload, reactor);
    });
    streamReader_->Await();
    streamReader_.reset(nullptr);
}
