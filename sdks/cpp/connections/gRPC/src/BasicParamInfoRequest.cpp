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

                if (!req_.oid_prefix().empty()) {
                    // Get parameter and collect responses
                    if (service->authorizationEnabled()) {
                        std::vector<std::string> scopes = service->getScopes(context_);
                        authz = catena::common::Authorizer{scopes};
                        param = dm_.getParam(req_.oid_prefix(), rc, authz);
                    } else {
                        param = dm_.getParam(req_.oid_prefix(), rc);
                        std::cout << "current path: " << req_.oid_prefix() << std::endl;
                    }
                
                    max_index_ = 0;

                    if (rc.status == catena::StatusCode::OK && param) {

                        // This is used to help find the length of the parent array
                        if (param->type().value() == catena::ParamType::STRUCT_ARRAY) {
                            for (size_t i = 0; ; i++) {
                                std::string array_path = req_.oid_prefix() + "/" + std::to_string(i);
                                auto check_param = dm_.getParam(array_path, rc);
                                if (!check_param) break;
                                max_index_ = i + 1;
                            }
                        }

                        // Add main response
                        responses_.emplace_back();
                        if (service->authorizationEnabled()) {
                            param->toProto(responses_.back(), authz);
                        } else {
                            param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                        }

                        // Update array length if this is an array
                        if (max_index_ > 0) {
                            updateArrayLengths(responses_.back().info().oid(), max_index_);
                        }

                        // Collect child responses if recursive
                        if (req_.recursive()) {
                            getChildren(param.get(), responses_.back().info().oid());
                        }

                        // Start writing responses
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[current_response_], this);
                    }
                }
                else if (req_.oid_prefix().empty() && !req_.recursive()) { //Collect top-level params
                    std::cout << "Collecting top-level params" << std::endl;

                    std::vector<std::unique_ptr<IParam>> top_level_params;

                    if (service->authorizationEnabled()) {
                        std::vector<std::string> scopes = service->getScopes(context_);
                        authz = catena::common::Authorizer{scopes};
                        top_level_params = dm_.getTopLevelParams(rc, authz);
                    }
                    else {
                        top_level_params = dm_.getTopLevelParams(rc);
                    }
                    
                    max_index_ = 0;

                    if (rc.status == catena::StatusCode::OK) {
                    
                        // Check each top level param for arrays
                        for (auto& top_level_param : top_level_params) {
                            responses_.emplace_back();
                            if (top_level_param->type().value() == catena::ParamType::STRUCT_ARRAY) {
                                std::string path = "/" + top_level_param->getDescriptor().getOid();
                                for (size_t i = 0; ; i++) {
                                    std::string array_path = path + "/" + std::to_string(i);
                                    auto check_param = dm_.getParam(array_path, rc);
                                    if (!check_param) break;
                                    max_index_ = i + 1;
                                }
                            }
                            
                            // Add param to response
                            if (service->authorizationEnabled()) {
                                top_level_param->toProto(responses_.back(), authz);
                            } else {
                                top_level_param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                            }

                            if (max_index_ > 0) {
                                updateArrayLengths(responses_.back().info().oid(), max_index_);
                            }
                        }

                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[current_response_], this);
                    }

                }
            } catch (...) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
            }
            break;

        case CallStatus::kWrite:
            if (current_response_ < responses_.size() - 1) {
                current_response_++;
            } else {
                // All responses written
                status_ = CallStatus::kPostWrite;
            }
            writer_.Write(responses_[current_response_], this);
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
            max_index_ = 0;

            // Handle array type parameters
            for (size_t i = 0; ; i++) {
                std::string array_path = child_path + "/" + std::to_string(i) + "/" + child_name;
                std::cout << "Currently looking at array element: " << array_path << std::endl;
                auto param = dm_.getParam(array_path, rc);

                if (!param) break;
                max_index_ = i + 1;
                
                responses_.emplace_back();
                param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                getChildren(param.get(), array_path);
            }

            updateArrayLengths(child_name, max_index_);
        } 
        
        else {
            max_index_ = 0;
            // For non-array parameters, still check for array indices
            for (size_t i = 0; ; i++) {
                std::string indexed_path = child_path + "/" + std::to_string(i) + "/" + child_name;
                std::cout << "Currently looking at indexed param: " << indexed_path << std::endl;
                auto param = dm_.getParam(indexed_path, rc);
                if (!param) break;
                max_index_ = i + 1;

                responses_.emplace_back();
                param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                getChildren(param.get(), indexed_path);
            }

            updateArrayLengths(child_name, max_index_);
        }

    }
}

void CatenaServiceImpl::BasicParamInfoRequest::updateArrayLengths(const std::string& array_name, size_t length) {
    if (length > 0) {
        for (auto it = responses_.rbegin(); it != responses_.rend(); ++it) {
            if (it->info().oid().find(array_name) != std::string::npos) {
                it->set_array_length(length);
            }
        }
    }
}

