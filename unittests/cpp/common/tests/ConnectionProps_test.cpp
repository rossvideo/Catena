/*
 * Copyright 2026 Ross Video Ltd
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
 * @brief This file is for testing the ConnectionProps class.
 * @author christian.twarog@rossvideo.com
 * @date 2026-02-05
 * @copyright Copyright © 2026 Ross Video Ltd
 */

#include <ConnectionProps.h>

// gtest
#include <gtest/gtest.h>

// common
#include <Logger.h>
#include "SharedFlags.h"

// boost
#include <boost/asio.hpp>

// std
#include <thread>
#include <chrono>
#include <regex>

using namespace catena::common;
using boost::asio::ip::tcp;

// Test fixture for ConnectionProps tests
class ConnectionPropsTest : public testing::Test {
  protected:
    // Set up and tear down Google Logging
    static void SetUpTestSuite() {
      absl::SetFlag(&FLAGS_log_dir, UNITTEST_LOG_DIR);
      Logger::init("ConnectionPropsTest");
    }

    static void TearDownTestSuite() {
    }

    // Helper function to make an HTTP GET request and return the response
    std::string makeHttpRequest(uint16_t port, const std::string& path) {
        try {
            boost::asio::io_context io_context;
            tcp::socket socket(io_context);
            tcp::resolver resolver(io_context);
            
            // Connect to localhost on the specified port
            auto endpoints = resolver.resolve("127.0.0.1", std::to_string(port));
            boost::asio::connect(socket, endpoints);
            
            // Send HTTP GET request
            std::string request = 
                "GET " + path + " HTTP/1.1\r\n"
                "Host: localhost\r\n"
                "Connection: close\r\n"
                "\r\n";
            boost::asio::write(socket, boost::asio::buffer(request));
            
            // Read response
            boost::asio::streambuf response_buf;
            boost::system::error_code error;
            boost::asio::read(socket, response_buf, boost::asio::transfer_all(), error);
            
            if (error && error != boost::asio::error::eof) {
                return "";
            }
            
            std::istream response_stream(&response_buf);
            std::string response;
            std::string line;
            while (std::getline(response_stream, line)) {
                response += line + "\n";
            }
            
            return response;
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    // Helper to extract status code from HTTP response
    int getStatusCode(const std::string& response) {
        std::regex status_regex(R"(HTTP/1\.1 (\d+))");
        std::smatch match;
        if (std::regex_search(response, match, status_regex)) {
            return std::stoi(match[1]);
        }
        return -1;
    }
    
    // Helper to extract body from HTTP response
    std::string getBody(const std::string& response) {
        size_t body_start = response.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            return response.substr(body_start + 4);
        }
        // Try with just \n\n (our helper uses \n)
        body_start = response.find("\n\n");
        if (body_start != std::string::npos) {
            std::string body = response.substr(body_start + 2);
            // Remove trailing newlines
            while (!body.empty() && body.back() == '\n') {
                body.pop_back();
            }
            return body;
        }
        return "";
    }

    // Helper to check if response contains a header
    bool hasHeader(const std::string& response, const std::string& header) {
        return response.find(header) != std::string::npos;
    }
};

/*
 * TEST 1 - Testing ConnectionProps constructor and getters with config.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_ConstructorAndGetters) {
    ConnectionPropsConfig config;
    config.protocol = "grpc";
    config.address = "192.168.1.100";
    config.service_port = 50051;
    config.node_id = "test-node-123";
    config.node_name = "Test Node";
    
    std::string endpoint = "/test/endpoint.xml";
    uint16_t port = 8081;
    
    ConnectionProps server(config, endpoint, port);
    
    EXPECT_EQ(server.getEndpoint(), endpoint) << "Endpoint should match constructor parameter";
    EXPECT_EQ(server.getPort(), port) << "Port should match constructor parameter";
    EXPECT_FALSE(server.isRunning()) << "Server should not be running initially";
}

/*
 * TEST 2 - Testing ConnectionProps start and stop.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_StartStop) {
    ConnectionPropsConfig config;
    config.protocol = "rest";
    config.address = "localhost";
    config.service_port = 443;
    
    ConnectionProps server(config, "/connect/test.xml", 8082);
    
    // Start the server
    EXPECT_TRUE(server.start()) << "Server should start successfully";
    EXPECT_TRUE(server.isRunning()) << "Server should be running after start";
    
    // Give server a moment to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop the server
    server.stop();
    EXPECT_FALSE(server.isRunning()) << "Server should not be running after stop";
}

/*
 * TEST 3 - Testing ConnectionProps with server already running.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_AlreadyRunning) {
    ConnectionPropsConfig config;
    config.protocol = "grpc";
    config.address = "localhost";
    config.service_port = 6254;
    
    ConnectionProps server(config, "/connect/test.xml", 8083);
    
    // Start the server
    EXPECT_TRUE(server.start());
    EXPECT_TRUE(server.isRunning());
    
    // Try to start again
    EXPECT_FALSE(server.start()) << "Starting already running server should return false";
    EXPECT_TRUE(server.isRunning()) << "Server should still be running";
    
    server.stop();
}

/*
 * TEST 4 - Testing HTTP response for configured endpoint with generated XML.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_ServeEndpoint) {
    ConnectionPropsConfig config;
    config.protocol = "grpc";
    config.address = "192.168.1.100";
    config.service_port = 50051;
    config.node_id = "test-device-abc123";
    config.node_name = "Test Device";
    config.refresh_interval = 15000;
    
    std::string endpoint = "/connect/connection-props.xml";
    ConnectionProps server(config, endpoint, 8084);
    
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Make request to the configured endpoint
    std::string response = makeHttpRequest(8084, endpoint);
    
    EXPECT_NE(response, "") << "Should receive a response";
    EXPECT_EQ(getStatusCode(response), 200) << "Should return 200 OK";
    EXPECT_TRUE(hasHeader(response, "Content-Type: application/xml")) 
        << "Should have XML content type";
    EXPECT_TRUE(hasHeader(response, "Access-Control-Allow-Origin: *")) 
        << "Should have CORS header";
    
    std::string body = getBody(response);
    // Verify generated XML contains expected values
    EXPECT_TRUE(body.find("<properties version=\"1.0\">") != std::string::npos);
    EXPECT_TRUE(body.find("equipmentType\">grpc<") != std::string::npos);
    EXPECT_TRUE(body.find("address\">192.168.1.100<") != std::string::npos);
    EXPECT_TRUE(body.find("port\">50051<") != std::string::npos);
    EXPECT_TRUE(body.find("node-id\">test-device-abc123<") != std::string::npos);
    EXPECT_TRUE(body.find("node-name\">Test Device<") != std::string::npos);
    EXPECT_TRUE(body.find("refresh-interval\">15000<") != std::string::npos);
    
    server.stop();
}

/*
 * TEST 5 - Testing HTTP response for unknown endpoint (404).
 */
TEST_F(ConnectionPropsTest, ConnectionProps_404Response) {
    ConnectionPropsConfig config;
    config.protocol = "rest";
    config.address = "localhost";
    config.service_port = 443;
    
    std::string endpoint = "/connect/connection-props.xml";
    ConnectionProps server(config, endpoint, 8086);
    
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Make request to unknown endpoint
    std::string response = makeHttpRequest(8086, "/unknown/path");
    
    EXPECT_NE(response, "") << "Should receive a response";
    EXPECT_EQ(getStatusCode(response), 404) << "Should return 404 Not Found";
    EXPECT_TRUE(hasHeader(response, "Content-Type: text/plain")) 
        << "Should have plain text content type";
    
    std::string body = getBody(response);
    EXPECT_TRUE(body.find("Not Found") != std::string::npos) 
        << "404 response should contain 'Not Found'";
    EXPECT_TRUE(body.find(endpoint) != std::string::npos) 
        << "404 response should mention the correct endpoint";
    
    server.stop();
}

/*
 * TEST 6 - Testing updateConfig functionality.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_UpdateContent) {
    ConnectionPropsConfig initial_config;
    initial_config.protocol = "grpc";
    initial_config.address = "localhost";
    initial_config.service_port = 50051;
    initial_config.node_name = "Initial Node";
    
    std::string endpoint = "/connect/test.xml";
    ConnectionProps server(initial_config, endpoint, 8087);
    
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify initial content
    std::string response1 = makeHttpRequest(8087, endpoint);
    std::string body1 = getBody(response1);
    EXPECT_TRUE(body1.find("node-name\">Initial Node<") != std::string::npos);
    EXPECT_TRUE(body1.find("port\">50051<") != std::string::npos);
    
    // Update config
    ConnectionPropsConfig updated_config;
    updated_config.protocol = "rest";
    updated_config.address = "192.168.1.200";
    updated_config.service_port = 443;
    updated_config.node_name = "Updated Node";
    updated_config.use_tls = true;
    
    server.updateConfig(updated_config);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Verify updated content
    std::string response2 = makeHttpRequest(8087, endpoint);
    std::string body2 = getBody(response2);
    EXPECT_TRUE(body2.find("equipmentType\">rest<") != std::string::npos) 
        << "Protocol should be updated to REST";
    EXPECT_TRUE(body2.find("address\">192.168.1.200<") != std::string::npos)
        << "Address should be updated";
    EXPECT_TRUE(body2.find("port\">443<") != std::string::npos)
        << "Port should be updated";
    EXPECT_TRUE(body2.find("node-name\">Updated Node<") != std::string::npos)
        << "Node name should be updated";
    EXPECT_TRUE(body2.find("https://") != std::string::npos)
        << "Should use HTTPS when use_tls is true";
    
    server.stop();
}

/*
 * TEST 7 - Testing destructor stops server.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_DestructorStopsServer) {
    ConnectionPropsConfig config1;
    config1.protocol = "grpc";
    config1.address = "localhost";
    config1.service_port = 50051;
    
    uint16_t port = 8088;
    {
        ConnectionProps server(config1, "/connect/test.xml", port);
        ASSERT_TRUE(server.start());
        EXPECT_TRUE(server.isRunning());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } // server destroyed here
    
    // Give it a moment for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Try to start a new server on the same port
    ConnectionPropsConfig config2;
    config2.protocol = "rest";
    config2.address = "localhost";
    config2.service_port = 443;
    
    ConnectionProps server2(config2, "/connect/test2.xml", port);
    EXPECT_TRUE(server2.start()) 
        << "Should be able to start server on port after previous server destroyed";
    server2.stop();
}

/*
 * TEST 8 - Testing port binding failure.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_PortBindingFailure) {
    ConnectionPropsConfig config1;
    config1.protocol = "grpc";
    config1.address = "localhost";
    config1.service_port = 50051;
    
    // Start first server
    ConnectionProps server1(config1, "/connect/test.xml", 8090);
    ASSERT_TRUE(server1.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    ConnectionPropsConfig config2;
    config2.protocol = "rest";
    config2.address = "localhost";
    config2.service_port = 443;
    
    // Try to start second server on same port
    ConnectionProps server2(config2, "/connect/test.xml", 8090);
    EXPECT_FALSE(server2.start()) 
        << "Should fail to start when port is already in use";
    EXPECT_FALSE(server2.isRunning()) 
        << "Server should not be running after failed start";
    
    server1.stop();
}

/*
 * TEST 9 - Testing concurrent requests.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_ConcurrentRequests) {
    ConnectionPropsConfig config;
    config.protocol = "grpc";
    config.address = "localhost";
    config.service_port = 50051;
    config.node_name = "Concurrent Test";
    
    ConnectionProps server(config, "/connect/test.xml", 8094);
    
    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Make multiple concurrent requests
    std::vector<std::thread> threads;
    std::vector<std::string> responses(5);
    
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([this, i, &responses]() {
            responses[i] = makeHttpRequest(8094, "/connect/test.xml");
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all responses are correct
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(getStatusCode(responses[i]), 200) 
            << "Request " << i << " should return 200";
        std::string body = getBody(responses[i]);
        EXPECT_TRUE(body.find("node-name\">Concurrent Test<") != std::string::npos)
            << "Request " << i << " should return correct content";
    }
    
    server.stop();
}
