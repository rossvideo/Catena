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
#include <GenericSetValue.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

// Generic constructor for SetValue and MultiSetValue gRPCs.
CatenaServiceImpl::GenericSetValue::GenericSetValue(CatenaServiceImpl *service, Device &dm, bool ok, int objectId)
    : service_{service}, dm_{dm}, objectId_(objectId), responder_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
        service->registerItem(this);
}

// Function defined by child classes to get respective gRPCs from the system.
void CatenaServiceImpl::GenericSetValue::request() {return;}

// Function defined by child classes to create new repsective gRPC.
void CatenaServiceImpl::GenericSetValue::create(CatenaServiceImpl *service, Device &dm, bool ok) {return;}

// Manages gRPC command execution process using the state variable status.
void CatenaServiceImpl::GenericSetValue::proceed(CatenaServiceImpl *service, bool ok) { 
    std::cout << typeName << "::proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;
    
    if(!ok){
        status_ = CallStatus::kFinish;
    }
    
    switch (status_) {
        /** 
         * kCreate: Updates status to kProcess and requests the SetValue
         * command from the service.
         */ 
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            request();
            break;
        /**
         * kProcess: Processes the request asyncronously, updating status to
         * kFinish and notifying the responder once finished.
         */
        case CallStatus::kProcess:
            // Used to serve other clients while processing.
            create(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            try {
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                /**
                 * Looping through each request. In case of SetValue, reqs_ is
                 * a MultiSetValuePayload containing a single request.
                 */
                for (auto &setValuePayload : reqs_.values()) {
                    std::string oid = setValuePayload.oid();
                    catena::Value value = setValuePayload.value();
                    // If authorization is enabled, check the client's scopes.
                    if (service->authorizationEnabled()) {
                        std::vector<std::string> clientScopes = service->getScopes(context_);  
                        catena::common::Authorizer authz{clientScopes};
                        Device::LockGuard lg(dm_);
                        rc = dm_.setValue(oid, value, authz);
                    } else {
                        Device::LockGuard lg(dm_);
                        rc = dm_.setValue(oid, value, catena::common::Authorizer::kAuthzDisabled);
                    }
                    // Breaking out of for loop in case of error.
                    if (rc.status != catena::StatusCode::OK) {
                        break;
                    }
                }
                // End of kProcess
                if (rc.status == catena::StatusCode::OK) {
                    status_ = CallStatus::kFinish;
                    responder_.Finish(catena::Empty{}, Status::OK, this);
                } else { // Error, end process.
                    errorStatus_ = Status(static_cast<grpc::StatusCode>(rc.status), rc.what());
                    status_ = CallStatus::kFinish;
                    responder_.Finish(::catena::Empty{}, errorStatus_, this);
                }
            } catch (...) { // Error, end process.
                errorStatus_ = Status(grpc::StatusCode::INTERNAL, "unknown error");
                status_ = CallStatus::kFinish;
                responder_.Finish(::catena::Empty{}, errorStatus_, this);
            }
            break;
        /**
         * kFinish: Final step of gRPC is the deregister the item from
         * CatenaServiceImpl.
         */
        case CallStatus::kFinish:
            std::cout << typeName << "[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
        // default: Error, end process.
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            responder_.FinishWithError(errorStatus, this);
    }
}
