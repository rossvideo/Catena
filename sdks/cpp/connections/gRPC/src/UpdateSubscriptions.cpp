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
#include <SubscriptionManager.h>

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

CatenaServiceImpl::UpdateSubscriptions::UpdateSubscriptions(CatenaServiceImpl *service, Device &dm, catena::grpc::SubscriptionManager& subscriptionManager, bool ok)
    : service_{service}, dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish},
        subscriptionManager_{subscriptionManager} {
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
            new UpdateSubscriptions(service_, dm_, subscriptionManager_, ok);
            context_.AsyncNotifyWhenDone(this);
            
            try {
                // Process the subscription updates
                std::cout << "Processing subscription updates for slot: " << req_.slot() << std::endl;
                
                catena::common::Authorizer* authz;
                std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                if (service->authorizationEnabled()) {
                    sharedAuthz = std::make_shared<catena::common::Authorizer>(getJWSToken());
                    authz = sharedAuthz.get();
                } else {
                    authz = &catena::common::Authorizer::kAuthzDisabled;
                }
                
                // Clear the responses vector to prepare for new responses
                responses_.clear();
                current_response_ = 0;
                
                // Process added OIDs
                for (const auto& oid : req_.added_oids()) {
                    std::cout << "Adding subscription for OID: " << oid << std::endl;
                    
                    try {
                        processSubscription(oid, *authz);
                    } catch (const std::exception& e) {
                        std::cout << "Error processing OID " << oid << ": " << e.what() << std::endl;
                    }
                }
                
                // Process removed OIDs
                for (const auto& oid : req_.removed_oids()) {
                    std::cout << "Removing subscription for OID: " << oid << std::endl;
                    subscriptionManager_.removeSubscription(oid);

                }
                
                // Always send the current set of subscribed parameters
                sendSubscribedParameters(*authz);
                
                // If we have responses to write, start writing them
                if (!responses_.empty()) {
                    writer_lock_.lock();
                    status_ = CallStatus::kWrite;
                    writer_.Write(responses_[0], this);
                    writer_lock_.unlock();
                } else {
                    // No responses to write, finish the RPC
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::OK, this);
                }
                
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

        case CallStatus::kWrite:
            try {
                // Validate that we have responses to write
                if (current_response_ >= responses_.size() || responses_.empty()) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status::OK, this);
                    break;
                }

                // Write responses sequentially until we're done
                writer_lock_.lock();
                if (current_response_ >= responses_.size() - 1) {
                    status_ = CallStatus::kFinish; 
                    writer_.Finish(Status::OK, this);
                } else {
                    current_response_++;
                    writer_.Write(responses_.at(current_response_), this);
                }
                writer_lock_.unlock();
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

void CatenaServiceImpl::UpdateSubscriptions::sendSubscribedParameters(catena::common::Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    
    auto& subscriptionManager = service_->subscriptionManager_;
    
    // First, handle Unique subscriptions
    for (const auto& oid : subscriptionManager.getUniqueSubscriptions()) {
        std::unique_ptr<IParam> param;
        
        {
            Device::LockGuard lg(dm_);
            param = dm_.getParam(oid, rc);
        }
        
        if (param) {
            catena::DeviceComponent_ComponentParam response;
            response.set_oid(oid);  // Set the OID in the response
            param->toProto(*response.mutable_param(), authz);
            responses_.push_back(response);
        }
    }
    
    // Then, handle wildcard subscriptions
    for (const auto& baseOid : subscriptionManager.getWildcardSubscriptions()) {
        std::vector<std::unique_ptr<IParam>> params;
        
        {
            Device::LockGuard lg(dm_);
            params = dm_.getTopLevelParams(rc);
        }
        
        for (auto& param : params) {
            std::string paramOid = param->getOid();
            if (paramOid.find(baseOid) == 0) {
                catena::DeviceComponent_ComponentParam response;
                response.set_oid(paramOid); 
                param->toProto(*response.mutable_param(), authz);
                responses_.push_back(response);
            }
        }
    }
}

void CatenaServiceImpl::UpdateSubscriptions::processSubscription(const std::string& oid, catena::common::Authorizer& authz) {
    std::cout << "processSubscription called with OID: " << oid << std::endl;
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    
    // Check if this is a wildcard subscription (ends with *)
    bool isWildcard = !oid.empty() && oid.back() == '*';
    std::cout << "isWildcard check: " << std::boolalpha << isWildcard << std::endl;
    std::string baseOid = isWildcard ? oid.substr(0, oid.length() - 1) : oid;
    // Remove trailing slash if present
    if (!baseOid.empty() && baseOid.back() == '/') {
        baseOid = baseOid.substr(0, baseOid.length() - 1);
    }
    std::cout << "baseOid: " << baseOid << std::endl;
    
    if (isWildcard) {
        std::cout << "Processing wildcard subscription" << std::endl;
        
        // Try to get the parameter directly first
        std::unique_ptr<IParam> param;
        {
            Device::LockGuard lg(dm_);
            param = dm_.getParam(baseOid, rc);
        }
        
        if (param) {
            // If we found the parameter, add the subscription
            std::cout << "Found matching parameter: " << baseOid << std::endl;
            subscriptionManager_.addSubscription(oid);
        } else {
            // If not found directly, try to find any parameters that start with this base OID
            std::vector<std::unique_ptr<IParam>> allParams;
            {
                Device::LockGuard lg(dm_);
                allParams = dm_.getTopLevelParams(rc);
            }
            
            bool foundMatch = false;
            for (auto& p : allParams) {
                std::string paramOid = p->getOid();
                std::cout << "Checking paramOid: " << paramOid << " against baseOid: " << baseOid << std::endl;
                if (paramOid.find(baseOid) == 0) {
                    foundMatch = true;
                    break;
                }
            }
            
            if (foundMatch) {
                std::cout << "Adding wildcard subscription: " << oid << std::endl;
                subscriptionManager_.addSubscription(oid);
            } else {
                std::cout << "No parameters found matching wildcard pattern: " << oid << std::endl;
            }
        }
    } else {
        std::cout << "Processing unique subscription" << std::endl;
        // Handle unique subscription
        std::unique_ptr<IParam> param;
        
        // Get the parameter
        {
            Device::LockGuard lg(dm_);
            param = dm_.getParam(baseOid, rc);
        }
        
        if (param) {
            std::cout << "Adding unique subscription: " << baseOid << std::endl;
            subscriptionManager_.addSubscription(baseOid);
        } else {
            std::cout << "Parameter not found for OID: " << baseOid << std::endl;
        }
    }
}