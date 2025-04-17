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
#include <DeviceRequest.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

// Counter for generating unique object IDs - static, so initializes at start
int CatenaServiceImpl::DeviceRequest::objectCounter_ = 0;

/** 
 * Constructor which initializes and registers the current DeviceRequest
 * object, then starts the process
 */
CatenaServiceImpl::DeviceRequest::DeviceRequest(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

/** 
 * Manages gRPC command execution process by transitioning between states and
 * handling errors and responses accordingly 
 */
void CatenaServiceImpl::DeviceRequest::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "DeviceRequest proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    // If the process is cancelled, finish the process
    if(!ok){
        std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }
    
    //State machine to manage the process
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestDeviceRequest(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                            this);
            break;

        /**
         * Processes the command by reading the initial request from the client
         * and transitioning to kRead
         */
        case CallStatus::kProcess:
            new DeviceRequest(service_, dm_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            // shutdownSignalId_ = shutdownSignal.connect([this](){
            //     context_.TryCancel();
            //     std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
            // });
            {
            try {
                bool shallowCopy = true; // controls whether shallow copy or deep copy is used
                dm_.detail_level(req_.detail_level());
                
                // Get service subscriptions from the manager
                subscribed_oids_ = service_->subscriptionManager_.getAllSubscribedOids(dm_);
                
                // If this request has subscriptions, add them
                if (!req_.subscribed_oids().empty()) {
                    // Add new subscriptions to both the manager and our tracking list
                    for (const auto& oid : req_.subscribed_oids()) {
                        catena::exception_with_status rc{"", catena::StatusCode::OK};
                        if (!service_->subscriptionManager_.addSubscription(oid, dm_, rc)) {
                            continue; // Skip if subscription fails to add
                        } else {
                            rpc_subscriptions_.push_back(oid);
                        }
                    }
                }
                
                // Get final list of subscriptions for this response
                subscribed_oids_ = service_->subscriptionManager_.getAllSubscribedOids(dm_);
                
                //Handle authorization
                std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                catena::common::Authorizer* authz;
                if (service_->authorizationEnabled()) {
                    sharedAuthz = std::make_shared<catena::common::Authorizer>(getJWSToken());
                    authz = sharedAuthz.get();
                } else {
                    authz = &catena::common::Authorizer::kAuthzDisabled;
                }

                // If we're in SUBSCRIPTIONS mode and have no subscriptions, we'll still send minimal set
                if (dm_.subscriptions() && subscribed_oids_.empty() && 
                    req_.detail_level() == catena::Device_DetailLevel_SUBSCRIPTIONS) {
                    serializer_ = dm_.getComponentSerializer(*authz, shallowCopy);
                } else {
                    serializer_ = dm_.getComponentSerializer(*authz, subscribed_oids_, shallowCopy);
                }

            // Likely authentication error, end process.
            } catch (catena::exception_with_status& err) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(err.status), err.what());
                writer_.Finish(errorStatus, this);
                break;
            }
            }
            status_ = CallStatus::kWrite;
            // fall thru to start writing
        
        /**
         * Writes the response to the client by sending the external object and
         * then continues to kPostWrite or kFinish
         */
        case CallStatus::kWrite:
            {   
                if (!serializer_) {
                    // It should not be possible to get here
                    status_ = CallStatus::kFinish;
                    grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
                    writer_.Finish(errorStatus, this);
                    break;
                } 
                
                try {     
                    catena::DeviceComponent component{};
                    {
                        Device::LockGuard lg(dm_);
                        component = serializer_->getNext();
                    }
                    status_ = serializer_->hasMore() ? CallStatus::kWrite : CallStatus::kPostWrite;
                    writer_.Write(component, this); 
                //Exception occured, finish the process
                } catch (catena::exception_with_status &e) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
                //Catch all other exceptions and finish the process
                } catch (...) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::CANCELLED, this);
                }
            }
            break;

        // Status after finishing writing the response, transitions to kFinish
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        /**
         * Deregisters the current ExternalObjectRequest object and finishes
         * the process
         */
        case CallStatus::kFinish:
            std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
            //shutdownSignal.disconnect(shutdownSignalId_);
            service->deregisterItem(this);
            break;

        // Throws an error if the state is not recognized
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}