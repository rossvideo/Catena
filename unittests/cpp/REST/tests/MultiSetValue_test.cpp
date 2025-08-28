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
#include "CommonTestHelpers.h"

// REST
#include "controllers/MultiSetValue.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTMultiSetValueTests : public RESTEndpointTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTMultiSetValueTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
  
    RESTMultiSetValueTests() : RESTEndpointTest() {
        // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm1_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(0);
        EXPECT_CALL(dm1_, commitMultiSetValue(testing::_, testing::_)).Times(0);
        
        // Set up default JWS token for tests
        jwsToken_ = getJwsToken(Scopes().getForwardMap().at(Scopes_e::kMonitor) + ":w");
    }

    /*
     * Creates a MultiSetValue handler object.
     */
    ICallData* makeOne() override { return MultiSetValue::makeOne(serverSocket_, context_, dms_); }

    /*
     * Streamlines the creation of endpoint input. 
     */
    void initPayload(uint32_t slot, const std::vector<std::pair<std::string, std::string>>& setValues) {
        slot_ = slot;
        for (auto& [oid, value] : setValues) {
            auto addedVal = inVal_.add_values();
            addedVal->set_oid(oid);
            addedVal->mutable_value()->set_string_value(value);
        }
        auto status = google::protobuf::util::MessageToJsonString(inVal_, &jsonBody_);
        ASSERT_TRUE(status.ok()) << "Failed to convert input value to JSON";
    }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        EXPECT_EQ(readResponse(), expectedResponse(expRc_));
    }

    // in vals
    st2138::MultiSetValuePayload inVal_;
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
 * TEST 2 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_Normal) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, catena::exception_with_status &ans, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 3 - MultiSetValue with authz on and valid token.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_AuthzValid) {
    initPayload(0, {{"/test_oid_1", "test_value_1"},{"/test_oid_2", "test_value_2"}});
    expRc_ = catena::exception_with_status("", catena::StatusCode::OK);
    authzEnabled_ = true;
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, catena::exception_with_status &ans, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, const IAuthorizer &authz) {
            // Checking that function gets correct inputs.
            EXPECT_EQ(src.SerializeAsString(), inVal_.SerializeAsString());
            EXPECT_EQ(!authzEnabled_, &authz == &Authorizer::kAuthzDisabled);
            // Setting the output status and returning true.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 4 - MultiSetValue with authz on and invalid token.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_AuthzInvalid) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid JWS Token", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    authzEnabled_ = true;
    jwsToken_ = "Bearer THIS SHOULD NOT PARSE";
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 5 - No device in the specified slot.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrInvalidSlot) {
    initPayload(dms_.size(), {});
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(slot_), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm1_, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);
    // Calling proceed and testing the output
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
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 7 - dm.trySetValue returns a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrTryReturnCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, catena::exception_with_status &ans, const IAuthorizer &authz) {
            // Setting the output status and returning false.
            ans = catena::exception_with_status(expRc_.what(), expRc_.status);
            return false;
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 8 - dm.trySetValue throws a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrTryThrowCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, catena::exception_with_status &ans, const IAuthorizer &authz) {
            // Throwing error and returning true.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return true;
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 9 - dm.trySetValue throws a std::runtime_error.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrTryThrowUnknown) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 10 - dm.commitSetValue returns a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrCommitReturnCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, const IAuthorizer &authz) {
            // Returning error status.
            return catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 11 - dm.commitSetValue throws a catena::Exception_With_Status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrCommitThrowCatena) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](st2138::MultiSetValuePayload src, const IAuthorizer &authz) {
            // Throwing error and returning ok.
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));
    // Calling proceed and testing the output
    testCall();
}

/*
 * TEST 12 - dm.commitSetValue throws a std::runtime_error.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_ErrCommitThrowUnknown) {
    initPayload(0, {});
    expRc_ = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1).WillOnce(testing::Return(true));
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}
