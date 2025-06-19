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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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
 * @brief This file is for testing the Languages.cpp file.
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
#include "RESTTest.h"
#include "MockSocketReader.h"
#include "MockDevice.h"

// REST
#include "controllers/Languages.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTLanguagesTests : public ::testing::Test, public RESTTest {
  protected:
    RESTLanguagesTests() : RESTTest(&serverSocket, &clientSocket) {}

    /*
     * Redirects cout and sets default expectations for each method.
     */
    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Default expectations for the context.
        EXPECT_CALL(context, method()).WillRepeatedly(::testing::ReturnRef(testMethod));
        EXPECT_CALL(context, origin()).WillRepeatedly(::testing::ReturnRef(origin));

        // Default expectations for the device model.
        EXPECT_CALL(dm, mutex()).WillRepeatedly(::testing::ReturnRef(mockMutex));

        // Creating Languages object.
        endpoint.reset(Languages::makeOne(serverSocket, context, dm));
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
    // Mock objects and endpoint.
    MockSocketReader context;
    std::mutex mockMutex;
    MockDevice dm;
    std::unique_ptr<ICallData> endpoint = nullptr;
};

/* 
 * TEST 0.1 - Creating a Languages object with makeOne.
 */
TEST_F(RESTLanguagesTests, Languages_Create) {
    ASSERT_TRUE(endpoint);
}

/* 
 * TEST 0.2 - Writing to console with Languages finish().
 */
TEST_F(RESTLanguagesTests, Languages_Finish) {
    // Calling finish and expecting the console output.
    endpoint->finish();
    ASSERT_TRUE(MockConsole.str().find("Languages[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - Languages proceed() with an invalid method.
 */
TEST_F(RESTLanguagesTests, Languages_BadMethod) {
    catena::exception_with_status rc("Bad method", catena::StatusCode::INVALID_ARGUMENT);
    testMethod = "BAD_METHOD";
    // Should not call any of these on a bad method.
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(0);
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/*
 * ============================================================================
 *                               GET Languages tests
 * ============================================================================
 * 
 * TEST 1.1 - GET Languages normal case.
 */
TEST_F(RESTLanguagesTests, Languages_GETNormal) {
    // Setting up the returnVal to test with.
    catena::LanguageList returnVal;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    returnVal.add_languages("en");
    returnVal.add_languages("fr");
    returnVal.add_languages("es");
    // Defining mock fuctions
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([&returnVal](catena::LanguageList& list) {
            list.CopyFrom(returnVal);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();

    std::string jsonBody;
    auto status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 1.2 - GET langauges returns an empty list.
 */
TEST_F(RESTLanguagesTests, Languages_GETEmpty) {
    // Setting up empty language list
    catena::LanguageList returnVal;
    catena::exception_with_status rc("No languages found", catena::StatusCode::NOT_FOUND);
    // Defining mock functions
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([&returnVal](catena::LanguageList& list) {
            list.CopyFrom(returnVal);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.3 - GET Languages throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagesTests, Languages_GETErrThrowCat) {
    // Setting up the rc to test with.
    catena::exception_with_status rc("Device not found", catena::StatusCode::NOT_FOUND);
    // Defining mock functions
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::LanguageList& list) {
            throw catena::exception_with_status(rc.what(), rc.status);
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.4 - GET Languages throws a std::runtime error.
 */
TEST_F(RESTLanguagesTests, Languages_GETErrThrowStd) {
    // Setting up the rc to test with.
    catena::exception_with_status rc("Standard error", catena::StatusCode::INTERNAL);
    // Defining mock functions
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([&rc](catena::LanguageList& list) {
            throw std::runtime_error(rc.what());
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 1.5 - GET Languages throws an unknown error.
 */
TEST_F(RESTLanguagesTests, Languages_GetErrThrowUnknown) {
    // Setting up the rc to test with.
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    // Defining mock functions
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(::testing::Invoke([](catena::LanguageList& list) {
            throw 0;
        }));
    // Calling proceed() and checking written response.
    endpoint->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}
