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

crow::response API::getValue(const crow::request& req) {
    try {
        uint32_t slot = 0;
        std::string oid = "";
        // Attempting to convert request body to JSON.
        auto json_body = crow::json::load(req.body);
        if (!json_body) {
            return crow::response(toCrowStatus_.at(catena::StatusCode::INVALID_ARGUMENT), "Invalid JSON");
        }

        // Extracting slot and oid from the JSON body.
        if (json_body.has("slot")) { slot = json_body["slot"].u(); }
        if (json_body.has("oid")) { oid = json_body["oid"].s(); }

        auto auth_header = req.get_header_value("Authorization");

        // Getting value at oid from device.
        catena::Value ans;
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        if(authorizationEnabled_) {
            catena::common::Authorizer authz{getJWSToken(req)};
            Device::LockGuard lg(dm_);
            rc = dm_.getValue(oid, ans, authz);
        } else {
            Device::LockGuard lg(dm_);
            rc = dm_.getValue(oid, ans, catena::common::Authorizer::kAuthzDisabled);
        }

        if (rc.status == catena::StatusCode::OK) {
            // Converting the value to JSON.
            std::string json_output;
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = true;
            auto status = MessageToJsonString(ans, &json_output, options);

            // Check if the conversion was successful.
            if (!status.ok()) {
                return crow::response(toCrowStatus_.at(catena::StatusCode::INVALID_ARGUMENT), "Failed to convert protobuf to JSON");
            }

            // Create and return a Crow response with JSON content type.
            crow::response res;
            res.code = toCrowStatus_.at(catena::StatusCode::OK);
            res.set_header("Content-Type", "application/json");
            res.write(json_output);
            return res;

        } else {
            return crow::response(toCrowStatus_.at(rc.status), rc.what());
        }
    // Likely authentication error, end process.
    } catch (catena::exception_with_status& err) {
        return crow::response(toCrowStatus_.at(err.status), err.what());
    } catch (...) { // Error, end process.
        return crow::response(toCrowStatus_.at(catena::StatusCode::UNKNOWN), "Unknown Error");
    }
}
