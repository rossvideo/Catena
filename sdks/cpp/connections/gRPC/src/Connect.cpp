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

// common
#include <Tags.h>

// connections/gRPC
#include <Connect.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

// Initializes the object counter for Connect to 0.
int CatenaServiceImpl::Connect::objectCounter_ = 0;

/**
 * Constructor which initializes and registers the current Connect object, 
 * then starts the process.
 */
CatenaServiceImpl::Connect::Connect(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

// Manages gRPC command execution process using the state variable status.
void CatenaServiceImpl::Connect::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "Connect proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    /**
     * The newest connect object (the one that has not yet been attached to a
     * client request) will send shutdown signal to cancel all open connections
     */
    if(!ok && status_ != CallStatus::kFinish){
        std::cout << "Connect[" << objectId_ << "] cancelled\n";
        std::cout << "Cancelling all open connections" << std::endl;
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
            service_->RequestConnect(&context_, &req_, &writer_, service_->cq_, service_->cq_, this);
            break;
        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            // Used to serve other clients while processing.
            new Connect(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            // Cancels all open connections if shutdown signal is sent.
            shutdownSignalId_ = shutdownSignal_.connect([this](){
                context_.TryCancel();
                hasUpdate_ = true;
                this->cv_.notify_one();
            });
            // Waiting for a value set by server to be sent to execute code.
            valueSetByServerId_ = dm_.valueSetByServer.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
                try{
                    // If Connect was cancelled, notify client and end process.
                    if (this->context_.IsCancelled()){
                        this->hasUpdate_ = true;
                        this->cv_.notify_one();
                        return;
                    }
                    /** 
                     * Set the object id and index to the values set by the
                     * client, then convert to a proto and assign it to the
                     * IParam p.
                     * Additionally, if authorization is enabled, check the
                     * client's scopes.
                     */
                    if(service_->authorizationEnabled()){
                        std::vector<std::string> clientScopes = service_->getScopes(context_);
                        catena::common::Authorizer authz{clientScopes};
                        if (authz.readAuthz(*p)){
                            this->res_.mutable_value()->set_oid(oid);
                            this->res_.mutable_value()->set_element_index(idx);
                            
                            catena::Value* value = this->res_.mutable_value()->mutable_value();
                            p->toProto(*value, authz);
                            this->hasUpdate_ = true;
                            this->cv_.notify_one();
                        }
                    } else {
                        this->res_.mutable_value()->set_oid(oid);
                        this->res_.mutable_value()->set_element_index(idx);

                        catena::Value* value = this->res_.mutable_value()->mutable_value();
                        p->toProto(*value, catena::common::Authorizer::kAuthzDisabled);
                        this->hasUpdate_ = true;
                        this->cv_.notify_one();
                    }
                }catch(catena::exception_with_status& why){
                    // if an error is thrown, no update is pushed to the client
                } 
            });
            // Waiting for a value set by client to be sent to execute code.
            valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
                try{
                    // If Connect was cancelled, notify client and end process.
                    if (this->context_.IsCancelled()){
                        this->hasUpdate_ = true;
                        this->cv_.notify_one();
                        return;
                    }
                    /** 
                     * Set the object id and index to the values set by the
                     * client, then convert to a proto and assign it to the
                     * IParam p.
                     * Additionally, if authorization is enabled, check the
                     * client's scopes.
                     */
                    if (service_->authorizationEnabled()){
                        std::vector<std::string> clientScopes = service_->getScopes(context_);
                        catena::common::Authorizer authz{clientScopes};
                        if (authz.readAuthz(*p)){
                            this->res_.mutable_value()->set_oid(oid);
                            this->res_.mutable_value()->set_element_index(idx);
                            catena::Value* value = this->res_.mutable_value()->mutable_value();
                            p->toProto(*value, authz);
                            this->hasUpdate_ = true;
                            this->cv_.notify_one(); 
                        }
                    } else { 
                        this->res_.mutable_value()->set_oid(oid);
                        this->res_.mutable_value()->set_element_index(idx);
                        catena::Value* value = this->res_.mutable_value()->mutable_value();
                        p->toProto(*value, catena::common::Authorizer::kAuthzDisabled);
                        this->hasUpdate_ = true;
                        this->cv_.notify_one();
                    }
                }catch(catena::exception_with_status& why){
                    // if an error is thrown, no update is pushed to the client
                } 
            });

            // send client a empty update with slot of the device
            {
                status_ = CallStatus::kWrite;
                catena::PushUpdates populatedSlots;
                populatedSlots.set_slot(dm_.slot());
                writer_.Write(populatedSlots, this);
            }
            break;
        /**
         * kWrite: Waits until an update to either set res to device's slot or
         * end the process.
         */
        case CallStatus::kWrite:
            // Waiting until there is an update to proceed.
            lock.lock();
            cv_.wait(lock, [this] { return hasUpdate_; });
            hasUpdate_ = false;
            // If connect was cancelled finish the process.
            if (context_.IsCancelled()) {
                status_ = CallStatus::kFinish;
                std::cout << "Connection[" << objectId_ << "] cancelled\n";
                writer_.Finish(Status::CANCELLED, this);
                break;
            // Send the client an update with the slot of the device.
            } else {
                res_.set_slot(dm_.slot());
                writer_.Write(res_, this);
            }
            lock.unlock();
            break;
        /**
         * kFinish: Ends the connection.
         */
        case CallStatus::kFinish:
            std::cout << "Connect[" << objectId_ << "] finished\n";
            shutdownSignal_.disconnect(shutdownSignalId_);
            dm_.valueSetByClient.disconnect(valueSetByClientId_);
            dm_.valueSetByServer.disconnect(valueSetByServerId_);
            service->deregisterItem(this);
            break;
        // default: Error, end process.
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}