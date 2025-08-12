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
#include <controllers/MultiSetValue.h>
#include <Logger.h>
using catena::gRPC::MultiSetValue;

// Initializes the object counter for SetValue to 0.
int MultiSetValue::objectCounter_ = 0;

MultiSetValue::MultiSetValue(IServiceImpl *service, SlotMap& dms, bool ok)
    : MultiSetValue(service, dms, ok, objectCounter_++) {
    typeName = "MultiSetValue";
    service_->registerItem(this);
    proceed(ok);
}

MultiSetValue::MultiSetValue(IServiceImpl *service, SlotMap& dms, bool ok, int objectId)
    : CallData(service), dms_{dms}, objectId_(objectId), responder_(&context_),
    status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {}

void MultiSetValue::request_() {
    service_->RequestMultiSetValue(&context_, &reqs_, &responder_, service_->cq(), service_->cq(), this);
}

void MultiSetValue::create_(bool ok) {
    new MultiSetValue(service_, dms_, ok);
}

void MultiSetValue::proceed(bool ok) { 
    DEBUG_LOG << typeName << "::proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: "
                << std::boolalpha << ok;

    // If the process is cancelled, finish the process
    if (!ok) {
        DEBUG_LOG << typeName << "[" << objectId_ << "] cancelled";
        status_ = CallStatus::kFinish;
    }
    
    switch (status_) {
        /** 
         * kCreate: Updates status to kProcess and requests the SetValue
         * command from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            request_();
            break;
        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            // Used to serve other clients while processing.
            create_(ok);
            context_.AsyncNotifyWhenDone(this);
            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            try {
                IDevice* dm = nullptr;
                // Convert to MultiSetValuePayload if not already.
                toMulti_();
                // Getting device at specified slot.
                if (dms_.contains(reqs_.slot())) {
                    dm = dms_.at(reqs_.slot());
                }
                // Making sure the device exists.
                if (!dm) {
                    rc = catena::exception_with_status("device not found in slot " + std::to_string(reqs_.slot()), catena::StatusCode::NOT_FOUND);
                } else {
                    /**
                     * Creating authorization object depending on client scopes.
                     * Shared ptr to maintain lifetime of object. Raw ptr ensures
                     * kAUthzDisabled is not deleted when out of scope.
                     */
                    std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                    catena::common::Authorizer* authz;
                    if (service_->authorizationEnabled()) {
                        sharedAuthz = std::make_shared<catena::common::Authorizer>(jwsToken_());
                        authz = sharedAuthz.get();
                    } else {
                        authz = &catena::common::Authorizer::kAuthzDisabled;
                    }
                    // Locking device and setting value(s).
                    std::lock_guard lg(dm->mutex());
                    // Trying and commiting the multiSetValue.
                    if (dm->tryMultiSetValue(reqs_, rc, *authz)) {
                        rc = dm->commitMultiSetValue(reqs_, *authz);
                    }
                }
            // ERROR
            } catch (catena::exception_with_status& err) {
                rc = catena::exception_with_status{err.what(), err.status};
            } catch (...) {
                rc = catena::exception_with_status{"unknown error", catena::StatusCode::UNKNOWN};
            }
            // Changing state to kFinish and writing response to client.
            status_ = CallStatus::kFinish;
            if (rc.status == catena::StatusCode::OK) {
                responder_.Finish(st2138::Empty{}, Status::OK, this);
            } else {
                responder_.FinishWithError(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
            }
            }
            break;
        /**
         * kFinish: Final step of gRPC is the deregister the item from
         * ServiceImpl.
         */
        case CallStatus::kFinish:
            DEBUG_LOG << typeName << "[" << objectId_ << "] finished";
            service_->deregisterItem(this);
            break;
        /*
         * default: Error, end process.
         * This should be impossible to reach.
         */
        default:// GCOVR_EXCL_START
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            responder_.FinishWithError(errorStatus, this);
            // GCOVR_EXCL_STOP
    }
}
