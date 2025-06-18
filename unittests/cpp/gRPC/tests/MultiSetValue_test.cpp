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
 * @brief This file is for testing the MultiSetValue.cpp file.
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
#include "controllers/MultiSetValue.h"

using namespace catena::common;
using namespace catena::gRPC;

// Fixture
class gRPCMultiSetValueTests : public GRPCTest {
  protected:
    /*
     * Creates a MultiSetValue handler object.
     */
    void makeOne() override { new MultiSetValue(&service, dm, true); }

    /*
     * Helper function which initializes a MultiSetValuePayload object.
     */
    void initPayload(uint32_t slot, const std::vector<std::pair<std::string, std::string>>& setValues) {
        inVal.set_slot(slot);
        for (const auto& [oid, value] : setValues) {
            auto addedVal = inVal.mutable_values()->Add();
            addedVal->set_oid(oid);
            addedVal->mutable_value()->set_string_value(value);
        }
    }

    /* 
     * Makes an async RPC to the MockServer and waits for a response before
     * comparing output.
     */
    void testRPC() {
        // Sending async RPC.
        client->async()->MultiSetValue(&clientContext, &inVal, &outVal, [this](grpc::Status status){
            outRc = status;
            done = true;
            cv.notify_one();
        });
        cv.wait(lock, [this] { return done; });
        // Comparing the results.
        EXPECT_EQ(outVal.SerializeAsString(), expVal.SerializeAsString());
        EXPECT_EQ(outRc.error_code(), static_cast<grpc::StatusCode>(expRc.status));
        EXPECT_EQ(outRc.error_message(), expRc.what());
        // Make sure another MultiSetValue handler was created.
        EXPECT_TRUE(asyncCall) << "Async handler was not created during runtime";
    }

    // in/out val
    catena::MultiSetValuePayload inVal;
    catena::Empty outVal;
    // Expected variables
    catena::Empty expVal;
};

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a MultiSetValue object.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_Create) {
    EXPECT_TRUE(asyncCall);
}

/*
 * TEST 2 - Normal case for MultiSetValue proceed().
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_Normal) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc = catena::exception_with_status("", catena::StatusCode::OK);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc.what(), expRc.status);
            return true;
        }));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 3 - MultiSetValue with authz on and valid token.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_AuthzValid) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc = catena::exception_with_status("", catena::StatusCode::OK);
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
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc.what(), expRc.status);
            return true;
        }));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal.SerializeAsString());
            EXPECT_EQ(!authzEnabled, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 4 - MultiSetValue with authz on and invalid token.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_AuthzInvalid) {
    expRc = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "Bearer THIS SHOULD NOT PARSE");
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC.
    testRPC();
}

/*
 * TEST 5 - MultiSetValue with authz on and invalid token.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_AuthzJWSNotFound) {
    expRc = catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    // Should not be able to find the bearer token.
    authzEnabled = true;
    clientContext.AddMetadata("authorization", "NOT A BEARER TOKEN");
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - dm.trySetValue returns a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrTryReturnCatena) {
    expRc = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Setting the output status and returning false.
            ans = catena::exception_with_status(expRc.what(), expRc.status);
            return false;
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 6 - dm.trySetValue throws a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrTryThrowCatena) {
    expRc = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Throwing error and returning true.
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return true;
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 7 - dm.trySetValue throws a std::runtime_error.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrTryThrowUnknown) {
    expRc = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 8 - dm.commitSetValue returns a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrCommitReturnCatena) {
    expRc = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Returning error status.
            return catena::exception_with_status(expRc.what(), expRc.status);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 9 - dm.commitSetValue throws a catena::Exception_With_Status.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrCommitThrowCatena) {
    expRc = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Throwing error and returning ok.
            throw catena::exception_with_status(expRc.what(), expRc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Sending the RPC
    testRPC();
}

/*
 * TEST 10 - dm.commitSetValue throws a std::runtime_error.
 */
TEST_F(gRPCMultiSetValueTests, MultiSetValue_ErrCommitThrowUnknown) {
    expRc = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc.what())));
    // Sending the RPC
    testRPC();
}
