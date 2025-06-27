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

// connections/gRPC
#include <controllers/UpdateSubscriptions.h>
using catena::gRPC::UpdateSubscriptions;

int UpdateSubscriptions::objectCounter_ = 0;

UpdateSubscriptions::UpdateSubscriptions(ICatenaServiceImpl *service, SlotMap& dms, bool ok)
    : CallData(service), dms_{dms}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed( ok);  // start the process
}

void UpdateSubscriptions::proceed(bool ok) {
    std::cout << "UpdateSubscriptions proceed[" << objectId_ << "]: "
              << timeNow() << " status: " << static_cast<int>(status_)
              << ", ok: " << std::boolalpha << ok << std::endl;

    // If the process is cancelled, finish the process
    if (!ok) {
        std::cout << "UpdateSubscriptions[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }

    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestUpdateSubscriptions(&context_, &req_, &writer_, service_->cq(), service_->cq(), this);
            break;

        case CallStatus::kProcess:
            new UpdateSubscriptions(service_, dms_, ok);
            context_.AsyncNotifyWhenDone(this);
            
            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            try {
                // Getting device at specified slot.
                if (dms_.contains(req_.slot())) {
                    dm_ = dms_.at(req_.slot());
                }
                // Making sure the device exists.
                if (!dm_) {
                    rc = catena::exception_with_status("device not found in slot " + std::to_string(req_.slot()), catena::StatusCode::NOT_FOUND);

                // Make sure subscriptions are enabled.
                } else if (!dm_->subscriptions()) {
                    rc = catena::exception_with_status("Subscriptions are not enabled for this device", catena::StatusCode::FAILED_PRECONDITION);

                } else {
                    // Supressing errors.
                    catena::exception_with_status supressErr{"", catena::StatusCode::OK};
                    // Creating authorizer.
                    if (service_->authorizationEnabled()) {
                        sharedAuthz_ = std::make_shared<catena::common::Authorizer>(jwsToken_());
                        authz_ = sharedAuthz_.get();
                    } else {
                        authz_ = &catena::common::Authorizer::kAuthzDisabled;
                    }
                    // Process added OIDs
                    for (const auto& oid : req_.added_oids()) {                    
                        service_->getSubscriptionManager().addSubscription(oid, *dm_, supressErr, *authz_);
                    }
                    // Process removed OIDs
                    for (const auto& oid : req_.removed_oids()) {     
                        service_->getSubscriptionManager().removeSubscription(oid, *dm_, supressErr);
                    }
                    // Getting all subscribed OIDs and entering kWrite
                    subbedOids_ = service_->getSubscriptionManager().getAllSubscribedOids(*dm_);
                    it_ = subbedOids_.begin();
                    status_ = CallStatus::kWrite;
                }
            // ERROR
            } catch (const catena::exception_with_status& err) {
                rc = catena::exception_with_status(err.what(), err.status);
            } catch (...) {
                rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
            }
            // Write error to client and break into kFinish.
            if (rc.status != catena::StatusCode::OK) {
                status_ = CallStatus::kFinish;
                writer_.Finish(grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                break;
            }
            // Fall through to kWrite.
            }

        case CallStatus::kWrite:
            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            std::unique_ptr<IParam> param = nullptr;
            catena::DeviceComponent_ComponentParam res;

            try {
                // Getting the next parameter while ignoring errors.
                while (!param && it_ != subbedOids_.end() && dm_) {
                    std::lock_guard lg(dm_->mutex());
                    catena::exception_with_status supressErr{"", catena::StatusCode::OK};
                    param = dm_->getParam(*it_, supressErr);
                    // If param exists then serialize the response.
                    if (param) {
                        res.Clear();
                        res.set_oid(param->getOid());
                        supressErr = param->toProto(*res.mutable_param(), *authz_);
                        if (supressErr.status != catena::StatusCode::OK) {
                            param = nullptr; // Most likely authz fail.
                        }
                    }
                    it_++;
                }
            // ERROR
            } catch (const catena::exception_with_status& err) {
                rc = catena::exception_with_status(err.what(), err.status);
            } catch (...) {
                rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
            }

            if (rc.status == catena::StatusCode::OK) {
                // If we have a parameter, send it to the client.
                if (param) {
                    writer_.Write(res, this);
                    break;
                // If we dont have a parameter the we are done. Enter kPostWrite.
                } else {
                    status_ = CallStatus::kPostWrite;
                }
            // Write error to client and break into kFinish.
            } else {
                status_ = CallStatus::kFinish;
                writer_.Finish(grpc::Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                break;
            }
            }
        
        // Status after finishing writing the response, transitions to kFinish
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        case CallStatus::kFinish:
            std::cout << "UpdateSubscriptions[" << objectId_ << "] finished\n";
            service_->deregisterItem(this);
            break;
        /*
         * default: Error, end process.
         * This should be impossible to reach.
         */
        default:// GCOVR_EXCL_START
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
            // GCOVR_EXCL_STOP
    }
}
