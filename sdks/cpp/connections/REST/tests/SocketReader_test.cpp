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

// Common
#include <SubscriptionManager.h>

namespace fs = std::filesystem;

#include "SocketReader.h"

// Fixture
class RESTSocketReaderTests : public ::testing::Test {
    protected:
    // Writes a request to a socket to later be read by the SocketReader.
    void SetUp() override {
        tcp::socket clientSocket(io_context);
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 0));
        // Linking client and server sockets.
        serverSocket.connect(acceptor.local_endpoint());
        acceptor.accept(clientSocket);
        // Compiling fields
        std::string fieldsStr = "";
        for (auto [key, value] : fields) {
            if (!fieldsStr.empty()) {
                fieldsStr += "&";
            }
            fieldsStr += key + "=" + value;
        }
        // Writing request to server.
        std::string request = method + " " + service + "/" + std::to_string(slot) + "?" + fieldsStr + " HTTP/1.1\n"
                            "Origin: " + origin + "\n"
                            "User-Agent: test_agent\n"
                            "Authorization: Bearer " + jwsToken + " \n"
                            "Content-Length: " + std::to_string(jsonBody.length()) + "\n"
                            "\r\n" + jsonBody + "\n"
                            "\r\n\r\n";
        boost::asio::write(clientSocket, boost::asio::buffer(request));
    }
  
    void TearDown() override { /* Cleanup code here */ }
    
    boost::asio::io_context io_context;
    tcp::socket serverSocket{io_context};
    catena::common::SubscriptionManager sm;    
    catena::REST::SocketReader socketReader{sm};

    // Test request data
    std::string method = "PUT";
    std::string service = "/v1/test-call";
    uint32_t slot = 1;
    // Should NOT have a field called "doesNotExist".
    std::unordered_map<std::string, std::string> fields = {
        {"testField1", "1"},
        {"testField2", "2"}
    };
    std::string jwsToken = "test_bearer";
    std::string origin = "test_origin";
    std::string jsonBody = "{\n  test_body\n}";
};

// Testing SocketReader with authorization disabled.
TEST_F(RESTSocketReaderTests, SocketReader_NormalCase) {
    // Reading the request from the serverSocket.
    socketReader.read(serverSocket, false);
    // Checking answers.
    EXPECT_EQ(socketReader.method(),               method    );
    EXPECT_EQ(socketReader.service(),              service   );
    EXPECT_EQ(socketReader.slot(),                 slot      );
    for (auto [key, value] : fields) {
        EXPECT_EQ(socketReader.hasField(key),       true     );
        EXPECT_EQ(socketReader.fields(key),         value    );
    }
    EXPECT_EQ(socketReader.hasField("doesNotExist"), false   );
    EXPECT_EQ(socketReader.fields("doesNotExist"),   ""      );
    EXPECT_EQ(socketReader.authorizationEnabled(),   false   );
    EXPECT_EQ(socketReader.jwsToken(),               ""      );
    EXPECT_EQ(socketReader.origin(),                 origin  );
    EXPECT_EQ(socketReader.jsonBody(),               jsonBody);
}

// Testing SocketReader with authorization enabled.
TEST_F(RESTSocketReaderTests, SocketReader_AuthzCase) {
    // Reading the request from the serverSocket.
    socketReader.read(serverSocket, true);
    // Checking answers.
    EXPECT_EQ(socketReader.method(),               method    );
    EXPECT_EQ(socketReader.service(),              service   );
    EXPECT_EQ(socketReader.slot(),                 slot      );
    for (auto [key, value] : fields) {
        EXPECT_EQ(socketReader.hasField(key),       true     );
        EXPECT_EQ(socketReader.fields(key),         value    );
    }
    EXPECT_EQ(socketReader.hasField("doesNotExist"), false   );
    EXPECT_EQ(socketReader.fields("doesNotExist"),   ""      );
    EXPECT_EQ(socketReader.authorizationEnabled(),   true    );
    EXPECT_EQ(socketReader.jwsToken(),               jwsToken);
    EXPECT_EQ(socketReader.origin(),                 origin  );
    EXPECT_EQ(socketReader.jsonBody(),               jsonBody);
}

// Testing to make sure SocketReader saved the Sub Manager.
// TEST_F(RESTSocketReaderTests, SocketReader_GetSubManager) {
//     // Checking answers.
//     EXPECT_EQ(socketReader.getSubscriptionManager(), sm);
// }
