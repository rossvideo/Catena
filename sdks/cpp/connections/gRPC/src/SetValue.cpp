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
#include <SetValue.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

int CatenaServiceImpl::SetValue::objectCounter_ = 0;

CatenaServiceImpl::SetValue::SetValue(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, responder_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    objectId_ = objectCounter_++;
    service->registerItem(this);
    proceed(service, ok);
}

void CatenaServiceImpl::SetValue::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "SetValue::proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    if(!ok){
        status_ = CallStatus::kFinish;
    }
    
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestSetValue(&context_, &req_, &responder_, service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            new SetValue(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            try {
                catena::exception_with_status rc{"", catena::StatusCode::OK};

                if (service->authorizationEnabled()) {
                    std::vector<std::string> clientScopes = service->getScopes(context_);  
                    catena::common::Authorizer authz{clientScopes};
                    Device::LockGuard lg(dm_);
                    rc = dm_.setValue(req_.oid(), *req_.mutable_value(), authz);
                } else {
                    Device::LockGuard lg(dm_);
                    rc = dm_.setValue(req_.oid(), *req_.mutable_value(), catena::common::Authorizer::kAuthzDisabled);
                }

                if (rc.status == catena::StatusCode::OK) {
                    status_ = CallStatus::kFinish;
                    responder_.Finish(catena::Empty{}, Status::OK, this);
                } else {
                    errorStatus_ = Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
                    status_ = CallStatus::kFinish;
                    responder_.Finish(::catena::Empty{}, errorStatus_, this);
                }
            } catch (...) {
                errorStatus_ = Status(grpc::StatusCode::INTERNAL, "unknown error");
                status_ = CallStatus::kFinish;
                responder_.Finish(::catena::Empty{}, errorStatus_, this);
            }
            break;

        case CallStatus::kFinish:
            std::cout << "SetValue[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;

        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            responder_.FinishWithError(errorStatus, this);
    }
}
