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

/**
 * Constructor which initializes and registers the current GetParam object, 
 * then starts the process.
 */

CatenaServiceImpl::GetParam::GetParam(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

/**
 * Manages the steps of the GetParam gRPC command through the state variable status.
 */
void CatenaServiceImpl::GetParam::proceed(CatenaServiceImpl *service, bool ok) {
    std::cout << "GetParam::proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: "
              << std::boolalpha << ok << std::endl;
    
    if(!ok) {
        std::cout << "GetParam[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
        service->deregisterItem(this);
        return;
    }
    
    switch (status_) {
        /** 
         * kCreate: Updates status to kProcess and requests the GetParam
         * command from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestGetParam(&context_, &req_, &writer_, service_->cq_, service_->cq_,
                                            this);
            break;

        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            new GetParam(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);  
            status_ = CallStatus::kWrite;
            // Falling through to kWrite.

        /**
         * kWrite: Writes the response to the client.
         */
        case CallStatus::kWrite:
            try {
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                // Creating authorizer.
                std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                catena::common::Authorizer* authz;
                if (service->authorizationEnabled()) {
                    sharedAuthz = std::make_shared<catena::common::Authorizer>(getJWSToken());
                    authz = sharedAuthz.get();
                } else {
                    authz = &catena::common::Authorizer::kAuthzDisabled;
                }
                // Getting the param.
                std::unique_ptr<IParam> param = dm_.getParam(req_.oid(), rc, *authz);
                // If everything was successful, writing to the client.
                if (rc.status == catena::StatusCode::OK && param) {
                    catena::DeviceComponent_ComponentParam response;
                    response.set_oid(param->getOid());
                    param->toProto(*response.mutable_param(), *authz);
                    status_ = CallStatus::kPostWrite;
                    writer_.Write(response, this);
                } else { // Error
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                }
            // Likely authentication error, end process.
            } catch (catena::exception_with_status& err) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(err.status), err.what());
                writer_.Finish(errorStatus, this);
            } catch (...) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
            }
            break;

        /**
         * kPostWrite: Finish writing the response to the client.
         */
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        /**
         * kFinish: Final step of gRPC is to deregister the item from
         * CatenaServiceImpl.
         */
        case CallStatus::kFinish:
            std::cout << "GetParam[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;

        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}
