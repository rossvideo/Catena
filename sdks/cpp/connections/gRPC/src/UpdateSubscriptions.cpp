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
#include <UpdateSubscriptions.h>

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

int CatenaServiceImpl::UpdateSubscriptions::objectCounter_ = 0;

CatenaServiceImpl::UpdateSubscriptions::UpdateSubscriptions(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // start the process
}

void CatenaServiceImpl::UpdateSubscriptions::proceed(CatenaServiceImpl *service, bool ok) {
    if (!service)
        return;

    std::cout << "UpdateSubscriptions proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
              << std::endl;

    if(!ok) {
        std::cout << "UpdateSubscriptions[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
        service->deregisterItem(this);
        return;
    }

    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestUpdateSubscriptions(&context_, &req_, &writer_, 
                        service_->cq_, service_->cq_, this);
            break;

        case CallStatus::kProcess:
            new UpdateSubscriptions(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            
            try {
                // Process the subscription updates
                std::cout << "Processing subscription updates for slot: " << req_.slot() << std::endl;
                
                // Use a dummy authorizer for now
                catena::common::Authorizer authz(std::vector<std::string>{});
                
                // Process added OIDs
                for (const auto& oid : req_.added_oids()) {
                    std::cout << "Adding subscription for OID: " << oid << std::endl;
                    
                    try {
                        // Check if this is a wildcard subscription (ends with *)
                        bool isWildcard = !oid.empty() && oid.back() == '*';
                        std::string baseOid = isWildcard ? oid.substr(0, oid.length() - 1) : oid;
                        
                        if (isWildcard) {
                            processWildcardSubscription(baseOid, authz);
                        } else {
                            processExactSubscription(oid, authz);
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Error processing OID " << oid << ": " << e.what() << std::endl;
                    }
                }
                
                // Process removed OIDs
                for (const auto& oid : req_.removed_oids()) {
                    std::cout << "Removing subscription for OID: " << oid << std::endl;
                    
                    // Check if this is a wildcard subscription (ends with *)
                    bool isWildcard = !oid.empty() && oid.back() == '*';
                    std::string baseOid = isWildcard ? oid.substr(0, oid.length() - 1) : oid;
                    
                    if (isWildcard) {
                        wildcardSubscriptions_.erase(baseOid);
                    } else {
                        exactSubscriptions_.erase(oid);
                    }
                }
                
                // When done writing responses, finish
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::OK, this);
                
            } catch (const catena::exception_with_status& e) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed to process request: " + std::string(e.what()) + 
                    " (Status: " + std::to_string(static_cast<int>(e.status)) + ")");
                writer_.Finish(errorStatus, this);
            } catch (...) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed due to unknown error in UpdateSubscriptions");
                writer_.Finish(errorStatus, this);
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

void CatenaServiceImpl::UpdateSubscriptions::processWildcardSubscription(const std::string& baseOid, catena::common::Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    std::vector<std::unique_ptr<IParam>> params;
    
    // Get top-level params as a starting point
    {
        Device::LockGuard lg(dm_);
        params = dm_.getTopLevelParams(rc);
    }
    
    // Filter and process params that match the prefix
    for (auto& param : params) {
        std::string paramOid = param->getOid();
        if (paramOid.find(baseOid) == 0) {
            // Send this parameter to the client
            res_ = {};  // Clear previous response
            param->toProto(*res_.mutable_param(), catena::common::Authorizer::kAuthzDisabled);
            writer_.Write(res_, this);
        }
    }
    
    // Add to wildcard subscriptions
    wildcardSubscriptions_.insert(baseOid);
}

void CatenaServiceImpl::UpdateSubscriptions::processExactSubscription(const std::string& oid, catena::common::Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param;
    
    // Get the parameter
    {
        Device::LockGuard lg(dm_);
        param = dm_.getParam(oid, rc);
    }
    
    if (param) {
        // Send this parameter to the client
        res_ = {};  // Clear previous response
        param->toProto(*res_.mutable_param(), catena::common::Authorizer::kAuthzDisabled);
        writer_.Write(res_, this);
        
        // Add to exact subscriptions
        exactSubscriptions_.insert(oid);
    } else {
        std::cout << "Parameter not found for OID: " << oid << std::endl;
    }
}