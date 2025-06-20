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
 * @date 25/05/14
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Test helpers
#include "RESTTest.h"

// REST
#include "controllers/GetPopulatedSlots.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTGetPopulatedSlotsTests : public RESTEndpointTest {
  protected:
    ICallData* makeOne() override { return GetPopulatedSlots::makeOne(serverSocket, context_, dm0_); }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        std::string expJson = "";
        if (!expVal_.slots().empty()) {
            auto status = google::protobuf::util::MessageToJsonString(expVal_, &expJson);
            EXPECT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
        }
        EXPECT_EQ(readResponse(), expectedResponse(expRc_, expJson));
    }

    // Expected values
    catena::SlotList expVal_;
};

/*
 * ============================================================================
 *                               GetPopulatedSlots tests
 * ============================================================================
 * 
 * TEST 1 - Creating a GetPopulatedSlots object with makeOne.
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_Create) {
    ASSERT_TRUE(endpoint_);
}

/* 
 * TEST 2 - Writing to console with GetPopulatedSlots finish().
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("GetPopulatedSlots[1] finished\n") != std::string::npos);
}

/*
 * TEST 3 - Normal case for GetPopulatedSlots proceed().
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_Normal) {
    expVal_.add_slots(dm0_.slot());
    // Sending the call
    testCall();
}

/* 
 * TEST 4 - dm.slot() throws an error.
 */
TEST_F(RESTGetPopulatedSlotsTests, GetPopulatedSlots_proceedErr) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, slot()).Times(1).WillOnce(::testing::Throw(std::runtime_error("Unknown error")));
    // Sending the call
    testCall();
}
