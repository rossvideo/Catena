
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


/**
 * @file DeviceRequest.cpp
 * @brief Implements Catena gRPC Device Request
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-06-08
 * @copyright Copyright © 2024 Ross Video Ltd
 */

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

int CatenaServiceImpl::DeviceRequest::objectCounter_ = 0;

CatenaServiceImpl::DeviceRequest::DeviceRequest(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::DeviceRequest::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "DeviceRequest proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    if(!ok){
        std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }
    
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestDeviceRequest(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                            this);
            break;

        case CallStatus::kProcess:
            new DeviceRequest(service_, dm_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            // shutdownSignalId_ = shutdownSignal.connect([this](){
            //     context_.TryCancel();
            //     std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
            // });
            {
                bool shallowCopy = true; // controls whether shallow copy or deep copy is used
                if (service->authorizationEnabled()) {
                    clientScopes_ = service->getScopes(context_);  
                    authz_ = std::make_unique<catena::common::Authorizer>(clientScopes_);
                    serializer_ = dm_.getComponentSerializer(*authz_, shallowCopy);
                } else {
                    serializer_ = dm_.getComponentSerializer(catena::common::Authorizer::kAuthzDisabled, shallowCopy);
                }
            }
            status_ = CallStatus::kWrite;
            // fall thru to start writing

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
                } catch (catena::exception_with_status &e) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
                } catch (...) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::CANCELLED, this);
                }
            }
            break;

        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        case CallStatus::kFinish:
            std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
            //shutdownSignal.disconnect(shutdownSignalId_);
            service->deregisterItem(this);
            break;

        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}
