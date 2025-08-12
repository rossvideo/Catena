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
#include <controllers/GetPopulatedSlots.h>
#include <Logger.h>
using catena::gRPC::GetPopulatedSlots;

// Initializes the object counter for GetPopulatedSlots to 0.
int GetPopulatedSlots::objectCounter_ = 0;

/**
 * Constructor which initializes and registers the current GetPopulatedSlots
 * object, then starts the process.
 */
GetPopulatedSlots::GetPopulatedSlots(IServiceImpl *service, SlotMap& dms, bool ok)
    : CallData(service), dms_{dms}, responder_(&context_), status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    objectId_ = objectCounter_++;
    service_->registerItem(this);
    proceed(ok);
}

// Manages gRPC command execution process using the state variable status.
void GetPopulatedSlots::proceed( bool ok) {
    DEBUG_LOG << "GetPopulatedSlots::proceed[" << objectId_ << "]: "
              << timeNow() << " status: " << static_cast<int>(status_)
              << ", ok: " << std::boolalpha << ok;

    // If the process is cancelled, finish the process
    if (!ok) {
        DEBUG_LOG << "GetPopulatedSlots[" << objectId_ << "] cancelled";
        status_ = CallStatus::kFinish;
    }

    switch(status_){
        /** 
         * kCreate: Updates status to kProcess and requests the
         * GetPopulatedSlots command from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestGetPopulatedSlots(&context_, &req_, &responder_, service_->cq(), service_->cq(), this);
            break;
        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            {
                // Used to serve other clients while processing.
                new GetPopulatedSlots(service_, dms_, ok);
                context_.AsyncNotifyWhenDone(this);
                st2138::SlotList ans;
                for (auto [slot, dm] : dms_) {
                    // If a devices exists at the slot, add it to the response.
                    if (dm) {
                        ans.add_slots(slot);
                    }
                }
                status_ = CallStatus::kFinish;
                responder_.Finish(ans, Status::OK, this);
            }
        break;
        /**
         * kFinish: Final step of gRPC is the deregister the item from
         * ServiceImpl.
         */
        case CallStatus::kFinish:
            DEBUG_LOG << "GetPopulatedSlots[" << objectId_ << "] finished";
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
