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
#include <controllers/ParamInfoRequest.h>
using catena::REST::ParamInfoRequest;

// common
#include <Tags.h>
#include <Authorization.h>
#include <ParamVisitor.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;
using catena::common::ParamVisitor;
using catena::common::IParam;
using catena::common::Authorizer;
using catena::common::IDevice;

// Initializes the object counter for ParamInfoRequest to 0.
int ParamInfoRequest::objectCounter_ = 0;

ParamInfoRequest::ParamInfoRequest(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm},
    rc_("", catena::StatusCode::OK), recursive_{false} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
    
    // Get recursive from query parameters - presence alone means true
    recursive_ = context_.hasField("recursive");
}

void ParamInfoRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    try {
        std::unique_ptr<IParam> param;
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        std::shared_ptr<Authorizer> sharedAuthz;
        Authorizer* authz;
        
        // Handle authorization setup
        try {
            if (context_.authorizationEnabled()) {
                sharedAuthz = std::make_shared<Authorizer>(context_.jwsToken());
                authz = sharedAuthz.get();
            } else {
                authz = &Authorizer::kAuthzDisabled;
            }
        } catch (const catena::exception_with_status& err) {
            rc_ = catena::exception_with_status(err.what(), err.status);
        } catch (const std::exception& e) {
            rc_ = catena::exception_with_status(std::string("Authorization setup failed: ") + e.what(), catena::StatusCode::UNAUTHENTICATED);
        }

        if (rc_.status == catena::StatusCode::OK) {
            // Mode 1: Get all top-level parameters without recursion
            if (context_.fqoid().empty() && !recursive_) {
                std::vector<std::unique_ptr<IParam>> top_level_params;

                {
                    std::lock_guard lg(dm_.mutex());
                    top_level_params = dm_.getTopLevelParams(rc, *authz);
                }

                if (rc.status != catena::StatusCode::OK) {
                    rc_ = catena::exception_with_status(rc.what(), rc.status);
                } else if (top_level_params.empty()) {
                    rc_ = catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);
                } else {
                    std::lock_guard lg(dm_.mutex());
                    responses_.clear();
                    // Process each top-level parameter
                    for (auto& top_level_param : top_level_params) {
                        try {
                            // Add the parameter to our response list
                            addParamToResponses_(top_level_param.get(), *authz);
                            // For array types, calculate and update array length
                            if (top_level_param->isArrayType()) {
                                uint32_t array_length = top_level_param->size();
                                if (array_length > 0) {
                                    updateArrayLengths_(top_level_param->getOid(), array_length);
                                }
                            }
                        } catch (const catena::exception_with_status& err) {
                            rc_ = catena::exception_with_status(err.what(), err.status);
                            break;
                        }
                    }
                }
            }
            // Mode 2: Get all top-level parameters with recursion
            else if (context_.fqoid().empty() && recursive_) {
                std::vector<std::unique_ptr<IParam>> top_level_params;

                {
                    std::lock_guard lg(dm_.mutex());
                    top_level_params = dm_.getTopLevelParams(rc, *authz);
                }

                if (rc.status != catena::StatusCode::OK) {
                    rc_ = catena::exception_with_status(rc.what(), rc.status);
                } else if (top_level_params.empty()) {
                    rc_ = catena::exception_with_status("No top-level parameters found", catena::StatusCode::NOT_FOUND);
                } else {
                    std::lock_guard lg(dm_.mutex());
                    responses_.clear();
                    // Process each top-level parameter recursively
                    for (auto& top_level_param : top_level_params) {
                        try {
                            // Add the parameter to our response list
                            addParamToResponses_(top_level_param.get(), *authz);
                            // For array types, calculate and update array length
                            if (top_level_param->isArrayType()) {
                                uint32_t array_length = top_level_param->size();
                                if (array_length > 0) {
                                    updateArrayLengths_(top_level_param->getOid(), array_length);
                                }
                            }
                            // Recursively traverse all children of the top-level parameter
                            ParamInfoVisitor visitor(dm_, *authz, responses_, *this);
                            ParamVisitor::traverseParams(top_level_param.get(), "/" + top_level_param->getOid(), dm_, visitor);
                        } catch (const catena::exception_with_status& err) {
                            rc_ = catena::exception_with_status(err.what(), err.status);
                            break;
                        }
                    }
                }
            }
            // Mode 3: Get a specific parameter and its children
            else if (!context_.fqoid().empty()) { 
                {
                    std::lock_guard lg(dm_.mutex());
                    param = dm_.getParam(context_.fqoid(), rc, *authz);
                }

                if (rc.status != catena::StatusCode::OK) {
                    rc_ = catena::exception_with_status(rc.what(), rc.status);
                } else if (!param) {
                    rc_ = catena::exception_with_status("Parameter not found: " + context_.fqoid(), catena::StatusCode::NOT_FOUND);
                } else {
                    responses_.clear();
                    
                    try {
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
                        if (recursive_) {
                            ParamInfoVisitor visitor(dm_, *authz, responses_, *this);
                            ParamVisitor::traverseParams(param.get(), context_.fqoid(), dm_, visitor);
                        }
                    } catch (const catena::exception_with_status& err) {
                        rc_ = catena::exception_with_status(err.what(), err.status);
                    }
                }
            }

        }
    } catch (const catena::exception_with_status& err) {
        rc_ = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& e) {
        rc_ = catena::exception_with_status(std::string("Unknown error in ParamInfoRequest: ") + e.what(), catena::StatusCode::UNKNOWN);
    } catch (...) {
        rc_ = catena::exception_with_status("Unknown error in ParamInfoRequest", catena::StatusCode::UNKNOWN);
    }
}

void ParamInfoRequest::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "ParamInfoRequest[" << objectId_ << "] finished\n";
    
    if (responses_.empty()) {
        // If no responses, send at least one response with the error status
        writer_.sendResponse(rc_);
    } else {
        for (auto& response : responses_) {
            writer_.sendResponse(rc_, response);
        }
    }
    
    socket_.close();
}

// Helper method to add a parameter to the responses
void ParamInfoRequest::addParamToResponses_(IParam* param, catena::common::Authorizer& authz) {
    responses_.emplace_back();
    auto& response = responses_.back();
    response.mutable_info();
    param->toProto(response, authz);
}

// Updates the array_length field in the protobuf responses
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

// Visits a parameter and adds it to the response vector
void ParamInfoRequest::ParamInfoVisitor::visit(IParam* param, const std::string& path) {
    // Only add non-array parameters that aren't the top-most parameter
    bool isTopParameter = path == request_.context_.fqoid() || path == "/" + param->getOid();
    bool isArray = param->isArrayType();
    if (isTopParameter || isArray) {
        return;
    }

    request_.addParamToResponses_(param, authz_);
}

// Visits an array and updates the array length information
void ParamInfoRequest::ParamInfoVisitor::visitArray(IParam* param, const std::string& path, uint32_t length) {
    // Only add array parameters that aren't the top-most parameter
    bool isTopParameter = path == request_.context_.fqoid() || path == "/" + param->getOid();
    if (isTopParameter) {
        return;
    }

    request_.addParamToResponses_(param, authz_);

    // Update array length information
    if (length > 0) {
        request_.updateArrayLengths_(param->getOid(), length);
    }
} 
