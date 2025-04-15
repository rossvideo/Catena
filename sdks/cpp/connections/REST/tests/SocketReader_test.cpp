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
 * @date 25/04/15
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>

namespace fs = std::filesystem;

#include "SocketReader.h"

// // Mocking the SocketReader interface
// class MockSocketReader : public catena::REST::ISocketReader {
//     public:
//         MOCK_METHOD(void, read, (tcp::socket& socket, bool authz), (override));
//         MOCK_METHOD(std::string&, method, (), (const, override));
//         MOCK_METHOD(std::string&, rpc, (), (const, override));
//         MOCK_METHOD(void, fields, ((std::unordered_map<std::string, std::string>&) fieldMap), (const, override));
//         MOCK_METHOD(std::string&, jwsToken, (), (const, override));
//         MOCK_METHOD(std::string&, jsonBody, (), (const, override));
//         MOCK_METHOD(bool, authorizationEnabled, (), (const, override));
// };

// Writes a request to and returns server socket for SocketReader tests.
std::unique_ptr<tcp::socket> mockServerSocket() {
    boost::asio::io_context io_context;
    tcp::socket serverSocket(io_context);
    tcp::socket clientSocket(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
    // Linking client and server sockets.
    serverSocket.connect(acceptor.local_endpoint());
    acceptor.accept(clientSocket);
    // Writing request to server.
    std::string request = "GET localhost:443/v1/GetValue/slot/1/oid/text_box HTTP/1.1\n"
                          "Origin: test_origin\n"
                          "User-Agent: test_agent\n"
                          "Authorization: Bearer test_bearer \n"
                          "Content-Length: 15\n"
                          "\r\n"
                          "{\n"
                          "  test_body\n"
                          "}\n"
                          "\r\n\r\n";
    boost::asio::write(clientSocket, boost::asio::buffer(request));

    // Returning the serverSocket.
    return std::make_unique<tcp::socket>(std::move(serverSocket));
}

// Testing SocketReader with authorization disabled.
TEST(REST_API_tests, SocketReader_NormalCase) {
    auto serverSocket = mockServerSocket();
    // Creating a SocketReader and reading the request.
    catena::REST::SocketReader socketReader;
    socketReader.read(*serverSocket, false);
    // Reading extracted fields from the socketReader.
    std::unordered_map<std::string, std::string> fields = {
        {"oid", ""},
        {"slot", ""}
    };
    socketReader.fields(fields);
    // Checking answers.
    EXPECT_EQ(socketReader.method(),               "GET"              );
    EXPECT_EQ(socketReader.rpc(),                  "/v1/GetValue"     );
    EXPECT_EQ(fields.at("slot"),                   "1"                );
    EXPECT_EQ(fields.at("oid"),                    "text_box"         );
    EXPECT_EQ(socketReader.authorizationEnabled(), false              );
    EXPECT_EQ(socketReader.jwsToken(),             ""                 );
    EXPECT_EQ(socketReader.origin(),               "test_origin"      );
    EXPECT_EQ(socketReader.jsonBody(),             "{\n  test_body\n}");
}

// Testing SocketReader with authorization enabled.
TEST(REST_API_tests, SocketReader_AuthzCase) {
    auto serverSocket = mockServerSocket();
    // Creating a SocketReader and reading the request.
    catena::REST::SocketReader socketReader;
    socketReader.read(*serverSocket, true);
    // Getting fields from the SocketReader.
    std::unordered_map<std::string, std::string> fields = {
        {"oid", ""},
        {"slot", ""}
    };
    socketReader.fields(fields);
    // Checking answer.
    EXPECT_EQ(socketReader.method(),               "GET"              );
    EXPECT_EQ(socketReader.rpc(),                  "/v1/GetValue"     );
    EXPECT_EQ(fields.at("slot"),                   "1"                );
    EXPECT_EQ(fields.at("oid"),                    "text_box"         );
    EXPECT_EQ(socketReader.authorizationEnabled(), true               );
    EXPECT_EQ(socketReader.jwsToken(),             "test_bearer"      );
    EXPECT_EQ(socketReader.origin(),               "test_origin"      );
    EXPECT_EQ(socketReader.jsonBody(),             "{\n  test_body\n}");
}
