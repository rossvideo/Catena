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
 * @file Connect.h
 * @brief Implements Catena gRPC Connect
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-06-08
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// connections/gRPC
#include <ServiceImpl.h>
#include <SubscriptionManager.h>

/**
* @brief CallData class for the Connect RPC
*/
class CatenaServiceImpl::Connect : public CallData {
    public:
        /**
         * @brief Constructor for the CallData class of the Connect gRPC.
         * Calls proceed() once initialized.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param dm - Address of the device to connect to.
         * @param ok - Flag to check if the command was successfully executed.
         */ 
        Connect(CatenaServiceImpl *service, Device &dm, bool ok);
        /**
         * @brief Manages the steps of the Connect gRPC command
         * through the state variable status. Returns the value of the
         * parameter specified by the user.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param ok - Flag to check if the command was successfully executed.
         */
        void proceed(CatenaServiceImpl *service, bool ok) override;

    private:
        /**
         * @brief Parent CatenaServiceImpl.
         */
        CatenaServiceImpl *service_;
        /**
         * @brief Server request (Info on connection).
         */
        catena::ConnectPayload req_;
        /**
         * @brief Server response (updates).
         */
        catena::PushUpdates res_;
        /**
         * @brief gRPC async writer to write updates.
         */
        ServerAsyncWriter<catena::PushUpdates> writer_;
        /**
         * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
         */
        CallStatus status_;
        /**
         * @brief The device to connect to.
         */
        Device &dm_;
        /**
         * @brief The mutex to for locking the object while writing
         */
        std::mutex mtx_;
        /**
         * @brief Condition variable to notify the object when the value has
         * been updated
         */
        std::condition_variable cv_;
        /**
         * @brief Flag to check if the value has been updated
         */
        bool hasUpdate_{false};
        /**
         * @brief ID of the Connect object
         */
        int objectId_;
        /**
         * @brief The total # of Connect objects.
         */
        static int objectCounter_;
        /**
         * @brief Unused???
         */
        unsigned int pushUpdatesId_;
        /**
         * @brief Id of operation waiting for valueSetByClient to be emitted.
         * Used when ending the connection.
         */
        unsigned int valueSetByClientId_;
        /**
         * @brief Id of operation waiting for valueSetByServer to be emitted.
         * Used when ending the connection.
         */
        unsigned int valueSetByServerId_;
        /**
         * @brief Id of operation waiting for languageAddedPushUpdate to be
         * emitted. Used when ending the connection.
         */
        unsigned int languageAddedId_;
  
        /**
         * @brief Signal emitted in the case of an error which requires the all
         * open connections to be shut down.
         */
        static vdk::signal<void()> shutdownSignal_;
        /**
         * @brief ID of the shutdown signal for the Connect object
        */
        unsigned int shutdownSignalId_;

        /**
         * @brief Updates the response message with parameter values and handles 
         * authorization checks.
         * 
         * @param oid - The OID of the value to update
         * @param idx - The index of the value to update
         * @param p - The parameter to update
         */
        void updateResponse(const std::string& oid, size_t idx, const IParam* p);
};
