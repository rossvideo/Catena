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

// protobuf
#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

#include <Status.h>

namespace fs = std::filesystem;

#include "SocketWriter.h"

// Converts entire socket response to string.
std::string read(tcp::socket& socket) {
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, "\r\n\r\n");
    std::istream response_stream(&buffer);
    return std::string(std::istreambuf_iterator<char>(response_stream), std::istreambuf_iterator<char>());
}

/*
 * Generates what the answer for SocketWriter should look like depending on
 * status and jsonBody.
 */
inline std::string SocketWriterAns(const std::string& jsonBody) {
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: " + std::to_string(jsonBody.length()) + "\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With\r\n"
           "Access-Control-Allow-Credentials: true\r\n"
           "Connection: close\r\n"
           "\r\n" +
           jsonBody;
}

// Testing writing one msg with SocketWriter.
TEST(REST_API_tests, SocketWriter_NormalWrite) {
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    tcp::socket server_socket(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
    // Linking client and server sockets.
    client_socket.connect(acceptor.local_endpoint());
    acceptor.accept(server_socket);
    // Creating server socket writer and writing message.
    catena::REST::SocketWriter writer(server_socket);
    catena::Empty msg = catena::Empty();
    writer.finish(msg);
    // Reading and checking the response from the client socket.
    EXPECT_EQ(read(client_socket), SocketWriterAns("{}\n"));
}

// Testing writing errors with SocketWriter.
TEST(REST_API_tests, SocketWriter_WriteError) {   
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    tcp::socket server_socket(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));

    // Linking client and server sockets.
    client_socket.connect(acceptor.local_endpoint());
    acceptor.accept(server_socket);

    // Creating server socket writer and writing error message.
    catena::REST::SocketWriter writer(server_socket);
    catena::exception_with_status msg("test_error", catena::StatusCode::INVALID_ARGUMENT);
    writer.write(msg);

    // Expected answer
    std::string ans = "HTTP/1.1 406 test_error\r\n"
                      "Content-Type: text/plain\r\n"
                      "Content-Length: 10\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                      "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With\r\n"
                      "Access-Control-Allow-Credentials: true\r\n"
                      "Connection: close\r\n"
                      "\r\n"
                      "test_error";    

    // Reading and checking the response from the client socket.
    EXPECT_EQ(read(client_socket), ans);
}

// Testing writing muliple buffered messages with SocketWriter.
TEST(REST_API_tests, SocketWriter_WriteMultiple) {
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    tcp::socket server_socket(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
    // Linking client and server sockets.
    client_socket.connect(acceptor.local_endpoint());
    acceptor.accept(server_socket);
    
    // Creating server socket writer and writing messages.
    catena::REST::SocketWriter writer(server_socket);
    std::vector<std::string> msgs = {
        "{\n \"stringValue\": \"Test string #1\"\n}",
        "{\n \"float32Value\": 2\n}",
        "{\n \"stringValue\": \"Test string #3\"\n}",
        "{\n \"stringValue\": \"Test string #4\"\n}",
        "{\n \"int32Value\": 5\n}",
    };
    for (std::string msgJson : msgs) {
        catena::Value msg;
        auto status = google::protobuf::util::JsonStringToMessage(absl::string_view(msgJson), &msg);
        writer.write(msg);
    }
    writer.finish();

    // Generating the expected response json body from msgs.
    std::string resBody = "{\n\"response\": [\n" + msgs[0];
    for (int i = 1; i < msgs.size(); i += 1) {
            resBody += ",\n" + msgs[i];
    }
    resBody += "\n]\n}";

    // Reading and checking the response from the server socket.
    EXPECT_EQ(read(client_socket), SocketWriterAns(resBody));
}

/**
 * @todo Below we are waiting on SSEWriter to be merged with dev.
 */

// Testing writing a number of messages with SSEWriter.
TEST(REST_API_tests, SSEWriter_NormalWrite) {}

// Testing writing a number of errors with SSEWriter.
TEST(REST_API_tests, SSEWriter_WriteError) {}

// Testing writing a mix of messages and errors with SSEWriter.
TEST(REST_API_tests, SSEWriter_WriteMix) {}
