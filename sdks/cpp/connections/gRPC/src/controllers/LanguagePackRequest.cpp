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
#include <controllers/LanguagePackRequest.h>
#include <Logger.h>
using catena::gRPC::LanguagePackRequest;

// Initializes the object counter for LanguagePackRequest to 0.
int LanguagePackRequest::objectCounter_ = 0;

LanguagePackRequest::LanguagePackRequest(ICatenaServiceImpl *service, SlotMap& dms, bool ok)
    : CallData(service), dms_{dms}, responder_(&context_), status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    objectId_ = objectCounter_++;
    service_->registerItem(this);
    proceed(ok);
}

void LanguagePackRequest::proceed(bool ok) {
    DEBUG_LOG << "LanguagePackRequest::proceed[" << objectId_ << "]: "
              << timeNow() << " status: " << static_cast<int>(status_)
              << ", ok: " << std::boolalpha << ok;
    
    // If the process is cancelled, finish the process
    if (!ok) {
        DEBUG_LOG << "LanguagePackRequest[" << objectId_ << "] cancelled";
        status_ = CallStatus::kFinish;
    }

    switch(status_){
        /** 
         * kCreate: Updates status to kProcess and requests the
         * LanguagePackRequest command from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestLanguagePackRequest(&context_, &req_, &responder_, service_->cq(), service_->cq(), this);
            break;
        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            // Used to serve other clients while processing.
            new LanguagePackRequest(service_, dms_, ok);
            context_.AsyncNotifyWhenDone(this);
            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            catena::DeviceComponent_ComponentLanguagePack ans;
            IDevice* dm = nullptr;
            try {
                // Getting device at specified slot.
                if (dms_.contains(req_.slot())) {
                    dm = dms_.at(req_.slot());
                }

                // Making sure the device exists.
                if (!dm) {
                    rc = catena::exception_with_status("device not found in slot " + std::to_string(req_.slot()), catena::StatusCode::NOT_FOUND);

                // Getting and returning the requested language.
                } else {
                    std::lock_guard lg(dm->mutex());
                    rc = dm->getLanguagePack(req_.language(), ans);
                    status_ = CallStatus::kFinish;
                }
            } catch (...) { // Error, end process.
                rc = catena::exception_with_status("unknown error", catena::StatusCode::UNKNOWN);
            }
            // Writing response to the client.
            status_ = CallStatus::kFinish;
            if (rc.status == catena::StatusCode::OK) {
                responder_.Finish(ans, Status::OK, this);
            } else {
                responder_.FinishWithError(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
            }
            }
            break;
        /**
         * kFinish: Final step of gRPC is the deregister the item from
         * CatenaServiceImpl.
         */
        case CallStatus::kFinish:
            DEBUG_LOG << "LanguagePackRequest[" << objectId_ << "] finished";
            service_->deregisterItem(this);
            break;
        /*
         * default: Error, end process.
         * This should be impossible to reach.
         */
        default: // GCOVR_EXCL_START
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            responder_.FinishWithError(errorStatus, this);
            // GCOVR_EXCL_STOP
    }
}
