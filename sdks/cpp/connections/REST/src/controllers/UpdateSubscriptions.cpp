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
using catena::REST::UpdateSubscriptions;

// Initializes the object counter for UpdateSubscriptions to 0.
int UpdateSubscriptions::objectCounter_ = 0;

UpdateSubscriptions::UpdateSubscriptions(tcp::socket& socket, ISocketReader& context, IDevice& dm)
    : socket_(socket),
      context_(context),
      dm_(dm),
      writer_(socket, context.origin()) {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void UpdateSubscriptions::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    catena::exception_with_status rc{"", catena::StatusCode::OK};

    try {
        // Parsing JSON body.
        catena::UpdateSubscriptionsPayload req;
        absl::Status status = google::protobuf::util::JsonStringToMessage(
        absl::string_view(context_.jsonBody()), &req);
        if (!status.ok()) {
            rc = catena::exception_with_status("Failed to parse JSON Body", catena::StatusCode::INVALID_ARGUMENT);
        } else {
            // Supressing errors.
            catena::exception_with_status supressErr{"", catena::StatusCode::OK};

            // Processing removed OIDs
            for (const auto& oid : req.removed_oids()) {     
                context_.getSubscriptionManager().removeSubscription(oid, dm_, supressErr);
            }

            // Processing added OIDs
            for (const auto& oid : req.added_oids()) {
                context_.getSubscriptionManager().addSubscription(oid, dm_, supressErr);
            }
        }
    // ERROR
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& err) {
        rc = catena::exception_with_status(err.what(), catena::StatusCode::UNKNOWN);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing rc to client.
    writer_.sendResponse(rc);
}

void UpdateSubscriptions::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "UpdateSubscriptions[" << objectId_ << "] finished\n";
}
