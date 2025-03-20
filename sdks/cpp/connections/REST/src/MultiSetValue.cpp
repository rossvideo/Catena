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

crow::response API::multiSetValue(catena::MultiSetValuePayload& payload, const crow::request& req) {
    try {
        // Setting up authorizer if enabled.
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        std::vector<std::string> clientScopes;
        if (authorizationEnabled_) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(getJWSToken(req));
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }
        // Locking device and setting value(s).
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        Device::LockGuard lg(dm_);
        // Trying and commiting the multiSetValue.
        if (dm_.tryMultiSetValue(payload, rc, *authz)) {
            dm_.commitMultiSetValue(payload, *authz);
        }
        // Finishing by converting to crow::response.
        if (rc.status == catena::StatusCode::OK) {
            auto ans = catena::Empty{};
            return finish(ans);
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

crow::response API::multiSetValue(const crow::request& req) {
    // Converting JSON to MultiSetValuePayload.
    catena::MultiSetValuePayload payload;
    absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(req.body), &payload);
    if (!status.ok()) {
        return crow::response(toCrowStatus_.at(catena::StatusCode::INVALID_ARGUMENT), "Failed to parse MultiSetValuePayload");
    }
    return multiSetValue(payload, req);
}
