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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
#include <controllers/UpdateSubscriptions.h>
using catena::gRPC::UpdateSubscriptions;

int UpdateSubscriptions::objectCounter_ = 0;

UpdateSubscriptions::UpdateSubscriptions(ICatenaServiceImpl *service, IDevice& dm, bool ok)
    : CallData(service), dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed( ok);  // start the process
}

void UpdateSubscriptions::proceed(bool ok) {
    std::cout << "UpdateSubscriptions proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
              << std::endl;

    if(!ok){
        std::cout << "UpdateSubscriptions[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }

    switch (status_) {
        case CallStatus::kCreate:
            std::cout << "UpdateSubscriptions[" << objectId_ << "] entering kCreate state" << std::endl;
            status_ = CallStatus::kProcess;
            service_->RequestUpdateSubscriptions(&context_, &req_, &writer_, 
                        service_->cq(), service_->cq(), this);
            break;

        case CallStatus::kProcess:
            new UpdateSubscriptions(service_, dm_, ok);
            context_.AsyncNotifyWhenDone(this);
            
            try {
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                catena::common::Authorizer* authz;
                std::shared_ptr<catena::common::Authorizer> sharedAuthz;
                if (service_->authorizationEnabled()) {
                    sharedAuthz = std::make_shared<catena::common::Authorizer>(jwsToken_());
                    authz = sharedAuthz.get();
                } else {
                    authz = &catena::common::Authorizer::kAuthzDisabled;
                }
                
                // Clear any existing responses
                responses_.clear();
                current_response_ = 0;
                
                // Process removed OIDs
                for (const auto& oid : req_.removed_oids()) {     
                    if (!service_->getSubscriptionManager().removeSubscription(oid, dm_, rc)) {
                        throw catena::exception_with_status(std::string("Failed to remove subscription: ") + rc.what(), rc.status);
                    }
                }
                
                // Process added OIDs
                for (const auto& oid : req_.added_oids()) {
                    std::cout << "Adding subscription for OID: " << oid << std::endl;
                    
                    if (!service_->getSubscriptionManager().addSubscription(oid, dm_, rc)) {
                        throw catena::exception_with_status(std::string("Failed to add subscription: ") + rc.what(), rc.status);
                    }
                    
                }
                
                // Now that all subscriptions are processed, send current values for all subscribed parameters
                try {
                    sendSubscribedParameters_(*authz);
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
            service_->deregisterItem(this);
            break;

        default:
            std::cout << "[" << objectId_ << "] Entering default state" << std::endl;
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}

void UpdateSubscriptions::sendSubscribedParameters_(catena::common::Authorizer& authz) {
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    
    // Get all subscribed OIDs from the manager
    const auto& subscribedOids = service_->getSubscriptionManager().getAllSubscribedOids(dm_);
    std::cout << "Got " << subscribedOids.size() << " subscribed OIDs" << std::endl;
    
    // Process each subscribed OID
    for (const auto& oid : subscribedOids) {
        std::cout << "Processing subscribed OID: " << oid << std::endl;
        std::unique_ptr<IParam> param;
        {
            std::lock_guard lg(dm_.mutex());
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

