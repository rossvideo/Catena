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
 * @brief Implementation of the HTTPServer utility class.
 * @file HTTPServer.cpp
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 */

#include <HTTPServer.h>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace catena::common;

std::string HTTPResponse::toString() const {
    std::stringstream response;
    
    // Status line
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers) {
        response << key << ": " << value << "\r\n";
    }
    
    // Blank line before body
    response << "\r\n";
    
    // Body
    response << body;
    
    return response.str();
}

bool HTTPServer::readRequest(tcp::socket& socket, HTTPRequest& request, boost::system::error_code& error_code) {
    try {
        // Read until we get the double CRLF (end of headers)
        boost::asio::streambuf request_buf;
        boost::asio::read_until(socket, request_buf, "\r\n\r\n", error_code);
        
        if (error_code && error_code != boost::asio::error::eof) {
            return false;
        }

        // Parse the request
        std::istream request_stream(&request_buf);
        
        // Parse request line (method, path, version)
        std::string request_line;
        std::getline(request_stream, request_line);
        
        // Remove trailing \r if present
        if (!request_line.empty() && request_line.back() == '\r') {
            request_line.pop_back();
        }
        
        std::istringstream request_line_stream(request_line);
        request_line_stream >> request.method >> request.path >> request.version;
        
        // Parse headers
        parseHeaders(request_stream, request.headers);
        
        // Check for request body
        auto content_length_str = getHeader(request.headers, "content-length", "0");
        std::size_t content_length = 0;
        
        try {
            if (!content_length_str.empty() && content_length_str[0] != '-') {
                content_length = std::stoull(content_length_str);
            }
        } catch (...) {
            // Invalid content-length
            error_code = boost::asio::error::invalid_argument;
            return false;
        }
        
        // Read any leftover data from the buffer first
        request.body = std::string((std::istreambuf_iterator<char>(request_stream)), std::istreambuf_iterator<char>());
        
        // If there's more body data to read
        if (content_length > 0 && request.body.size() < content_length) {
            std::size_t leftover = content_length - request.body.size();
            std::size_t start = request.body.size();
            request.body.resize(content_length);
            
            boost::asio::read(socket, boost::asio::buffer(&request.body[start], leftover), error_code);
            
            if (error_code) {
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception&) {
        error_code = boost::asio::error::invalid_argument;
        return false;
    }
}

bool HTTPServer::writeResponse(tcp::socket& socket, const HTTPResponse& response, boost::system::error_code& error_code) {
    try {
        std::stringstream response_stream;
        
        // Status line
        response_stream << "HTTP/1.1 " << response.status_code << " " << response.status_message << "\r\n";
        
        // Check if Content-Length header exists
        bool has_content_length = false;
        for (const auto& [key, value] : response.headers) {
            std::string lower_key = toLowercase(key);
            if (lower_key == "content-length") {
                has_content_length = true;
            }
            response_stream << key << ": " << value << "\r\n";
        }
        
        // Add Content-Length if not present
        if (!has_content_length) {
            response_stream << "Content-Length: " << response.body.length() << "\r\n";
        }
        
        // Blank line before body
        response_stream << "\r\n";
        
        // Body
        response_stream << response.body;
        
        // Write to socket
        boost::asio::write(socket, boost::asio::buffer(response_stream.str()), error_code);
        
        return !error_code;
        
    } catch (const std::exception&) {
        error_code = boost::asio::error::invalid_argument;
        return false;
    }
}

HTTPResponse HTTPServer::createResponse(int status_code, const std::string& status_message,
                                       const std::string& content_type, const std::string& body) {
    HTTPResponse response;
    response.status_code = status_code;
    response.status_message = status_message;
    response.headers["Content-Type"] = content_type;
    response.headers["Connection"] = "close";
    response.body = body;
    return response;
}

void HTTPServer::parseHeaders(std::istream& header_stream, std::map<std::string, std::string>& headers) {
    std::string header;
    
    while (std::getline(header_stream, header) && header != "\r") {
        // Parse "<name>: <value>"
        std::size_t sep = header.find(':');
        if (sep == std::string::npos) {
            continue;
        }
        
        std::string name = header.substr(0, sep);
        std::string value = header.substr(sep + 1);
        
        // Trim leading spaces from value
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(0, 1);
        }
        
        // Trim trailing CR and spaces from value
        while (!value.empty() && (value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
        
        // Store with lowercase key for case-insensitive lookup
        headers[toLowercase(name)] = value;
    }
}

std::string HTTPServer::getHeader(const std::map<std::string, std::string>& headers,
                                 const std::string& name,
                                 const std::string& default_value) {
    std::string lower_name = toLowercase(name);
    auto it = headers.find(lower_name);
    if (it != headers.end()) {
        return it->second;
    }
    return default_value;
}

std::string HTTPServer::toLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string HTTPServer::formatResponse(int status_code,
                                      const std::string& status_message,
                                      const std::map<std::string, std::string>& headers,
                                      const std::string& body,
                                      bool include_content_length) {
    std::stringstream response;
    
    // Status line
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    
    // Check if Content-Length header exists
    bool has_content_length = false;
    for (const auto& [key, value] : headers) {
        std::string lower_key = toLowercase(key);
        if (lower_key == "content-length") {
            has_content_length = true;
        }
        response << key << ": " << value << "\r\n";
    }
    
    // Add Content-Length if requested and not present
    if (include_content_length && !has_content_length) {
        response << "Content-Length: " << body.length() << "\r\n";
    }
    
    // Blank line before body
    response << "\r\n";
    
    // Body
    response << body;
    
    return response.str();
}

boost::asio::awaitable<void> HTTPServer::readWithTimeout(tcp::socket& socket, std::string& buffer, 
                                                         std::size_t start, std::size_t bytes, int timeout_ms) {
    using namespace boost::asio::experimental::awaitable_operators;
    boost::asio::steady_timer timer(socket.get_executor());
    timer.expires_after(std::chrono::milliseconds(timeout_ms));
    
    // Async_read and timer run simultaneously, the other cancels when one finishes
    auto result = co_await (boost::asio::async_read(socket, boost::asio::buffer(&buffer[start], bytes), 
                            boost::asio::use_awaitable) ||
                            timer.async_wait(boost::asio::use_awaitable));

    // index of 0 = read finished first
    if (result.index() != 0) {
        throw std::runtime_error("Timed out");
    }
}

boost::asio::awaitable<void> HTTPServer::readUntilWithTimeout(tcp::socket& socket, boost::asio::streambuf& buffer,
                                                              std::string_view delimiter, int timeout_ms) {
    using namespace boost::asio::experimental::awaitable_operators;
    boost::asio::steady_timer timer(socket.get_executor());
    timer.expires_after(std::chrono::milliseconds(timeout_ms));

    // Async_read and timer run simultaneously, the other cancels when one finishes
    auto result = co_await (boost::asio::async_read_until(socket, buffer, delimiter, boost::asio::use_awaitable) ||
                            timer.async_wait(boost::asio::use_awaitable));

    // index of 0 = read finished first
    if (result.index() != 0) {
        throw std::runtime_error("Timed out");
    }
}

boost::asio::awaitable<void> HTTPServer::readRequestAsync(tcp::socket& socket, HTTPRequest& request, int timeout_ms) {
    // Read headers (until double CRLF)
    boost::asio::streambuf buffer;
    co_await readUntilWithTimeout(socket, buffer, "\r\n\r\n", timeout_ms);
    
    std::istream header_stream(&buffer);

    // Parse request line
    std::string request_line;
    std::getline(header_stream, request_line);
    
    // Remove trailing \r if present
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }
    
    std::istringstream request_line_stream(request_line);
    request_line_stream >> request.method >> request.path >> request.version;

    // Parse headers
    parseHeaders(header_stream, request.headers);
    
    // Check for request body
    auto content_length_str = getHeader(request.headers, "content-length", "0");
    std::size_t content_length = 0;
    
    try {
        if (!content_length_str.empty() && content_length_str[0] != '-') {
            content_length = std::stoull(content_length_str);
        }
    } catch (...) {
        throw std::runtime_error("Invalid Content-Length");
    }
    
    // Read any leftover data from the buffer first
    request.body = std::string((std::istreambuf_iterator<char>(header_stream)), std::istreambuf_iterator<char>());
    
    // If there's more body data to read
    if (content_length > 0 && request.body.size() < content_length) {
        std::size_t leftover = content_length - request.body.size();
        std::size_t start = request.body.size();
        request.body.resize(content_length);
        
        co_await readWithTimeout(socket, request.body, start, leftover, timeout_ms);
    }
}
