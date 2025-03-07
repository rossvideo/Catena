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

/**
 * @file GetParam.h
 * @brief Implements Catena gRPC GetParam
 * @author john.naylor@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-02-06
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// connections/gRPC
#include <ServiceImpl.h>

/**
* @brief CallData class for the BasicParamInfoRequest RPC
*/
class CatenaServiceImpl::BasicParamInfoRequest : public CallData {
    public:
        /**
         * @brief Constructor for the CallData class of the BasicParamInfoRequest
         * gRPC. Calls proceed() once initialized.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param dm - Address of the device to get the value from.
         * @param ok - Flag to check if the command was successfully executed.
         */ 
        BasicParamInfoRequest(CatenaServiceImpl *service, Device &dm, bool ok);

        // /**
        //  * @brief Destructor for the BasicParamInfoRequest class.
        //  */
        // ~BasicParamInfoRequest();

        /**
         * @brief Manages the steps of the BasicParamInfoRequest gRPC command
         * through the state variable status. Returns the value of the
         * parameter specified by the user.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param ok - Flag to check if the command was successfully executed.
         */
        void proceed(CatenaServiceImpl *service, bool ok) override;

    private:

        /**
         * @brief Updates the array lengths of the responses.
         * 
         * @param array_name - The name of the array.
         * @param length - The length of the array.
         */
        void updateArrayLengths(const std::string& array_name, uint32_t length);


        /**
         * @brief Gets the children of the parameter.
         * 
         * @param param - The parameter to get the children from.
         * @param oid - The oid of the parameter.
         */
       void getChildren(IParam* current_param, const std::string& current_path, catena::common::Authorizer& authz);

        /**
         * @brief Parent CatenaServiceImpl.
         */
        CatenaServiceImpl *service_;

        /**
         * @brief The context of the gRPC command (ServerContext) for use in 
         * _responder and other gRPC objects/functions.
         */
        ServerContext context_;

        /**
         * @brief The client's scopes.
         */
        std::vector<std::string> clientScopes_;

        /**
         * @brief The request payload.
         */
        catena::BasicParamInfoRequestPayload req_;

        /**
         * @brief The response payload.
         */
        catena::PushUpdates res_;

        /**
         * @brief gRPC async response writer.
         */
        ServerAsyncWriter<catena::BasicParamInfoResponse> writer_;
        
        /**
         * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
         */
        CallStatus status_;

        /**
         * @brief The device to get the value from.
         */
        Device &dm_;
        
        /**
         * @brief The object's unique id.
         */
        int objectId_;

        /**
         * @brief The object's unique id counter.
         */
        static int objectCounter_;  


        /**
         * @brief The vector of BasicParamInfoResponse objects.
         */
        std::vector<catena::BasicParamInfoResponse> responses_;

        /**
         * @brief The current response index.
         */
        uint32_t current_response_{0};

        /**
         * @brief The mutex for the writer lock.
         */
        std::mutex mtx_;

        /**
         * @brief The writer lock.
         */
        std::unique_lock<std::mutex> writer_lock_{mtx_, std::defer_lock};
};