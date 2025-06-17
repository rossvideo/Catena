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
#include "RESTTest.h"

// REST
#include "SocketReader.h"
using namespace catena::REST;

// Fixture
class RESTSocketReaderTests : public ::testing::Test, public RESTTest {
  protected:
    // Defining the in/out sockets.
    RESTSocketReaderTests() : RESTTest(&clientSocket, &serverSocket) {}

    // Writes a request to a socket to later be read by the SocketReader.
    void SetUp() override {
        origin = "test_origin";
        // Making sure the reader properly adds the subscriptions manager.
        EXPECT_EQ(&socketReader.getSubscriptionManager(), &sm);
        EXPECT_EQ(socketReader.EOPath(), EOPath);
    }
  
    void TearDown() override { /* Cleanup code here */ }

    void testCall(catena::REST::RESTMethod method,
                  uint32_t slot,
                  std::string endpoint,
                  std::string fqoid,
                  bool stream,
                  std::unordered_map<std::string, std::string> fields,
                  bool authz,
                  std::string jwsToken,
                  std::string origin,
                  catena::Device_DetailLevel detailLevel,
                  std::string language,
                  std::string jsonBody) {
        // Writing the request to the socket and reading.
        writeRequest(method, slot, endpoint, fqoid, stream, fields,
                     jwsToken, origin, detailLevel, language, jsonBody);
        socketReader.read(serverSocket, authz);
        // Validating the results.
        if (!authz) { jwsToken = ""; }
        if (detailLevel ==  catena::Device_DetailLevel_UNSET) {
            detailLevel = catena::Device_DetailLevel_NONE;
        }
        EXPECT_EQ(socketReader.method(), method);
        EXPECT_EQ(socketReader.slot(), slot);
        EXPECT_EQ(socketReader.endpoint(), endpoint);
        EXPECT_EQ(socketReader.fqoid(), fqoid);
        for (auto [key, value] : fields) {
            EXPECT_EQ(socketReader.hasField(key), true);
            EXPECT_EQ(socketReader.fields(key), value);
        }
        EXPECT_EQ(socketReader.hasField("doesNotExist"), false);
        EXPECT_EQ(socketReader.fields("doesNotExist"), "");
        EXPECT_EQ(socketReader.jwsToken(), jwsToken);
        EXPECT_EQ(socketReader.origin(), origin);
        EXPECT_EQ(socketReader.detailLevel(), detailLevel);
        EXPECT_EQ(socketReader.jsonBody(), jsonBody);
        EXPECT_EQ(socketReader.authorizationEnabled(), authz);
        EXPECT_EQ(socketReader.stream(), stream);
    }

    // Variables to test on creation.
    catena::common::SubscriptionManager sm;
    std::string EOPath = "/test/eo/path";
    // The SocketReader object.
    SocketReader socketReader{sm, EOPath};
};

/*
 * ============================================================================
 *                             SSEWriter tests
 * ============================================================================
 * 
 * TEST 1 - Initializing the SocketReader with subscriptionManager.
 */
TEST_F(RESTSocketReaderTests, SocketReader_Create) {
    // Testing socketReader creation from setUp() step.
}

/* 
 * TEST 2 - Reading from socket with authz disabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_NormalCase) {
    // Authz false by default.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {{"test-field-1", "1"}, {"test-field-2", "2"}}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
}

/* 
 * TEST 3 - Reading from socket with authz disabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_StreamCase) {
    // Authz false by default.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", true, {{"test-field-1", "1"}, {"test-field-2", "2"}}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
}

/* 
 * TEST 4 - Reading from socket with authz enabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_AuthzCase) {
    // Setting authz to true and calling validate.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {{"test-field-1", "1"}, {"test-field-2", "2"}}, true, "test-jws-token", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
}

/* 
 * TEST 5 - Testing parsing of health endpoint.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointHealth) {
    // GET /v1/health
    testCall(catena::REST::Method_GET, 0, "/health", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
}

/* 
 * TEST 6 - Testing parsing of discovery endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointDiscovery) {
    // GET /v1/devices
    testCall(catena::REST::Method_GET, 0, "/devices", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // GET /v1/{slot}
    testCall(catena::REST::Method_GET, 1, "/", "", false, {}, false, "", "*", catena::Device_DetailLevel_FULL, "en", "");
    // GET /v1/{slot}/stream
    testCall(catena::REST::Method_GET, 1, "/", "", true, {}, false, "", "*", catena::Device_DetailLevel_FULL, "en", "");
}

/* 
 * TEST 7 - Testing parsing of commands endpoint.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointCommands) {
    testCall(catena::REST::Method_POST, 0, "/commands", "/play", false, {{"respond", "true"}}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
}

/* 
 * TEST 8 - Testing parsing of assets endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointAssets) {
    // GET /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/assets", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // POST /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_POST, 1, "/assets", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
    // PUT /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_PUT, 1, "/assets", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // DELETE /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_DELETE, 1, "/assets", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // GET /v1/{slot}/assets/{fqoid}/stream
    testCall(catena::REST::Method_GET, 1, "/assets", "/test/oid", true, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
}

/* 
 * TEST 9 - Testing parsing of parameters endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointParameters) {
    // GET /v1/{slot}/value/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/value", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // PUT /v1/{slot}/value/{fqoid}
    testCall(catena::REST::Method_PUT, 1, "/value", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
    // PUT /v1/{slot}/values
    testCall(catena::REST::Method_PUT, 1, "/values", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
    // GET /v1/{slot}/param/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/param", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");

}

/* 
 * TEST 10 - Testing parsing of subscriptions endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointSubscriptions) {
    // GET /v1/{slot}/basic-param/{fqoid}/stream
    testCall(catena::REST::Method_GET, 1, "/basic-param", "/test/oid", true, {{"recursive", "true"}}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // GET /v1/{slot}/basic-param/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/basic-param", "/test/oid", false, {{"recursive", "true"}}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // GET /v1/{slot}/subscriptions/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/subscriptions", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // PUT /v1/{slot}/value/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/subscriptions", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
}

/* 
 * TEST 11 - Testing parsing of updates endpoint.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointUpdates) {
    // GET /v1/{slot}/connect
    testCall(catena::REST::Method_GET, 1, "/connect", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
}

/* 
 * TEST 12 - Testing parsing of languages endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointLanguages) {
    // GET /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_GET, 1, "/langauge-pack", "/en", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // POST /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_POST, 1, "/langauge-pack", "/en", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
    // DELETE /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_DELETE, 1, "/langauge-pack", "/en", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
    // PUT /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_PUT, 1, "/langauge-pack", "/en", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "{test_json_body}");
    // GET /v1/{slot}/langauges
    testCall(catena::REST::Method_GET, 1, "/langauges", "", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", "");
}

/* 
 * TEST 13 - Testing with a long json body.
 */
TEST_F(RESTSocketReaderTests, SocketReader_LongJsonBody) {
    // SocketReader should be able to handle json bodies of any length.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_NONE, "en", std::string(10000, 'a'));
}

/* 
 * TEST 14 - Testing with unset headers.
 */
TEST_F(RESTSocketReaderTests, SocketReader_HeadersUnset) {
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {}, false, "", "*", catena::Device_DetailLevel_UNSET, "", "");
}
