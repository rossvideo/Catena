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
            context_.AsyncNotifyWhenDone(this);
            
            try {
                // Process the subscription updates
                std::cout << "Processing subscription updates for slot: " << req_.slot() << std::endl;
                std::cout << "Number of OIDs to add: " << req_.added_oids_size() << std::endl;
                std::cout << "Number of OIDs to remove: " << req_.removed_oids_size() << std::endl;
                
                // Print the OIDs to be added
                for (const auto& oid : req_.added_oids()) {
                    std::cout << "OID to add: " << oid << std::endl;
                }
                
                // Print the OIDs to be removed
                for (const auto& oid : req_.removed_oids()) {
                    std::cout << "OID to remove: " << oid << std::endl;
                }
                
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
                    std::cout << "Current subscription count before removal: " 
                              << subscriptionManager_.getAllSubscribedOids(dm_).size() << std::endl;
                    if (!subscriptionManager_.removeSubscription(oid)) {
                        std::cout << "Failed to remove subscription for OID: " << oid << std::endl;
                        throw catena::exception_with_status("Failed to remove subscription for OID: " + oid, 
                                                          catena::StatusCode::INTERNAL);
                    }
                    std::cout << "Successfully removed subscription for OID: " << oid << std::endl;
                    std::cout << "Current subscription count after removal: " 
                              << subscriptionManager_.getAllSubscribedOids(dm_).size() << std::endl;
                }
                
                // Process added OIDs
                for (const auto& oid : req_.added_oids()) {
                    std::cout << "Adding subscription for OID: " << oid << std::endl;
                    std::cout << "Current subscription count before addition: " 
                              << subscriptionManager_.getAllSubscribedOids(dm_).size() << std::endl;
                    if (!subscriptionManager_.addSubscription(oid, dm_)) {
                        std::cout << "Failed to add subscription for OID: " << oid << std::endl;
                        throw catena::exception_with_status("Failed to add subscription for OID: " + oid, 
                                                          catena::StatusCode::INTERNAL);
                    }
                    std::cout << "Successfully added subscription for OID: " << oid << std::endl;
                    std::cout << "Current subscription count after addition: " 
                              << subscriptionManager_.getAllSubscribedOids(dm_).size() << std::endl;
                }
                
                // Now that all subscriptions are processed, send current values for all subscribed parameters
                std::cout << "Getting current subscribed parameters..." << std::endl;
                try {
                    sendSubscribedParameters(*authz);
                    std::cout << "Number of responses to send: " << responses_.size() << std::endl;
                } catch (const catena::exception_with_status& e) {
                    std::cout << "Error getting subscribed parameters: " << e.what() << std::endl;
                    // Don't throw here - we still want to finish the request successfully
                    responses_.clear();
                }

                // If we have responses, start writing them
                if (!responses_.empty()) {
                    writer_lock_.lock();
                    status_ = CallStatus::kWrite;
                    writer_.Write(responses_[current_response_], this);
                    writer_lock_.unlock();
                } else {
                    std::cout << "No responses to send, finishing..." << std::endl;
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status::OK, this);
                }
                
            } catch (const catena::exception_with_status& e) {
                std::cout << "Error processing subscriptions: " << e.what() << std::endl;
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(e.status), e.what());
                writer_.Finish(errorStatus, this);
            } catch (...) {
                std::cout << "Unknown error processing subscriptions" << std::endl;
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::INTERNAL, 
                    "Failed due to unknown error in UpdateSubscriptions");
                writer_.Finish(errorStatus, this);
            }
            break;

        case CallStatus::kWrite:
            std::cout << "[" << objectId_ << "] Entering kWrite state" << std::endl;
            try {
                if (!ok) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status::CANCELLED, this);
                    break;
                }

                writer_lock_.lock();
                current_response_++;
                if (current_response_ < responses_.size()) {
                    writer_.Write(responses_[current_response_], this);
                } else {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(grpc::Status::OK, this);
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
            new UpdateSubscriptions(service_, dm_, subscriptionManager_, ok);
            break;

        default:
            std::cout << "[" << objectId_ << "] Entering default state" << std::endl;
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}

void CatenaServiceImpl::UpdateSubscriptions::sendSubscribedParameters(catena::common::Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    
    // Get all subscribed OIDs from the manager
    const auto& subscribedOids = subscriptionManager_.getAllSubscribedOids(dm_);
    std::cout << "Got " << subscribedOids.size() << " subscribed OIDs" << std::endl;
    
    // Process each subscribed OID
    for (const auto& oid : subscribedOids) {
        std::cout << "Processing subscribed OID: " << oid << std::endl;
        std::unique_ptr<IParam> param;
        {
            Device::LockGuard lg(dm_);
            param = dm_.getParam(oid, rc);
        }
        
        if (rc.status != catena::StatusCode::OK) {
            std::cout << "Error getting parameter: " << rc.what() << std::endl;
            throw catena::exception_with_status("Failed to get parameter for OID: " + oid, rc.status);
        }
        
        if (!param) {
            std::cout << "Parameter not found: " << oid << std::endl;
            throw catena::exception_with_status("Parameter not found for OID: " + oid, 
                                              catena::StatusCode::NOT_FOUND);
        }

        catena::DeviceComponent_ComponentParam response;
        response.set_oid(oid);
        param->toProto(*response.mutable_param(), authz);
        responses_.push_back(response);
        std::cout << "Added response for OID: " << oid << std::endl;
    }
}

