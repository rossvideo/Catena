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
 * @brief This file is for testing the ListLanguages.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"

// gRPC
#include "controllers/ListLanguages.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCListLanguagesTests : public GRPCTest {
  protected:
    /*
     * Creates a ListLanguages handler object.
     */
    void makeOne() override { new ListLanguages(&service_, dms_, true); }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client_->async()->ListLanguages(&clientContext_, &inVal_, &outVal_, [this](grpc::Status status){
            outRc_ = status;
            done_ = true;
            cv_.notify_one();
        });
        cv_.wait(lock_, [this] { return done_; });
        // Comparing the results.
        EXPECT_EQ(outVal_.SerializeAsString(), expVal_.SerializeAsString());
        EXPECT_EQ(outRc_.error_code(), static_cast<grpc::StatusCode>(expRc_.status));
        EXPECT_EQ(outRc_.error_message(), expRc_.what());
        // Make sure another ListLanguages handler was created.
        EXPECT_TRUE(asyncCall_) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::Slot inVal_;
    catena::LanguageList outVal_;
    // Expected variables
    catena::LanguageList expVal_;
};

/*
 * ============================================================================
 *                               ListLanguages tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ListLanguages object.
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_Create) {
    EXPECT_TRUE(asyncCall_);
}

/*
 * TEST 2 - Normal case for ListLanguages proceed().
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_Normal) {
    *expVal_.add_languages() = "en";
    *expVal_.add_languages() = "fr";
    *expVal_.add_languages() = "es";
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([this](catena::LanguageList &list){
            list.CopyFrom(expVal_);
        }));
    EXPECT_CALL(dm1_, toProto(::testing::An<catena::LanguageList&>())).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 2 - No device in the specified slot.
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_ErrInvalidSlot) {
    inVal_.set_slot(dms_.size());
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(dms_.size()), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(::testing::An<catena::LanguageList&>())).Times(0);
    EXPECT_CALL(dm1_, toProto(::testing::An<catena::LanguageList&>())).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 3 - dm.toProto() throws a catena::exception_with_status.
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_Err) {
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([this](catena::LanguageList &list){
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    EXPECT_CALL(dm1_, toProto(::testing::An<catena::LanguageList&>())).Times(0);
    // Sending the RPC.
    testRPC();
}
