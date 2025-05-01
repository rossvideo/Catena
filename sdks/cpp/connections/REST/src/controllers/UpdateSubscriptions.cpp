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

// connections/REST
#include <controllers/UpdateSubscriptions.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>
#include <map>

namespace catena::REST {

int UpdateSubscriptions::objectCounter_ = 0;

UpdateSubscriptions::UpdateSubscriptions(tcp::socket& socket, ISocketReader& context, IDevice& dm)
    : socket_(socket),
      context_(context),
      dm_(dm),
      rc_("", catena::StatusCode::OK) {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
    try {
        // Parse JSON body if present
        if (!context_.jsonBody().empty()) {
            absl::Status status = google::protobuf::util::JsonStringToMessage(
                absl::string_view(context_.jsonBody()), &req_);
            if (!status.ok()) {
                rc_ = catena::exception_with_status("Failed to parse UpdateSubscriptionsPayload: " + status.ToString(), catena::StatusCode::INVALID_ARGUMENT);
                finish();
                return;
            }
        }
    } catch (...) {
        rc_ = catena::exception_with_status("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        finish();
        return;
    }
}

void UpdateSubscriptions::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    try {
        //I don't think authz is currently used in this controller
        catena::common::Authorizer* authz;
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }

        // Process removed OIDs
        for (const auto& oid : req_.removed_oids()) {     
            if (!context_.getSubscriptionManager().removeSubscription(oid, dm_, rc_)) {
                break;
            }
        }

        // Process added OIDs
        for (const auto& oid : req_.added_oids()) {
            if (!context_.getSubscriptionManager().addSubscription(oid, dm_, rc_)) {
                break;
            }
        }

    } catch (const catena::exception_with_status& e) {
        rc_ = catena::exception_with_status(e.what(), e.status);
    } catch (const std::exception& e) {
        rc_ = catena::exception_with_status(e.what(), catena::StatusCode::UNKNOWN);
    }

    finish();
}

void UpdateSubscriptions::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "UpdateSubscriptions[" << objectId_ << "] finished\n";
    
    if (!writer_) {
        writer_ = std::make_unique<SSEWriter>(socket_, context_.origin(), catena::REST::codeMap_.at(rc_.status));
    }
    socket_.close();
}

} // namespace catena::REST 
