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

int CatenaServiceImpl::Connect::objectCounter_ = 0;

CatenaServiceImpl::Connect::Connect(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::Connect::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "Connect proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    // The newest connect object (the one that has not yet been attached to a client request)
    // will send shutdown signal to cancel all open connections
    if(!ok && status_ != CallStatus::kFinish){
        std::cout << "Connect[" << objectId_ << "] cancelled\n";
        std::cout << "Cancelling all open connections" << std::endl;
        shutdownSignal_.emit();
        status_ = CallStatus::kFinish;
    }

    std::unique_lock<std::mutex> lock{mtx_, std::defer_lock};
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestConnect(&context_, &req_, &writer_, service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            new Connect(service_, dm_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            shutdownSignalId_ = shutdownSignal_.connect([this](){
                context_.TryCancel();
                hasUpdate_ = true;
                this->cv_.notify_one();
            });
            valueSetByServerId_ = dm_.valueSetByServer.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
                try{
                    if (this->context_.IsCancelled()){
                        this->hasUpdate_ = true;
                        this->cv_.notify_one();
                        return;
                    }

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
            valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
                try{
                    if (this->context_.IsCancelled()){
                        this->hasUpdate_ = true;
                        this->cv_.notify_one();
                        return;
                    }

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

        case CallStatus::kWrite:
            lock.lock();
            cv_.wait(lock, [this] { return hasUpdate_; });
            hasUpdate_ = false;
            if (context_.IsCancelled()) {
                status_ = CallStatus::kFinish;
                std::cout << "Connection[" << objectId_ << "] cancelled\n";
                writer_.Finish(Status::CANCELLED, this);
                break;
            } else {
                res_.set_slot(dm_.slot());
                writer_.Write(res_, this);
            }
            lock.unlock();
            break;

        case CallStatus::kFinish:
            std::cout << "Connect[" << objectId_ << "] finished\n";
            shutdownSignal_.disconnect(shutdownSignalId_);
            dm_.valueSetByClient.disconnect(valueSetByClientId_);
            dm_.valueSetByServer.disconnect(valueSetByServerId_);
            service->deregisterItem(this);
            break;

        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}
