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
 * @brief This file is for testing the MultiSetValue.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/12
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
#include "controllers/MultiSetValue.h"
#include "SocketWriter.h"

using namespace catena::common;
using namespace catena::REST;

// Fixture
class RESTMultiSetValueTests : public ::testing::Test, public SocketHelper {
  protected:
    RESTMultiSetValueTests() : SocketHelper(&serverSocket, &clientSocket) {}

    void SetUp() override {
        // Redirecting cout to a stringstream for testing.
        oldCout = std::cout.rdbuf(MockConsole.rdbuf());

        // Creating multiSetValue object.
        EXPECT_CALL(context, origin()).Times(1).WillOnce(::testing::ReturnRef(origin));
        multiSetValue = MultiSetValue::makeOne(serverSocket, context, dm);
    }

    void TearDown() override {
        std::cout.rdbuf(oldCout); // Restoring cout
        // Cleanup code here
        if (multiSetValue) {
            delete multiSetValue;
        }
    }
    std::stringstream MockConsole;
    std::streambuf* oldCout;
    
    MockSocketReader context;
    MockDevice dm;
    catena::REST::ICallData* multiSetValue = nullptr;
};

/*
 * ============================================================================
 *                               MultiSetValue tests
 * ============================================================================
 * 
 * TEST 1 - Creating a MultiSetValue object with makeOne.
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_create) {
    // Making sure multiSetValue is created from the SetUp step.
    ASSERT_TRUE(multiSetValue);
}

/* 
 * TEST 2 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedNormal) {
    
}

/* 
 * TEST 3 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedTryErr) {
    
}

/* 
 * TEST 4 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedTryThrowCatena) {
    
}

/* 
 * TEST 5 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedTryThrowUnknown) {
    
}

/* 
 * TEST 6 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedCommitErr) {
    
}

/* 
 * TEST 7 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedCommitThrowCatena) {
    
}

/* 
 * TEST 8 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedCommitThrowUnknown) {
    
}

/* 
 * TEST 9 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedAuthzValid) {
    
}

/* 
 * TEST 10 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedAuthzInvalid) {
    
}

/* 
 * TEST 11 - Normal case for MultiSetValue proceed().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_proceedFailParse) {
    
}

/* 
 * TEST 12 - Writing to console with MultiSetValue finish().
 */
TEST_F(RESTMultiSetValueTests, MultiSetValue_finish) {
    // Calling finish and expecting the console output.
    multiSetValue->finish();
    // Idk why I cant use .contains() here :/
    ASSERT_TRUE(MockConsole.str().find("MultiSetValue[11] finished\n") != std::string::npos);
}
