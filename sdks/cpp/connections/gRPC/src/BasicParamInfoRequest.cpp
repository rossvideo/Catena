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

                //Gather all top-level params if prefix is empty and recursive is false
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

                    if (rc.status == catena::StatusCode::OK) {
                        Device::LockGuard lg(dm_);
                        responses_.clear();  
                        for (auto& top_level_param : top_level_params) {
                            responses_.emplace_back();
                            if (service->authorizationEnabled()) {
                                top_level_param->toProto(responses_.back(), authz);
                            } else {
                                top_level_param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                            }
                        }
                        
                        writer_lock_.lock();
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[0], this);  //Write the first response
                        writer_lock_.unlock();
                        break;  
                    }


                    //Get a parameter based on the given path
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
                        std::cout << "current path: " << req_.oid_prefix() << std::endl;
                    }


                    // if (param->getDescriptor().type() == catena::ParamType::STRUCT_VARIANT) {
                    //     std::cout << "param is a variant" << std::endl;
                    // }

                    max_index_ = 0;
 
                    if (rc.status == catena::StatusCode::OK && param) {
                        // Calculate array length if this is a struct array
                        if (isArrayType(param->type().value())) {
                            max_index_ = calculateArrayLength(req_.oid_prefix());
                        }else{
                            std::cout << "param is not an array type" << std::endl;
                        }

                        // Add main response
                        responses_.emplace_back();
                        if (service->authorizationEnabled()) {
                            param->toProto(responses_.back(), authz);
                        } else {
                            param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                        }

                        if (max_index_ > 0) {
                            updateArrayLengths(responses_.back().info().oid(), max_index_);
                        }   

                        if (req_.recursive()) {
                            getChildren(param.get(), req_.oid_prefix(), authz);
                        }

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
                // First, check for if responses exist
                if (current_response_ >= responses_.size() || responses_.empty()) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status(grpc::StatusCode::INTERNAL, "No more responses"), this);
                    break;
                }

                // Then validate the current response
                if (!responses_[current_response_].has_info()) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Invalid response"), this);
                    break;
                }

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


/** Finds how many array elements exist at a path
 *  e.g., if we have /device/0, /device/1, /device/2,
 *  returns 3
 */
uint32_t CatenaServiceImpl::BasicParamInfoRequest::calculateArrayLength(const std::string& base_path) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    uint32_t length = 0;
    
    // Keep checking indices until we find one that doesn't exist
    for (uint32_t i = 0; ; i++) {
        Path path{base_path, i};
        auto param = dm_.getParam(path.toString(), rc);
        if (!param) break;
        length = i + 1;
    }
    
    return length;
}


/** Updates the array_length field in the protobuf responses
 * for all responses that contain array_name in their OID
 */
void CatenaServiceImpl::BasicParamInfoRequest::updateArrayLengths(const std::string& array_name, uint32_t length) {
    if (length > 0) {
        for (auto it = responses_.rbegin(); it != responses_.rend(); ++it) {
            if (it->info().oid().find(array_name) != std::string::npos) {
                it->set_array_length(length);
            }
        }
    }
}


/** Recursively gets all children of a parameter */
void CatenaServiceImpl::BasicParamInfoRequest::getChildren(IParam* current_param, const std::string& current_path, catena::common::Authorizer& authz) {
    const auto& descriptor = current_param->getDescriptor();
    catena::exception_with_status rc{"", catena::StatusCode::OK};

    for (const auto& [child_name, child_desc] : descriptor.getAllSubParams()) {
    
         // Skip invalid child names (empty or absolute paths)
        if (child_name.empty() || child_name[0] == '/') continue;
        
        Path child_path{current_path, child_name};
        
        rc = catena::exception_with_status{"", catena::StatusCode::OK};
        
        auto sub_param = dm_.getParam(child_path.toString(), rc);        
        if (rc.status == catena::StatusCode::OK && sub_param) {
            if (isArrayType(sub_param->type().value())) {  
                /** TODO: ARRAY LOGIC*/
            }
            else {
                responses_.emplace_back();
                if (service_->authorizationEnabled()) {
                    Device::LockGuard lg(dm_);
                    sub_param->toProto(responses_.back(), authz);
                } else {
                    Device::LockGuard lg(dm_);
                    sub_param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                }                
                getChildren(sub_param.get(), child_path.toString(), authz);
            }
        }        
    }
}