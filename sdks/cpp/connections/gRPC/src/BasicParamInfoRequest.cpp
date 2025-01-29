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
#include <BasicParamInfoRequest.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>
#include <filesystem>

int CatenaServiceImpl::BasicParamInfoRequest::objectCounter_ = 0;

CatenaServiceImpl::BasicParamInfoRequest::BasicParamInfoRequest(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::BasicParamInfoRequest::proceed(CatenaServiceImpl *service, bool ok) {
    if (!service || status_ == CallStatus::kFinish) {
        return;
    }

    std::cout << "BasicParamInfoRequest proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
              << std::endl;

    if(!ok) {
        std::cout << "BasicParamInfoRequest[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
        service->deregisterItem(this);
        return;
    }

    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestBasicParamInfoRequest(&context_, &req_, &writer_, 
                        service_->cq_, service_->cq_, this);
            break;


        case CallStatus::kProcess:
            new BasicParamInfoRequest(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            status_ = CallStatus::kWrite;
            //break; // No break so it is allowed to fall through to the write case 

        case CallStatus::kWrite:
            try {
                std::unique_ptr<IParam> param;
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                catena::common::Authorizer authz = std::vector<std::string>{};  

                // Get the parameter with or without authorization
                if (service->authorizationEnabled()) {
                    std::vector<std::string> scopes = service->getScopes(context_);
                    authz = catena::common::Authorizer{scopes};  
                    Device::LockGuard lg(dm_);
                    param = dm_.getParam(req_.oid_prefix(), rc, authz);
                } else {
                    Device::LockGuard lg(dm_);
                    param = dm_.getParam(req_.oid_prefix(), rc);
                }

                if (rc.status == catena::StatusCode::OK && param) {
                    std::vector<catena::BasicParamInfoResponse> responses;
                    
                    // Add response for the requested parameter
                    catena::BasicParamInfoResponse response;
                    auto* info = response.mutable_info();
                    info->set_oid(req_.oid_prefix());
                    info->set_type(param->type().value());
                    if (auto template_oid = param->getTemplateOid()) { //Does not work...
                        info->set_template_oid(*template_oid);
                    }

                    //Param has a getConstraint function, but there is no setter in info!!!
                    
                    responses.push_back(response);

                    // Get child responses if recursive
                    if (req_.recursive()) {
                        catena::common::Path child_path("");
                        catena::exception_with_status child_status{"", catena::StatusCode::OK};
                        
                        std::function<void(const std::unique_ptr<IParam>&, const std::string&)> getChildren = 
                            [&](const std::unique_ptr<IParam>& current_param, const std::string& current_path) {
                                if (auto child_param = current_param->getParam(child_path, 
                                        service->authorizationEnabled() ? authz : catena::common::Authorizer::kAuthzDisabled, 
                                        child_status)) {
                                    catena::BasicParamInfoResponse child_response;
                                    auto* child_info = child_response.mutable_info();
                                    child_info->set_oid(current_path + "/" + child_path.front_as_string());
                                    child_info->set_type(child_param->type().value());
                                    if (auto template_oid = child_param->getTemplateOid()) {
                                        child_info->set_template_oid(*template_oid);
                                    }
                                    responses.push_back(child_response);
                                    getChildren(child_param, child_info->oid());
                                }
                            };
                        
                        getChildren(param, req_.oid_prefix());
                    }

                    // Write all responses
                    for (const auto& resp : responses) {
                        writer_.Write(resp, this);
                    }

                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::OK, this);
                    break;
                }
                else {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                    break;
                }

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
            std::cout << "[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;
    }
}