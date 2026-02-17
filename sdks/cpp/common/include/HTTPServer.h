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
 * @file HTTPServer.h
 * @brief Provides basic HTTP server utilities for reading and writing HTTP requests/responses
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @copyright Copyright © 2026 Ross Video Ltd
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <string>
#include <map>
#include <functional>

using boost::asio::ip::tcp;

namespace catena {
namespace common {

/**
 * @brief Structure representing a parsed HTTP request
 */
struct HTTPRequest {
    std::string method;                              // HTTP method (GET, POST, etc.)
    std::string path;                                // Request path/URL
    std::string version;                             // HTTP version (HTTP/1.1, etc.)
    std::map<std::string, std::string> headers;      // HTTP headers (case-insensitive keys, lowercase)
    std::string body;                                // Request body (if any)
};

/**
 * @brief Structure representing an HTTP response
 */
struct HTTPResponse {
    int status_code = 200;                           // HTTP status code
    std::string status_message = "OK";               // HTTP status message
    std::map<std::string, std::string> headers;      // HTTP headers
    std::string body;                                // Response body
    
    /**
     * @brief Generate the complete HTTP response string
     * @return The formatted HTTP response
     */
    std::string toString() const;
};

/**
 * @brief Utility class for HTTP socket operations
 * 
 * Provides static methods for reading HTTP requests and writing HTTP responses
 * on TCP sockets. This class consolidates common HTTP functionality used by
 * various components in the system.
 */
class HTTPServer {
public:
    /**
     * @brief Read and parse an HTTP request from a socket
     * 
     * Reads the HTTP request headers and body (if present) from the socket.
     * Parses the request line, headers, and content.
     * 
     * @param socket The TCP socket to read from
     * @param request Output parameter for the parsed request
     * @param error_code Output parameter for any errors that occur
     * @return true if successful, false on error
     */
    static bool readRequest(tcp::socket& socket, HTTPRequest& request, boost::system::error_code& error_code);

    /**
     * @brief Write an HTTP response to a socket
     * 
     * Formats and writes the HTTP response to the socket, including status line,
     * headers, and body. Automatically adds Content-Length header if not present.
     * 
     * @param socket The TCP socket to write to
     * @param response The response to write
     * @param error_code Output parameter for any errors that occur
     * @return true if successful, false on error
     */
    static bool writeResponse(tcp::socket& socket, const HTTPResponse& response, boost::system::error_code& error_code);

    /**
     * @brief Create a simple HTTP response
     * 
     * Helper method to quickly create a response with status, content type, and body.
     * 
     * @param status_code HTTP status code
     * @param status_message HTTP status message
     * @param content_type Content-Type header value
     * @param body Response body
     * @return HTTPResponse object ready to be written
     */
    static HTTPResponse createResponse(int status_code, const std::string& status_message,
                                       const std::string& content_type, const std::string& body);

    /**
     * @brief Parse HTTP headers from a stream
     * 
     * Reads and parses HTTP headers, performing case-insensitive header name matching.
     * Header names are stored in lowercase for consistent access.
     * 
     * @param header_stream Input stream containing headers
     * @param headers Output map for parsed headers (keys are lowercase)
     */
    static void parseHeaders(std::istream& header_stream, std::map<std::string, std::string>& headers);

    /**
     * @brief Get a header value with case-insensitive lookup
     * 
     * @param headers The headers map
     * @param name The header name (case-insensitive)
     * @param default_value Value to return if header not found
     * @return The header value or default_value if not found
     */
    static std::string getHeader(const std::map<std::string, std::string>& headers,
                                const std::string& name,
                                const std::string& default_value = "");

    /**
     * @brief Format an HTTP response as a string
     * 
     * Formats the HTTP response including status line, headers, and body.
     * This is useful for components that want to generate the HTTP response
     * string themselves but still use the standard HTTP formatting.
     * 
     * @param status_code HTTP status code
     * @param status_message HTTP status message
     * @param headers Map of HTTP headers
     * @param body Response body
     * @param include_content_length If true, automatically add Content-Length header
     * @return Formatted HTTP response string
     */
    static std::string formatResponse(int status_code,
                                     const std::string& status_message,
                                     const std::map<std::string, std::string>& headers,
                                     const std::string& body,
                                     bool include_content_length = true);

    /**
     * @brief Asynchronously read HTTP request with timeout (coroutine version)
     * 
     * Reads HTTP request using coroutines with timeout support. Reads headers
     * and body (if Content-Length is present) from the socket.
     * 
     * @param socket The TCP socket to read from
     * @param request Output parameter for the parsed request
     * @param timeout_ms Timeout in milliseconds
     * @return Awaitable that completes when read is done or throws on timeout/error
     * @throws std::runtime_error on timeout or read errors
     */
    static boost::asio::awaitable<void> readRequestAsync(tcp::socket& socket, HTTPRequest& request, int timeout_ms);

    /**
     * @brief Asynchronously read exact number of bytes with timeout
     * 
     * Helper coroutine for reading a specific number of bytes with timeout.
     * 
     * @param socket The TCP socket to read from
     * @param buffer The buffer to read into
     * @param start Starting position in buffer
     * @param bytes Number of bytes to read
     * @param timeout_ms Timeout in milliseconds
     * @return Awaitable that completes when read is done
     * @throws std::runtime_error on timeout
     */
    static boost::asio::awaitable<void> readWithTimeout(tcp::socket& socket, std::string& buffer, 
                                                        std::size_t start, std::size_t bytes, int timeout_ms);

    /**
     * @brief Asynchronously read until delimiter with timeout
     * 
     * Helper coroutine for reading until a delimiter is found, with timeout.
     * 
     * @param socket The TCP socket to read from
     * @param buffer The streambuf to read into
     * @param delimiter The delimiter to search for
     * @param timeout_ms Timeout in milliseconds
     * @return Awaitable that completes when delimiter is found
     * @throws std::runtime_error on timeout
     */
    static boost::asio::awaitable<void> readUntilWithTimeout(tcp::socket& socket, boost::asio::streambuf& buffer,
                                                             std::string_view delimiter, int timeout_ms);

private:
    /**
     * @brief Convert string to lowercase
     * @param str Input string
     * @return Lowercase version of input
     */
    static std::string toLowercase(const std::string& str);
};

} // namespace common
} // namespace catena
