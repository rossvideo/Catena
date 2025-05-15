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
#include <controllers/GetParam.h>
using catena::gRPC::GetParam;

int GetParam::objectCounter_ = 0;

/**
 * Constructor which initializes and registers the current GetParam object, 
 * then starts the process.
 */

GetParam::GetParam(ICatenaServiceImpl *service, IDevice& dm, bool ok)
    : CallData(service), dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(ok);  // start the process
}

/**
 * Manages the steps of the GetParam gRPC command through the state variable status.
 */
void GetParam::proceed( bool ok) {
    std::cout << "GetParam::proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: "
              << std::boolalpha << ok << std::endl;
    
    if(!ok) {
        std::cout << "GetParam[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
        service_->deregisterItem(this);
        return;
    }
    
    switch (status_) {
        /*
         * kCreate: Updates status to kProcess and requests the GetParam
         * command from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestGetParam(&context_, &req_, &writer_, service_->cq(), service_->cq(),
                                            this);
            break;

        /*
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            new GetParam(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);

            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            // Creating authorizer.
            if (service_->authorizationEnabled()) {
                sharedAuthz_ = std::make_shared<catena::common::Authorizer>(jwsToken_());
                authz_ = sharedAuthz_.get();
            } else {
                authz_ = &catena::common::Authorizer::kAuthzDisabled;
            }
            // Getting the param.
            std::unique_ptr<IParam> param = dm_.getParam(req_.oid(), rc, *authz_);
            // Everything went well, fall through to kWrite.
            if (param && rc.status == catena::StatusCode::OK) {
                pds_.push_back(&param->getDescriptor());
                status_ = CallStatus::kWrite;
            // Error along the way, finish call with error.
            } else {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(rc.status), rc.what());
                writer_.Finish(errorStatus, this);
                break;
            }
            }

        /*
         * kWrite: Writes the response to the client.
         */
        case CallStatus::kWrite:
            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            catena::DeviceComponent_ComponentParam response;
            // Getting and writing param descriptor.
            auto pd = std::move(pds_.back());
            pds_.pop_back();
            response.set_oid(pd->getOid());
            pd->toProto(*response.mutable_param(), *authz_);
            // If param has children, add child oids to the oids_ vector.
            for (auto [oid, childDesc] : pd->getAllSubParams()) {
                response.add_sub_params(oid);
                pds_.push_back(childDesc);
            }
            // Write the param descriptor
            writer_.Write(response, this);
            // If there is no more to write, change to kPostWrite.
            if (pds_.empty()) {
                status_ = CallStatus::kPostWrite;
            }
            }
            break;

        /*
         * kPostWrite: Finish writing the response to the client.
         */
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        /*
         * kFinish: Final step of gRPC is to deregister the item from
         * CatenaServiceImpl.
         */
        case CallStatus::kFinish:
            std::cout << "GetParam[" << objectId_ << "] finished\n";
            service_->deregisterItem(this);
            break;

        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}
