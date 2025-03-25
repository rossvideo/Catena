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

void API::multiSetValue(catena::MultiSetValuePayload& payload, SocketWriter& writer, catena::common::Authorizer* authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    // Trying and commiting the multiSetValue.
    {
    Device::LockGuard lg(dm_);
    if (dm_.tryMultiSetValue(payload, rc, *authz)) {
        rc = dm_.commitMultiSetValue(payload, *authz);
    }
    }
    // Writing response.
    if (rc.status == catena::StatusCode::OK)   {
        catena::Empty ans = catena::Empty();
        writer.write(ans);
    } else {
        writer.write(rc);
    }
}

void API::multiSetValue(std::string& jsonPayload, Tcp::socket& socket, catena::common::Authorizer* authz) {
    // Creating SocketWriter.
    SocketWriter writer(socket);
    try {
        // Creating MultiSetValuePayload and converting to JSON.
        catena::MultiSetValuePayload payload;
        absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(jsonPayload), &payload);
        if (status.ok()) {
            multiSetValue(payload, writer, authz);
        } else {
            catena::exception_with_status err("Failed to convert protobuf to JSON", catena::StatusCode::INVALID_ARGUMENT);
            writer.write(err);
        }
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer.write(err);
    } catch (...) {
        catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
        writer.write(err);
    }
}
