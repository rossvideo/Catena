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
#include "controllers/ListLanguages.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCListLanguagesTests : public GRPCTest {
  protected:
    /*
     * Creates a ListLanguages handler object.
     */
    void makeOne() override { new ListLanguages(&service, dms, true); }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client->async()->ListLanguages(&clientContext, &inVal, &outVal, [this](grpc::Status status){
            outRc = status;
            done = true;
            cv.notify_one();
        });
        cv.wait(lock, [this] { return done; });
        // Comparing the results.
        EXPECT_EQ(outVal.SerializeAsString(), expVal.SerializeAsString());
        EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
        EXPECT_EQ(outRc.error_message(), expRc.what());
        // Make sure another ListLanguages handler was created.
        EXPECT_TRUE(asyncCall) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::Slot inVal;
    catena::LanguageList outVal;
    // Expected variables
    catena::LanguageList expVal;
};

/*
 * ============================================================================
 *                               ListLanguages tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ListLanguages object.
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_Create) {
    EXPECT_TRUE(asyncCall);
}

/*
 * TEST 2 - Normal case for ListLanguages proceed().
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_Normal) {
    *expVal.add_languages() = "en";
    *expVal.add_languages() = "fr";
    *expVal.add_languages() = "es";
    // Setting expectations
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([this](catena::LanguageList &list){
            list.CopyFrom(expVal);
        }));
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 2 - Normal case for ListLanguages proceed().
 */
TEST_F(gRPCListLanguagesTests, ListLanguages_Err) {
    expRc = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([this](catena::LanguageList &list){
            throw catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC.
    testRPC();
}
