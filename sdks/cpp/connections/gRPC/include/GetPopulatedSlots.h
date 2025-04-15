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
 * @file GetPopulatedSlots.h
 * @brief Implements Catena gRPC GetPopulatedSlots
 * @author john.naylor@rossvideo.com
 * @author john.danen@rossvideo.com
 * @author isaac.robert@rossvideo.com
 * @date 2024-06-08
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// connections/gRPC
#include <ServiceImpl.h>

/**
* @brief CallData class for the GetPopulatedSlots RPC
*/
class CatenaServiceImpl::GetPopulatedSlots : public CallData{
    public:
        /**
         * @brief Constructor for the CallData class of the GetPopulatedSlots
         * gRPC. Calls proceed() once initialized.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param dm - Address of the device to get the populated slots of.
         * @param ok - Flag to check if the command was successfully executed.
         */ 
        GetPopulatedSlots(CatenaServiceImpl *service, IDevice* dm, bool ok);
        /**
        * @brief Manages the steps of the GetPopulatedSlots gRPC command
        * through the state variable status. Returns the number of populated
        * slots to the client.
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
         * @brief Server request (empty).
         */
        catena::Empty req_;
        /**
         * @brief Server response (list of populated slots).
         */
        catena::SlotList res_;
        /**
         * @brief gRPC async response writer.
         */
        ServerAsyncResponseWriter<::catena::SlotList> responder_;
        /**
         * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
         */
        CallStatus status_;
        /**
         * @brief The device to get the populated slots of.
         */
        IDevice* dm_;
        /**
         * @brief The object's unique id.
         */
        int objectId_;
        /**
         * @brief The total # of GetPopulatedSlots objects.
         */
        static int objectCounter_;
};
