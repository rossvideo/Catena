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
#include <controllers/ExternalObjectRequest.h>
#include <Logger.h>
using catena::gRPC::ExternalObjectRequest;

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
int ExternalObjectRequest::objectCounter_ = 0;

/** 
 * Constructor which initializes and registers the current
 * ExternalObjectRequest object, then starts the process
 */
ExternalObjectRequest::ExternalObjectRequest(IServiceImpl *service, SlotMap& dms, bool ok)
    : CallData(service), dms_{dms}, writer_(&context_),
    status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(ok);  // start the process
}

/** 
 * Manages gRPC command execution process by transitioning between states and
 * handling errors and responses accordingly 
 */
void ExternalObjectRequest::proceed(bool ok) {
    DEBUG_LOG << "ExternalObjectRequest proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok;
    
    // If the process is cancelled, finish the process
    if (!ok) {
        DEBUG_LOG << "ExternalObjectRequest[" << objectId_ << "] cancelled";
        status_ = CallStatus::kFinish;
    }

    //State machine to manage the process
    switch (status_) {
        /**
         * Initial state, sets up reques to execute command and transitions to
         * kProcess
         */
        case CallStatus::kCreate:
            status_ = CallStatus::kProcess;
            service_->RequestExternalObjectRequest(&context_, &req_, &writer_, service_->cq(), service_->cq(),
                                            this);
            break;

        /** 
         * Processes the command by reading the initial request from the client
         * and transitioning to kRead
         */
        case CallStatus::kProcess:
            new ExternalObjectRequest(service_, dms_, ok);  // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            status_ = CallStatus::kWrite;
            // fall thru to start writing

        /**
         * Writes the response to the client by sending the external object and
         * then continues to kPostWrite or kFinish
         */
        case CallStatus::kWrite:
            try {
                DEBUG_LOG << "sending external object " << req_.oid() <<"\n";
                std::string path = service_->EOPath();
                path.append(req_.oid());

                // Check if the file exists
                if (!std::filesystem::exists(path)) {
                    DEBUG_LOG << "ExternalObjectRequest[" << objectId_ << "] file not found";
                    if(req_.oid()[0] != '/'){
                        std::stringstream why;
                        why << __PRETTY_FUNCTION__ << "\nfile '" << req_.oid() << "' not found. HINT: Make sure oid starts with '/' prefix.";
                        throw catena::exception_with_status(why.str(), catena::StatusCode::NOT_FOUND);
                    }else{
                        std::stringstream why;
                        why << __PRETTY_FUNCTION__ << "\nfile '" << req_.oid() << "' not found";
                        throw catena::exception_with_status(why.str(), catena::StatusCode::NOT_FOUND);
                    }
                }
                // Read the file into a byte array
                std::ifstream file(path, std::ios::binary);
                std::vector<char> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                st2138::ExternalObjectPayload obj;
                obj.mutable_payload()->set_payload(file_data.data(), file_data.size()); 
                //obj.mutable_payload()->set_meta(file.);

                //For now we are sending the whole file in one go
                DEBUG_LOG << "ExternalObjectRequest[" << objectId_ << "] sent";
                status_ = CallStatus::kPostWrite; 
                writer_.Write(obj, this);
            // Exception occured, finish the process
            } catch (catena::exception_with_status &e) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status(static_cast<grpc::StatusCode>(e.status), e.what()), this);
            // Catch all other exceptions and finish the process
            } catch (...) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status::CANCELLED, this);
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
            DEBUG_LOG << "ExternalObjectRequest[" << objectId_ << "] finished";
            service_->deregisterItem(this);
            break;

        // Throws an error if the state is not recognized
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}