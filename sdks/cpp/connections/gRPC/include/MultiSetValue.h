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
 * @file MultiSetValue.h
 * @brief Implements Catena gRPC MultiSetValue
 * @author benjamin.whitten@rossvideo.com
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-01-??
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// connections/gRPC
#include <ServiceImpl.h>
#include <SetValue.h>

/**
* @brief CallData class for the MultiSetValue RPC
*/
class CatenaServiceImpl::MultiSetValue : public CallData {
    public:
        MultiSetValue(CatenaServiceImpl *service, Device &dm, bool ok);
        void proceed(CatenaServiceImpl *service, bool ok) override;
    private:
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
         * @brief Server request (Info on value to set).
         */
        catena::MultiSetValuePayload req_;
        /**
         * @brief Server response (UNUSED).
         */
        catena::Value res_;
        /**
         * @brief gRPC async response writer.
         */
        ServerAsyncResponseWriter<catena::Empty> responder_;
        /**
         * @brief The gRPC command's state (kCreate, kProcess, kFinish, etc.).
         */
        CallStatus status_;
        /**
         * @brief The device containing the value to set.
         */
        Device &dm_;
        /**
         * The status of the transaction for use in responder.finish functions.
         */
        Status errorStatus_;
        /**
         * @brief The object's unique id.
         */
        int objectId_;
        /**
         * @brief The total # of SetValue objects.
         */
        static int objectCounter_;
};
