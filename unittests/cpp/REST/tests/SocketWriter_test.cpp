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

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

// Test helpers
#include "RESTTest.h"

// REST
#include "SocketWriter.h"
using namespace catena::REST;

// Fixture
class RESTSocketWriterTests : public ::testing::Test, public RESTTest {
  protected:
    RESTSocketWriterTests() : SocketHelper(&serverSocket, &clientSocket) {
        origin = "test-origin.com";
    }
  
    void SetUp() override { /* Setup code here */ }
  
    void TearDown() override { /* Cleanup code here */ }
};

/*
 * ============================================================================
 *                             SocketWriter tests
 * ============================================================================
 * 
 * TEST 1 - SocketWriter writes a message without error.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_Write200) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    catena::Value msg;
    msg.set_string_value("Test string");

    // Initializing SocketWriter with serverSocket and writing message.
    SocketWriter writer(serverSocket, origin);
    writer.sendResponse(rc, msg);

    // Reading from clientSocket and checking the response.
    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(msg, &jsonBody, options);
    EXPECT_EQ(readResponse(), expectedResponse(rc, jsonBody));
}

/* 
 * TEST 2 - SocketWriter writes no content.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_Write204) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::NO_CONTENT);
    catena::Empty emptyMsg = catena::Empty();

    // Initializing SocketWriter with serverSocket and writing message.
    SocketWriter writer(serverSocket, origin);
    writer.sendResponse(rc, emptyMsg);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 3 - SocketWriter writes an error.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_WriteErr) {
    // msg variables.
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    catena::Value msg;
    msg.set_string_value("Test string");

    // Initializing SocketWriter with serverSocket and writing message.
    SocketWriter writer(serverSocket, origin);
    writer.sendResponse(rc, msg);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 4 - SocketWriter buffers multiple messages.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_Buffer200) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::vector<catena::Value> msgs = {catena::Value(), catena::Value(), catena::Value()};
    msgs[0].set_string_value("test-string-1");
    msgs[1].set_string_value("test-string-2");
    msgs[2].set_string_value("test-string-3");
    std::string expJson = "";
    google::protobuf::util::JsonPrintOptions options; // Default options

    // Initializing SocketWriter with serverSocket and writing messages.
    SocketWriter writer(serverSocket, origin, true);
    for (const auto& msg : msgs) {
        writer.sendResponse(rc, msg);
        // Adding to expected response.
        std::string jsonBody;
        auto status = google::protobuf::util::MessageToJsonString(msg, &jsonBody, options);
        if (expJson.empty()) {
            expJson = "{\"data\":[" + jsonBody;
        } else {
            expJson += "," + jsonBody;
        }
    }
    expJson += "]}";
    // Writing an empty message to flush response.
    writer.sendResponse(rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedResponse(rc, expJson));
}

/* 
 * TEST 5 - SocketWriter writes an error with buffer = true.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_BufferErrBegin) {
    // msg variables.
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    
    // Initializing SocketWriter with serverSocket and writing error.
    SocketWriter writer(serverSocket, origin, true);
    writer.sendResponse(rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}

/* 
 * TEST 6 - SocketWriter writes an error after buffering multiple messages.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_BufferErrEnd) {
    // msg variables.
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    std::vector<catena::Value> msgs = {catena::Value(), catena::Value(), catena::Value()};
    msgs[0].set_string_value("test-string-1");
    msgs[1].set_string_value("test-string-2");
    msgs[2].set_string_value("test-string-3");

    // Initializing SocketWriter with serverSocket and writing messages.
    SocketWriter writer(serverSocket, origin, true);
    for (const auto& msg : msgs) {
        writer.sendResponse(catena::exception_with_status("", catena::StatusCode::OK), msg);
    }
    // Writing error.
    writer.sendResponse(rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedResponse(rc));
}


/*
 * ============================================================================
 *                               SSEWriter tests
 * ============================================================================
 * 
 * TEST 1 - SSEWriter writes a four messages without error.
 */
TEST_F(RESTSocketWriterTests, SSEWriter_Write200) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::vector<std::string> msgs = {
        "{\"stringValue\":\"Test string #1\"}",
        "{\"float32Value\":2}",
        "{\"stringValue\":\"Test string #3\"}",
        "{\"int32Value\":5}",
    };

    // Initializing SSEWriter with serverSocket and writing messages.
    SSEWriter writer(serverSocket, origin);
    for (std::string msgJson : msgs) {
        catena::Value msg;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(msgJson), &msg);
        writer.sendResponse(rc, msg);
    }

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, msgs));
}

/* 
 * TEST 2 - SSEWriter writes an empty message.
 */
TEST_F(RESTSocketWriterTests, SSEWriter_WriteEmpty) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::vector<std::string> msgs = {};
    catena::Empty emptyMsg = catena::Empty();

    // Initializing SSEWriter with serverSocket and writing messages.
    SSEWriter writer(serverSocket, origin);
    writer.sendResponse(rc, emptyMsg);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, msgs));
}

/* 
 * TEST 3 - SSEWriter writes an empty message in the middle of its process.
 */
TEST_F(RESTSocketWriterTests, SSEWriter_WriteEmptyEnd) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::vector<std::string> msgs = {
        "{\"stringValue\":\"Test string #1\"}",
        "{\"float32Value\":2}"
    };
    catena::Empty emptyMsg = catena::Empty();

    // Initializing SSEWriter with serverSocket and writing messages.
    SSEWriter writer(serverSocket, origin);
    for (std::string msgJson : msgs) {
        catena::Value msg;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(msgJson), &msg);
        writer.sendResponse(rc, msg);
    }
    writer.sendResponse(rc, emptyMsg);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, msgs));
}

/* 
 * TEST 4 - SSEWriter writes an error before its first message write.
 */
TEST_F(RESTSocketWriterTests, SSEWriter_WriteErrBegin) {
    // msg variables.
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    catena::Empty emptyMsg = catena::Empty();

    // Initializing SSEWriter with serverSocket and writing error.
    SSEWriter writer(serverSocket, origin);
    writer.sendResponse(rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc));
}

/* 
 * TEST 5 - SSEWriter writes an error in the middle of its process.
 */
TEST_F(RESTSocketWriterTests, SSEWriter_WriteErrEnd) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::vector<std::string> msgs = {
        "{\"stringValue\":\"Test string #1\"}",
        "{\"float32Value\":2}"
    };
    catena::exception_with_status err("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    catena::Empty emptyMsg = catena::Empty();

    // Initializing SSEWriter with serverSocket and writing messages.
    SSEWriter writer(serverSocket, origin);
    for (std::string msgJson : msgs) {
        catena::Value msg;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(msgJson), &msg);
        writer.sendResponse(rc, msg);
    }
    writer.sendResponse(rc, emptyMsg);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSEResponse(rc, msgs));
}
