/*
 * Copyright 2025 Ross Video Ltd
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

#pragma once

// Connections/REST
#include "api.h"
#include <rpc/Connect.h>
using catena::API;

class API::Connect : public CallData, public catena::common::Connect {
    public:
        Connect(std::string& request, Tcp::socket& socket, Device& dm, catena::common::Authorizer* authz);
        void proceed() override;
    private:
        bool isCancelled() override { return !this->socket_.is_open(); }

        /**
         * @brief The total # of Connect objects.
         */
        static int objectCounter_;
        /**
         * @brief ID of the Connect object
         */
        int objectId_;
        Tcp::socket& socket_;
        ChunkedWriter writer_;

        /**
         * @brief The mutex to for locking the object while writing
         */
        std::mutex mtx_;

        // Signal ID
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

        const std::map<std::string, catena::Device_DetailLevel> dlMap_ = {
            {"FULL", catena::Device_DetailLevel_FULL},
            {"SUBSCRIPTIONS", catena::Device_DetailLevel_SUBSCRIPTIONS},
            {"MINIMAL", catena::Device_DetailLevel_MINIMAL},
            {"COMMANDS", catena::Device_DetailLevel_COMMANDS},
            {"NONE", catena::Device_DetailLevel_NONE}
        };
};

