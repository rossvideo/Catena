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
 * @date 2025-01-20
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// connections/gRPC
#include <ServiceImpl.h>
#include <GenericSetValue.h>

/**
* @brief CallData class for the MultiSetValue RPC
*/
class CatenaServiceImpl::MultiSetValue : public GenericSetValue {
    public:
        /**
         * @brief Constructor for the CallData class of the MultiSetValue
         * gRPC. Calls proceed() once initialized.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param dm - Address of the device to get the value from.
         * @param ok - Flag to check if the command was successfully executed.
         */ 
        MultiSetValue(CatenaServiceImpl *service, Device &dm, bool ok);
        /**
         * @brief Requests Multui Set Value from the system and sets the
         * request to the MultiSetValuePayload in GenericSetValue.
         */
        void request() override;
        /**
         * @brief Creates a new MultiSetValue object to serve other clients
         * while processing.
         *
         * @param service - Pointer to the parent CatenaServiceImpl.
         * @param dm - Address of the device to get the value from.
         * @param ok - Flag to check if the command was successfully executed.
         */ 
        void create(CatenaServiceImpl *service, Device &dm, bool ok) override;
    private:
        /**
         * @brief The total # of SetValue objects.
         */
        static int objectCounter_;
};
