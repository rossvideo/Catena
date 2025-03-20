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

API::API(Device &dm, uint16_t port) : version_{"1.0.0"}, port_{port}, dm_{dm} {
    CROW_ROUTE(app_, "/v1/GetPopulatedSlots")
    ([this]() { return this->getPopulatedSlots(); });
    CROW_ROUTE(app_, "/v1/GetValue")
    ([this](const crow::request& req) { return this->getValue(req); });
}

std::string API::version() const {
  return version_;
}

void API::run() {
    // if (is_port_in_use_()) {
    //     std::cerr << "Port " << port_ << " is already in use" << std::endl;
    //     return;
    // }
    // asio::ssl::context ssl_context(asio::ssl::context::tlsv12);

    // // find our certs
    // std::string path_to_certs(absl::GetFlag(FLAGS_certs));
    // expandEnvVariables(path_to_certs);
    
    // path_to_certs = "/home/vboxuser/test_certs";

    // // Set up SSL/TLS
    // ssl_context.set_options(asio::ssl::context::default_workarounds | 
    //                         asio::ssl::context::no_sslv2 | 
    //                         asio::ssl::context::single_dh_use);

    // // Load certificate and private key files
    // ssl_context.use_certificate_chain_file(path_to_certs + "/server.crt");
    // ssl_context.use_private_key_file(path_to_certs + "/server.key", asio::ssl::context::pem);
    // ssl_context.load_verify_file(path_to_certs + "/ca.crt");
    app_.port(port_).run();
    // app_.port(port_).ssl(std::move(ssl_context)).run();
}

crow::response API::getPopulatedSlots() {
    SlotList slotList;
    slotList.add_slots(dm_.slot());

    // Convert the SlotList message to JSON
    std::string json_output;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    auto status = MessageToJsonString(slotList, &json_output, options);

    // Check if the conversion was successful
    if (!status.ok()) {
        return crow::response(500, "Failed to convert protobuf to JSON");
    }

    // Create a Crow response with JSON content type
    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.write(json_output);
    return res;
}


crow::response API::getValue(const crow::request& req) {
    uint32_t slot = 0;
    std::string oid = "";
    // Attempting to convert request body to JSON.
    auto json_body = crow::json::load(req.body);
    if (!json_body) {
        return crow::response(400, "Invalid JSON");
    }

    // Extracting slot and oid from the JSON body.
    if (json_body.has("slot")) { slot = json_body["slot"].u(); }
    if (json_body.has("oid")) { oid = json_body["oid"].s(); }

    // Getting value at oid from device.
    catena::Value ans;
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    Device::LockGuard lg(dm_);
    rc = dm_.getValue(oid, ans, catena::common::Authorizer::kAuthzDisabled);

    if (rc.status == catena::StatusCode::OK) {
        // Converting the value to JSON.
        std::string json_output;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        auto status = MessageToJsonString(ans, &json_output, options);

        // Check if the conversion was successful.
        if (!status.ok()) {
            return crow::response(500, "Failed to convert protobuf to JSON");
        }

        // Create and return a Crow response with JSON content type.
        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_output);
        return res;

    } else {
        return crow::response(400, rc.what());
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