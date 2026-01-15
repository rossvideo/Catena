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
 * @brief This file is for testing the SocketWriter.cpp file.
 * @author benjamin.whitten@rossvideo.com
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2025/11/26
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// Common
#include <SubscriptionManager.h>
#include <Logger.h>
#include <SharedFlags.h>

// Test helpers
#include "RESTTest.h"
#include "MockServiceImpl.h"
#include "MockSubscriptionManager.h"

// REST
#include "SocketReader.h"

using namespace catena::REST;

// Fixture
class RESTSocketReaderTests : public testing::Test, public RESTTest {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
        absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
        Logger::init("RESTSocketReaderTest");
    }

    static void TearDownTestSuite() {
    }
  
    // Defining the in/out sockets.
    RESTSocketReaderTests() : RESTTest(&clientSocket_, &serverSocket_) {}

    // Writes a request to a socket to later be read by the SocketReader.
    void SetUp() override {
        origin_ = "test_origin";
        // Setting up expectations for the mock service.
        EXPECT_CALL(service_, subscriptionManager()).WillRepeatedly(testing::ReturnRef(sm_));
        EXPECT_CALL(service_, EOPath()).WillRepeatedly(testing::ReturnRef(EOPath_));
        EXPECT_CALL(service_, version()).WillRepeatedly(testing::ReturnRef(version_));
        // Making sure the reader properly adds the subscriptions manager.
        EXPECT_EQ(socketReader.service(), &service_);
        EXPECT_EQ(&socketReader.subscriptionManager(), &sm_);
        EXPECT_EQ(socketReader.EOPath(), EOPath_);
    }
  
    void TearDown() override { /* Cleanup code here */ }

    void testCall(RESTMethod method,
                  uint32_t slot,
                  std::string endpoint,
                  std::string fqoid,
                  bool stream,
                  std::unordered_map<std::string, std::string> fields,
                  bool authz,
                  std::string jwsToken,
                  std::string origin,
                  st2138::Device_DetailLevel detailLevel,
                  std::string language,
                  std::string requestStart,
                  std::string jsonBody) {
        // Setting up expectations for the mock service.
        EXPECT_CALL(service_, authorizationEnabled()).WillRepeatedly(testing::Return(authz));
        // Writing the request to the socket and reading.
        writeRequest(method, slot, endpoint, fqoid, stream, fields,
                     jwsToken, origin, detailLevel, language, requestStart, jsonBody);
        socketReader.read(serverSocket_);
        // Validating the results.
        if (!authz) { jwsToken = ""; }
        if (detailLevel ==  st2138::Device_DetailLevel_UNSET) {
            detailLevel = st2138::Device_DetailLevel_NONE;
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
        EXPECT_EQ(socketReader.requestStart(), requestStart != "" ? stod(requestStart) : 0.0);
        EXPECT_EQ(socketReader.requestReceived() > (requestStart != "" ? stod(requestStart) : 0.0), true);
    }

    // Variables to test on creation.
    MockSubscriptionManager sm_;
    std::string EOPath_ = "/test/eo/path";
    std::string version_ = "v1";

    // Mock service implementation.
    MockServiceImpl service_;

    // The SocketReader object.
    SocketReader socketReader{&service_};
};

/*
 * ============================================================================
 *                            SocketReader tests
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
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {{"test-field-1", "1"}, {"test-field-2", "2"}}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
}

/* 
 * TEST 3 - Reading from socket with authz disabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_StreamCase) {
    // Authz false by default.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", true, {{"test-field-1", "1"}, {"test-field-2", "2"}}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
}

/* 
 * TEST 4 - Reading from socket with authz enabled.
 */
TEST_F(RESTSocketReaderTests, SocketReader_AuthzCase) {
    // Setting authz to true and calling validate.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {{"test-field-1", "1"}, {"test-field-2", "2"}}, true, "test-jws-token", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
}

/* 
 * TEST 5 - Testing parsing of health endpoint.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointHealth) {
    // GET /v1/health
    testCall(catena::REST::Method_GET, 0, "/health", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
}

/* 
 * TEST 6 - Testing parsing of discovery endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointDiscovery) {
    // GET /v1/devices
    testCall(catena::REST::Method_GET, 0, "/devices", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // GET /v1/{slot}
    testCall(catena::REST::Method_GET, 1, "/", "", false, {}, false, "", "*", st2138::Device_DetailLevel_FULL, "en", "0.0", "");
    // GET /v1/{slot}/stream
    testCall(catena::REST::Method_GET, 1, "/", "", true, {}, false, "", "*", st2138::Device_DetailLevel_FULL, "en", "0.0", "");
}

/* 
 * TEST 7 - Testing parsing of commands endpoint.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointCommands) {
    testCall(catena::REST::Method_POST, 0, "/commands", "/play", false, {{"respond", "true"}}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
}

/* 
 * TEST 8 - Testing parsing of assets endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointAssets) {
    // GET /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/assets", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // POST /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_POST, 1, "/assets", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
    // PUT /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_PUT, 1, "/assets", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // DELETE /v1/{slot}/assets/{fqoid}
    testCall(catena::REST::Method_DELETE, 1, "/assets", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // GET /v1/{slot}/assets/{fqoid}/stream
    testCall(catena::REST::Method_GET, 1, "/assets", "/test/oid", true, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
}

/* 
 * TEST 9 - Testing parsing of parameters endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointParameters) {
    // GET /v1/{slot}/value/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/value", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // PUT /v1/{slot}/value/{fqoid}
    testCall(catena::REST::Method_PUT, 1, "/value", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
    // PUT /v1/{slot}/values
    testCall(catena::REST::Method_PUT, 1, "/values", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
    // GET /v1/{slot}/param/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/param", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");

}

/* 
 * TEST 10 - Testing parsing of subscriptions endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointSubscriptions) {
    // GET /v1/{slot}/param-info/{fqoid}/stream
    testCall(catena::REST::Method_GET, 1, "/param-info", "/test/oid", true, {{"recursive", "true"}}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // GET /v1/{slot}/param-info/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/param-info", "/test/oid", false, {{"recursive", "true"}}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // GET /v1/{slot}/subscriptions/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/subscriptions", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // PUT /v1/{slot}/value/{fqoid}
    testCall(catena::REST::Method_GET, 1, "/subscriptions", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
}

/* 
 * TEST 11 - Testing parsing of updates endpoint.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointUpdates) {
    // GET /v1/{slot}/connect
    testCall(catena::REST::Method_GET, 1, "/connect", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
}

/* 
 * TEST 12 - Testing parsing of languages endpoints.
 */
TEST_F(RESTSocketReaderTests, SocketReader_EndpointLanguages) {
    // GET /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_GET, 1, "/langauge-pack", "/en", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // POST /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_POST, 1, "/langauge-pack", "/en", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
    // DELETE /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_DELETE, 1, "/langauge-pack", "/en", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
    // PUT /v1/{slot}/langauge-pack/{language-code}
    testCall(catena::REST::Method_PUT, 1, "/langauge-pack", "/en", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "{test_json_body}");
    // GET /v1/{slot}/langauges
    testCall(catena::REST::Method_GET, 1, "/langauges", "", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", "");
}

/* 
 * TEST 13 - Testing with a long json body.
 */
TEST_F(RESTSocketReaderTests, SocketReader_LongJsonBody) {
    // SocketReader should be able to handle json bodies of any length.
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_NONE, "en", "0.0", std::string(10000, 'a'));
}

/* 
 * TEST 14 - Testing with unset headers.
 */
TEST_F(RESTSocketReaderTests, SocketReader_HeadersUnset) {
    testCall(catena::REST::Method_GET, 1, "/test-call", "/test/oid", false, {}, false, "", "*", st2138::Device_DetailLevel_UNSET, "", "", "");
}

/*
 * TEST 15 - Headers are case-insensitive (per HTTP spec).
 */
TEST_F(RESTSocketReaderTests, SocketReader_HeaderCaseInsensitive) {
    // Enable authz so Authorization header is parsed.
    EXPECT_CALL(service_, authorizationEnabled()).WillRepeatedly(testing::Return(true));
    // Build and send request with non-canonical header casing
    const RESTMethod method = catena::REST::Method_GET;
    const uint32_t slot = 1;
    const std::string endpoint = "/test-call";
    const std::string fqoid = "/test/oid";
    const bool stream = false;
    const std::unordered_map<std::string, std::string> fields = {{"test-field-1", "1"}, {"test-field-2", "2"}};
    const std::string jwsToken = "test-jws-token";
    const std::string origin = "*";
    const auto detailLevel = st2138::Device_DetailLevel_NONE;
    const std::string language = "en";
    const std::string jsonBody = "{test_json_body}";
    const std::string requestStart = "1768323281.123";
    writeRequestWithHeaderNames(method, slot, endpoint, fqoid, stream, fields,
                                jwsToken, origin, detailLevel, language, requestStart, jsonBody,
                                "origin",            // lower-case
                                "AUTHORIZATION",     // upper-case
                                "dEtAiL-LeVeL",      // mixed-case
                                "LANGUAGE",          // upper-case
                                "content-length",    // lower-case
                                "ReQuEsT-sTaRt");    // mixed-case

    socketReader.read(serverSocket_);
    // Validate results identical to normal case
    EXPECT_EQ(socketReader.method(), method);
    EXPECT_EQ(socketReader.slot(), slot);
    EXPECT_EQ(socketReader.endpoint(), endpoint);
    EXPECT_EQ(socketReader.fqoid(), fqoid);
    for (auto [key, value] : fields) {
        EXPECT_EQ(socketReader.hasField(key), true);
        EXPECT_EQ(socketReader.fields(key), value);
    }
    // Authorization header should be parsed despite header-name case differences
    EXPECT_EQ(socketReader.jwsToken(), jwsToken);
    EXPECT_EQ(socketReader.origin(), origin);
    EXPECT_EQ(socketReader.detailLevel(), detailLevel);
    EXPECT_EQ(socketReader.jsonBody(), jsonBody);
    EXPECT_EQ(socketReader.authorizationEnabled(), true);
    EXPECT_EQ(socketReader.stream(), stream);
    EXPECT_EQ(socketReader.requestStart(), std::stod(requestStart));
    EXPECT_EQ(socketReader.requestReceived() > std::stod(requestStart), true);
}

/*
 * TEST 16 - Header line without colon is ignored.
 */
TEST_F(RESTSocketReaderTests, SocketReader_HeaderWithoutColonIgnored) {
    // Enable authz so Authorization header is parsed.
    EXPECT_CALL(service_, authorizationEnabled()).WillRepeatedly(testing::Return(true));
    // Build and send request with an extra malformed header line (no ':')
    const RESTMethod method = catena::REST::Method_GET;
    const uint32_t slot = 1;
    const std::string endpoint = "/test-call";
    const std::string fqoid = "/test/oid";
    const bool stream = false;
    const std::unordered_map<std::string, std::string> fields = {{"test-field-1", "1"}, {"test-field-2", "2"}};
    const std::string jwsToken = "test-jws-token";
    const std::string origin = "*";
    const auto detailLevel = st2138::Device_DetailLevel_NONE;
    const std::string language = "en";
    const std::string jsonBody = "{test_json_body}";
    const std::string requestStart = "1768323281.123";
    writeRequestWithHeaderNames(method, slot, endpoint, fqoid, stream, fields,
                                jwsToken, origin, detailLevel, language, requestStart, jsonBody,
                                "Origin",
                                "Authorization",
                                "Detail-Level",
                                "Language",
                                "Content-Length",
                                "Request-Start",
                                /*extraHeaderLines=*/{"This-Is-A-Bad-Header-Without-Colon"});

    socketReader.read(serverSocket_);
    // Validate results identical to normal case; malformed header should be ignored
    EXPECT_EQ(socketReader.method(), method);
    EXPECT_EQ(socketReader.slot(), slot);
    EXPECT_EQ(socketReader.endpoint(), endpoint);
    EXPECT_EQ(socketReader.fqoid(), fqoid);
    for (auto [key, value] : fields) {
        EXPECT_EQ(socketReader.hasField(key), true);
        EXPECT_EQ(socketReader.fields(key), value);
    }
    EXPECT_EQ(socketReader.jwsToken(), jwsToken);
    EXPECT_EQ(socketReader.origin(), origin);
    EXPECT_EQ(socketReader.detailLevel(), detailLevel);
    EXPECT_EQ(socketReader.jsonBody(), jsonBody);
    EXPECT_EQ(socketReader.authorizationEnabled(), true);
    EXPECT_EQ(socketReader.stream(), stream);
    EXPECT_EQ(socketReader.requestStart(), std::stod(requestStart));
    EXPECT_EQ(socketReader.requestReceived() > std::stod(requestStart), true);
}

/**
 * TEST 17 - Invalid Request-Start value is set to default(0.0)
 */
TEST_F(RESTSocketReaderTests, SocketReader_InvalidRequestStart) {
    // Enable authz so Authorization header is parsed.
    EXPECT_CALL(service_, authorizationEnabled()).WillRepeatedly(testing::Return(true));
    // Build and send request with an extra malformed header line (no ':')
    const RESTMethod method = catena::REST::Method_GET;
    const uint32_t slot = 1;
    const std::string endpoint = "/test-call";
    const std::string fqoid = "/test/oid";
    const bool stream = false;
    const std::unordered_map<std::string, std::string> fields = {{"test-field-1", "1"}, {"test-field-2", "2"}};
    const std::string jwsToken = "test-jws-token";
    const std::string origin = "*";
    const auto detailLevel = st2138::Device_DetailLevel_NONE;
    const std::string language = "en";
    const std::string jsonBody = "{test_json_body}";
    // Test with non-number/period in value
        writeRequestWithHeaderNames(method, slot, endpoint, fqoid, stream, fields,
                                jwsToken, origin, detailLevel, language, "123@.123", jsonBody,
                                "Origin",
                                "Authorization",
                                "Detail-Level",
                                "Language",
                                "Content-Length",
                                "Request-Start");

    socketReader.read(serverSocket_);
    EXPECT_EQ(socketReader.requestStart(), 0.0);
    // Test with multiple periods
        writeRequestWithHeaderNames(method, slot, endpoint, fqoid, stream, fields,
                                jwsToken, origin, detailLevel, language, "123.123.", jsonBody,
                                "Origin",
                                "Authorization",
                                "Detail-Level",
                                "Language",
                                "Content-Length",
                                "Request-Start");

    socketReader.read(serverSocket_);
    EXPECT_EQ(socketReader.requestStart(), 0.0);
    // Test with negative value
        writeRequestWithHeaderNames(method, slot, endpoint, fqoid, stream, fields,
                                jwsToken, origin, detailLevel, language, "-123.123", jsonBody,
                                "Origin",
                                "Authorization",
                                "Detail-Level",
                                "Language",
                                "Content-Length",
                                "Request-Start");

    socketReader.read(serverSocket_);
    EXPECT_EQ(socketReader.requestStart(), 0.0);
}