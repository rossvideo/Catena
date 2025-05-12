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
 * @brief This file is for testing the SocketWriter.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/12
 * @copyright Copyright © 2025 Ross Video Ltd
 */

 // gtest
#include <gtest/gtest.h>

// std
#include <string>

// Common
#include <SubscriptionManager.h>

// Test helpers
#include "SocketHelper.h"

// REST
#include "SocketReader.h"
using namespace catena::REST;

// Fixture
class RESTSocketReaderTests : public ::testing::Test, public SocketHelper {
  protected:
    // Defining the in/out sockets.
    RESTSocketReaderTests() : SocketHelper(&clientSocket, &serverSocket) {}

    // Writes a request to a socket to later be read by the SocketReader.
    void SetUp() override {
        origin = "test_origin";
        // Making sure the reader properly adds the subscriptions manager.
        EXPECT_EQ(&socketReader.getSubscriptionManager(), &sm);
    }
  
    void TearDown() override { /* Cleanup code here */ }

    /*
     * Since all of the tests are essentially the write->read->validate, here
     * is all the steps compiled. Diferences between tests will come from
     * setting the member variables of the fixture.
     */
    void TestSocketReader() {
        // Write
        writeRequest(method, endpoint, slot, fields,
                     jwsToken, jsonBody, dl, language);
        
        // Read
        socketReader.read(serverSocket, authz);
        
        // Validate
        // Updating jwsToken and DetailLevel
        if (!authz) { jwsToken = ""; }
        auto& dlMap = catena::common::DetailLevel().getForwardMap();
        // Checking answers.
        EXPECT_EQ(socketReader.method(),                 method  );
        EXPECT_EQ(socketReader.endpoint(),               endpoint);
        EXPECT_EQ(socketReader.slot(),                   slot    );
        for (auto [key, value] : fields) {
            EXPECT_EQ(socketReader.hasField(key),        true    );
            EXPECT_EQ(socketReader.fields(key),          value   );
        }
        EXPECT_EQ(socketReader.hasField("doesNotExist"), false   );
        EXPECT_EQ(socketReader.fields("doesNotExist"),   ""      );
        EXPECT_EQ(socketReader.authorizationEnabled(),   authz   );
        EXPECT_EQ(socketReader.jwsToken(),               jwsToken);
        EXPECT_EQ(socketReader.origin(),                 origin  );
        EXPECT_EQ(socketReader.jsonBody(),               jsonBody);
        EXPECT_EQ(dlMap.at(socketReader.detailLevel()),  dl      );
        EXPECT_EQ(socketReader.language(),               language);
    }

    // SocketReader obj.
    catena::common::SubscriptionManager sm;    
    SocketReader socketReader{sm};
    // Test request data
    std::string method = "PUT";
    std::string endpoint = "/v1/test-call";
    uint32_t slot = 1;
    std::unordered_map<std::string, std::string> fields = {
        {"testField1", "1"},
        {"testField2", "2"}
        // DO NOT ADD A FIELD CALLED "doesNotExist".
    };
    bool authz = false;
    std::string jwsToken = "test_bearer";
    std::string jsonBody = "{\n  test_body\n}";
    std::string dl = "FULL";
    std::string language = "test_language";
};

/*
 * ============================================================================
 *                             SSEWriter tests
 * ============================================================================
 * 
 * TEST 1 - Reading from socket with authz disabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_NormalCase) {
    // Authz false by default.
    TestSocketReader();
}

/* 
 * TEST 2 - Reading from socket with authz enabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_AuthzCase) {
    // Setting authz to true and calling validate.
    authz = true;
    TestSocketReader();
}

/* 
 * TEST 3 - Reading connect from socket (No slot required)
 */
TEST_F(RESTSocketReaderTests, SocketReader_NoSlotConnect) {
    // Connect does not require a slot specified.
    endpoint = "/v1/connect";
    slot = 0;
    TestSocketReader();
}

/* 
 * TEST 4 - Reading get-populated-slots from socket (No slot required)
 */
TEST_F(RESTSocketReaderTests, SocketReader_NoSlotGetPopulatedSlots) {
    // GetPopulatedSlots does not require a slot specified.
    endpoint = "/v1/get-populated-slots";
    slot = 0;
    TestSocketReader();
}

/* 
 * TEST 5 - Reading request from socket with no slot specified (Error).
 */
TEST_F(RESTSocketReaderTests, SocketReader_NoSlotError) {
    // All other calls require a slot specified.
    slot = 0;
    ASSERT_THROW(TestSocketReader(), catena::exception_with_status);
}

/* 
 * TEST 6 - Reading request from socket with no slot specified.
 */
TEST_F(RESTSocketReaderTests, SocketReader_LongJsonBody) {
    // Setting json body to just a string of 10000 'a's.
    jsonBody = std::string(10000, 'a');
    TestSocketReader();
}
