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
 * @brief A parent class for REST test fixtures.
 * @author benjamin.whitten@rossvideo.com
 * @date 25/05/12
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// std
#include <string>

// common
#include <Status.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// REST
#include "SocketWriter.h"
using namespace catena::REST;

/*
 * RESTTest class inherited by test fixtures to provide functions for
 * writing, reading, and verifying requests and responses.
 */
class RESTTest {
  protected:
    // Constructor to connecting sockets (write(in) -> read(out)).
    RESTTest(tcp::socket* in, tcp::socket* out) : writeSocket(in), readSocket(out) {
        // Neither can be nullptr.
        if (!writeSocket || !readSocket) {
            throw std::invalid_argument("RESTTest: Both sockets must be provided.");
        }
        // Connecting sockets (write(in) -> read(out)).
        readSocket->connect(acceptor.local_endpoint());
        acceptor.accept(*writeSocket);
    }

    // Writes a request to the writeSocket which can later be read by SocketReader.
    void writeRequest(const std::string& method,
                      uint32_t slot,
                      const std::string& endpoint,
                      const std::string& fqoid,
                      bool stream,
                      const std::unordered_map<std::string, std::string>& fields,
                      const std::string& jwsToken,
                      const std::string& origin,
                      const std::string& detailLevel,
                      const std::string& language,
                      const std::string& jsonBody) {
        // Compiling path:
        std::string request = "";
        request += method + " /st2138-api/v1";
        if (slot != 0) {
            request += "/" + std::to_string(slot);
        }
        request += endpoint;
        if (!fqoid.empty()) {
            request += fqoid;
        }
        if (stream) {
            request += "/stream";
        }
        // compiling fields:
        std::string fieldsStr = "";
        for (auto [fName, fValue] : fields) {
            if (fieldsStr.empty()) {
                fieldsStr += "?" + fName + "=" + fValue;
            } else {
                fieldsStr += "&" + fName + "=" + fValue;
            }
        }
        request += fieldsStr;
        // Compiling headers:
        request += " HTTP/1.1\n"
                   "Origin: " + origin + "\n"
                   "User-Agent: test_agent\n"
                   "Authorization: Bearer " + jwsToken + " \n";
        if (!detailLevel.empty()) {
            request += "Detail-Level: " + detailLevel + " \n";
        }
        if (!language.empty()) {
            request += "Language: " + language + " \n";
        }
        // Adding jsonBody.
        request += "Content-Length: " + std::to_string(jsonBody.length()) + "\r\n\r\n"
                   + jsonBody + "\n"
                   "\r\n\r\n";
        boost::asio::write(*writeSocket, boost::asio::buffer(request));
    }

    // Returns whatever has been writen to the readSocket..
    // *Note: This only reads a limited amount of data (up to 4096 bytes). This
    // suffices for testing, mostly because I don't feel like making it dynamic.
    std::string readResponse() {
        boost::asio::streambuf buffer;
        boost::asio::read_until(*readSocket, buffer, "\r\n\r\n");
        std::istream response_stream(&buffer);
        return std::string(std::istreambuf_iterator<char>(response_stream), std::istreambuf_iterator<char>());
    }

    // Returns what an expect response from SocketWriter should look like.
    inline std::string expectedResponse(const catena::exception_with_status& rc, const std::string& jsonBody = "") {
        http_exception_with_status httpStatus = codeMap_.at(rc.status);
        return "HTTP/1.1 " + std::to_string(httpStatus.first) + " " + httpStatus.second + "\r\n"
               "Content-Type: application/json\r\n"
               "Connection: close\r\n"
               "Content-Length: " + std::to_string(jsonBody.length()) + "\r\n" // will = 0 in case of error or empty.
               "Access-Control-Allow-Origin: " + origin + "\r\n"
               "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
               "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
               "Access-Control-Allow-Credentials: true\r\n\r\n" +
               jsonBody;
    }

    // Returns what an expect response from SocketWriter with buffer=true should look like.
    inline std::string expectedResponse(const catena::exception_with_status& rc, const std::vector<std::string>& msgs) {
        // Compiling body response from messages.
        std::string jsonBody = "";
        if (rc.status == catena::StatusCode::OK) {
            for (const std::string& msg : msgs) {
                if (jsonBody.empty()) {
                    jsonBody += "{\"data\":[" + msg;
                } else {
                    jsonBody += "," + msg;
                }
            }
            jsonBody += "]}";
        }
        return expectedResponse(rc, jsonBody);
    }

    // Returns what an expect response from SSEWriter should look like.
    inline std::string expectedSSEResponse(const catena::exception_with_status& rc, const std::vector<std::string>& msgs = {}) {
        http_exception_with_status httpStatus = codeMap_.at(rc.status);
        // Compiling body response from messages.
        std::string jsonBody = "";
        if (httpStatus.first < 300) {
            for (const std::string& msg : msgs) {
                jsonBody += "data: " + msg + "\n\n";
            }
        }
        return "HTTP/1.1 " + std::to_string(httpStatus.first) + " " + httpStatus.second + "\r\n"
               "Content-Type: text/event-stream\r\n"
               "Cache-Control: no-cache\r\n"
               "Connection: keep-alive\r\n"
               "Access-Control-Allow-Origin: " + origin + "\r\n"
               "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
               "Access-Control-Allow-Headers: Content-Type, Authorization, accept, Origin, X-Requested-With, Language, Detail-Level\r\n"
               "Access-Control-Allow-Credentials: true\r\n\r\n" +
               jsonBody;
    }

    // Debug helper to check socket status
    std::string getSocketStatus() const {
        return "available: " + std::to_string(readSocket->available()) + 
               ", open: " + std::to_string(readSocket->is_open());
    }

    std::string origin = "*";
    boost::asio::io_context io_context;
    tcp::socket clientSocket{io_context};
    tcp::socket serverSocket{io_context};
    tcp::acceptor acceptor{io_context, tcp::endpoint(tcp::v4(), 0)};

  private:
    tcp::socket* writeSocket = nullptr;
    tcp::socket* readSocket = nullptr;
};
