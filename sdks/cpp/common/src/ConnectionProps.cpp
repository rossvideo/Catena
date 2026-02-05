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
  * @brief Implementation of the ConnectionProps class.
  * @file ConnectionProps.cpp
  * @copyright Copyright © 2026 Ross Video Ltd
  * @author Christian Twarog (christian.twarog@rossvideo.com)
  */

#include <ConnectionProps.h>
#include <Logger.h>
#include <sstream>
#include <regex>

using namespace catena::common;

ConnectionProps::ConnectionProps(
    const std::string& endpoint,
    const std::string& response_content,
    uint16_t port)
    : endpoint_(endpoint),
      response_content_(response_content),
      port_(port),
      running_(false) {
    LOG(INFO) << "Connection props server constructed with endpoint: " << endpoint
              << ", port: " << port;
}

ConnectionProps::~ConnectionProps() {
    stop();
}

bool ConnectionProps::start() {
    if (running_) {
        LOG(WARNING) << "Connection props server already running on port " << port_;
        return false;
    }

    try {
        io_context_ = std::make_unique<boost::asio::io_context>();
        acceptor_ = std::make_unique<tcp::acceptor>(
            *io_context_,
            tcp::endpoint(tcp::v4(), port_)
        );

        running_ = true;
        server_thread_ = std::make_unique<std::thread>([this]() {
            run();
        });

        LOG(INFO) << "Connection props server started on port " << port_ 
                  << ", serving " << endpoint_;
        return true;

    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to start connection props server: " << e.what();
        running_ = false;
        return false;
    }
}

void ConnectionProps::stop() {
    if (!running_) {
        return;
    }

    LOG(INFO) << "Stopping connection props server on port " << port_;
    running_ = false;

    // Stop the io_context
    if (io_context_) {
        io_context_->stop();
    }

    // Close the acceptor
    if (acceptor_ && acceptor_->is_open()) {
        boost::system::error_code ec;
        acceptor_->close(ec);
        if (ec) {
            LOG(WARNING) << "Error closing acceptor: " << ec.message();
        }
    }

    // Wait for the server thread to finish
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

    LOG(INFO) << "Connection props server stopped";
}

void ConnectionProps::updateContent(const std::string& content) {
    std::lock_guard<std::mutex> lock(content_mutex_);
    response_content_ = content;
    VLOG(1) << "Connection props server content updated";
}

void ConnectionProps::run() {
    while (running_) {
        try {
            tcp::socket socket(*io_context_);
            
            // Accept with timeout using async_accept and a timer
            boost::system::error_code ec;
            acceptor_->accept(socket, ec);

            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    // Acceptor was closed, exit gracefully
                    break;
                }
                LOG(WARNING) << "Accept error: " << ec.message();
                continue;
            }

            if (!running_) {
                break;
            }

            // Handle the request in the current thread (it's very lightweight)
            handleRequest(std::move(socket));

        } catch (const std::exception& e) {
            if (running_) {
                LOG(ERROR) << "Exception in connection props server: " << e.what();
            }
            break;
        }
    }
}

void ConnectionProps::handleRequest(tcp::socket socket) {
    try {
        // Read the HTTP request
        boost::asio::streambuf request_buf;
        boost::system::error_code ec;
        
        // Read until we get the double CRLF (end of headers)
        boost::asio::read_until(socket, request_buf, "\r\n\r\n", ec);
        
        if (ec && ec != boost::asio::error::eof) {
            LOG(WARNING) << "Error reading request: " << ec.message();
            return;
        }

        // Parse the request line
        std::istream request_stream(&request_buf);
        std::string method, path, version;
        request_stream >> method >> path >> version;

        VLOG(2) << "Connection props request: " << method << " " << path;

        // Generate and send response
        std::string response = generateResponse(path);
        boost::asio::write(socket, boost::asio::buffer(response), ec);

        if (ec) {
            LOG(WARNING) << "Error writing response: " << ec.message();
        }

        // Close the socket
        socket.close(ec);

    } catch (const std::exception& e) {
        LOG(WARNING) << "Exception handling request: " << e.what();
    }
}

std::string ConnectionProps::generateResponse(const std::string& path) {
    std::stringstream response;

    // Check if the path matches our endpoint
    if (path == endpoint_) {
        // Serve the discovery content
        std::lock_guard<std::mutex> lock(content_mutex_);
        
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: application/xml\r\n"
                 << "Content-Length: " << response_content_.length() << "\r\n"
                 << "Connection: close\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type\r\n"
                 << "\r\n"
                 << response_content_;
    } else if (path == "/health") {
        // Simple health check endpoint
        std::string health = "OK";
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/plain\r\n"
                 << "Content-Length: " << health.length() << "\r\n"
                 << "Connection: close\r\n"
                 << "\r\n"
                 << health;
    } else {
        // 404 Not Found for all other paths
        std::string not_found = "Not Found - Only " + endpoint_ + " is available";
        response << "HTTP/1.1 404 Not Found\r\n"
                 << "Content-Type: text/plain\r\n"
                 << "Content-Length: " << not_found.length() << "\r\n"
                 << "Connection: close\r\n"
                 << "\r\n"
                 << not_found;
    }

    return response.str();
}
