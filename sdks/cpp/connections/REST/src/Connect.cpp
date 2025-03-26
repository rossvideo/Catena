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

void API::updateResponse(bool& hasUpdate, catena::PushUpdates& res, catena::common::Authorizer* authz, const std::string& oid, const int32_t idx, const IParam* p) {
    if (authorizationEnabled_ && !authz->readAuthz(*p)) {
        return;
    }

    res.mutable_value()->set_oid(oid);
    res.mutable_value()->set_element_index(idx);
    
    catena::Value* value = res.mutable_value()->mutable_value();

    catena::exception_with_status rc{"", catena::StatusCode::OK};
    rc = p->toProto(*value, *authz);
    //If the param conversion was successful, send the update
    if (rc.status == catena::StatusCode::OK) {
        hasUpdate = true;
    }
}

void API::connect(std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz) {
    // Creating a SocketWriter.
    ChunkedWriter writer(socket);
    try {        
        // Parsing fields
        std::unordered_map<std::string, std::string> fields = {
            {"force_connection", ""},
            {"user_agent", ""},
            {"detail_level", ""},
            {"language", ""}
        };
        parseFields(request, fields);

        bool hasUpdate = false;
        catena::PushUpdates res;
        // Write the HTTP response headers to stream.
        catena::exception_with_status status{"", catena::StatusCode::OK};
        writer.writeHeaders(status);

        // Setting up signal handlers.
        unsigned int valueSetByServerId_ = dm_.valueSetByServer.connect([this, &hasUpdate, &res, authz](const std::string& oid, const IParam* p, const int32_t idx){
            updateResponse(hasUpdate, res, authz, oid, idx, p);
        });

        // Waiting for a value set by client to be sent to execute code.
        unsigned int valueSetByClientId_ = dm_.valueSetByClient.connect([this, &hasUpdate, &res, authz](const std::string& oid, const IParam* p, const int32_t idx){
            updateResponse(hasUpdate, res, authz, oid, idx, p);
        });

        while (true) {
            if (hasUpdate) {
                writer.write(res);
                hasUpdate = false;
            }
        }
        
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer.write(err);
    } catch (...) {
        catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
        writer.write(err);
    }
}
