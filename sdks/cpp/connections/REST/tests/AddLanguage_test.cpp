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
 * @brief This file is for testing the AddLanguage.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/13
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
#include "MockClasses.h"

// REST
#include "controllers/AddLanguage.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTAddLanguageTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTAddLanguageTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating AddLanguage object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        addLanguage = AddLanguage::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (addLanguage) {
            delete addLanguage;
        }
    }

    // A collection of calls expected in every proceed test save for when json
    // fails to parse.
    void expectedProceedCalls() {
        EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
        EXPECT_CALL(context, fields("id")).Times(1).WillOnce(::testing::ReturnRef(language));
        EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
        EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(authz));
        if (authz) {
            EXPECT_CALL(context, jwsToken()).Times(1).WillOnce(::testing::ReturnRef(mockToken));
        }
    }

    std::stringstream MockConsole;
    std::streambuf* oldCout;

    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* addLanguage = nullptr;

    // Test Values
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
    std::string language = "en";
    std::mutex mockMutex;
    std::string jsonBody = "{\"name\":\"English\",\"words\":{\"greeting\":\"Hello\",\"parting\":\"Goodbye\"}}";
    bool authz = false;
};

/*
 * ============================================================================
 *                               AddLanguage tests
 * ============================================================================
 * 
 * TEST 1 - Creating a AddLanguage object with makeOne.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_create) {
    // Making sure addLanguage is created from the SetUp step.
    ASSERT_TRUE(addLanguage);
}

/* 
 * TEST 2 - Normal case for AddLanguage proceed() without authz.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedNormal) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("", catena::StatusCode::OK);

    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3 - dm.addLangauge() returns an error.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedErrReturn) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Invalid language pack", catena::StatusCode::INVALID_ARGUMENT);
    
    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - dm.addLangauge() throws a catena error.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedErrThrowCatena) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Invalid language pack", catena::StatusCode::INVALID_ARGUMENT);
    
    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // Here to ignore errors.
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 5 - dm.addLangauge() throws an std error.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedErrThrowUnknown) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // Here to ignore errors.
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 6 - Normal case for AddLanguage proceed() with authz.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedAuthzValid) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    authz = true;

    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 7 - Normal case for AddLanguage proceed() with authz.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedAuthzInvalid) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    authz = true;
    // Not a token so it should get rejected by the authorizer.
    mockToken = "THIS SHOULD NOT PARSE";

    // Defining mock fuctions
    expectedProceedCalls();

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 8 - dm.addLangauge() returns an error with authz.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedAuthzErrReturn) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Invalid language pack", catena::StatusCode::INVALID_ARGUMENT);
    authz = true;

    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 9 - dm.addLangauge() throws a catena error with authz.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedAuthzErrThrowCatena) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Invalid language pack", catena::StatusCode::INVALID_ARGUMENT);
    authz = true;

    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // Here to ignore errors.
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 10 - dm.addLangauge() throws an std error with authz.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedAuthzErrThrowUnkown) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    authz = true;

    // Defining mock fuctions
    expectedProceedCalls();
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, addLanguage(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // Here to ignore errors.
        }));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 11 - AddLanguage->proceed() fails to parse json body.
 */
TEST_F(RESTAddLanguageTests, AddLanguage_proceedFailParse) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
    jsonBody = "THIS IS NOT JSON";

    // Defining mock functions.
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, fields("id")).Times(1).WillOnce(::testing::ReturnRef(language));
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));

    // Calling proceed() and checking written response.
    addLanguage->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 12 - Writing to console with AddLanguage finish().
 */
TEST_F(RESTAddLanguageTests, AddLanguage_finish) {
    // Calling finish and expecting the console output.
    addLanguage->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("AddLanguage[11] finished\n") != std::string::npos);
}
