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
                
                // Clear any existing responses
                responses_.clear();
                current_response_ = 0;
                
                // Process removed OIDs first to ensure clean state
                for (const auto& oid : req_.removed_oids()) {
                    std::cout << "Removing subscription for OID: " << oid << std::endl;
                    if (subscriptionManager_.isWildcard(oid)) {
                        // For wildcard removals, we need to remove all matching subscriptions
                        std::string baseOid = oid.substr(0, oid.length() - 2); // Remove "/*"
                        auto allSubscribed = subscriptionManager_.getAllSubscribedOids(dm_);
                        for (const auto& subscribedOid : allSubscribed) {
                            // Remove if it starts with the base OID
                            if (subscribedOid.find(baseOid) == 0) {
                                subscriptionManager_.removeSubscription(subscribedOid);
                            }
                        }
                    } else {
                        subscriptionManager_.removeSubscription(oid);
                    }
                }
                
                // Process added OIDs
                for (const auto& oid : req_.added_oids()) {
                    std::cout << "Adding subscription for OID: " << oid << std::endl;
                    try {
                        processSubscription(oid, *authz);
                    } catch (const std::exception& e) {
                        std::cout << "Error processing OID " << oid << ": " << e.what() << std::endl;
                    }
                }
                
                // Now that all subscriptions are processed, send current values for all subscribed parameters
                sendSubscribedParameters(*authz);

                // If we have responses, start writing them
                if (!responses_.empty()) {
                    status_ = CallStatus::kWrite;
                    writer_.Write(responses_[current_response_], this);
                } else {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status::OK, this);
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
            if (!ok) {
                status_ = CallStatus::kFinish;
                writer_.Finish(grpc::Status::CANCELLED, this);
                break;
            }

            current_response_++;
            if (current_response_ < responses_.size()) {
                writer_.Write(responses_[current_response_], this);
            } else {
                status_ = CallStatus::kFinish;
                writer_.Finish(grpc::Status::OK, this);
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
    catena::exception_with_status rc{"", catena::StatusCode::OK};

    // Add the subscription first
    subscriptionManager_.addSubscription(oid);

    // Check if this is a wildcard subscription
    if (subscriptionManager_.isWildcard(oid)) {
        // Extract base OID (everything before the wildcard)
        std::string baseOid = oid.substr(0, oid.length() - 2); // Remove "/*"
        
        // Get the base parameter
        auto baseParam = dm_.getParam(baseOid, rc);
        if (!baseParam) {
            std::cout << "Failed to get base parameter for wildcard subscription: " << rc.what() << std::endl;
            return;
        }

        // Add subscription for base parameter
        subscriptionManager_.addSubscription(baseOid);

        // If base param is a STRUCT or STRUCT_ARRAY, get its children
        if (baseParam->type().value() == catena::ParamType::STRUCT || 
            baseParam->type().value() == catena::ParamType::STRUCT_ARRAY) {
            
            const auto& descriptor = baseParam->getDescriptor();
            const auto& subParams = descriptor.getAllSubParams();

            // For STRUCT_ARRAY, we need to iterate through array indices
            if (baseParam->type().value() == catena::ParamType::STRUCT_ARRAY) {
                for (size_t i = 0; i < baseParam->size(); i++) {
                    // Create path with array index
                    Path path(baseOid);
                    path.push_back(i);

                    // Get array element
                    auto arrayElem = dm_.getParam(path.toString(true), rc);
                    if (!arrayElem) {
                        std::cout << "Failed to get array element at index " << i << ": " << rc.what() << std::endl;
                        continue;
                    }

                    // Add subscription for array element
                    subscriptionManager_.addSubscription(path.toString(true));

                    // Process each child of the array element
                    for (const auto& [childOid, childDesc] : subParams) {
                        Path childPath = path;
                        childPath.push_back(childOid);
                        
                        auto childParam = dm_.getParam(childPath.toString(true), rc);
                        if (childParam) {
                            // Add subscription for child
                            subscriptionManager_.addSubscription(childPath.toString(true));

                            catena::DeviceComponent_ComponentParam response;
                            response.set_oid(childPath.toString(true));
                            auto err = childParam->toProto(*response.mutable_param(), authz);
                            if (err.status != catena::StatusCode::OK) {
                                std::cout << "Failed to serialize parameter " << childPath.toString(true) << ": " << err.what() << std::endl;
                                continue;
                            }
                            responses_.push_back(response);
                        }
                    }
                }
            } else { // STRUCT type
                // Process each child directly
                for (const auto& [childOid, childDesc] : subParams) {
                    Path childPath(baseOid);
                    childPath.push_back(childOid);

                    auto childParam = dm_.getParam(childPath.toString(true), rc);
                    if (childParam) {
                        // Add subscription for child
                        subscriptionManager_.addSubscription(childPath.toString(true));

                        catena::DeviceComponent_ComponentParam response;
                        response.set_oid(childPath.toString(true));
                        auto err = childParam->toProto(*response.mutable_param(), authz);
                        if (err.status != catena::StatusCode::OK) {
                            std::cout << "Failed to serialize parameter " << childPath.toString(true) << ": " << err.what() << std::endl;
                            continue;
                        }
                        responses_.push_back(response);
                    }
                }
            }
        }
    } else {
        // Non-wildcard subscription - get the parameter directly
        auto param = dm_.getParam(oid, rc);
        if (param) {
            catena::DeviceComponent_ComponentParam response;
            response.set_oid(oid);
            auto err = param->toProto(*response.mutable_param(), authz);
            if (err.status != catena::StatusCode::OK) {
                std::cout << "Failed to serialize parameter " << oid << ": " << err.what() << std::endl;
                return;
            }
            responses_.push_back(response);
        }
    }
}