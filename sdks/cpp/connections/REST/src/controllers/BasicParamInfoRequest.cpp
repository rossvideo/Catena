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

// connections/REST
#include <controllers/BasicParamInfoRequest.h>
using catena::REST::BasicParamInfoRequest;

// common
#include <Tags.h>
#include <ParamVisitor.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;
using catena::common::ParamVisitor;
using catena::common::IParam;
using catena::common::Authorizer;

// Initializes the object counter for BasicParamInfoRequest to 0.
int BasicParamInfoRequest::objectCounter_ = 0;

BasicParamInfoRequest::BasicParamInfoRequest(tcp::socket& socket, SocketReader& context, catena::common::Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm}, ok_{true}, recursive_{false} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
    
    // Parsing fields and assigning to respective variables.
    try {
        // Handle URL-encoded empty values
        std::string oid_prefix_value = context.fields("oid_prefix");
        if (oid_prefix_value == "%7B%7D" || oid_prefix_value == "%7Boid_prefix%7D" || oid_prefix_value.empty()) {
            oid_prefix_ = "";
        } else {
            oid_prefix_ = "/" + oid_prefix_value;
        }
        recursive_ = context.fields("recursive") == "true";
    // Parse error
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.write(err);
        ok_ = false;
    }
}

void BasicParamInfoRequest::proceed() {
    writeConsole(CallStatus::kCreate, ok_);
    if (!ok_) {
        finish();
        return;
    }

    writeConsole(CallStatus::kProcess, ok_);
    if (!ok_) {
        finish();
        return;
    }

    try {
        std::unique_ptr<IParam> param;
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        std::shared_ptr<Authorizer> sharedAuthz;
        Authorizer* authz;
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &Authorizer::kAuthzDisabled;
        }

        // Mode 1: Get all top-level parameters without recursion
        if (oid_prefix_.empty() && !recursive_) {
            std::vector<std::unique_ptr<IParam>> top_level_params;

            {
                catena::common::Device::LockGuard lg(dm_);
                top_level_params = dm_.getTopLevelParams(rc, *authz);
            }

            if (rc.status == catena::StatusCode::OK && !top_level_params.empty()) {
                catena::common::Device::LockGuard lg(dm_);
                responses_.clear();
                // Process each top-level parameter
                for (auto& top_level_param : top_level_params) {
                    // Add the parameter to our response list
                    addParamToResponses(top_level_param.get(), *authz);
                    // For array types, calculate and update array length
                    if (top_level_param->isArrayType()) {
                        uint32_t array_length = top_level_param->size();
                        if (array_length > 0) {
                            updateArrayLengths(top_level_param->getOid(), array_length);
                        }
                    }
                }
                writer_lock_.lock();
                for (auto& response : responses_) {
                    writer_.write(response);
                }
                writer_lock_.unlock();
            }
        }
        // Mode 2: Get a specific parameter and its children
        else if (!oid_prefix_.empty()) { 
            {
                catena::common::Device::LockGuard lg(dm_);
                param = dm_.getParam(oid_prefix_, rc, *authz);
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
                        updateArrayLengths(param->getOid(), array_length);
                    }
                }
                
                // If recursive is true, collect all parameter info recursively through visitor pattern
                if (recursive_) {
                    BasicParamInfoVisitor visitor(dm_, *authz, responses_, *this);
                    ParamVisitor::traverseParams(param.get(), oid_prefix_, dm_, visitor);
                }

                // Write all responses to the client
                writer_lock_.lock();
                for (auto& response : responses_) {
                    writer_.write(response);
                }
                writer_lock_.unlock();
            } else {
                throw catena::exception_with_status(rc.what(), rc.status);
            }
        }
        // Mode 3: Get all top-level parameters with recursion
        else if (oid_prefix_.empty() && recursive_) {
            std::vector<std::unique_ptr<IParam>> top_level_params;

            {
                catena::common::Device::LockGuard lg(dm_);
                top_level_params = dm_.getTopLevelParams(rc, *authz);
            }

            if (rc.status == catena::StatusCode::OK && !top_level_params.empty()) {
                catena::common::Device::LockGuard lg(dm_);
                responses_.clear();
                // Process each top-level parameter recursively
                for (auto& top_level_param : top_level_params) {
                    // Add the parameter to our response list
                    addParamToResponses(top_level_param.get(), *authz);
                    // For array types, calculate and update array length
                    if (top_level_param->isArrayType()) {
                        uint32_t array_length = top_level_param->size();
                        if (array_length > 0) {
                            updateArrayLengths(top_level_param->getOid(), array_length);
                        }
                    }
                    // Recursively traverse all children of the top-level parameter
                    BasicParamInfoVisitor visitor(dm_, *authz, responses_, *this);
                    ParamVisitor::traverseParams(top_level_param.get(), "/" + top_level_param->getOid(), dm_, visitor);
                }
                writer_lock_.lock();
                for (auto& response : responses_) {
                    writer_.write(response);
                }
                writer_lock_.unlock();
            }
        }
    } catch (catena::exception_with_status& err) {
        writer_.write(err);
    } catch (...) {
        catena::exception_with_status err("Unknown error in BasicParamInfoRequest", catena::StatusCode::UNKNOWN);
        writer_.write(err);
    }
}

void BasicParamInfoRequest::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    writer_.finish();
    std::cout << "BasicParamInfoRequest[" << objectId_ << "] finished\n";
}

// Helper method to add a parameter to the responses
void BasicParamInfoRequest::addParamToResponses(IParam* param, catena::common::Authorizer& authz) {
    responses_.emplace_back();
    auto& response = responses_.back();
    response.mutable_info();
    param->toProto(response, authz);
}

// Updates the array_length field in the protobuf responses
void BasicParamInfoRequest::updateArrayLengths(const std::string& array_name, uint32_t length) {
    if (length > 0) {
        for (auto it = responses_.rbegin(); it != responses_.rend(); ++it) {
            // Only update if the OID exactly matches the array name
            if (it->info().oid() == array_name) {
                it->set_array_length(length);
            }
        }
    }
}

// Visits a parameter and adds it to the response vector
void BasicParamInfoRequest::BasicParamInfoVisitor::visit(IParam* param, const std::string& path) {
    // Only add non-array parameters that aren't the top-most parameter
    bool isTopParameter = path == request_.oid_prefix_ || path == "/" + param->getOid();
    bool isArray = param->isArrayType();
    if (isTopParameter || isArray) {
        return;
    }

    request_.addParamToResponses(param, authz_);
}

// Visits an array and updates the array length information
void BasicParamInfoRequest::BasicParamInfoVisitor::visitArray(IParam* param, const std::string& path, uint32_t length) {
    // Only add array parameters that aren't the top-most parameter
    bool isTopParameter = path == request_.oid_prefix_ || path == "/" + param->getOid();
    if (isTopParameter) {
        return;
    }

    request_.addParamToResponses(param, authz_);

    // Update array length information
    if (length > 0) {
        request_.updateArrayLengths(param->getOid(), length);
    }
} 