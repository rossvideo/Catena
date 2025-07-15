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
#include <controllers/Connect.h>
#include <Logger.h>
using catena::gRPC::Connect;
using catena::common::ILanguagePack;

// Initializes the object counter for Connect to 0.
int catena::gRPC::Connect::objectCounter_ = 0;

/**
 * Constructor which initializes and registers the current Connect object, 
 * then starts the process.
 */
catena::gRPC::Connect::Connect(ICatenaServiceImpl *service, SlotMap& dms, bool ok)
    : CallData(service), writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish}, 
        catena::common::Connect(dms, service->getSubscriptionManager()) {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(ok);  // start the process
}

// Manages gRPC command execution process using the state variable status.
void catena::gRPC::Connect::proceed(bool ok) {
    DEBUG_LOG << "Connect proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: "
                << std::boolalpha << ok;

    /**
     * The newest connect object (the one that has not yet been attached to a
     * client request) will send shutdown signal to cancel all open connections
     */
    if (!ok && status_ != CallStatus::kFinish) {
        DEBUG_LOG << "Connect[" << objectId_ << "] cancelled";
        DEBUG_LOG << "Cancelling all open connections";
        shutdownSignal_.emit();
        status_ = CallStatus::kFinish;
    }

    std::unique_lock<std::mutex> lock{mtx_, std::defer_lock};
    switch (status_) {
        /** 
         * kCreate: Updates status to kProcess and requests the Connect command
         * from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestConnect(&context_, &req_, &writer_, service_->cq(), service_->cq(), this);
            break;
        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            // Used to serve other clients while processing.
            new Connect(service_, dms_, ok);
            context_.AsyncNotifyWhenDone(this);
            try {
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                initAuthz_(jwsToken_(), service_->authorizationEnabled());
                // Cancels all open connections if shutdown signal is sent.
                shutdownSignalId_ = shutdownSignal_.connect([this](){
                    context_.TryCancel();
                    hasUpdate_ = true;
                    this->cv_.notify_one();
                });

                // Set detail level from request
                detailLevel_ = req_.detail_level();

                // Connecting to each device in dms_.
                for (auto [slot, dm] : dms_) {
                    if (dm) {
                        // Waiting for a value set by server to be sent to execute code.
                        valueSetByServerIds_[slot] = dm->getValueSetByServer().connect([this, slot](const std::string& oid, const IParam* p){
                            updateResponse_(oid, p, slot);
                        });
                        // Waiting for a value set by client to be sent to execute code.
                        valueSetByClientIds_[slot] = dm->getValueSetByClient().connect([this, slot](const std::string& oid, const IParam* p){
                            updateResponse_(oid, p, slot);
                        });
                        // Waiting for a language to be added to execute code.
                        languageAddedIds_[slot] = dm->getLanguageAddedPushUpdate().connect([this, slot](const ILanguagePack* l) {
                            updateResponse_(l, slot);
                        });
                        // Send client a empty update with slot of the device
                        catena::PushUpdates populatedSlots;
                        populatedSlots.set_slot(slot);
                        writer_.Write(populatedSlots, this);
                    }
                }

                status_ = CallStatus::kWrite;

            } catch (catena::exception_with_status& rc) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(rc.status), rc.what());
                writer_.Finish(errorStatus, this);
            }
            break;
        /**
         * kWrite: Waits until an update to either set res to device's slot or
         * end the process.
         */
        case CallStatus::kWrite:
            lock.lock();
            cv_.wait(lock, [this] { return hasUpdate_; });
            hasUpdate_ = false;
            // If connect was cancelled finish the process.
            if (context_.IsCancelled()) {
                status_ = CallStatus::kFinish;
                DEBUG_LOG << "Connection[" << objectId_ << "] cancelled";
                writer_.Finish(Status::CANCELLED, this);
                break;
            // Send the client an update with the slot of the device.
            } else {
                writer_.Write(res_, this);
            }
            lock.unlock();
            break;
        /**
         * kFinish: Ends the connection.
         */
        case CallStatus::kFinish:
            DEBUG_LOG << "Connect[" << objectId_ << "] finished";
            // Disconnecting all initialized listeners.
            if (shutdownSignalId_ != 0) { shutdownSignal_.disconnect(shutdownSignalId_); }
            for (auto [slot, dm] : dms_) {
                if (dm) {
                    if (valueSetByClientIds_.contains(slot)) {
                        dm->getValueSetByClient().disconnect(valueSetByClientIds_[slot]);
                    }
                    if (valueSetByServerIds_.contains(slot)) {
                        dm->getValueSetByServer().disconnect(valueSetByServerIds_[slot]);
                    }
                    if (languageAddedIds_.contains(slot)) {
                        dm->getLanguageAddedPushUpdate().disconnect(languageAddedIds_[slot]);
                    }
                }
            }
            service_->deregisterItem(this);
            break;
        // default: Error, end process.
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}

// Returns true if the connection has been cancelled.
bool catena::gRPC::Connect::isCancelled() {
    return context_.IsCancelled();
}
