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
 * @date 25/05/12
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
#include "SocketHelper.h"
#include "RESTMockClasses.h"
#include "../../common/tests/CommonMockClasses.h"

// REST
#include "controllers/MultiSetValue.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTMultiSetValueTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTMultiSetValueTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating multiSetValue object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        multiSetValue = MultiSetValue::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (multiSetValue) {
            delete multiSetValue;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* multiSetValue = nullptr;

    std::mutex mockMutex;
    std::string jsonBody = "{\"values\":["
                           "{\"oid\":\"/text_box\",\"value\":{\"string_value\":\"test value 1\"}},"
                           "{\"oid\":\"/text_box\",\"value\":{\"string_value\":\"test value 2\"}}"
                           "]}";
};

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a MultiSetValue object with makeOne.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_create) {
    // Making sure multiSetValue is created from the SetUp step.
    ASSERT_TRUE(multiSetValue);
}

/* 
 * TEST 2 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(catena::exception_with_status(rc.what(), rc.status)));

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3 - trySetValue returns an error.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedTryErr) {
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::MultiSetValuePayload src, catena::exception_with_status &ans, Authorizer &authz) -> bool {
            ans = catena::exception_with_status(rc.what(), rc.status);
            return false;
        }));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - trySetValue throws a catena::exception_with_status.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedTryThrowCatena) {
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::MultiSetValuePayload src, catena::exception_with_status &ans, Authorizer &authz) -> bool {
            throw catena::exception_with_status(rc.what(), rc.status);
            return true;
        }));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 5 - trySetValue throws an std::runtime_error.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedTryThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::MultiSetValuePayload src, catena::exception_with_status &ans, Authorizer &authz) -> bool {
            throw std::runtime_error(rc.what());
            return true;
        }));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 6 - commitSetValue returns an error.
 * This should not be possible in a normal run.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedCommitErr) {
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(catena::exception_with_status(rc.what(), rc.status)));

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 7 - commitSetValue throws a catena::exception_with_status.
 * This should not be possible in a normal run.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedCommitThrowCatena) {
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::MultiSetValuePayload src, Authorizer &authz) -> catena::exception_with_status {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 8 - commitSetValue throws an std::runtime_error.
 * This should not be possible in a normal run.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedCommitThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::MultiSetValuePayload src, Authorizer &authz) -> catena::exception_with_status {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK);
        }));

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 9 - MultiSetValue with authz on and an valid token.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    /* Authz just tests for a properly encrypted token, proxy handles authz.
     * This is a random RSA token I made jwt.io it is not a security risk I
     * swear. */
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

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(catena::exception_with_status(rc.what(), rc.status)));

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 10 - MultiSetValue with authz on and an invalid token.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    // Not a token so it should get rejected by the authorizer.
    std::string mockToken = "THIS SHOULD NOT PARSE";

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
    EXPECT_CALL(dm, mutex()).Times(0);
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(0);

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 11 - MultiSetValue toMulti fails to parse the JSON.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedFailParse) {
    catena::exception_with_status rc("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
    jsonBody = "Not a JSON string";

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(0);

    multiSetValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 12 - Writing to console with MultiSetValue finish().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_finish) {
    // Calling finish and expecting the console output.
    multiSetValue->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("MultiSetValue[11] finished\n") != std::string::npos);
}
