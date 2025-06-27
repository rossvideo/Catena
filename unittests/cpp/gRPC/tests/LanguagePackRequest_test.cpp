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
 * @brief This file is for testing the LanguagePackRequest.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"

// gRPC
#include "controllers/LanguagePackRequest.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCLanguagePackRequestTests : public GRPCTest {
  protected:
    /*
     * Creates a LanguagePackRequest handler object.
     */
    void makeOne() override { new LanguagePackRequest(&service_, dms_, true); }

    /*
     * Helper function which initializes a LanguagePackRequestPayload object.
     */
    void initPayload(uint32_t slot, const std::string& language) {
        inVal_.set_slot(slot);
        inVal_.set_language(language);
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client_->async()->LanguagePackRequest(&clientContext_, &inVal_, &outVal_, [this](grpc::Status status){
            outRc_ = status;
            done_ = true;
            cv_.notify_one();
        });
        cv_.wait(lock_, [this] { return done_; });
        // Comparing the results.
        EXPECT_EQ(outVal_.SerializeAsString(), expVal_.SerializeAsString());
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another LanguagePackRequest handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::LanguagePackRequestPayload inVal_;
    catena::DeviceComponent_ComponentLanguagePack outVal_;
    // Expected variables
    catena::DeviceComponent_ComponentLanguagePack expVal_;
};

/*
 * ============================================================================
 *                               LanguagePackRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating a LanguagePackRequest object.
 */
TEST_F(gRPCLanguagePackRequestTests, LanguagePackRequest_Create) {
    EXPECT_TRUE(asyncCall_);
}

/* 
 * TEST 2 - Normal case for LanguagePackRequest proceed().
 */
TEST_F(gRPCLanguagePackRequestTests, LanguagePackRequest_Normal) {
    initPayload(0, "en");
    expVal_.set_language(inVal_.language());
    auto languagePack = expVal_.mutable_language_pack();
    languagePack->set_name("English");
    (*languagePack->mutable_words())["greeting"] = "Hello";
    // Setting expecteations
    EXPECT_CALL(dm0_, getLanguagePack(inVal_.language(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack){
            pack.CopyFrom(expVal_);
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    EXPECT_CALL(dm1_, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/* 
 * TEST 3 - No device in the specified slot.
 */
TEST_F(gRPCLanguagePackRequestTests, LanguagePackRequest_ErrInvalidSlot) {
    initPayload(dms_.size(), "en");
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);
    // Setting expecteations
    EXPECT_CALL(dm0_, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/* 
 * TEST 4 - dm.getLanguagePack returns a catena::Exception_With_Status.
 */
TEST_F(gRPCLanguagePackRequestTests, LanguagePackRequest_ErrReturn) {
    expRc_ = catena::exception_with_status("Language pack en not found", catena::StatusCode::NOT_FOUND);    
    // Setting expecteations
    EXPECT_CALL(dm0_, getLanguagePack(inVal_.language(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack){
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    EXPECT_CALL(dm1_, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/* 
 * TEST 5 - dm.getLanguagePack throws a catena::Exception_With_Status.
 */
TEST_F(gRPCLanguagePackRequestTests, LanguagePackRequest_ErrThrow) {
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expecteations
    EXPECT_CALL(dm0_, getLanguagePack(inVal_.language(), ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack){
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    EXPECT_CALL(dm1_, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}
