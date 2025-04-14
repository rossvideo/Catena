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

#include <interface/device.pb.h>

#include <Status.h>

namespace fs = std::filesystem;

#include "SocketWriter.h"

TEST(REST_API_tests, SocketWriter_NormalWrite) {
    // Variables.
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    tcp::socket server_socket(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
    // Connecting client and server sockets and writing message.
    client_socket.connect(acceptor.local_endpoint());
    acceptor.accept(server_socket);
    catena::Empty ans = catena::Empty();
    catena::REST::SocketWriter writer(server_socket);
    writer.write(ans);
    // Reading and checking the response from the server socket.
    boost::asio::streambuf buffer;
    boost::asio::read_until(client_socket, buffer, "\r\n\r\n");
    std::istream response_stream(&buffer);
    std::string response;
    std::getline(response_stream, response);
    EXPECT_EQ(response, "HTTP/1.1 200 OK\r");
    // Cycle to json body
    while(std::getline(response_stream, response) && response != "\r") {}
    // Checking the json body.
    std::string jsonBody;
    std::getline(response_stream, jsonBody);
    EXPECT_EQ(jsonBody, "{}");
}

TEST(REST_API_tests, SocketWriter_WriteError) {
    // Variables.
    boost::asio::io_context io_context;
    tcp::socket client_socket(io_context);
    tcp::socket server_socket(io_context);
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
    // Connecting client and server sockets and writing message.
    client_socket.connect(acceptor.local_endpoint());
    acceptor.accept(server_socket);
    catena::exception_with_status error("test_error", catena::StatusCode::INVALID_ARGUMENT);
    catena::REST::SocketWriter writer(server_socket);
    writer.write(error);
    // Reading and checking the response from the server socket.
    boost::asio::streambuf buffer;
    boost::asio::read_until(client_socket, buffer, "\r\n\r\n");
    std::istream response_stream(&buffer);
    std::string response;
    std::getline(response_stream, response);
    EXPECT_EQ(response, "HTTP/1.1 406 test_error\r");
}

TEST(REST_API_tests, SocketWriter_WriteMultiple) {}

TEST(REST_API_tests, SSEWriter_NormalWrite) {}

TEST(REST_API_tests, SSEWriter_WriteError) {}

TEST(REST_API_tests, SSEWriter_WriteMultiple) {}
