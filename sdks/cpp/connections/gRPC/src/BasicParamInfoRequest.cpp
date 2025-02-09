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
    if (!service || status_ == CallStatus::kFinish) 
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
                        std::vector<std::string> scopes = std::move(service->getScopes(context_));
                        //TODO: This copy^ needs to be done in the constructor of Authorizer
                        authz = catena::common::Authorizer{scopes};
                        top_level_params = dm_.getTopLevelParams(rc, authz);
                    } else {
                        Device::LockGuard lg(dm_);
                        top_level_params = dm_.getTopLevelParams(rc);
                    }

                    if (rc.status == catena::StatusCode::OK) {
                        Device::LockGuard lg(dm_);
                        for (auto& top_level_param : top_level_params) {
                            responses_.emplace_back();
                            if (service->authorizationEnabled()) {
                                top_level_param->toProto(responses_.back(), authz);
                            } else {
                                top_level_param->toProto(responses_.back(), catena::common::Authorizer::kAuthzDisabled);
                            }
                        }
                           
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[current_response_], this);
                    }
                }
            } catch (const catena::exception_with_status& e) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed to process request: " + std::string(e.what()) + 
                    " (Status: " + std::to_string(static_cast<int>(e.status)) + ")");
                writer_.Finish(errorStatus, this);
                break;
            } catch (const std::exception& e) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Unexpected error: " + std::string(e.what()));
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
            if (context_.IsCancelled()) {
                Device::LockGuard lg(dm_);
                std::cout << "[" << objectId_ << "] cancelled during write\n";
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
                break;
            }

            if (current_response_ < responses_.size() - 1) {
                Device::LockGuard lg(dm_);
                status_ = CallStatus::kFinish;
                writer_.Write(responses_[++current_response_], this);
            } else {
                Device::LockGuard lg(dm_);
                std::cout << "BasicParamInfoRequest proceed[" << objectId_ << "] writing final response\n";
                status_ = CallStatus::kFinish; 
                writer_.Finish(Status::OK, this);
            }
            break;
            
        //It does not go to kFinish, and I'm not sure why...
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
