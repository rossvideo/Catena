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

// common
#include <Tags.h>

// connections/gRPC
#include <controllers/DeviceRequest.h>
#include <ISubscriptionManager.h>

using catena::gRPC::DeviceRequest;

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

// Counter for generating unique object IDs - static, so initializes at start
int DeviceRequest::objectCounter_ = 0;

/** 
 * Constructor which initializes and registers the current DeviceRequest
 * object, then starts the process
 */
DeviceRequest::DeviceRequest(ICatenaServiceImpl *service, IDevice& dm, bool ok)
    : CallData(service), dm_{dm}, writer_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(ok);  // start the process
}

/** 
 * Manages gRPC command execution process by transitioning between states and
 * handling errors and responses accordingly 
 */
void DeviceRequest::proceed(bool ok) {
    std::cout << "DeviceRequest proceed[" << objectId_ << "]: " << timeNow()
              << " status: " << static_cast<int>(status_) << ", ok: "
              << std::boolalpha << ok << std::endl;
    
    // If the process is cancelled, finish the process
    if(!ok){
        std::cout << "DeviceRequest[" << objectId_ << "] cancelled\n";
        status_ = CallStatus::kFinish;
    }
    
    //State machine to manage the process
    switch (status_) {
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestDeviceRequest(&context_, &req_, &writer_, service_->cq(), service_->cq(), this);
            break;

        /**
         * Processes the command by reading the initial request from the client
         * and transitioning to kRead
         */
        case CallStatus::kProcess:
            new DeviceRequest(service_, dm_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            try {
                bool shallowCopy = true; // controls whether shallow copy or deep copy is used
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                
                // Setting up authorizer object.
                if (service_->authorizationEnabled()) {
                    // Authorizer throws an error if invalid jws token so no need to handle rc.
                    sharedAuthz_ = std::make_shared<catena::common::Authorizer>(getJWSToken_(rc));
                    authz_ = sharedAuthz_.get();
                } else {
                    authz_ = &catena::common::Authorizer::kAuthzDisabled;
                }

                // req_.detail_level defaults to FULL
                catena::Device_DetailLevel dl = req_.detail_level();

                // Getting subscribed oids if dl == SUBSCRIPTIONS.
                if (dl == catena::Device_DetailLevel_SUBSCRIPTIONS) {
                    // Add new subscriptions to both the manager and our tracking list
                    for (const auto& oid : req_.subscribed_oids()) {
                        if (!service_->getSubscriptionManager().addSubscription(oid, dm_, rc)) {
                            throw catena::exception_with_status(std::string("Failed to add subscription: ") + rc.what(), rc.status);
                        }
                    }
                    // Get service subscriptions from the manager
                    subscribedOids_ = service_->getSubscriptionManager().getAllSubscribedOids(dm_);
                }

                // Getting the serializer object.
                serializer_ = dm_.getComponentSerializer(*authz_, subscribedOids_, dl, shallowCopy);

            // Likely authentication error, end process.
            } catch (catena::exception_with_status& err) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(static_cast<grpc::StatusCode>(err.status), err.what());
                writer_.Finish(errorStatus, this);
                break;
            } catch (...) {
                status_ = CallStatus::kFinish;
                grpc::Status errorStatus(grpc::StatusCode::UNKNOWN, "unknown error");
                writer_.Finish(errorStatus, this);
                break;
            }
            status_ = CallStatus::kWrite;
            // fall thru to start writing
        
        /**
         * Writes the response to the client by sending the external object and
         * then continues to kPostWrite or kFinish
         */
        case CallStatus::kWrite:
            {   
                if (!serializer_) {
                    // It should not be possible to get here
                    status_ = CallStatus::kFinish;
                    grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
                    writer_.Finish(errorStatus, this);
                    break;
                } 
                
                try {     
                    catena::DeviceComponent component{};
                    {
                        std::lock_guard lg(dm_.mutex());
                        component = serializer_->getNext();
                    }
                    status_ = serializer_->hasMore() ? CallStatus::kWrite : CallStatus::kPostWrite;
                    writer_.Write(component, this); 
                //Exception occured, finish the process
                } catch (catena::exception_with_status &e) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
                //Catch all other exceptions and finish the process
                } catch (...) {
                    status_ = CallStatus::kFinish;
                    writer_.Finish(Status::CANCELLED, this);
                }
            }
            break;

        // Status after finishing writing the response, transitions to kFinish
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        /**
         * Deregisters the current ExternalObjectRequest object and finishes
         * the process
         */
        case CallStatus::kFinish:
            std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
            service_->deregisterItem(this);
            break;

        // Throws an error if the state is not recognized
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}
