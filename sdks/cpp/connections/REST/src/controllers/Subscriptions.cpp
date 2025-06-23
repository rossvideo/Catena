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
#include <controllers/Subscriptions.h>
using catena::REST::Subscriptions;

// Initializes the object counter for Subscriptions to 0.
int Subscriptions::objectCounter_ = 0;

Subscriptions::Subscriptions(tcp::socket& socket, ISocketReader& context, IDevice& dm)
    : socket_(socket), context_(context), dm_(dm) {
    
    // GET (stream)
    if (context.method() == "GET" && context.stream()) {
        writer_ = std::make_unique<SSEWriter>(socket_, context_.origin());
    // GET (no stream) or PUT
    } else {
        writer_ = std::make_unique<SocketWriter>(socket_, context_.origin(), true);
    }

    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void Subscriptions::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    catena::exception_with_status supressErr{"", catena::StatusCode::OK};

    try {
        if (dm_.subscriptions()) {
            // Creating authorizer.
            catena::common::Authorizer* authz = nullptr;
            std::shared_ptr<catena::common::Authorizer> sharedAuthz;
            if (context_.authorizationEnabled()) {
                sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
                authz = sharedAuthz.get();
            } else {
                authz = &catena::common::Authorizer::kAuthzDisabled;
            }

            // GET/subscriptions - Get and write all subscribed OIDs.
            if (context_.method() == "GET") {
                auto subbedOids = context_.getSubscriptionManager().getAllSubscribedOids(dm_);
                for (auto oid : subbedOids) {
                    supressErr = catena::exception_with_status{"", catena::StatusCode::OK};
                    catena::DeviceComponent_ComponentParam res;
                    auto param = dm_.getParam(oid, supressErr, *authz);
                    // Converting to proto.
                    if (param) {
                        res.set_oid(param->getOid());
                        supressErr = param->toProto(*res.mutable_param(), *authz);
                    }
                    // Writing if successful.
                    if (supressErr.status == catena::StatusCode::OK) {
                        writeConsole_(CallStatus::kWrite, socket_.is_open());
                        writer_->sendResponse(rc, res);
                    }
                }

            // PUT/subscriptions - Add/remove subscriptions.
            } else if (context_.method() == "PUT") {
                // Parsing JSON body.
                catena::UpdateSubscriptionsPayload req;
                absl::Status status = google::protobuf::util::JsonStringToMessage(absl::string_view(context_.jsonBody()), &req);
                if (!status.ok()) {
                    rc = catena::exception_with_status("Failed to parse JSON Body", catena::StatusCode::INVALID_ARGUMENT);
            
                } else {
                    // Processing removed OIDs
                    for (const auto& oid : req.removed_oids()) {
                        context_.getSubscriptionManager().removeSubscription(oid, dm_, supressErr);
                    }
                    // Processing added OIDs
                    for (const auto& oid : req.added_oids()) {
                        context_.getSubscriptionManager().addSubscription(oid, dm_, supressErr, *authz);
                    }
                }

            // Invalid method.
            } else {
                rc = catena::exception_with_status("", catena::StatusCode::INVALID_ARGUMENT);
            }
        } else {
            rc = catena::exception_with_status("Subscriptions are not enabled for this device", catena::StatusCode::FAILED_PRECONDITION);
        }
    // ERROR
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing rc to client.
    writer_->sendResponse(rc);
}

void Subscriptions::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << context_.method() << " Subscriptions[" << objectId_ << "] finished\n";
}
