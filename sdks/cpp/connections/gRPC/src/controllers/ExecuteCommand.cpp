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
#include <controllers/ExecuteCommand.h>
using catena::gRPC::ExecuteCommand;

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

//Counter for generating unique object IDs - static, so initializes at start
int ExecuteCommand::objectCounter_ = 0;

/** 
 * Constructor which initializes and registers the current ExecuteCommand
 * object, then starts the process
 */
ExecuteCommand::ExecuteCommand(ICatenaServiceImpl *service, IDevice& dm, bool ok)
    : CallData(service), dm_{dm}, writer_(&context_),
      status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service_->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(ok);  // Start the process
}

/**
 * Manages gRPC command execution process by transitioning between states and
 * handling errors and responses accordingly 
 */
void ExecuteCommand::proceed(bool ok) {
    std::cout << "ExecuteCommand proceed[" << objectId_ << "]: " << timeNow()
                << " status: " << static_cast<int>(status_) << ", ok: " << std::boolalpha << ok
                << std::endl;

    // If the process is cancelled, finish the process
    if(!ok){
        std::cout << "ExecuteCommand[" << objectId_ << "] cancelled\n";
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
            service_->RequestExecuteCommand(&context_, &req_, &writer_, service_->cq(), service_->cq(), this);
            break;
        /**
         * Processes the command by reading the initial request from the client
         * and transitioning to kRead
         */
        case CallStatus::kProcess:
            new ExecuteCommand(service_, dm_, ok); // to serve other clients
            context_.AsyncNotifyWhenDone(this);
            { // rc scope
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            try {
                std::unique_ptr<IParam> command = nullptr;
                // Getting the command.
                if (service_->authorizationEnabled()) {
                    catena::common::Authorizer authz{jwsToken_()};
                    command = dm_.getCommand(req_.oid(), rc, authz);
                } else {
                    command = dm_.getCommand(req_.oid(), rc, catena::common::Authorizer::kAuthzDisabled);
                }
                // Executing the command if found.
                if (command != nullptr) {
                    res_ = command->executeCommand(req_.value());
                    status_ = CallStatus::kWrite; 
                }
            // ERROR
            } catch (catena::exception_with_status& err) {
                rc = catena::exception_with_status(err.what(), err.status);
            } catch (...) {
                rc = catena::exception_with_status("unknown error", catena::StatusCode::INTERNAL);
            }
            // Ending process if an error occured, falling through otherwise.
            if (rc.status != catena::StatusCode::OK) {
                status_ = CallStatus::kFinish;
                writer_.Finish(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                break;
            }
            } // rc scope

        /**
         * Writes the response to the client by sending the external object and
         * then continues to kPostWrite or kFinish
         */
        case CallStatus::kWrite:
            /*
             * Currently only sends on response to the client. Add stream
             * ability later.
             */
            writer_.Write(res_, this);
            status_ = CallStatus::kPostWrite;
            break;

        // Status after finishing writing the response, transitions to kFinish
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            writer_.Finish(Status::OK, this);
            break;

        /**
         * Deregisters the current ExecuteCommand object and finishes the
         * process
         */
        case CallStatus::kFinish:
            std::cout << "ExecuteCommand[" << objectId_ << "] finished\n";
            service_->deregisterItem(this);
            break;

        // Throws an error if the state is not recognized
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            writer_.Finish(errorStatus, this);
    }
}