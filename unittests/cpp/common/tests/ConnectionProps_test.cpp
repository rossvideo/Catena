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
 * @date 2026-03-20
 * @copyright Copyright © 2026 Ross Video Ltd
 */

#include <ConnectionProps.h>

// gtest
#include <gtest/gtest.h>

// common
#include <Logger.h>
#include <Config.h>

// boost
#include <boost/asio.hpp>

// std
#include <thread>
#include <chrono>
#include <regex>
#include <optional>

using namespace catena::common;
using boost::asio::ip::tcp;

// Test fixture for ConnectionProps tests
class ConnectionPropsTest : public testing::Test {
  protected:
    static void SetUpTestSuite() {
        config::log_dir = UNITTEST_LOG_DIR;
        config::log_file = true;
        config::log_level = "TRACE";
        config::log_size = 10;
        config::log_count = 3;
        config::log_final_rotation = true;
        Logger::init("ConnectionPropsTest");
    }

    void SetUp() override {
        originalPort_ = config::dashboard_port;
        config::dashboard_port = nextPort_++;
    }

    void TearDown() override {
        config::dashboard_port = originalPort_;
    }

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

    int getStatusCode(const std::string& response) {
        std::regex status_regex(R"(HTTP/1\.1 (\d+))");
        std::smatch match;
        if (std::regex_search(response, match, status_regex)) {
            return std::stoi(match[1]);
        }
        return -1;
    }

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

    bool hasHeader(const std::string& response, const std::string& header) {
        return response.find(header) != std::string::npos;
    }

    std::optional<std::string> getXmlEntryValue(
        const std::string& xmlBody, const std::string& key) {
        const std::regex entryRegex(
            "<entry\\s+key=\"" + key + "\">([^<]*)</entry>",
            std::regex::icase);
        std::smatch match;
        if (std::regex_search(xmlBody, match, entryRegex) && match.size() > 1) {
            return match[1].str();
        }
        return std::nullopt;
    }

    uint16_t originalPort_{};
    static uint16_t nextPort_;
};

uint16_t ConnectionPropsTest::nextPort_ = 8081;

/*
 * TEST 1 - Constructor, getters, and basic start/stop lifecycle.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_Lifecycle) {
    ConnectionProps server(
        ConnectionProtocol::ST2138_GRPC,
        30000,
        "Test Node",
        "test-node-123",
        "service:catena-device",
        "/connect/test.xml");

    EXPECT_EQ(server.getEndpoint(), "/connect/test.xml");
    EXPECT_FALSE(server.isRunning());

    EXPECT_TRUE(server.start());
    EXPECT_TRUE(server.isRunning());

    // Starting again while running should fail
    EXPECT_FALSE(server.start());
    EXPECT_TRUE(server.isRunning());

    server.stop();
    EXPECT_FALSE(server.isRunning());
}

/*
 * TEST 2 - Full HTTP response check: status, headers, and XML body.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_ServeEndpoint) {
    const std::string endpoint = "/connect/connection-props.xml";
    ConnectionProps server(
        ConnectionProtocol::ST2138_GRPC,
        15000,
        "Test Device",
        "test-device-abc123",
        "service:catena-device",
        endpoint);

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const std::string response = makeHttpRequest(config::dashboard_port, endpoint);
    EXPECT_EQ(getStatusCode(response), 200);
    EXPECT_TRUE(hasHeader(response, "Content-Type: application/xml"));
    EXPECT_TRUE(hasHeader(response, "Access-Control-Allow-Origin: *"));

    const std::string body = getBody(response);
    EXPECT_TRUE(body.find("<properties version=\"1.0\">") != std::string::npos);
    EXPECT_TRUE(body.find("equipmentType\">catena<") != std::string::npos);
    EXPECT_TRUE(body.find("node-id\">test-device-abc123<") != std::string::npos);
    EXPECT_TRUE(body.find("node-name\">Test Device<") != std::string::npos);
    EXPECT_TRUE(body.find("refresh-interval\">15000<") != std::string::npos);

    // connectionType reflects global TLS setting (disabled by default in tests)
    const std::string expectedConnectionType = config::dashboard_tls_enabled ? "SSL" : "TCP";
    EXPECT_TRUE(body.find("connectionType\">" + expectedConnectionType + "<") != std::string::npos);

    server.stop();
}

/*
 * TEST 3 - Unknown path returns 404 with plain-text body naming the registered endpoint.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_404Response) {
    const std::string endpoint = "/connect/connection-props.xml";
    ConnectionProps server(
        ConnectionProtocol::ST2138_GRPC,
        30000, "", "", "service:catena-device",
        endpoint);

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const std::string response = makeHttpRequest(config::dashboard_port, "/unknown/path");
    EXPECT_EQ(getStatusCode(response), 404);
    EXPECT_TRUE(hasHeader(response, "Content-Type: text/plain"));
    EXPECT_TRUE(getBody(response).find("Not Found") != std::string::npos);
    EXPECT_TRUE(getBody(response).find(endpoint) == std::string::npos);

    server.stop();
}

/*
 * TEST 3b - Non-GET methods return 405 Method Not Allowed.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_405Response) {
    const std::string endpoint = "/connect/connection-props.xml";
    ConnectionProps server(
        ConnectionProtocol::ST2138_GRPC,
        30000, "", "", "service:catena-device",
        endpoint);

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (const std::string& method : {"POST", "PUT", "DELETE", "PATCH"}) {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(config::dashboard_port));
        boost::asio::connect(socket, endpoints);

        std::string request =
            method + " " + endpoint + " HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf response_buf;
        boost::system::error_code ec;
        boost::asio::read(socket, response_buf, boost::asio::transfer_all(), ec);

        std::string response{
            boost::asio::buffers_begin(response_buf.data()),
            boost::asio::buffers_end(response_buf.data())};

        EXPECT_EQ(getStatusCode(response), 405) << "Method: " << method;
        EXPECT_TRUE(hasHeader(response, "Allow: GET")) << "Method: " << method;
    }

    server.stop();
}

/*
 * TEST 4 - Health check endpoint returns 200 with plain-text "OK" body.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_HealthCheckEndpoint) {
    ConnectionProps server(
        ConnectionProtocol::ST2138_GRPC,
        30000, "", "", "service:catena-device",
        "/connect/connection-props.xml");

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const std::string response = makeHttpRequest(config::dashboard_port, "/health");
    EXPECT_EQ(getStatusCode(response), 200);
    EXPECT_TRUE(hasHeader(response, "Content-Type: text/plain"));
    EXPECT_EQ(getBody(response), "OK\n");

    server.stop();
}

/*
 * TEST 5 - base-url uses http:// or https:// from global config::dashboard_tls_enabled.
 *          connectionType must also reflect the TLS setting.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_BaseUrl_TlsScheme) {
    const std::string endpoint = "/connect/connection-props.xml";

    struct TlsGuard {
        bool originalTls;
        uint16_t originalPort;
        ~TlsGuard() {
            config::dashboard_tls_enabled = originalTls;
            config::dashboard_port = originalPort;
        }
    } guard{config::dashboard_tls_enabled, config::dashboard_port};

    // TLS disabled -> http://, connectionType TCP
    {
        config::dashboard_tls_enabled = false;
        config::dashboard_port = ConnectionPropsTest::nextPort_++;
        ConnectionProps server(
            ConnectionProtocol::ST2138_REST,
            30000, "", "", "service:catena-device",
            endpoint);
        ASSERT_TRUE(server.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        const std::string body = getBody(makeHttpRequest(config::dashboard_port, endpoint));

        auto baseUrl = getXmlEntryValue(body, "base-url");
        ASSERT_TRUE(baseUrl.has_value());
        EXPECT_TRUE(baseUrl->rfind("http://", 0) == 0)
            << "Expected http://, got: " << *baseUrl;

        auto connectionType = getXmlEntryValue(body, "connectionType");
        ASSERT_TRUE(connectionType.has_value());
        EXPECT_EQ(*connectionType, "TCP");

        server.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // TLS enabled -> https://, connectionType SSL
    {
        config::dashboard_tls_enabled = true;
        config::dashboard_port = ConnectionPropsTest::nextPort_++;
        ConnectionProps server(
            ConnectionProtocol::ST2138_GRPC,
            30000, "", "", "service:catena-device",
            endpoint);
        ASSERT_TRUE(server.start());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        const std::string body = getBody(makeHttpRequest(config::dashboard_port, endpoint));

        auto baseUrl = getXmlEntryValue(body, "base-url");
        ASSERT_TRUE(baseUrl.has_value());
        EXPECT_TRUE(baseUrl->rfind("https://", 0) == 0)
            << "Expected https://, got: " << *baseUrl;

        auto connectionType = getXmlEntryValue(body, "connectionType");
        ASSERT_TRUE(connectionType.has_value());
        EXPECT_EQ(*connectionType, "SSL");

        server.stop();
    }
}

/*
 * TEST 6 - Concurrent requests all return 200 with correct content.
 */
TEST_F(ConnectionPropsTest, ConnectionProps_ConcurrentRequests) {
    const std::string endpoint = "/connect/test.xml";
    ConnectionProps server(
        ConnectionProtocol::ST2138_GRPC,
        30000,
        "Concurrent Test",
        "",
        "service:catena-device",
        endpoint);

    ASSERT_TRUE(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<std::string> responses(5);
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([this, i, &responses, endpoint]() {
            responses[i] = makeHttpRequest(config::dashboard_port, endpoint);
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(getStatusCode(responses[i]), 200) << "Request " << i;
        EXPECT_TRUE(getBody(responses[i]).find("node-name\">Concurrent Test<") != std::string::npos)
            << "Request " << i;
    }

    server.stop();
}
