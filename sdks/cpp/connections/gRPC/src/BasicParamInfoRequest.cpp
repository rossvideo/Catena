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
#include <map>

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
            
            try {
                std::unique_ptr<IParam> param;
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                catena::common::Authorizer authz = std::vector<std::string>{};  

                // Single lock for the entire operation
                Device::LockGuard lg(dm_);

                // Get parameter and collect responses
                if (service->authorizationEnabled()) {
                    std::vector<std::string> scopes = service->getScopes(context_);
                    authz = catena::common::Authorizer{scopes};
                    param = dm_.getParam(req_.oid_prefix(), rc, authz);
                } else {
                    param = dm_.getParam(req_.oid_prefix(), rc);
                    std::cout << "current path: " << req_.oid_prefix() << std::endl;
                }

                if (rc.status == catena::StatusCode::OK && param) {
                    // Add main response
                    responses_.emplace_back();
                    if (service->authorizationEnabled()) {
                        param->toProto(responses_.back(), authz);
                    } else {
                        param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                    }

                    // Collect child responses if recursive
                    if (req_.recursive()) {
                        getChildren(param.get(), responses_.back().info().oid());
                    }

                    // Start writing responses
                    status_ = CallStatus::kWrite;
                    writer_.Write(responses_[current_response_], this);
                }
            } catch (...) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
            }
            break;

        case CallStatus::kWrite:
            if (current_response_ < responses_.size() - 1) {
                // Write next response
                current_response_++;
                writer_.Write(responses_[current_response_], this);
            } else {
                // All responses written
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::OK, this);
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

void CatenaServiceImpl::BasicParamInfoRequest::getChildren(IParam* current_param, const std::string& current_path) {
    const auto& descriptor = current_param->getDescriptor();
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    
    for (const auto& [child_name, child_desc] : descriptor.getAllSubParams()) {
        if (child_name.empty() || child_name[0] == '/') continue;
        
        auto child_type = child_desc->type();
        std::string child_path = (current_path[0] == '/' ? current_path : "/" + current_path);
        
        if (child_type == catena::ParamType::STRUCT_ARRAY) {
            // Handle array type parameters
            for (size_t i = 0; ; i++) {
                std::string array_path = child_path + "/" + std::to_string(i) + "/" + child_name;
                std::cout << "Currently looking at array element: " << array_path << std::endl;
                auto param = dm_.getParam(array_path, rc);
                if (!param) {
                    if (i > 0) {
                        responses_.back().set_array_length(i);
                    }
                    break;
                }

                responses_.emplace_back();
                param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                getChildren(param.get(), array_path);
            }
        } else {
            // For non-array parameters, still check for array indices
            for (size_t i = 0; ; i++) {
                std::string indexed_path = child_path + "/" + std::to_string(i) + "/" + child_name;
                std::cout << "Currently looking at indexed param: " << indexed_path << std::endl;
                auto param = dm_.getParam(indexed_path, rc);
                if (!param) break;

                responses_.emplace_back();
                param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                getChildren(param.get(), indexed_path);
            }
        }
    }
}