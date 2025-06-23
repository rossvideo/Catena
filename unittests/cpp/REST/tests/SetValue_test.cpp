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

// Test helpers
#include "RESTTest.h"

// REST
#include "controllers/SetValue.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTSetValueTests : public RESTEndpointTest {
  protected:
    /*
     * Creates a SetValue handler object.
     */
    ICallData* makeOne() override { return SetValue::makeOne(serverSocket_, context_, dm0_); }

    /*
     * Streamlines the creation of endpoint input. 
     */
    void initPayload(uint32_t slot, const std::string& oid, const std::string& value) {
        slot_ = slot;
        fqoid_ = oid;
        inVal_.set_string_value(value);
        auto status = google::protobuf::util::MessageToJsonString(inVal_, &jsonBody_);
        ASSERT_TRUE(status.ok()) << "Failed to convert input value to JSON";
    }

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        EXPECT_EQ(readResponse(), expectedResponse(expRc_));
    }

    // in vals
    catena::Value inVal_;
};

/*
 * ============================================================================
 *                               SetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a SetValue object with makeOne.
 */
TEST_F(RESTSetValueTests, SetValue_Create) {
    ASSERT_TRUE(endpoint_);
}

/* 
 * TEST 2 - Writing to console with SetValue finish().
 */
TEST_F(RESTSetValueTests, SetValue_Finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("SetValue[1] finished\n") != std::string::npos);
}

/* 
 * TEST 3 - Normal case for SetValue toMulti_().
 */
TEST_F(RESTSetValueTests, SetValue_Normal) {
    initPayload(0, "/test_oid", "test_value");
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(1)
        .WillOnce(testing::Invoke([this](catena::MultiSetValuePayload src, catena::exception_with_status &ans, catena::common::Authorizer &authz) {
            // Checking that function gets correct inputs.
            auto val = (*src.mutable_values())[0];
            EXPECT_EQ(val.oid(), fqoid_);
            EXPECT_EQ(val.value().SerializeAsString(), inVal_.SerializeAsString());
            return true;
        }));
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(1).WillOnce(testing::Return(catena::exception_with_status(expRc_.what(), expRc_.status)));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 4 - SetValue toMulti_() fails to parse the JSON.
 */
TEST_F(RESTSetValueTests, SetValue_FailParse) {
    expRc_ = catena::exception_with_status("Failed to convert JSON to protobuf", catena::StatusCode::INVALID_ARGUMENT);
    jsonBody_ = "Not a JSON string";
    // Setting expectations
    EXPECT_CALL(dm0_, tryMultiSetValue(testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(dm0_, commitMultiSetValue(testing::_, testing::_)).Times(0);
    // Calling proceed and testing the output
    testCall();
}
