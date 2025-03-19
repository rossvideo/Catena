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
 * @file UpdateSubscriptions.h
 * @brief Implements Catena gRPC UpdateSubscriptions
 * @author john.naylor@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-02-27
 * @copyright Copyright © 2024 Ross Video Ltd
 */

//connections/gRPC
#include <ServiceImpl.h>
#include <atomic>
#include <mutex>
#include <SubscriptionManager.h>

/**
 * @brief CallData class for the UpdateSubscriptions RPC
 */
class CatenaServiceImpl::UpdateSubscriptions : public CallData {
    public:
        /**
         * @brief Constructor for the CallData class of the UpdateSubscriptions RPC
         * gRPC. Calls proceed() once initialized.
         * @param service The CatenaServiceImpl instance
         * @param dm The device model
         * @param subscriptionManager The subscription manager to use
         * @param ok Flag indicating if initialization was successful
         */
        UpdateSubscriptions(CatenaServiceImpl *service, Device &dm, catena::grpc::SubscriptionManager& subscriptionManager, bool ok);

        /**
         * @brief Manages the steps of the UpdateSubscriptions gRPC command
         * through the state variable status. Returns the value of the
         * parameter specified by the user.
         */
        void proceed(CatenaServiceImpl *service, bool ok) override;

    private:
        /**
         * @brief Helper method to process a subscription
         * @param baseOid The base OID 
         * @param authz The authorizer to use for access control
         */
        void processSubscription(const std::string& baseOid, catena::common::Authorizer& authz);


        /**
         * @brief Helper method to send all currently subscribed parameters
         * @param authz The authorizer to use for access control
         */
        void sendSubscribedParameters(catena::common::Authorizer& authz);

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
        catena::UpdateSubscriptionsPayload req_;

        /**
         * @brief The response payload for a single response.
         */
        catena::DeviceComponent_ComponentParam res_;

        /**
         * @brief Vector to store all responses to be sent
         */
        std::vector<catena::DeviceComponent_ComponentParam> responses_;

        /**
         * @brief Current response index being processed
         */
        uint32_t current_response_{0};

        /**
         * @brief gRPC async response writer.
         */
        ServerAsyncWriter<catena::DeviceComponent_ComponentParam> writer_;

        /**
         * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
         */
        CallStatus status_;

        /**
         * @brief The device to get the value from.
         */
        Device &dm_;

        /**
         * @brief The object's unique id counter.
         */
        static int objectCounter_;  

        /**
         * @brief The object's unique id.
         */
        int objectId_;
        
        /**
         * @brief The mutex for the writer lock.
         */
        std::mutex mtx_;

        /**
         * @brief The writer lock.
         */
        std::unique_lock<std::mutex> writer_lock_{mtx_, std::defer_lock};

        /**
         * @brief Reference to the subscription manager
         */
        catena::grpc::SubscriptionManager& subscriptionManager_;
};