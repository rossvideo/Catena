/*
 * Copyright 2025 Ross Video Ltd
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

// connections/gRPC
#include <controllers/ParamInfoRequest.h>
#include <Logger.h>
using catena::gRPC::ParamInfoRequest;

int ParamInfoRequest::objectCounter_ = 0;

ParamInfoRequest::ParamInfoRequest(ICatenaServiceImpl *service, SlotMap& dms, bool ok)
    : CallData(service), dms_{dms}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(ok);  // start the process
}

void ParamInfoRequest::proceed(bool ok) {
    DEBUG_LOG << "ParamInfoRequest::proceed[" << objectId_ << "]: "
              << timeNow() << " status: " << static_cast<int>(status_)
              << ", ok: " << std::boolalpha << ok;

    // If the process is cancelled, finish the process
    if (!ok) {
        DEBUG_LOG << "ParamInfoRequest[" << objectId_ << "] cancelled";
        status_ = CallStatus::kFinish;
    }

    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestParamInfoRequest(&context_, &req_, &writer_, 
                        service_->cq(), service_->cq(), this);
            break;  

        case CallStatus::kProcess:

            new ParamInfoRequest(service_, dms_, ok);
            context_.AsyncNotifyWhenDone(this);
            
            try {
                std::unique_ptr<IParam> param;
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                std::shared_ptr<Authorizer> sharedAuthz;
                Authorizer* authz;
                IDevice* dm = nullptr;
                // Getting device at specified slot.
                if (dms_.contains(req_.slot())) {
                    dm = dms_.at(req_.slot());
                }
                // Making sure the device exists.
                if (!dm) {
                    throw catena::exception_with_status("device not found in slot " + std::to_string(req_.slot()), catena::StatusCode::NOT_FOUND);
                }
                if (service_->authorizationEnabled()) {
                    sharedAuthz = std::make_shared<Authorizer>(jwsToken_());
                    authz = sharedAuthz.get();
                } else {
                    authz = &Authorizer::kAuthzDisabled;
                }
                // Mode 1: Get all top-level parameters
                if (req_.oid_prefix().empty() && !req_.recursive()) {
                    std::vector<std::unique_ptr<IParam>> top_level_params;
                    {
                    std::lock_guard lg(dm->mutex());
                    top_level_params = dm->getTopLevelParams(rc, *authz);
                    }
                    if (rc.status == catena::StatusCode::OK && !top_level_params.empty()) {
                        std::lock_guard lg(dm->mutex());                    
                        responses_.clear();  
                        // Process each top-level parameter
                        for (auto& top_level_param : top_level_params) {
                            // Add the parameter to our response list
                            addParamToResponses(top_level_param.get(), *authz);
                            // For array types, calculate and update array length
                            if (top_level_param->isArrayType()) {
                                uint32_t array_length = top_level_param->size();
                                if (array_length > 0) {
                                    updateArrayLengths_(top_level_param->getOid(), array_length);
                                }
                            }   
                        }                        
                        // Begin writing responses back to the client
                        writer_lock_.lock();
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[0], this);  //Write the first response
                        writer_lock_.unlock();
                        break;  
                    } else if (rc.status != catena::StatusCode::OK) {
                        throw catena::exception_with_status(rc.what(), rc.status);
                    } else {
                        throw catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);
                    }

                // Mode 2: Get ALL parameters recursively
                } else if (req_.oid_prefix().empty() && req_.recursive()) {
                    std::vector<std::unique_ptr<IParam>> top_level_params;
                    {
                        std::lock_guard lg(dm->mutex());
                        top_level_params = dm->getTopLevelParams(rc, *authz);
                    }
                    if (rc.status == catena::StatusCode::OK && !top_level_params.empty()) {
                        std::lock_guard lg(dm->mutex());                     
                        responses_.clear();  
                        // Process each top-level parameter recursively
                        for (auto& top_level_param : top_level_params) {
                            // Add the parameter to our response list
                            addParamToResponses(top_level_param.get(), *authz);
                            // For array types, calculate and update array length
                            if (top_level_param->isArrayType()) {
                                uint32_t array_length = top_level_param->size();
                                if (array_length > 0) {
                                    updateArrayLengths_(top_level_param->getOid(), array_length);
                                }
                            }
                            // Collect all parameter info recursively through visitor pattern
                            ParamInfoVisitor visitor(*dm, *authz, responses_, *this);
                            ParamVisitor::traverseParams(top_level_param.get(), "/" + top_level_param->getOid(), *dm, visitor, *authz);
                        }                        
                        // Begin writing responses back to the client
                        writer_lock_.lock();
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[0], this);  //Write the first response
                        writer_lock_.unlock();
                        break;  
                    } else if (rc.status != catena::StatusCode::OK) {
                        throw catena::exception_with_status(rc.what(), rc.status);
                    } else {
                        throw catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);
                    }
                // Mode 3: Get a specific parameter and its children
                } else if (!req_.oid_prefix().empty()) { 
                    {
                    std::lock_guard lg(dm->mutex());
                    param = dm->getParam(req_.oid_prefix(), rc, *authz);
                    }
                    if (rc.status == catena::StatusCode::OK && param) {
                        responses_.clear();
                        // Add the main parameter first 
                        responses_.emplace_back();
                        param->toProto(responses_.back(), *authz);                        
                        // If the parameter is an array, update the array length
                        if (param->isArrayType()) {
                            uint32_t array_length = param->size();
                            if (array_length > 0) {
                                updateArrayLengths_(param->getOid(), array_length);
                            }
                        }                        
                        // If recursive is true, collect all parameter info recursively through visitor pattern
                        if (req_.recursive()) {
                            ParamInfoVisitor visitor(*dm, *authz, responses_, *this);
                            ParamVisitor::traverseParams(param.get(), req_.oid_prefix(), *dm, visitor, *authz);
                        }
                        // Begin writing responses back to the client
                        writer_lock_.lock();
                        status_ = CallStatus::kWrite;
                        writer_.Write(responses_[0], this); //Write the first response
                        writer_lock_.unlock();
                        break;
                    } else if (rc.status != catena::StatusCode::OK) {
                        throw catena::exception_with_status(rc.what(), rc.status);
                    } else {
                        throw catena::exception_with_status("Parameter not found: " + req_.oid_prefix(), catena::StatusCode::NOT_FOUND);
                    }
                }
            } catch (catena::exception_with_status& err) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(err.status), err.what());
                writer_.Finish(errorStatus, this);
                break;
            } catch (...) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed due to unknown error in ParamInfoRequest");
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
            DEBUG_LOG << "[" << objectId_ << "] finished with status: " 
                      << (context_.IsCancelled() ? "CANCELLED" : "OK");
            service_->deregisterItem(this);
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
void ParamInfoRequest::updateArrayLengths_(const std::string& array_name, uint32_t length) {
    if (length > 0) {
        for (auto it = responses_.rbegin(); it != responses_.rend(); ++it) {
            // Only update if the OID exactly matches the array name
            if (it->info().oid() == array_name) {
                it->set_array_length(length);
            }
        }
    }
}

// Helper method to add a parameter to the responses
void ParamInfoRequest::addParamToResponses(IParam* param, Authorizer& authz) {
    responses_.emplace_back();
    auto& response = responses_.back();
    response.mutable_info();
    param->toProto(response, authz);
}

// Visits a parameter and adds it to the response vector
void ParamInfoRequest::ParamInfoVisitor::visit(IParam* param, const std::string& path) {
    // Only add non-array parameters that aren't the top-most parameter
    bool isTopParameter = path == request_.req_.oid_prefix() || path == "/" + param->getOid();
    bool isArray = param->isArrayType();
    if (isTopParameter || isArray) {
        return;
    }

    request_.addParamToResponses(param, authz_);
}

// Visits an array and updates the array length information
void ParamInfoRequest::ParamInfoVisitor::visitArray(IParam* param, const std::string& path, uint32_t length) {
    // Only add array parameters that aren't the top-most parameter
    bool isTopParameter = path == request_.req_.oid_prefix() || path == "/" + param->getOid();
    if (isTopParameter) {
        return;
    }

    request_.addParamToResponses(param, authz_);

    // Update array length information
    if (length > 0) {
        request_.updateArrayLengths_(param->getOid(), length);
    }
}
