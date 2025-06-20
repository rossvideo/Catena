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
 * @brief This file is for testing the MultiSetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/14
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"

// REST
#include "controllers/MultiSetValue.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTMultiSetValueTests : public RESTEndpointTest {
  protected:
    /*
     * Creates a MultiSetValue handler object.
     */
    ICallData* makeOne() override { return MultiSetValue::makeOne(serverSocket, context_, dm0_); }

    /*
     * Streamlines the creation of MultiSetValuePayload. 
     */
    void initPayload(uint32_t slot, const std::vector<std::pair<std::string, std::string>>& setValues) {
        slot_ = slot;
        for (auto& [oid, value] : setValues) {
            auto addedVal = inVal_.add_values();
            addedVal->set_oid(oid);
            addedVal->mutable_value()->set_string_value(value);
        }
        auto status = google::protobuf::util::MessageToJsonString(inVal_, &jsonBody_);
        EXPECT_TRUE(status.ok()) << "Failed to convert input value to JSON";
    }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        EXPECT_EQ(readResponse(), expectedResponse(expRc_));
    }

    // in vals
    catena::MultiSetValuePayload inVal_;
};

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a MultiSetValue object.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_Create) {
    EXPECT_TRUE(endpoint_);
}

/* 
 * TEST 2 - Writing to console with MultiSetValue finish().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_Finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("MultiSetValue[1] finished\n") != std::string::npos);
}

/*
 * TEST 3 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_Normal) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Sending the call
    testCall();
}

/*
 * TEST 4 - MultiSetValue with authz on and valid token.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_AuthzValid) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    // Adding authorization mockToken metadata. This it a random RSA token.
    authzEnabled_ = true;
    jwsToken_ = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &catena::common::Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Sending the call
    testCall();
}

/*
 * TEST 5 - MultiSetValue with authz on and invalid token.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_AuthzInvalid) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled_ = true;
    jwsToken_ = "Bearer THIS SHOULD NOT PARSE";
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the call
    testCall();
}

/*
 * TEST 6 - MultiSetValue fails to parse the json body.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_FailParse) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
    jsonBody_ = "Not a JSON string";
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    // Sending the call
    testCall();
}

/*
 * TEST 7 - dm.trySetValue returns a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrTryReturnCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Setting the output status and returning false.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return false;
        }));
    // Sending the call
    testCall();
}

/*
 * TEST 8 - dm.trySetValue throws a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrTryThrowCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Throwing error and returning true.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    // Sending the call
    testCall();
}

/*
 * TEST 9 - dm.trySetValue throws a std::runtime_error.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrTryThrowUnknown) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    // Sending the call
    testCall();
}

/*
 * TEST 10 - dm.commitSetValue returns a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrCommitReturnCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Returning error status.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Sending the call
    testCall();
}

/*
 * TEST 11 - dm.commitSetValue throws a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrCommitThrowCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([this](catena::MultiSetValuePayload src, catena::common::Authorizer &authz) {
            // Throwing error and returning ok.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Sending the call
    testCall();
}

/*
 * TEST 12 - dm.commitSetValue throws a std::runtime_error.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrCommitThrowUnknown) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Throw(std::runtime_error(expRc_.what())));
    // Sending the call
    testCall();
}
