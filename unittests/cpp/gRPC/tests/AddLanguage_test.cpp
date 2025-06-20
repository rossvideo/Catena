/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in souexpRce and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of souexpRce code must retain the above copyright notice,
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
 * @brief This file is for testing the AddLanguage.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/18
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "GRPCTest.h"

// gRPC
#include "controllers/AddLanguage.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCAddLanguageTests : public GRPCTest {
  protected:
    /*
     * Creates an AddLangauge handler object.
     */
    void makeOne() override { new AddLanguage(&service, dm, true); }

    /*
     * Helper function which initializes an AddLanguagePayload object.
     */
    void initPayload(uint32_t slot, const std::string& id, const std::string& name, const std::unordered_map<std::string, std::string>& words) {
        inVal.set_slot(slot);
        inVal.set_id(id);
        auto pack = inVal.mutable_language_pack();
        pack->set_name(name);
        for (const auto& word : words) {
            (*pack->mutable_words())[word.first] = word.second;
        }
    }

    /* 
    * Makes an async RPC to the MockServer and waits for a response before
    * comparing output.
    */
    void testRPC() {
        // Sending async RPC.
        client->async()->AddLanguage(&clientContext, &inVal, &outVal, [this](grpc::Status status){
            outRc = status;
            done = true;
            cv.notify_one();
        });
        cv.wait(lock, [this] { return done; });
        // Comparing the results.
        EXPECT_EQ(outVal.SerializeAsString(), expVal.SerializeAsString());
        EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
        EXPECT_EQ(outRc.error_message(), expRc.what());
        // Make sure another AddLanguage handler was created.
        EXPECT_TRUE(asyncCall) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::AddLanguagePayload inVal;
    catena::Empty outVal;
    // Expected variables
    catena::Empty expVal;
};

/*
 * ============================================================================
 *                               AddLanguage tests
 * ============================================================================
 * 
 * TEST 1 - Creating a AddLanguage object.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_Create) {
    EXPECT_TRUE(asyncCall);
}

/*
 * TEST 2 - Normal case for AddLanguage proceed().
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_Normal) {
    initPayload(1, "en", "English", {{"greeting", "Hello"}});
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(language.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 3 - AddLanguage with authz on and valid token.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_AuthzValid) {
    initPayload(1, "en", "English", {{"greeting", "Hello"}});
    // Adding authorization mockToken metadata. This it a random RSA token.
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
    // Mocking kProcess and kFinish functions
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(language.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 4 - AddLanguage with authz on and invalid token.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_AuthzInvalid) {
    expRc = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 5 - AddLanguage with authz on and invalid token.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_AuthzJWSNotFound) {
    expRc = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token.
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - dm.addLanguage() returns a catena::exception_with_status.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_ErrReturnCatena) {
    expRc = catena::exception_with_status("Language already exists", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 7 - dm.addLanguage() throws a catena::exception_with_status.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_ErrThrowCatena) {
    expRc = catena::exception_with_status("Language already exists", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 8 - dm.addLanguage() throws a std::runtime_exception.
 */
TEST_F(gRPCAddLanguageTests, AddLanguage_ErrThrowUnknown) {
    expRc = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    // Sending the RPC
    testRPC();
}
