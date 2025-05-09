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
 * @date 25/05/09
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

#include <Status.h>

namespace fs = std::filesystem;

#include "SocketWriter.h"
using namespace catena::REST;

// Fixture
class RESTSocketWriterTests : public ::testing::Test {
  protected:
    // Connecting sockets (write(server_socket) -> read(client_socket)).
    void SetUp() override {
        clientSocket.connect(acceptor.local_endpoint());
        acceptor.accept(serverSocket);
    }
  
    void TearDown() override { /* Cleanup code here */ }

    // Returns what the writer wrote to the client socket.
    // *Note: This only reads a limited amount of data (up to 4096 bytes). This
    // suffices for testing, mostly because I don't feel like making it dynamic.
    std::string readResponse() {
        boost::asio::streambuf buffer;
        boost::asio::read_until(clientSocket, buffer, "\r\n\r\n");
        std::istream response_stream(&buffer);
        return std::string(std::istreambuf_iterator<char>(response_stream), std::istreambuf_iterator<char>());
    }

    // Returns what the expected answer for should look like for the SocketWriter.
    inline std::string expected(const std::string& jsonBody, const catena::exception_with_status& rc) {
        http_exception_with_status httpStatus = codeMap_.at(rc.status);
        return "HTTP/1.1 " + std::to_string(httpStatus.first) + " " + httpStatus.second + "\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: " + std::to_string(jsonBody.length()) + "\r\n"
               "Connection: close\r\n" +
               CORS +
               "\r\n" +
               jsonBody;
    }

    // Returns what the expected answer for should look like for the SSEWriter.
    inline std::string expectedSSE(const std::vector<std::string> msgs, const catena::exception_with_status& rc) {
        http_exception_with_status httpStatus = codeMap_.at(rc.status);
        // Compiling body response from messages.
        std::string body = "";
        for (std::string msg : msgs) {
            body += "data: " + msg + "\n\n";
        }
        return "HTTP/1.1 " + std::to_string(httpStatus.first) + " " + httpStatus.second + "\r\n"
               "Content-Type: text/event-stream\r\n"
               "Cache-Control: no-cache\r\n"
               "Connection: keep-alive\r\n" +
               CORS +
               "\r\n" +
               body;
    }
    
    boost::asio::io_context io_context;
    tcp::socket clientSocket{io_context};
    tcp::socket serverSocket{io_context};
    tcp::acceptor acceptor{io_context, tcp::endpoint(tcp::v4(), 0)};
    std::string CORS = "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                       "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
                       "Access-Control-Allow-Credentials: true\r\n";
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
    SocketWriter writer(serverSocket);
    writer.finish(msg, rc);

    // Reading from clientSocket and checking the response.
    std::string jsonBody;
    google::protobuf::util::JsonPrintOptions options; // Default options
    auto status = google::protobuf::util::MessageToJsonString(msg, &jsonBody, options);
    EXPECT_EQ(readResponse(), expected(jsonBody, rc));
}

/* 
 * TEST 2 - SocketWriter writes no content.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_Write204) {
    // msg variables.
    catena::exception_with_status rc("", catena::StatusCode::NO_CONTENT);
    catena::Value msg;
    msg.set_string_value("Test string");

    // Initializing SocketWriter with serverSocket and writing message.
    SocketWriter writer(serverSocket);
    writer.finish(msg, rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expected("", rc));
}

/* 
 * TEST 2 - SocketWriter writes an error.
 */
TEST_F(RESTSocketWriterTests, SocketWriter_WriteErr) {
    // msg variables.
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    catena::Value msg;
    msg.set_string_value("Test string");

    // Initializing SocketWriter with serverSocket and writing message.
    SocketWriter writer(serverSocket);
    writer.finish(msg, rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expected("", rc));
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
    SSEWriter writer(serverSocket);
    for (std::string msgJson : msgs) {
        catena::Value msg;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(msgJson), &msg);
        writer.write(msg);
    }

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSE(msgs, rc));
}

/* 
 * TEST 2 - SSEWriter writes an error before its first message write.
 */
TEST_F(RESTSocketWriterTests, SSEWriter_WriteErrBegin) {
    // msg variables.
    catena::exception_with_status rc("Invalid argument", catena::StatusCode::INVALID_ARGUMENT);
    std::vector<std::string> msgs = {};
    catena::Empty emptyMsg = catena::Empty();

    // Initializing SSEWriter with serverSocket and writing error.
    SSEWriter writer(serverSocket);
    writer.finish(emptyMsg, rc);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSE(msgs, rc));
}

/* 
 * TEST 3 - SSEWriter writes an error in the middle of its process.
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
    SSEWriter writer(serverSocket);
    for (std::string msgJson : msgs) {
        catena::Value msg;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(msgJson), &msg);
        writer.write(msg);
    }
    writer.finish(emptyMsg, err);

    // Reading from clientSocket and checking the response.
    EXPECT_EQ(readResponse(), expectedSSE(msgs, rc));
}
