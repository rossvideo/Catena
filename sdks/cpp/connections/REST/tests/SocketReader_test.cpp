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

//#include "../src/Device.cpp" // Include the file to test
// 

#include "interface/ISocketReader.h"

// Mocking the SocketReader interface
class MockSocketReader : public catena::REST::ISocketReader {
    public:
        MOCK_METHOD(void, read, (tcp::socket& socket, bool authz), (override));
        MOCK_METHOD(std::string&, method, (), (const, override));
        MOCK_METHOD(std::string&, rpc, (), (const, override));
        MOCK_METHOD(void, fields, ((std::unordered_map<std::string, std::string>&) fieldMap), (const, override));
        MOCK_METHOD(std::string&, jwsToken, (), (const, override));
        MOCK_METHOD(std::string&, jsonBody, (), (const, override));
        MOCK_METHOD(bool, authorizationEnabled, (), (const, override));
};

TEST(SocketReaderTests, SocketReader_read) {
    // Create a mock socket
    boost::asio::io_context io_context;
    tcp::socket mock_socket(io_context);
    mock_socket.send(boost::asio::buffer("GET http://localhost:443/v1/GetValue/slot/1/oid/text_box"));

    // Create a mock SocketReader
    MockSocketReader mock_reader;

    // Set up expectations
    EXPECT_CALL(mock_reader, read(testing::_, testing::_)).Times(1);

    // Call the read method
    mock_reader.read(mock_socket, false);
}
