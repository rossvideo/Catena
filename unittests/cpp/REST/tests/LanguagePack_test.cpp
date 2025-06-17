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
    RESTLanguagePackTests() : RESTTest(&serverSocket, &clientSocket) {}

    /*
     * Redirects cout and sets default expectations for each method.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Default expectations for the context.
        EXPECT_CALL(context, fqoid()).WillRepeatedly(::testing::ReturnRefOfCopy("/" + testLanguage));
        EXPECT_CALL(context, method()).WillRepeatedly(::testing::ReturnRef(testMethod));
        EXPECT_CALL(context, slot()).WillRepeatedly(::testing::Return(0));
        EXPECT_CALL(context, jwsToken()).WillRepeatedly(::testing::ReturnRef(testToken));
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));
        EXPECT_CALL(context, jsonBody()).WillRepeatedly(::testing::ReturnRef(testJsonBody));
        EXPECT_CALL(context, authorizationEnabled()).WillRepeatedly(::testing::Invoke([this](){ return authzEnabled; }));

        // Default expectations for the device model.
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMutex));
        EXPECT_CALL(dm, hasLanguage(testLanguage)).WillRepeatedly(::testing::Invoke([this](){ return testMethod == "PUT"; }));

        // Init testVal.
        testVal.set_language(testLanguage);
        catena::LanguagePack *pack = testVal.mutable_language_pack();
        pack->set_name("English");
        pack->mutable_words()->insert({"Hello", "Goodbye"});
        auto status = google::protobuf::util::MessageToJsonString(*pack, &testJsonBody);
        ASSERT_TRUE(status.ok());

        // Creating LanguagePack object.
        endpoint.reset(LanguagePack::makeOne(serverSocket, context, dm));
    }

    /*
     * Restores cout after each test.
     */
    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
    }

    // Cout variables.
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    // Context variables.
    std::string testMethod = "GET";
    std::string testLanguage = "en";
    std::string testToken = "";
    std::string testJsonBody;
    bool authzEnabled = false;
    // Mock objects and endpoint.
    MockSocketReader context;
    std::mutex mockMutex;
    MockDevice dm;
    std::unique_ptr<ICallData> endpoint = nullptr;
    // Test value.
    catena::DeviceComponent_ComponentLanguagePack testVal;
};

/*
 * TEST 0.1 - Creating a LanguagePack object with makeOne.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_Create) {
    // Making sure languagePack is created from the SetUp step.
    ASSERT_TRUE(endpoint);
}

/* 
 * TEST 0.2 - Writing to console with LanguagePack finish().
 */
TEST_F(RESTLanguagePackTests, LanguagePack_Finish) {
    // Calling finish and expecting the console output.
    endpoint->finish();
    ASSERT_TRUE(MockConsole.str().find("LanguagePack[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - LanguagePack proceed() with an invalid method.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_BadMethod) {
    catena::exception_with_status rc("Bad method", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "BAD_METHOD";
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0);
    // Should not call any of these on a bad method.
    EXPECT_CALL(dm, getLanguagePack(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(dm, removeLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              GET LanguagePack tests
 * ============================================================================
 * 
 * TEST 1.1 - GET LangaugePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testJsonBody.clear();
    auto status = google::protobuf::util::MessageToJsonString(testVal, &testJsonBody);
    ASSERT_TRUE(status.ok());
    // Setting expectations.
    EXPECT_CALL(context, authorizationEnabled()).Times(0); // No authz on GET.
    EXPECT_CALL(context, jwsToken()).Times(0);             // No authz on GET.
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, this](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            pack.CopyFrom(testVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc, testJsonBody));
}

/* 
 * TEST 1.2 - GET LanguagePack dm.getLanguagePack() returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    // Defining mock fuctions
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.3 - GET LanguagePack dm.getLanguagePack() throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrThrowCat) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    // Defining mock fuctions
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.4 - GET LanguagePack dm.getLanguagePack() throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrThrowStd) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::INTERNAL);
    // Defining mock fuctions
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.5 - GET LanguagePack dm.getLanguagePack() throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_GETErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    // Defining mock fuctions
    EXPECT_CALL(dm, getLanguagePack(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw 0;
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              POST LanguagePack tests
 * ============================================================================
 * 
 * TEST 2.1 - POST LanguagePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "POST";
    // Setting expectations.
    EXPECT_CALL(context, jwsToken()).Times(0); // Authz disabled.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testVal.language());
            EXPECT_EQ(language.language_pack().SerializeAsString(), testVal.language_pack().SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.2 - POST LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "POST";
    authzEnabled = true;
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
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testVal.language());
            EXPECT_EQ(language.language_pack().SerializeAsString(), testVal.language_pack().SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.3 - POST LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testMethod = "POST";
    authzEnabled = true;
    testToken = "Invalid token";
    // Setting expectations.
    // Should not call if authz fails.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.4 - POST LanguagePack with invalid json.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTFailParse) {
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "POST";
    testJsonBody = "Not a JSON string";
    // Setting expectations.
    // Should not call if json fails to parse.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.5 - POST LanguagePack overwrite error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTOverwrite) {
    catena::exception_with_status rc("", catena::StatusCode::PERMISSION_DENIED);
    testMethod = "POST";
    // Setting expectations.
    EXPECT_CALL(dm, hasLanguage(testLanguage)).WillOnce(::testing::Return(true));
    // Should not call if trying to overwrite on POST.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.6 - POST LanguagePack returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "POST";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.7 - POST LanguagePack throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrThrowCat) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "POST";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.8 - POST LanguagePack throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrThrowStd) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::INTERNAL);
    testMethod = "POST";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 2.9 - POST LanguagePack throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_POSTErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testMethod = "POST";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw 0;
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              PUT LanguagePack tests
 * ============================================================================
 * 
 * TEST 3.1 - PUT LanguagePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "PUT";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testVal.language());
            EXPECT_EQ(language.language_pack().SerializeAsString(), testVal.language_pack().SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.2 - PUT LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "PUT";
    authzEnabled = true;
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
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc, this](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            // Make sure correct values are passed in.
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            EXPECT_EQ(language.id(), testVal.language());
            EXPECT_EQ(language.language_pack().SerializeAsString(), testVal.language_pack().SerializeAsString());
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.3 - PUT LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testMethod = "PUT";
    authzEnabled = true;
    testToken = "Invalid token";
    // Setting expectations.
    // Should not call if authz fails.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.4 - PUT LanguagePack with invalid json.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTFailParse) {
    catena::exception_with_status rc("", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "PUT";
    testJsonBody = "Not a JSON string";
    // Setting expectations.
    // Should not call if json fails to parse.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.5 - PUT LanguagePack add error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTNew) {
    catena::exception_with_status rc("", catena::StatusCode::PERMISSION_DENIED);
    testMethod = "PUT";
    // Setting expectations.
    EXPECT_CALL(dm, hasLanguage(testLanguage)).WillOnce(::testing::Return(false));
    // Should not call if trying to add on PUT.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.6 - PUT LanguagePack returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "PUT";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.7 - PUT LanguagePack throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrThrowCat) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "PUT";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.8 - PUT LanguagePack throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrThrowStd) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::INTERNAL);
    testMethod = "PUT";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3.9 - PUT LanguagePack throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_PUTErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testMethod = "PUT";
    // Setting expectations.
    EXPECT_CALL(dm, addLanguage(::testing::_, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::AddLanguagePayload &language, catena::common::Authorizer &authz) {
            throw 0;
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                              DELETE LanguagePack tests
 * ============================================================================
 * 
 * TEST 4.1 - DELETE LangaugePack normal case.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETENormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "DELETE";
    // Setting expectations.
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            EXPECT_EQ(&authz, &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.2 - PUT LanguagePack with a valid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEAuthzValid) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    testMethod = "DELETE";
    authzEnabled = true;
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
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            EXPECT_FALSE(&authz == &catena::common::Authorizer::kAuthzDisabled);
            return catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.3 - DELETE LanguagePack with an invalid token.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEAuthzInvalid) {
    catena::exception_with_status rc("", catena::StatusCode::UNAUTHENTICATED);
    testMethod = "DELETE";
    authzEnabled = true;
    testToken = "Invalid token";
    // Setting expectations.
    // Should not call if authz fails.
    EXPECT_CALL(dm, removeLanguage(::testing::_, ::testing::_)).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.4 - DELETE LanguagePack dm.getLanguagePack() returns a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrReturn) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "DELETE";
    // Defining mock fuctions
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.5 - DELETE LanguagePack dm.getLanguagePack() throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrThrowCat) {
    catena::exception_with_status rc("Test error", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "DELETE";
    // Defining mock fuctions
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.6 - DELETE LanguagePack dm.getLanguagePack() throws a std::runtime error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrThrowStd) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::INTERNAL);
    testMethod = "DELETE";
    // Defining mock fuctions
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            throw std::runtime_error(rc.what());
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4.6 - DELETE LanguagePack dm.getLanguagePack() throws an unknown error.
 */
TEST_F(RESTLanguagePackTests, LanguagePack_DELETEErrThrowUnknown) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    testMethod = "DELETE";
    // Defining mock fuctions
    EXPECT_CALL(dm, removeLanguage(testLanguage, ::testing::_)).Times(1)
        .WillOnce(::testing::Invoke([&rc](const std::string &languageId, catena::common::Authorizer &authz) {
            throw 0;
            return catena::exception_with_status("", catena::StatusCode::OK); // should not return.
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}
