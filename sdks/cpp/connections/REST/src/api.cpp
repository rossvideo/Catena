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
    authorizationEnabled_ = absl::GetFlag(FLAGS_authz);
    // authorizationEnabled_ = false;
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
            try {
                /*
                 * Move all of this into router? Saves a bunch of input args.
                 */
                // Reading the headers.
                boost::asio::streambuf buffer;
                boost::asio::read_until(socket, buffer, "\r\n\r\n");
                std::istream header_stream(&buffer);

                // Extracting method and request form the first line (URL)
                std::string header;
                std::getline(header_stream, header);
                std::string method = header.substr(0, header.find(" "));
                std::string request = header.substr(header.find("/"));

                // Setting up authorizer and getting contentLength (if present)
                std::size_t contentLength = 0;
                std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                catena::common::Authorizer* authz = nullptr;
                if (!authorizationEnabled_) {
                    authz = &catena::common::Authorizer::kAuthzDisabled;
                }
                while(std::getline(header_stream, header) && header != "\r") {
                    // Getting bearer token if authorization is enabled.
                    if (authorizationEnabled_ && header.starts_with("Authorization: Bearer ")) {
                        std::string jwsToken = header.substr(std::string("Authorization: Bearer ").length());
                        jwsToken.erase(jwsToken.length() - 1); // rm newline
                        sharedAuthz = std::make_shared<catena::common::Authorizer>(jwsToken);
                        authz = sharedAuthz.get();
                    }
                    // Getting body content-Length
                    if (header.starts_with("Content-Length: ")) {
                        contentLength = stoi(header.substr(std::string("Content-Length: ").length()));
                    }
                }
                /*
                 * If there is a body, we need to handle leftover data from
                 * above and append the rest.
                 */
                std::string jsonPayload = "";
                if (contentLength > 0) {
                    jsonPayload = std::string((std::istreambuf_iterator<char>(header_stream)), std::istreambuf_iterator<char>());
                    if (jsonPayload.size() < contentLength) {
                        std::size_t remainingLength = contentLength - jsonPayload.size();
                        jsonPayload.resize(contentLength);
                        boost::asio::read(socket, boost::asio::buffer(&jsonPayload[contentLength - remainingLength], remainingLength));
                    }
                }

                // Routing the request based on the name.
                route(method, request, jsonPayload, socket, authz);

            // Catching errors.
            } catch (catena::exception_with_status& err) {
                SocketWriter writer(socket);
                writer.write(err);
            } catch (...) {
                SocketWriter writer(socket);
                catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
                writer.write(err);
            }
        }).detach();
    }
}

void API::parseFields(std::string& request, std::unordered_map<std::string, std::string>& fields) const {
    if (fields.size() == 0) {
        throw catena::exception_with_status("No fields found", catena::StatusCode::INVALID_ARGUMENT);
    } else {
        std::string fieldName = "";
        for (auto& [nextField, value] : fields) {
            // If not the first iteration, find next field and get value of the current one.
            if (fieldName != "") {
                std::size_t end = request.find("/" + nextField + "/");
                if (end == std::string::npos) {
                    throw catena::exception_with_status("Could not find field " + nextField, catena::StatusCode::INVALID_ARGUMENT);
                }
                fields.at(fieldName) = request.substr(0, end);
            }
            // Update for the next iteration.
            fieldName = nextField;
            std::size_t start = request.find("/" + fieldName + "/") + fieldName.size() + 2;
            if (start == std::string::npos) {
                throw catena::exception_with_status("Could not find field " + fieldName, catena::StatusCode::INVALID_ARGUMENT);
            }
            request = request.substr(start);
        }
        // We assume the last field is until the end of the request.
        fields.at(fieldName) = request.substr(0, request.find(" HTTP/1.1"));
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