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
 * @brief This file is for testing the ListLanguages.cpp file.
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
#include "controllers/ListLanguages.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTListLanguagesTests : public ::testing::Test, public RESTTest {
  protected:
    RESTListLanguagesTests() : RESTTest(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating ListLanguages object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        listLanguages = ListLanguages::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (listLanguages) {
            delete listLanguages;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    MockDevice dm;
    std::mutex mockMutex;
    catena::REST::ICallData* listLanguages = nullptr;
};

/*
 * ============================================================================
 *                               ListLanguages tests
 * ============================================================================
 * 
 * TEST 1 - Creating a ListLanguages object with makeOne.
 */
TEST_F(RESTListLanguagesTests, ListLanguages_create) {
    ASSERT_TRUE(listLanguages);
}

/* 
 * TEST 2 - Normal case for ListLanguages proceed().
 */
TEST_F(RESTListLanguagesTests, ListLanguages_proceedNormal) {
    // Setting up the returnVal to test with.
    catena::LanguageList returnVal;
    catena::exception_with_status rc("", catena::StatusCode::OK);
    returnVal.add_languages("en");
    returnVal.add_languages("fr");
    returnVal.add_languages("es");

    // Defining mock fuctions
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1);
    ON_CALL(dm, toProto(::testing::An<catena::LanguageList&>()))
        .WillByDefault(::testing::Invoke([&returnVal](catena::LanguageList& list) {
            list.CopyFrom(returnVal);
        }));

    // Calling proceed() and checking written response.
    listLanguages->proceed();

    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(returnVal, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 3 - dm.toProto() throws an error.
 */
TEST_F(RESTListLanguagesTests, ListLanguages_proceedErr) {
    // Setting up the rc to test with.
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);

    // Defining mock fuctions
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, toProto(::testing::An<catena::LanguageList&>())).Times(1);
    ON_CALL(dm, toProto(::testing::An<catena::LanguageList&>()))
        .WillByDefault(::testing::Invoke([&rc](catena::LanguageList& list) {
            throw catena::exception_with_status(rc.what(), rc.status);
        }));

    // Calling proceed() and checking written response.
    listLanguages->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - Writing to console with ListLanguages finish().
 */
TEST_F(RESTListLanguagesTests, ListLanguages_finish) {
    // Calling finish and expecting the console output.
    listLanguages->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("ListLanguages[3] finished\n") != std::string::npos);
}
