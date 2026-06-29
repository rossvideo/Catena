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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
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
#include <Config.h>

namespace {

std::string protocolToString(catena::common::ConnectionProtocol protocol) {
    switch (protocol) {
        case catena::common::ConnectionProtocol::ST2138_REST:
            return "st2138-rest";
        case catena::common::ConnectionProtocol::ST2138_GRPC:
            return "st2138-grpc";
    }

    return "";
}

} // namespace

using namespace catena::common;

ConnectionProps::ConnectionProps(
    ConnectionProtocol protocol,
    uint32_t refresh_interval,
    const std::string& node_name,
    const std::string& node_id,
    const std::string& service_name,
    const std::string& endpoint)
    : protocol_(protocol),
      refresh_interval_(refresh_interval),
      node_name_(node_name),
      node_id_(node_id),
      service_name_(service_name),
      endpoint_(endpoint),
      running_(false),
      io_context_(),
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), config::dashboard_port)) {
    response_content_ = generateXml();
    LOG(INFO) << "Connection props server constructed with endpoint: " << endpoint
                            << ", port: " << config::dashboard_port << ", protocol: " << protocolToString(protocol_);
}

ConnectionProps::~ConnectionProps() {
    stop();
}

bool ConnectionProps::start() {
    if (running_) {
        LOG(WARNING) << "Connection props server already running on port " << config::dashboard_port;
        return false;
    }

    running_ = true;
    server_thread_ = std::make_unique<std::thread>([this]() {
        run();
    });

    LOG(INFO) << "Connection props server started on port " << config::dashboard_port
              << ", serving " << endpoint_;
    return true;
}

void ConnectionProps::stop() {
    if (!running_) {
        return;
    }

    LOG(INFO) << "Stopping connection props server on port " << config::dashboard_port;
    running_ = false;

    // Send a dummy connection to wake up the blocking accept(), same as ServiceImpl
    tcp::socket dummy_socket(io_context_);
    boost::system::error_code ec;
    dummy_socket.connect(tcp::endpoint(tcp::v4(), config::dashboard_port), ec);
    if (!ec) {
        dummy_socket.close(ec);
    }

    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

    io_context_.stop();
    acceptor_.close();

    LOG(INFO) << "Connection props server stopped";
}

std::string ConnectionProps::generateXml() {
    std::stringstream xml;
    //const std::string protocol = protocolToString(protocol_);
    
    xml << "<properties version=\"1.0\">\n"
        << "    <comment>DashBoard Device Connection Settings</comment>\n";
    
    // Base URL and service URL
    std::string scheme = config::dashboard_tls_enabled ? "https" : "http";
    xml << "    <entry key=\"base-url\">" << scheme << "://" << config::hostname << "/</entry>\n";
    
    xml << "    <entry key=\"serviceUrl\">" << service_name_ << "</entry>\n";
    
    // Equipment type
    // have this for potential future use in dashboard to differentiate between REST and gRPC devices
    //xml << "    <entry key=\"equipmentType\">" << protocol << "</entry>\n";
    xml << "    <entry key=\"equipmentType\">catena</entry>\n";
    
    // Address and port
    xml << "    <entry key=\"address\">" << config::hostname << "</entry>\n"
        << "    <entry key=\"port\">" << config::port << "</entry>\n";

    const std::string connection_type = config::dashboard_tls_enabled ? "SSL" : "TCP";
    xml << "    <entry key=\"connectionType\">" << connection_type << "</entry>\n";
    
    // Node ID and name (if provided)
    if (node_id_ != "") {
        xml << "    <entry key=\"node-id\">" << node_id_ << "</entry>\n";
    }
    if (node_name_ != "") {
        xml << "    <entry key=\"node-name\">" << node_name_ << "</entry>\n";
    }
    
    // Index URL and refresh interval
    xml << "    <entry key=\"index-url\">connect/connection-props.xml</entry>\n"
        << "    <entry key=\"refresh-interval\">" << refresh_interval_ << "</entry>\n";
    
    xml << "</properties>";
    
    return xml.str();
}

void ConnectionProps::run() {
    while (running_) {
        try {
            tcp::socket socket(io_context_);

            boost::system::error_code ec;
            acceptor_.accept(socket, ec);

            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    break;
                }
                if (running_) {
                    LOG(WARNING) << "Accept error: " << ec.message();
                }
                continue;
            }

            if (!running_) {
                break;
            }

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

        LOG(TRACE) << "Connection props request: " << method << " " << path;

        if (method != "GET") {
            const std::string body = "Method Not Allowed\n";
            const std::string response =
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Allow: GET\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" + body;
            boost::asio::write(socket, boost::asio::buffer(response));
            return;
        }

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
        std::string not_found = "Not Found";
        response << "HTTP/1.1 404 Not Found\r\n"
                 << "Content-Type: text/plain\r\n"
                 << "Content-Length: " << not_found.length() << "\r\n"
                 << "Connection: close\r\n"
                 << "\r\n"
                 << not_found;
    }

    return response.str();
}
