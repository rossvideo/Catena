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
 * @brief This file is for testing the LanguagePackRequest.cpp file.
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
#include "RESTMockClasses.h"
#include "../../common/tests/CommonMockClasses.h"

// REST
#include "controllers/LanguagePackRequest.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTLanguagePackRequestTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTLanguagePackRequestTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating LanguagePackRequest object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        languagePackRequest = LanguagePackRequest::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (languagePackRequest) {
            delete languagePackRequest;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;

    std::string language = "en";
    
    MockSocketReader context;
    MockDevice dm;
    std::mutex mockMutex;
    catena::REST::ICallData* languagePackRequest = nullptr;
};

/*
 * ============================================================================
 *                               LanguagePackRequest tests
 * ============================================================================
 * 
 * TEST 1 - Creating a LanguagePackRequest object with makeOne.
 */
TEST_F(RESTLanguagePackRequestTests, LanguagePackRequest_create) {
    // Making sure languagePackRequest is created from the SetUp step.
    ASSERT_TRUE(languagePackRequest);
}

/* 
 * TEST 2 - Normal case for LanguagePackRequest proceed().
 */
TEST_F(RESTLanguagePackRequestTests, LanguagePackRequest_proceedNormal) {
    // Setting up the returnVal to test with.
    catena::DeviceComponent_ComponentLanguagePack returnVal;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    returnVal.set_language("en");
    catena::LanguagePack *pack = returnVal.mutable_language_pack();
    pack->set_name("English");
    pack->mutable_words()->insert({"Hello", "Goodbye"});

    // Defining mock fuctions
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(context, fields("language")).Times(1).WillOnce(::testing::ReturnRef(language));
    EXPECT_CALL(dm, getLanguagePack(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getLanguagePack(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc, &returnVal](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            pack.CopyFrom(returnVal);
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePackRequest->proceed();

    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 3 - dm.getLanguagePack() returns an error.
 */
TEST_F(RESTLanguagePackRequestTests, LanguagePackRequest_proceedErrReturn) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Language pack not found", catena::StatusCode::NOT_FOUND);

    // Defining mock fuctions
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(context, fields("language")).Times(1).WillOnce(::testing::ReturnRef(language));
    EXPECT_CALL(dm, getLanguagePack(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getLanguagePack(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            return catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    languagePackRequest->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - dm.getLanguagePack() throws an error.
 */
TEST_F(RESTLanguagePackRequestTests, LanguagePackRequest_proceedErrThrow) {
    // Setting up the returnVal to test with.
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Defining mock fuctions
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(context, fields("language")).Times(1).WillOnce(::testing::ReturnRef(language));
    EXPECT_CALL(dm, getLanguagePack(::testing::_, ::testing::_)).Times(1);
    ON_CALL(dm, getLanguagePack(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke([&rc](const std::string &languageId, catena::DeviceComponent_ComponentLanguagePack &pack) {
            throw catena::exception_with_status(rc.what(), rc.status);
            return catena::exception_with_status("", catena::StatusCode::OK); // Here to ignore errors.
        }));

    // Calling proceed() and checking written response.
    languagePackRequest->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 5 - Writing to console with LanguagePackRequest finish().
 */
TEST_F(RESTLanguagePackRequestTests, LanguagePackRequest_finish) {
    // Calling finish and expecting the console output.
    languagePackRequest->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("LanguagePackRequest[4] finished\n") != std::string::npos);
}
