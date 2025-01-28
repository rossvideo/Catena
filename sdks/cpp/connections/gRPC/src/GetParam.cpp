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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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
#include <GetParam.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

int CatenaServiceImpl::GetParam::objectCounter_ = 0;

CatenaServiceImpl::GetParam::GetParam(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::GetParam::proceed(CatenaServiceImpl *service, bool ok) {
    if (!service || status_ == CallStatus::kFinish) {
        return;
    }

    std::cout << "GetParam proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
              << std::endl;

    if(!ok) {
        std::cout << "GetParam[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
        service->deregisterItem(this);
        return;
    }
    
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestGetParam(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                            this);
            break;

        case CallStatus::kProcess:
            new GetParam(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);  
            status_ = CallStatus::kWrite;
            //break;

        case CallStatus::kWrite:
            try {
                std::cout << "sending param component for OID: '" << req_.oid() << "'" << std::endl;
                std::cout << "slot: " << req_.slot() << std::endl;
                //std::cout << "authorization enabled: " << std::boolalpha << service->authorizationEnabled() << std::endl;
                std::unique_ptr<IParam> param;
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                catena::common::Authorizer authz{std::vector<std::string>{}};  // Initialize empty
                
                if (service->authorizationEnabled()) {
                    std::vector<std::string> scopes = service->getScopes(context_);
                    authz = catena::common::Authorizer{scopes};  // Assign new value
                    {
                        Device::LockGuard lg(dm_);
                        param = dm_.getParam(req_.oid(), rc, authz);
                    }
                } else {
                    {
                        Device::LockGuard lg(dm_);
                        param = dm_.getParam(req_.oid(), rc);
                    }
                }
                
                if (rc.status == catena::StatusCode::OK && param) {
                    catena::DeviceComponent_ComponentParam response;
                    if (service->authorizationEnabled()) {
                        param->toProto(*response.mutable_param(), authz);
                    } else {
                        param->toProto(*response.mutable_param(), catena::common::Authorizer::kAuthzDisabled);
                    }
                    writer_.Write(response, this);
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::OK, this);
                    break;
                } else {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                    break;
                }
            } catch (...) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
            }
            break;

        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        case CallStatus::kFinish:
            std::cout << "GetParam[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
    }
}
