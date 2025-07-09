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

// Test helpers
#include "RESTTest.h"

// REST
#include "controllers/Languages.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTLanguagesTests : public RESTEndpointTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        Logger::StartLogging("RESTLanguagesTest");
    }

    static void TearDownTestSuite() {
        google::ShutdownGoogleLogging();
    }
    
    RESTLanguagesTests() : RESTEndpointTest() {
        // Default expectations for the device model 1 (should not be called).
        EXPECT_CALL(dm1_, toProto(testing::An<catena::LanguageList&>())).Times(0);
    }
    /*
     * Creates a Languages handler object.
     */
    ICallData* makeOne() override { return Languages::makeOne(serverSocket_, context_, dms_); }  

    /*
     * Calls proceed and tests the response.
     */
    void testCall() {
        endpoint_->proceed();
        std::string expJson = "";
        if (!expVal_.languages().empty()) {
            auto status = google::protobuf::util::MessageToJsonString(expVal_, &expJson);
            ASSERT_TRUE(status.ok()) << "Failed to convert expected value to JSON";
        }
        EXPECT_EQ(readResponse(), expectedResponse(expRc_, expJson));
    }

    // Expected values
    catena::LanguageList expVal_;
};

/* 
 * TEST 0.1 - Creating a Languages object with makeOne.
 */
TEST_F(RESTLanguagesTests, Languages_Create) {
    ASSERT_TRUE(endpoint_);
}

/* 
 * TEST 0.2 - Writing to console with Languages finish().
 */
TEST_F(RESTLanguagesTests, Languages_Finish) {
    endpoint_->finish();
    ASSERT_TRUE(MockConsole_.str().find("Languages[1] finished\n") != std::string::npos);
}

/* 
 * TEST 0.3 - Languages proceed() with an invalid method.
 */
TEST_F(RESTLanguagesTests, Languages_BadMethod) {
    expRc_ = catena::exception_with_status("Bad method", catena::StatusCode::UNIMPLEMENTED);
    method_ = Method_NONE;
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 0.3 - Languages proceed() with an invalid method.
 */
TEST_F(RESTLanguagesTests, Languages_InvalidSlot) {
    slot_ = dms_.size();
    expRc_ = catena::exception_with_status("device not found in slot " + std::to_string(slot_), catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(0);
    // Calling proceed and testing the output
    testCall();
}

/*
 * ============================================================================
 *                               GET Languages tests
 * ============================================================================
 * 
 * TEST 1.1 - GET Languages normal case.
 */
TEST_F(RESTLanguagesTests, Languages_GETNormal) {
    expVal_.add_languages("en");
    expVal_.add_languages("fr");
    expVal_.add_languages("es");
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(testing::Invoke([this](catena::LanguageList& list) {
            list.CopyFrom(expVal_);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.2 - GET langauges returns an empty list.
 */
TEST_F(RESTLanguagesTests, Languages_GETEmpty) {
    expRc_ = catena::exception_with_status("No languages found", catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(testing::Return());
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.3 - GET Languages throws a catena::exception_with_status error.
 */
TEST_F(RESTLanguagesTests, Languages_GETErrThrowCat) {
    expRc_ = catena::exception_with_status("Device not found", catena::StatusCode::NOT_FOUND);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(testing::Invoke([this](catena::LanguageList& list) {
            throw catena::exception_with_status(expRc_.what(), expRc_.status);
        }));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.4 - GET Languages throws a std::runtime error.
 */
TEST_F(RESTLanguagesTests, Languages_GETErrThrowStd) {
    expRc_ = catena::exception_with_status("Standard error", catena::StatusCode::INTERNAL);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(testing::Throw(std::runtime_error(expRc_.what())));
    // Calling proceed and testing the output
    testCall();
}

/* 
 * TEST 1.5 - GET Languages throws an unknown error.
 */
TEST_F(RESTLanguagesTests, Languages_GetErrThrowUnknown) {
    expRc_ = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    // Setting expectations
    EXPECT_CALL(dm0_, toProto(testing::An<catena::LanguageList&>())).Times(1)
        .WillOnce(testing::Throw(0));
    // Calling proceed and testing the output
    testCall();
}
