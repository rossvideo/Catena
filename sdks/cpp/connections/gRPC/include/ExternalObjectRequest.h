#pragma once

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

/**
 * @file ExternalObjectRequest.h
 * @brief Implements Catena gRPC ExternalObjectRequest
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-06-08
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// connections/gRPC
#include <ServiceImpl.h>


/**
* @brief CallData class for the ExternalObjectRequest RPC
*/
class CatenaServiceImpl::ExternalObjectRequest : public CallData {
    public:
        /**
         * @brief Constructor for ExternalObjectRequest class
         *
         * @param service the service to which the request is made
         * @param dm the device for which the request is made
         * @param ok flag to check if request is successful 
         */
        ExternalObjectRequest(CatenaServiceImpl *service, Device &dm, bool ok);
        /**
         * @brief Destrutor for ExternalObjectRequest, although it isn't used.
         */
        ~ExternalObjectRequest() {}
        /**
         * @brief Manages gRPC request through a state machine
         *
         * @param service the service to which the request is made
         * @param ok flag to check if request is successful        
         */
        void proceed(CatenaServiceImpl *service, bool ok) override;

    private:
        /**
         * @brief Pointer to CatenaServiceImpl
         */
        CatenaServiceImpl *service_;
        /**
         * @brief Context for gRPC command request for the server to use to
         * send data back to the client
         */
        ServerContext context_;
        /**
         * @brief Request payload for external object
         */
        catena::ExternalObjectRequestPayload req_;
        /**
         * @brief Stream for reading and writing gRPC messages
         */
        ServerAsyncWriter<catena::ExternalObjectPayload> writer_;
        /**
         * @brief Represents the current status of the call within the state
         * machine (kCreate, kProcess, kFinish, etc.)
         */
        CallStatus status_;
        /**
         * @brief Reference to the device to which the request is made
         */
        Device &dm_;
        /**
         * @brief Unique identifier for command object
         */
        int objectId_;
        /**
         * @brief Counter to generate unique object IDs for each new object
         */
        static int objectCounter_;
};