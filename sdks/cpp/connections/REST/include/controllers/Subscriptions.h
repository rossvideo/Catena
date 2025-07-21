#pragma once

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

/**
 * @file Subscriptions.h
 * @brief Implements Catena REST Subscriptions
 * @author zuhayr.sarker@rossvideo.com
 * @date 2025-04-28
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// connections/REST
#include <ISubscriptionManager.h>

// common
#include <IDevice.h>
#include <rpc/TimeNow.h>

// Connections/REST
#include "interface/ISocketReader.h"
#include <SocketWriter.h>
#include "interface/ICallData.h"
#include "interface/IServiceImpl.h"

#include <Logger.h>

namespace catena {
namespace REST {

/**
 * @brief ICallData class for the Subscriptions REST controller.
 */
class Subscriptions : public ICallData {
public:
    // Specifying which Device and IParam to use (defaults to catena::...)
    using IDevice = catena::common::IDevice;
    using IParam = catena::common::IParam;
    using SlotMap = catena::common::SlotMap;

    /**
     * @brief Constructor for the Subscriptions controller.
     *
     * @param service Pointer to the parent CatenaServiceImpl.
     * @param socket The socket to write the response to.
     * @param context The ISocketReader object.
     * @param dms A map of slots to ptrs to their corresponding device.
     */ 
    Subscriptions(ICatenaServiceImpl *service, tcp::socket& socket, ISocketReader& context, SlotMap& dms);
    
    /**
     * @brief Subscriptions's main process.
     */
    void proceed() override;
    
    /**
     * @brief Creates a new controller object for use with GenericFactory.
     * 
     * @param service Pointer to the parent CatenaServiceImpl.
     * @param socket The socket to write the response stream to.
     * @param context The ISocketReader object.
     * @param dms A map of slots to ptrs to their corresponding device.
     */
    static ICallData* makeOne(ICatenaServiceImpl *service, tcp::socket& socket, ISocketReader& context, SlotMap& dms) {
        return new Subscriptions(service, socket, context, dms);
    }

private:
    /**
     * @brief Helper function to write status messages to the API console.
     * 
     * @param status The current state of the RPC (kCreate, kFinish, etc.)
     * @param ok The status of the RPC (open or closed).
     */
    inline void writeConsole_(CallStatus status, bool ok) const override {
        DEBUG_LOG << RESTMethodMap().getForwardMap().at(context_.method())
                  << " Subscriptions::proceed[" << objectId_ << "]: "
                  << catena::common::timeNow() << " status: "
                  << static_cast<int>(status) << ", ok: " << std::boolalpha << ok;
    }

    /**
     * @brief The socket to write the response to.
     */
    tcp::socket& socket_;
    /**
     * @brief The ISocketReader object.
     */
    ISocketReader& context_;
    /**
     * @brief The SocketWriter object for writing to socket_.
     */
    std::unique_ptr<ISocketWriter> writer_ = nullptr;
    /**
     * @brief A map of slots to ptrs to their corresponding device.
     */
    SlotMap& dms_;
    /**
     * @brief Pointer to the CatenaServiceImpl.
     */
    ICatenaServiceImpl *service_;

    /**
     * @brief ID of the Subscriptions object
     */
    int objectId_;
    /**
     * @brief The total # of Subscriptions objects.
     */
    static int objectCounter_;
};

} // namespace REST 
} // namespace catena
