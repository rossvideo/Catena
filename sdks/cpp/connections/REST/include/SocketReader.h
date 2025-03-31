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
 * @file SocketReader.h
 * @brief Helper class used to read from a socket using boost.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

#include <string>
#include <iostream>

namespace catena {
namespace REST {

/**
 * @brief Helper class used to read from a socket using boost.
 */
class SocketReader {
  public:
    /**
     * @brief Constructor for SocketReader.
     * @param socket The socket to read from.
     * @param authz Flag to indicate if authorization is enabled.
     * 
     * @todo Make parse fields better once we actually know what format the URL
     * should be in.
     */
    SocketReader(tcp::socket& socket, bool authz = false) {
        // Reading the headers.
        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, "\r\n\r\n");
        std::istream header_stream(&buffer);

        // Getting the first line from the stream (URL).
        std::string header;
        std::getline(header_stream, header);
        // For loops used for (probably negligible) performance boost.
        // Parsing URL for method_.
        std::size_t i = 0;
        for (; i < header.length(); i += 1) {
            if (header[i] == ' ') { break; }
            method_ += header[i];
        }
        // Parsing URL for rpc_.
        bool started = false;
        uint16_t slashNum = 0;
        for (; i < header.length(); i += 1) {
            if (started) {
                if (header[i] == '/') {
                    slashNum += 1;
                }
                if (slashNum == 3 || header[i] == ' ') {
                    i += 1;
                    break;
                }
                rpc_ += header[i];
            } else {
                if (header[i] == '/') {
                    started = true;
                    i -= 1;
                }
            }
        }

        // Parse fields until " http/..."
        for (; i < header.length(); i += 1) {
            if (header[i] == ' ') {
                break;
            }
            req_ += header[i];
        }

        // Looping through headers to retrieve JWS token and json body len.
        std::size_t contentLength = 0;
        while(std::getline(header_stream, header) && header != "\r") {
            // authz=false once found to avoid further str comparisons.
            if (authz && header.starts_with("Authorization: Bearer ")) {
                jwsToken_ = header.substr(std::string("Authorization: Bearer ").length());
                // Removing newline.
                jwsToken_.erase(jwsToken_.length() - 1);
                authz = false;
            }
            // Getting body content-Length
            else if (header.starts_with("Content-Length: ")) {
                contentLength = stoi(header.substr(std::string("Content-Length: ").length()));
            }
        }
        std::cout<<"Content-Length: "<<contentLength<<std::endl;
        // If body exists, we need to handle leftover data and append the rest.
        if (contentLength > 0) {
            jsonBody_ = std::string((std::istreambuf_iterator<char>(header_stream)), std::istreambuf_iterator<char>());
            if (jsonBody_.size() < contentLength) {
                std::size_t remainingLength = contentLength - jsonBody_.size();
                std::cout<<"jsonBody-Size"<<jsonBody_.size()<<std::endl;
                std::cout<<"Remaining-Length: "<<remainingLength<<std::endl;
                jsonBody_.resize(contentLength);
                boost::asio::read(socket, boost::asio::buffer(&jsonBody_[contentLength - remainingLength], remainingLength));
            }
        }
    }

    /**
     * @brief Returns the method of the request.
     */
    const std::string& method() const { return method_; }
    /**
     * @brief Returns the rpc of the request.
     */
    const std::string& rpc() const { return rpc_; }
    /**
     * @brief Returns the req of the request.
     */
    const std::string& req() const { return req_; }
    /**
     * @brief Returns the client's jws token.
     */
    const std::string& jwsToken() const { return jwsToken_; }
    /**
     * @brief Returns the json body of the request, which may be empty.
     */
    const std::string& jsonBody() const { return jsonBody_; }

  private:
    std::string method_ = "";
    std::string rpc_ = "";
    std::string req_ = "";
    std::string jwsToken_ = "";
    std::string jsonBody_ = "";
};

}; // Namespace REST
}; // Namespace catena