/*
 * Copyright 2024 Ross Video Ltd
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

// common
#include <Tags.h>

#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
#include <utils.h>

#include <api.h>

#include "absl/flags/flag.h"

#include <iostream>
#include <regex>

using catena::API;

// expand env variables
void expandEnvVariables(std::string &str) {
    static std::regex env{"\\$\\{([^}]+)\\}"};
    std::smatch match;
    while (std::regex_search(str, match, env)) {
        const char *s = getenv(match[1].str().c_str());
        const std::string var(s == nullptr ? "" : s);
        str.replace(match[0].first, match[0].second, var);
    }
}

API::API(Device &dm, uint16_t port) : version_{"1.0.0"}, port_{port}, dm_{dm},
    acceptor_{io_context_, Tcp::endpoint(Tcp::v4(), port)} {
    // Flag does not really work at the moment :/
    // authorizationEnabled_ = absl::GetFlag(FLAGS_authz);
    authorizationEnabled_ = true;
    if (authorizationEnabled_) {
        std::cerr<<"Authorization enabled"<<std::endl;
    }
}

std::string API::version() const {
  return version_;
}

void API::run() {
    // TLS handled by Envoyproxy
    while (true) {
        // Waiting for a connection.
        Tcp::socket socket(io_context_);
        acceptor_.accept(socket);

        // When a connection has been made, detatch to handle asynchronously.
        std::thread([this, socket = std::move(socket)]() mutable {
            // Reading the request.
            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, "\r\n\r\n");
            std::istream request_stream(&buffer);
            std::string header;
            std::getline(request_stream, header);
            // Extracting method and request
            std::string method = header.substr(0, header.find(" "));
            std::string request = header.substr(header.find("/"));

            // Setting up authorizer if enabled.
            std::shared_ptr<catena::common::Authorizer> sharedAuthz;
            catena::common::Authorizer* authz = nullptr;
            if (authorizationEnabled_) {
                while(std::getline(request_stream, header) && header != "\r") {
                    try {
                        if (header.starts_with("Authorization: Bearer ")) {
                            std::string jwsToken = header.substr(std::string("Authorization: Bearer ").length());
                            jwsToken.erase(jwsToken.length() - 1); // rm newline
                            sharedAuthz = std::make_shared<catena::common::Authorizer>(jwsToken);
                            authz = sharedAuthz.get();
                            break;
                        }
                    } catch (catena::exception_with_status& err) {
                        // TODO later
                    }
                }
            } else {
                authz = &catena::common::Authorizer::kAuthzDisabled;
            }

            // Routing the request based on the name.
            route(method, request, socket, authz);
        }).detach();
    }
}

std::string API::getJWSToken(const crow::request& req) const {
    std::string auth_header = req.get_header_value("Authorization");
    if (!auth_header.empty() ? !auth_header.starts_with("Bearer "): true) {
        throw catena::exception_with_status("JWS bearer token not found", catena::StatusCode::UNAUTHENTICATED);
    }
    // Getting token (after "bearer") and returning as an std::string.
    return auth_header.substr(std::string("Bearer ").length());
}

std::string API::getField(std::string& request, std::string field) const {
    std::string foundField = "";
    field = "/" + field + "/";
    std::size_t pos = request.find(field) + field.length();
    // Extracts the value between field and the next "/"
    if (pos != std::string::npos) {
        foundField = request.substr(pos);
        foundField = foundField.substr(0, foundField.find("/"));
    }
    return foundField;
}

crow::response API::finish(google::protobuf::Message& msg) const {
    crow::response res;
    
    // Converting the value to JSON.
    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &json_output, options);

    // Check if the conversion was successful.
    if (!status.ok()) {
        res = crow::response(toCrowStatus_.at(catena::StatusCode::INVALID_ARGUMENT), "Failed to convert protobuf to JSON");
    } else {
        // Create and return a Crow response with JSON content type.
        res.code = toCrowStatus_.at(catena::StatusCode::OK);
        res.set_header("Content-Type", "application/json");
        res.write(json_output);
    }
    return res;
}

void API::write(Tcp::socket& socket, google::protobuf::Message& msg) const {   
    // Converting the value to JSON.
    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(msg, &json_output, options);

    // Check if the conversion was successful.
    if (!status.ok()) {
        throw catena::exception_with_status("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
    } else {
        std::string chunk_size = std::format("{:x}", json_output.size());
        boost::asio::write(socket, boost::asio::buffer(chunk_size + "\r\n" + json_output + "\r\n"));
    }
}

bool API::is_port_in_use_() const {
    try {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::acceptor acceptor(io_context, 
                    boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_));
        return false;  // Port is not in use
    } catch (const boost::system::system_error& e) {
        return true;  // Port is in use
    }
}