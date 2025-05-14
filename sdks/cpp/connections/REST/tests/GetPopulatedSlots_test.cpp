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
 * @brief This file is for testing the GetPopulatedSlots.cpp file.
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
#include "controllers/GetPopulatedSlots.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTGetPopulatedSlotsTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTGetPopulatedSlotsTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating GetPopulatedSlots object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        getPopulatedSlots = GetPopulatedSlots::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (getPopulatedSlots) {
            delete getPopulatedSlots;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    MockDevice dm;
    std::mutex mockMutex;
    catena::REST::ICallData* getPopulatedSlots = nullptr;
};

/*
 * ============================================================================
 *                               GetPopulatedSlots tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetPopulatedSlots object with makeOne.
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_create) {
    ASSERT_TRUE(getPopulatedSlots);
}

/* 
 * TEST 2 - Normal case for GetPopulatedSlots proceed().
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_proceedNormal) {
    catena::exception_with_status rc("OK", catena::StatusCode::OK);
    uint32_t slot = 1;
    // Expected JSON response.
    catena::SlotList slotList;
    slotList.add_slots(slot);

    EXPECT_CALL(dm, slot()).Times(1).WillOnce(::testing::Return(1));

    getPopulatedSlots->proceed();

    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(slotList, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 3 - dm.slot() throws an error.
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_proceedErr) {
    catena::exception_with_status rc("Unknown error", catena::StatusCode::UNKNOWN);
    EXPECT_CALL(dm, slot()).Times(1).WillOnce(::testing::Throw(std::runtime_error("Unknown error")));
    getPopulatedSlots->proceed();
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - Writing to console with GetPopulatedSlots finish().
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_finish) {
    // Calling finish and expecting the console output.
    getPopulatedSlots->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("GetPopulatedSlots[3] finished\n") != std::string::npos);
}
