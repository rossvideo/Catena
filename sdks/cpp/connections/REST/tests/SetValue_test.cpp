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
 * @brief This file is for testing the SetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/14
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
#include "controllers/SetValue.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTSetValueTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTSetValueTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating setValue object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        setValue = SetValue::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (setValue) {
            delete setValue;
        }
    }

    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* setValue = nullptr;
};

/*
 * ============================================================================
 *                               SetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a SetValue object with makeOne.
 */
TEST_F(RESTSetValueTests, SetValue_create) {
    // Making sure setValue is created from the SetUp step.
    ASSERT_TRUE(setValue);
}

/* 
 * TEST 2 - Normal case for SetValue toMulti_().
 */
TEST_F(RESTSetValueTests, SetValue_proceedNormal) {
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::mutex mockMutex;
    std::string jsonBody = "{\"oid\":\"/text_box\",\"value\":{\"string_value\":\"test value 1\"}}";

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(1).WillOnce(::testing::Return(false));
    EXPECT_CALL(dm, mutex()).Times(1).WillOnce(::testing::ReturnRef(mockMutex));
    EXPECT_CALL(dm, tryMultiSetValue(::testing::_, ::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(true));
    EXPECT_CALL(dm, commitMultiSetValue(::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(catena::exception_with_status(rc.what(), rc.status)));

    setValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3 - TEST 11 - SetValue toMulti_() fails to parse the JSON.
 */
TEST_F(RESTSetValueTests, SetValue_proceedFailParse) {
    catena::exception_with_status rc("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
    std::string jsonBody = "Not a JSON string";

    // Defining mock functions
    EXPECT_CALL(context, jsonBody()).Times(1).WillOnce(::testing::ReturnRef(jsonBody));
    EXPECT_CALL(context, slot()).Times(1).WillOnce(::testing::Return(1));
    EXPECT_CALL(context, authorizationEnabled()).Times(0);

    setValue->proceed();

    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - Writing to console with SetValue finish().
 */
TEST_F(RESTSetValueTests, SetValue_finish) {
    // Calling finish and expecting the console output.
    setValue->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("SetValue[3] finished\n") != std::string::npos);
}
