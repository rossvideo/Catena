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
#include <ExecuteCommand.h>

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
int CatenaServiceImpl::ExecuteCommand::objectCounter_ = 0;

/** 
 * Constructor which initializes and registers the current ExecuteCommand
 * object, then starts the process
 */
CatenaServiceImpl::ExecuteCommand::ExecuteCommand(CatenaServiceImpl *service, Device &dm, bool ok)
    : service_{service}, dm_{dm}, stream_(&context_),
        status_{ok ? CallStatus::kCreate : CallStatus::kFinish} {
    service->registerItem(this);
    objectId_ = objectCounter_++;
    proceed(service, ok);  // Start the process
}

/**
 * Manages gRPC command execution process by transitioning between states and
 * handling errors and responses accordingly 
 */
void CatenaServiceImpl::ExecuteCommand::proceed(CatenaServiceImpl *service, bool ok) {
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
            service_->RequestExecuteCommand(&context_, &stream_, service_->cq_, service_->cq_, this);
            break;

        /**
         * Processes the command by reading the initial request from the client
         * and transitioning to kRead
         */
        case CallStatus::kProcess:
            new ExecuteCommand(service_, dm_, ok);
            /** 
             * ExecuteCommand RPC always begins with reading initial request
             * from client
             */
            status_ = CallStatus::kRead;
            stream_.Read(&req_, this);
            break;

        /** 
         * Reads the command request from the client and transitions to kRead.
         * Should always tranistion to this state after calling stream_.Read()
         */
        case CallStatus::kRead:
        {
            catena::exception_with_status rc{"", catena::StatusCode::OK};
            std::unique_ptr<IParam> command = nullptr;
            // Check if authorization is enabled
            if (service_->authorizationEnabled()) {
                std::vector<std::string> clientScopes = service->getScopes(context_);
                catena::common::Authorizer authz{clientScopes};
                command = dm_.getCommand(req_.oid(), rc, authz);
            } else {
                command = dm_.getCommand(req_.oid(), rc, catena::common::Authorizer::kAuthzDisabled);
            }

            // If the command is not found, return an error
            if (command == nullptr) {
                status_ = CallStatus::kFinish;
                stream_.Finish(Status(static_cast<grpc::StatusCode>(rc.status), rc.what()), this);
                break;
            }
            // Execute the command
            res_ = command->executeCommand(req_.value());
            /**
             * @todo: Implement streaming response
             * For now we are not streaming the response so we can finish the
             * call
             */
            status_ = CallStatus::kPostWrite; 
            stream_.Write(res_, this);
            break;
        }

        /**
         * Status after reading the request, which transitions to kRead to
         * handle subsequent requests
         */
        case CallStatus::kWrite:
            status_ = CallStatus::kRead;
            stream_.Read(&req_, this);
            break;

        // Status after finishing writing the response, transitions to kFinish
        case CallStatus::kPostWrite:
            status_ = CallStatus::kFinish;
            stream_.Finish(Status::OK, this);
            break;

        /**
         * Deregisters the current ExecuteCommand object and finishes the
         * process
         */
        case CallStatus::kFinish:
            std::cout << "ExecuteCommand[" << objectId_ << "] finished\n";
            service->deregisterItem(this);
            break;

        // Throws an error if the state is not recognized
        default:
            status_ = CallStatus::kFinish;
            grpc::Status errorStatus(grpc::StatusCode::INTERNAL, "illegal state");
            stream_.Finish(errorStatus, this);
    }
}