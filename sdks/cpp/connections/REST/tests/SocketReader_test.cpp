//  Copyright 2025 Ross Video Ltd
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  1. Redistributions of source code must retain the above copyright notice,
//  this list of conditions and the following disclaimer.
//  
//  2. Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//  
//  3. Neither the name of the copyright holder nor the names of its
//  contributors may be used to endorse or promote products derived from this
//  software without specific prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
// 
// 
//  this file is for testing the Device.cpp file
//
//  Author: benjamin.whitten@rossvideo.com
//  Date: 25/04/11
// 
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

// Helper function which writes a fake request to the client socket.
void mockRequest(tcp::socket& client_socket, boost::asio::io_context& io_context) {
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
    tcp::socket server_socket(io_context);
    std::thread server_thread([&]() {
        // Accepting client connection and writing a fake GET request.
        acceptor.accept(server_socket);
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
        boost::asio::write(server_socket, boost::asio::buffer(request));
    });
    // Connecting the client socket to the acceptor and waiting for response.
    client_socket.connect(acceptor.local_endpoint());
    server_thread.join();
}

// Testing SocketReader with authorization disabled.
TEST(SocketReaderTests, SocketReader_read) {
    // Creating a client socket and sending a fake GET request to it.
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    mockRequest(client_socket, io_context);
    // Creating a SocketReader and reading the request.
    catena::REST::SocketReader socket_reader;
    socket_reader.read(client_socket, false);
    // Getting answers from the SocketReader.
    std::string method = socket_reader.method();
    std::string call = socket_reader.rpc();
    std::unordered_map<std::string, std::string> fields = {
        {"oid", ""},
        {"slot", ""}
    };
    socket_reader.fields(fields);
    bool authz = socket_reader.authorizationEnabled();
    std::string token = socket_reader.jwsToken();
    std::string origin = socket_reader.origin();
    std::string userAgent = socket_reader.userAgent();
    std::string jsonBody = socket_reader.jsonBody();
    // Checking answer.
    EXPECT_EQ(method, "GET");
    EXPECT_EQ(call, "/v1/GetValue");
    EXPECT_EQ(fields.at("slot"), "1");
    EXPECT_EQ(fields.at("oid"), "text_box");
    EXPECT_EQ(authz, false);
    EXPECT_EQ(token, "");
    EXPECT_EQ(origin, "test_origin");
    EXPECT_EQ(userAgent, "test_agent");
    EXPECT_EQ(jsonBody, "{\n  test_body\n}");
}

// Testing SocketReader with authorization enabled.
TEST(SocketReaderTests, SocketReader_read_athz) {
    // Creating a client socket and sending a fake GET request to it.
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    mockRequest(client_socket, io_context);
    // Creating a SocketReader and reading the request.
    catena::REST::SocketReader socket_reader;
    socket_reader.read(client_socket, true);
    // Getting answers from the SocketReader.
    std::string method = socket_reader.method();
    std::string call = socket_reader.rpc();
    std::unordered_map<std::string, std::string> fields = {
        {"oid", ""},
        {"slot", ""}
    };
    socket_reader.fields(fields);
    bool authz = socket_reader.authorizationEnabled();
    std::string token = socket_reader.jwsToken();
    std::string origin = socket_reader.origin();
    std::string userAgent = socket_reader.userAgent();
    std::string jsonBody = socket_reader.jsonBody();
    // Checking answer.
    EXPECT_EQ(method, "GET");
    EXPECT_EQ(call, "/v1/GetValue");
    EXPECT_EQ(fields.at("slot"), "1");
    EXPECT_EQ(fields.at("oid"), "text_box");
    EXPECT_EQ(authz, true);
    EXPECT_EQ(token, "test_bearer");
    EXPECT_EQ(origin, "test_origin");
    EXPECT_EQ(userAgent, "test_agent");
    EXPECT_EQ(jsonBody, "{\n  test_body\n}");
}
