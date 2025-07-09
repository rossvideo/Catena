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
#include "GRPCTest.h"

// gRPC
#include "controllers/Connect.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCConnectTests : public GRPCTest {
  protected:
    /*
     * Creates an Connect handler object.
     */
    void makeOne() override {
        EXPECT_CALL(service_, getSubscriptionManager()).WillRepeatedly(testing::ReturnRef(subManager));
        new catena::gRPC::Connect(&service_, dms_, true);
    }

    /*
     * This is a test class which makes an async RPC to the MockServer on
     * construction and compares the streamed-back response.
     */
    class StreamReader : public grpc::ClientReadReactor<catena::PushUpdates> {
      public:
        StreamReader(std::vector<catena::PushUpdates>* outVals_, grpc::Status* outRc_, uint32_t reads = 0)
            : outVals_(outVals_), outRc_(outRc_), reads_{reads} {}
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
                std::cerr<<"Reading"<<std::endl;
                outVals_->emplace_back(outVal_);
                currentRead_++;
                if (currentRead_ == reads_) {
                    std::cerr<<"Done"<<std::endl;
                    done_ = true;
                    cv_.notify_one();
                }
                StartRead(&outVal_);
            }
        }
        /*
         * Triggers when the RPC is finished and notifies Await().
         */
        void OnDone(const grpc::Status& status) override {
            *outRc_ = status;
            if (!done_) {
                done_ = true;
                cv_.notify_one();
            }
        }
        /*
         * Blocks until the RPC is finished. 
         */
        inline void Await() {
            std::cerr<<"Awaiting..."<<std::endl;
            cv_.wait(lock_, [this] { return done_; });
        }
        
      private:
        // Pointers to the output variables.
        grpc::Status* outRc_;
        std::vector<catena::PushUpdates>* outVals_;

        uint32_t reads_ = 0;
        uint32_t currentRead_ = 0;

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
        std::cerr<<"Expecting pushUpdate"<<std::endl;
        // Add PushUpdate.
        expVals_.push_back(catena::PushUpdates());
        auto pushUpdate = expVals_.back();
        pushUpdate.set_slot(slot);
        // Set value.
        pushUpdate.mutable_value()->set_oid(oid);
        pushUpdate.mutable_value()->mutable_value()->set_string_value(stringVal);
    }
    /*
     * Adds a LanguagePack to the expected values.
     */
    void expLanguage(uint32_t slot, const std::string& oid, const std::string& language, const std::unordered_map<std::string, std::string>& words) {
        // Add pushUpdate.
        expVals_.push_back(catena::PushUpdates());
        auto pushUpdate = expVals_.back();
        pushUpdate.set_slot(slot);
        // Set language pack
        auto langComponent = pushUpdate.mutable_device_component()->mutable_language_pack();
        langComponent->set_language(language);
        langComponent->mutable_language_pack()->set_name(language);
        for (auto& [key, value] : words) {
            (*langComponent->mutable_language_pack()->mutable_words())[key] = value;
        }
    }
    void connect(uint32_t reads = 0) {
        std::thread clientThread([this, reads]{
            // Sending async RPC.
            streamReader_.reset(new StreamReader(&outVals_, &outRc_, reads));
            streamReader_->MakeCall(client_.get(), &clientContext_, &inVal_);
            std::cerr<<"Awaiting"<<std::endl;
            streamReader_->Await();
        });
        clientThread.join();
    }
    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Comparing the results.
        std::cerr<<"Comparing"<<std::endl;
        ASSERT_EQ(outVals_.size(), expVals_.size()) << "Output missing >= 1 PushUpdate";
        for (uint32_t i = 0; i < outVals_.size(); i++) {
            EXPECT_EQ(outVals_[i].SerializeAsString(), expVals_[i].SerializeAsString());
        }
        // Make sure another Command handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
        std::cerr<<"Done with RPC"<<std::endl;
    }

    ~gRPCConnectTests() {
        std::cerr<<"Destructing"<<std::endl;
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
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

TEST_F(gRPCConnectTests, Connect_ValueSetByClient) {
    MockParam param0;
    MockParam param1;
    std::cerr<<"Setting payload and expectations"<<std::endl;
    initPayload("en", catena::Device_DetailLevel_FULL, "test_user_agent", false);
    expPushValue(0, "test_param_0", "test_value_0");
    // expPushValue(1, "test_param_1", "test_value_1");
    // Setting expectations
    // EXPECT_CALL(param0, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
    //     .WillOnce([](catena::Value& dst, Authorizer&) {
    //         dst.set_string_value("test_value_0");
    //         return catena::exception_with_status("", catena::StatusCode::OK);
    //     });
    // EXPECT_CALL(param1, toProto(testing::An<catena::Value&>(), testing::_)).Times(1)
    //     .WillOnce([](catena::Value& dst, Authorizer&) {
    //         dst.set_string_value("test_value_1");
    //         return catena::exception_with_status("", catena::StatusCode::OK);
    //     });
    std::cerr<<"Connecting"<<std::endl;
    connect(1); // Connecting to the service.
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for the RPC to start.
    // std::cerr<<"Sending sig 0"<<std::endl;
    // dm0_.valueSetByClient.emit("test_param_0", &param0);
    // std::cerr<<"Sending sig 1"<<std::endl;
    // dm1_.valueSetByClient.emit("test_param_1", &param1);
    // testRPC();
}

// TEST_F(gRPCConnectTests, Connect_ValueSetByServer) {
    
// }

// TEST_F(gRPCConnectTests, Connect_LanguageAddedPushUpdate) {
    
// }
