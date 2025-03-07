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
    if (!service)
        return;

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
            
            try {
                std::unique_ptr<IParam> param;
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                catena::common::Authorizer authz = std::vector<std::string>{};

                // Mode 1: Get all top-level parameters
                if (req_.oid_prefix().empty() && !req_.recursive()) {
                    std::vector<std::unique_ptr<IParam>> top_level_params;

                    if (service->authorizationEnabled()) {
                        Device::LockGuard lg(dm_);

                        /** TODO: This copy needs to be done in the constructor of Authorizer */
                        std::vector<std::string> scopes = std::move(service->getScopes(context_));

                        authz = catena::common::Authorizer{scopes};
                        top_level_params = dm_.getTopLevelParams(rc, authz);
                    } else {
                        Device::LockGuard lg(dm_);
                        top_level_params = dm_.getTopLevelParams(rc);
                    }

                    if (rc.status == catena::StatusCode::OK && !top_level_params.empty()) {
                        Device::LockGuard lg(dm_);
                        
                        responses_.clear();  
                        // Process each top-level parameter
                        for (auto& top_level_param : top_level_params) {
                            // Create a path for the parameter and check if it's an array type
                            Path top_level_path{top_level_param->getOid()};
                            
                            // Add the parameter to our response list
                            responses_.emplace_back();
                            if (service->authorizationEnabled()) {
                                top_level_param->toProto(responses_.back(), authz);
                            } else {
                                top_level_param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                            }

                            // For array types, calculate and update array length
                            if (top_level_param->isArrayType()) {
                                uint32_t array_length = top_level_param->size();
                                if (array_length > 0) {
                                    updateArrayLengths(top_level_param->getOid(), array_length);
                                }
                            }   
                        }
                        
                        // Begin writing responses back to the client
                        writer_lock_.lock();
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[0], this);  //Write the first response
                        writer_lock_.unlock();
                        break;  
                    }

                // Mode 2: Get a specific parameter and its children
                } else if (!req_.oid_prefix().empty()) { 
                    if (service->authorizationEnabled()) {
                        Device::LockGuard lg(dm_);

                        /** TODO: This copy needs to be done in the constructor of Authorizer */
                        std::vector<std::string> scopes = std::move(service->getScopes(context_));

                        authz = catena::common::Authorizer{scopes};
                        param = dm_.getParam(req_.oid_prefix(), rc, authz);
                    } else {
                        Device::LockGuard lg(dm_);
                        param = dm_.getParam(req_.oid_prefix(), rc);
                    }

                    if (rc.status == catena::StatusCode::OK && param) {
                        // Add the main parameter to the response list
                        responses_.emplace_back();
                        if (service->authorizationEnabled()) {
                            param->toProto(responses_.back(), authz);
                        } else {
                            param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                        }

                        // Calculate and update array length if this parameter is an array
                        if (param->isArrayType()) {
                            uint32_t array_length = param->size();
                            if (array_length > 0) {
                                updateArrayLengths(param->getOid(), array_length);
                            }
                        }   

                        // If recursive flag is set, get all children of this parameter
                        if (req_.recursive()) {
                            getChildren(param.get(), req_.oid_prefix(), authz);
                        }

                        // Begin writing responses back to the client
                        writer_lock_.lock();
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[0], this); //Write the first response
                        writer_lock_.unlock();
                        break;
                    } else {
                        throw catena::exception_with_status(rc.what(), rc.status);
                    }
                }

            } catch (const catena::exception_with_status& e) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed to process request: " + std::string(e.what()) + 
                    " (Status: " + std::to_string(static_cast<int>(e.status)) + ")");
                writer_.Finish(errorStatus, this);
                break;
            } catch (...) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed due to unknown error in BasicParamInfoRequest");
                writer_.Finish(errorStatus, this);
                break;
            }

        case CallStatus::kWrite:
            try {
                // Validate that we have responses to write
                if (current_response_ >= responses_.size() || responses_.empty()) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status(grpc::StatusCode::INTERNAL, "No more responses"), this);
                    break;
                }

                // Ensure the current response is valid
                if (!responses_[current_response_].has_info()) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Invalid response"), this);
                    break;
                }

                // Write responses sequentially until we're done
                if (current_response_ < responses_.size()) {
                    writer_lock_.lock(); //Lock the writer for writing
                    if (current_response_ >= responses_.size()-1) {
                        status_ = CallStatus::kFinish; 
                        writer_.Finish(Status::OK, this);
                    } else {
                        current_response_++;
                        writer_.Write(responses_.at(current_response_), this);
                    }
                    writer_lock_.unlock();
                    break;
                }        
            } catch (const std::exception& e) {
                status_ = CallStatus::kFinish;
                writer_.Finish(grpc::Status(grpc::StatusCode::INTERNAL, 
                    "Error writing response: " + std::string(e.what())), this);
            }
            break;
            
        case CallStatus::kFinish:
            std::cout << "[" << objectId_ << "] finished with status: " 
                      << (context_.IsCancelled() ? "CANCELLED" : "OK") << "\n";
            service->deregisterItem(this);
            break;

        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}



/** Updates the array_length field in the protobuf responses
 * for all responses that contain array_name in their OID
 */
void CatenaServiceImpl::BasicParamInfoRequest::updateArrayLengths(const std::string& array_name, uint32_t length) {
    if (length > 0) {
        for (auto it = responses_.rbegin(); it != responses_.rend(); ++it) {
            // Only update if the OID exactly matches the array name
            if (it->info().oid() == array_name) {
                it->set_array_length(length);
            }
        }
    }
}

/**
 * Recursively gets all children of a parameter
 */
void CatenaServiceImpl::BasicParamInfoRequest::getChildren(IParam* current_param, const std::string& current_path, catena::common::Authorizer& authz) {
    const auto& descriptor = current_param->getDescriptor();
    catena::exception_with_status rc{"", catena::StatusCode::OK};

    // Lambda function to process children of a parameter
    auto processChildren = [&](const std::string& parent_path) {
        for (const auto& [child_name, child_desc] : descriptor.getAllSubParams()) {
            if (child_name.empty() || child_name[0] == '/') continue;
            
            rc = catena::exception_with_status{"", catena::StatusCode::OK};
            Path child_path{parent_path, child_name};
            auto sub_param = dm_.getParam(child_path.toString(), rc);
            
            if (rc.status == catena::StatusCode::OK && sub_param) {
                // Add all parameters (array or not) when found as children
                responses_.emplace_back();
                if (service_->authorizationEnabled()) {
                    Device::LockGuard lg(dm_);
                    sub_param->toProto(responses_.back(), authz);
                } else {
                    Device::LockGuard lg(dm_);
                    sub_param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                }

                // If this child is an array, calculate and update its length
                if (sub_param->isArrayType()) {
                    uint32_t array_length = sub_param->size();
                    if (array_length > 0) {
                        updateArrayLengths(sub_param->getOid(), array_length);
                    }
                    
                    // Process array children if we're not already inside an array element
                    if (!Path{parent_path}.back_is_index()) {
                        getChildren(sub_param.get(), child_path.toString(), authz);
                    }
                }
            }
        }
    };

    // Check if current parameter is an array type
    if (current_param->isArrayType()) {
        uint32_t array_length = current_param->size();
        
        // Process each array element's children
        for (uint32_t i = 0; i < array_length; i++) {
            Path indexed_path{current_path, i};
            auto indexed_param = dm_.getParam(indexed_path.toString(), rc);
            if (!indexed_param) continue;

            // Process children of this array element
            processChildren(indexed_path.toString());
        }

        // Update the array length for the array itself
        if (array_length > 0) {
            updateArrayLengths(current_param->getOid(), array_length);
        }
    } else {
        // For non-array types, process children normally
        processChildren(current_path);
    }
}