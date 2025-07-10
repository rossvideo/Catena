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
 * @date 25/07/??
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "MockParam.h"
#include "MockSubscriptionManager.h"
#include "MockLanguagePack.h"
#include "GRPCTest.h"

// gRPC
#include "controllers/Connect.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCConnectTests : public GRPCTest {
  protected:
    gRPCConnectTests() { expRc_ = catena::exception_with_status("Cancelled on the server side", catena::StatusCode::CANCELLED); }
    
    /*
     * Creates an Connect handler object.
     */
    void makeOne() override {
        EXPECT_CALL(service_, getSubscriptionManager()).WillRepeatedly(testing::ReturnRef(subManager));
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
                testThread = std::make_unique<std::thread>([&]{testCall_->proceed(ok);});
            }
        }
        if (testThread) { testThread->join(); }
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and compares the streamed-back response.
     */
    class StreamReader : public grpc::ClientReadReactor<catena::PushUpdates> {
      public:
        StreamReader(std::vector<catena::PushUpdates>* outVals_, grpc::Status* outRc_, uint32_t reads = 1)
            : outVals_(outVals_), outRc_(outRc_), reads_(reads) {}
        /*
         * This function makes an async RPC to the MockServer.
         */
        void MakeCall(catena::CatenaService::Stub* client, grpc::ClientContext* clientContext, const catena::ConnectPayload* inVal) {
            // Sending async RPC.
            client->async()->Connect(clientContext, inVal, this);
            StartRead(&outVal_);
            StartCall();
        }
        /*
        * Triggers when a read is done_ and adds output to outVals_.
        */
        void OnReadDone(bool ok) override {
            if (ok) {
                outVals_->emplace_back(outVal_);
                done_ = true;
                cv_.notify_one();
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
        inline void Await() {
            cv_.wait(lock_, [this] { return done_; });
            done_ = false;
        }
        
      private:
        // Pointers to the output variables.
        grpc::Status* outRc_;
        std::vector<catena::PushUpdates>* outVals_;

        uint32_t reads_;
        uint32_t current_ = 0;

        catena::PushUpdates outVal_;
        bool done_ = false;
        std::condition_variable cv_;
        std::mutex cv_mtx_;
        std::unique_lock<std::mutex> lock_{cv_mtx_};
    };

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
            EXPECT_EQ(outRc_.error_message(), expRc_.what());
        }
    }

    // in/out val
    catena::ConnectPayload inVal_;
    std::vector<catena::PushUpdates> outVals_;
    // Expected variables
    std::vector<catena::PushUpdates> expVals_;
    bool respond_ = false;

    MockSubscriptionManager subManager;
    std::unique_ptr<StreamReader> streamReader_ = nullptr;

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
 * TEST 2 - Testing Connect RPC's ability to connect and disconnect to the
 *          mockDevice signals.
 */
TEST_F(gRPCConnectTests, Connect_ConnectDisconnect) {
    streamReader_.reset(new StreamReader(&outVals_, &outRc_));
    streamReader_->MakeCall(client_.get(), &clientContext_, &inVal_);
    streamReader_->Await();
    testRPC();
}

TEST_F(gRPCConnectTests, Connect_ValueSetByClient) {
    MockParam param0, param1;
    initPayload("en", catena::Device_DetailLevel::Device_DetailLevel_FULL, "", false);
    expPushValue(0, "oid0", "value0");
    expPushValue(1, "oid1", "value1");
    // Setting expectations
    EXPECT_CALL(param0, getScope()).WillRepeatedly(testing::ReturnRef(""));
    EXPECT_CALL(param0, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, Authorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            dst.set_string_value("value0");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(param1, getScope()).WillRepeatedly(testing::ReturnRef(""));
    EXPECT_CALL(param1, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, Authorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            dst.set_string_value("value1");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Making call
    streamReader_.reset(new StreamReader(&outVals_, &outRc_));
    streamReader_->MakeCall(client_.get(), &clientContext_, &inVal_);
    streamReader_->Await();
    dm0_.valueSetByClient.emit("oid0", &param0);
    streamReader_->Await();
    dm1_.valueSetByClient.emit("oid1", &param1);
    streamReader_->Await();
    testRPC();
}

TEST_F(gRPCConnectTests, Connect_ValueSetByServer) {
    MockParam param0, param1;
    initPayload("en", catena::Device_DetailLevel::Device_DetailLevel_FULL, "", false);
    expPushValue(0, "oid0", "value0");
    expPushValue(1, "oid1", "value1");
    // Setting expectations
    EXPECT_CALL(param0, getScope()).WillRepeatedly(testing::ReturnRef(""));
    EXPECT_CALL(param0, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, Authorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            dst.set_string_value("value0");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(param1, getScope()).WillRepeatedly(testing::ReturnRef(""));
    EXPECT_CALL(param1, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::Value& dst, Authorizer& authz) {
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            dst.set_string_value("value1");
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Making call
    streamReader_.reset(new StreamReader(&outVals_, &outRc_));
    streamReader_->MakeCall(client_.get(), &clientContext_, &inVal_);
    streamReader_->Await();
    dm0_.valueSetByServer.emit("oid0", &param0);
    streamReader_->Await();
    dm1_.valueSetByServer.emit("oid1", &param1);
    streamReader_->Await();
    testRPC();
}

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
    streamReader_.reset(new StreamReader(&outVals_, &outRc_));
    streamReader_->MakeCall(client_.get(), &clientContext_, &inVal_);
    streamReader_->Await();
    dm0_.languageAddedPushUpdate.emit(&languagePack0);
    streamReader_->Await();
    dm1_.languageAddedPushUpdate.emit(&languagePack1);
    streamReader_->Await();
    testRPC();
}
