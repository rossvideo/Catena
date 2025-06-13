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
 * @brief This file is for testing the LanguagePack.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/06/12
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
#include "RESTTest.h"
#include "MockSocketReader.h"
#include "MockDevice.h"

// REST
#include "controllers/LanguagePack.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTLanguagePackTests : public ::testing::Test, public RESTTest {
  protected:
    RESTLanguagePackTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        ON_CALL(context, slot()).WillByDefault(::testing::Return(0));
        ON_CALL(context, fqoid()).WillByDefault(::testing::ReturnRefOfCopy("/" + testLanguage));
        ON_CALL(context, method()).WillByDefault(::testing::ReturnRef(testMethod));
        ON_CALL(context, authorizationEnabled()).WillByDefault(::testing::Return(false));
        ON_CALL(context, jwsToken()).WillByDefault(::testing::ReturnRef(testToken));
        ON_CALL(context, jsonBody()).WillByDefault(::testing::ReturnRef(testJsonBody));
        ON_CALL(dm, mutex()).WillByDefault(::testing::ReturnRef(mockMutex));

        // Creating LanguagePack object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        languagePack = LanguagePack::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (languagePack) {
            delete languagePack;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    std::string testMethod = "GET";
    std::string testLanguage = "en";
    std::string testToken = "";
    std::string testJsonBody;
    
    MockSocketReader context;
    MockDevice dm;
    std::mutex mockMutex;
    catena::REST::ICallData* languagePack = nullptr;
};

/*
 * TEST 0.1 - Creating a LanguagePack object with makeOne.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_Create) {
    // Making sure languagePack is created from the SetUp step.
    ASSERT_TRUE(languagePack);
}

/* 
 * TEST 0.2 - Writing to console with LanguagePack finish().
 */
TEST_F(RESTLanguagePackTests, LanguagePack_Finish) {
    // Calling finish and expecting the console output.
    languagePack->finish();
    ASSERT_TRUE(MockConsole.str().find("LanguagePack[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - LanguagePack proceed() with an invalid method.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_BadMethod) {
    catena::exception_with_status rc("Bad method", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "BAD_METHOD";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    // Should not call any of these on a bad method.
    EXPECT_CALL(dm, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm, removeLanguage(::testing::_, ::testing::_)).Times(0);
    
    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              GET LanguagePack tests
 * ============================================================================
 * 
 * TEST 1.1 - GET LangaugePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GetNormal) {
    catena::DeviceComponent_ComponentLanguagePack returnVal;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    returnVal.set_language(testLanguage);
    catena::LanguagePack *pack = returnVal.mutable_language_pack();
    pack->set_name("English");
    pack->mutable_words()->insert({"Hello", "Goodbye"});

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(0); // Should not call on GET.
    EXPECT_CALL(context, jwsToken()).Times(0); // Should not call on GET.
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, &returnVal](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            pack.CopyFrom(returnVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();

    auto status = google::protobuf::util::MessageToJsonString(returnVal, &testJsonBody);
    EXPECT_EQ(readResponse(), expectedResponse(rc, testJsonBody));
}

/* 
 * TEST 1.2 - GET LanguagePack dm.getLanguagePack() returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GetErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);

    // Defining mock fuctions
    EXPECT_CALL(context, authorizationEnabled()).Times(0); // Should not call on GET.
    EXPECT_CALL(context, jwsToken()).Times(0); // Should not call on GET.
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.3 - GET LanguagePack dm.getLanguagePack() throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GetErrThrowCatena) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);

    // Defining mock fuctions
    EXPECT_CALL(context, authorizationEnabled()).Times(0); // Should not call on GET.
    EXPECT_CALL(context, jwsToken()).Times(0); // Should not call on GET.
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.4 - GET LanguagePack dm.getLanguagePack() throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GetErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Defining mock fuctions
    EXPECT_CALL(context, authorizationEnabled()).Times(0); // Should not call on GET.
    EXPECT_CALL(context, jwsToken()).Times(0); // Should not call on GET.
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              POST LanguagePack tests
 * ============================================================================
 * 
 * TEST 2.1 - POST LanguagePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "POST";
    catena::LanguagePack pack;
    pack.set_name("English");
    pack.mutable_words()->insert({"Hello", "Goodbye"});
    auto status = google::protobuf::util::MessageToJsonString(pack, &testJsonBody);

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, &pack, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testLanguage);
            EXPECT_EQ(language.language_pack().SerializeAsString(), pack.SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.2 - POST LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "POST";
    catena::LanguagePack pack;
    pack.set_name("English");
    pack.mutable_words()->insert({"Hello", "Goodbye"});
    auto status = google::protobuf::util::MessageToJsonString(pack, &testJsonBody);
    testToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, &pack, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testLanguage);
            EXPECT_EQ(language.language_pack().SerializeAsString(), pack.SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.3 - POST LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testMethod = "POST";
    testToken = "Invalid token";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1);
    EXPECT_CALL(context, jsonBody()).Times(0);
    EXPECT_CALL(dm, hasLanguage(::testing::_)).Times(0);
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.4 - POST LanguagePack with invalid json.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostFailParse) {
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "POST";
    testJsonBody = "Not a JSON string";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.5 - POST LanguagePack overwrite error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostOverwrite) {
    catena::exception_with_status rc("", catena::StatusCode::PERMISSION_DENIED);
    testMethod = "POST";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.6 - POST LanguagePack returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "POST";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.7 - POST LanguagePack throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostErrThrowCatena) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "POST";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.8 - POST LanguagePack returns a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PostErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testMethod = "POST";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              PUT LanguagePack tests
 * ============================================================================
 * 
 * TEST 3.1 - PUT LanguagePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "PUT";
    catena::LanguagePack pack;
    pack.set_name("English");
    pack.mutable_words()->insert({"Hello", "Goodbye"});
    auto status = google::protobuf::util::MessageToJsonString(pack, &testJsonBody);

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, &pack, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testLanguage);
            EXPECT_EQ(language.language_pack().SerializeAsString(), pack.SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.2 - PUT LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "PUT";
    catena::LanguagePack pack;
    pack.set_name("English");
    pack.mutable_words()->insert({"Hello", "Goodbye"});
    auto status = google::protobuf::util::MessageToJsonString(pack, &testJsonBody);
    testToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, &pack, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testLanguage);
            EXPECT_EQ(language.language_pack().SerializeAsString(), pack.SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.3 - PUT LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testMethod = "PUT";
    testToken = "Invalid token";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1);
    EXPECT_CALL(context, jsonBody()).Times(0);
    EXPECT_CALL(dm, hasLanguage(::testing::_)).Times(0);
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.4 - PUT LanguagePack with invalid json.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutFailParse) {
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "PUT";
    testJsonBody = "Not a JSON string";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.5 - PUT LanguagePack add error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutOverwrite) {
    catena::exception_with_status rc("", catena::StatusCode::PERMISSION_DENIED);
    testMethod = "PUT";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.6 - PUT LanguagePack returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "PUT";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.7 - PUT LanguagePack throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutErrThrowCatena) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "PUT";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.8 - PUT LanguagePack returns a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PutErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testMethod = "PUT";
    testJsonBody = "{}";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(context, jsonBody()).Times(1);
    EXPECT_CALL(dm, hasLanguage(testLanguage)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              DELETE LanguagePack tests
 * ============================================================================
 * 
 * TEST 4.1 - DELETE LangaugePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DeleteNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "DELETE";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.2 - PUT LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DeleteAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "DELETE";
    testToken = "eyJhbGciOiJSUzI1NiIsInR5cCI6ImF0K2p3dCJ9.eyJzdWIiOiIxMjM0NTY3"
                "ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwic2NvcGUiOiJzdDIxMzg6bW9uOncgc"
                "3QyMTM4Om9wOncgc3QyMTM4OmNmZzp3IHN0MjEzODphZG06dyIsImlhdCI6MT"
                "UxNjIzOTAyMiwibmJmIjoxNzQwMDAwMDAwLCJleHAiOjE3NTAwMDAwMDB9.dT"
                "okrEPi_kyety6KCsfJdqHMbYkFljL0KUkokutXg4HN288Ko9653v0khyUT4UK"
                "eOMGJsitMaSS0uLf_Zc-JaVMDJzR-0k7jjkiKHkWi4P3-CYWrwe-g6b4-a33Q"
                "0k6tSGI1hGf2bA9cRYr-VyQ_T3RQyHgGb8vSsOql8hRfwqgvcldHIXjfT5wEm"
                "uIwNOVM3EcVEaLyISFj8L4IDNiarVD6b1x8OXrL4vrGvzesaCeRwP8bxg4zlg"
                "_wbOSA8JaupX9NvB4qssZpyp_20uHGh8h_VC10R0k9NKHURjs9MdvJH-cx1s1"
                "46M27UmngWUCWH6dWHaT2au9en2zSFrcWHw";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1);
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.3 - DELETE LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DeleteAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testMethod = "DELETE";
    testToken = "Invalid token";

    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(context, jwsToken()).Times(1);
    EXPECT_CALL(context, jsonBody()).Times(0);
    EXPECT_CALL(dm, removeLanguage(::testing::_, ::testing::_)).Times(0);

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.4 - DELETE LanguagePack dm.getLanguagePack() returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DeleteErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "DELETE";

    // Defining mock fuctions
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.5 - DELETE LanguagePack dm.getLanguagePack() throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DeleteErrThrowCatena) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "DELETE";

    // Defining mock fuctions
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.6 - DELETE LanguagePack dm.getLanguagePack() throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DeleteErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testMethod = "DELETE";

    // Defining mock fuctions
    EXPECT_CALL(context, authorizationEnabled()).Times(1);
    EXPECT_CALL(context, jwsToken()).Times(0);
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));

    // Calling proceed() and checking written response.
    languagePack->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}
