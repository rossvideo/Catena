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
 * @file ICatenaServiceImpl.h
 * @brief Interface for the REST API implementation
 * @author Benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

// common
#include <Status.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <IDevice.h>
#include <ISubscriptionManager.h>
#include <rpc/IConnect.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// std
#include <string>
#include <iostream>
#include <regex>

using namespace catena::common;

namespace catena {
namespace REST {

/**
 * @brief Interface for the REST::CatenaServiceImpl class
 */
class ICatenaServiceImpl {
  public:
    ICatenaServiceImpl() = default;
    ICatenaServiceImpl(const ICatenaServiceImpl&) = delete;
    ICatenaServiceImpl& operator=(const ICatenaServiceImpl&) = delete;
    ICatenaServiceImpl(ICatenaServiceImpl&&) = delete;
    ICatenaServiceImpl& operator=(ICatenaServiceImpl&&) = delete;
    virtual ~ICatenaServiceImpl() = default;

    /**
     * @brief Returns the API's version.
     */
    virtual const std::string& version() const = 0;
    /**
     * @brief Starts the API.
     */
    virtual void run() = 0;
    /**
     * @brief Shuts down the running API server. This should only be called some time.
     * after a call to run().
     */
    virtual void Shutdown() = 0;
    /**
     * @brief Returns true if authorization is enabled.
     */
    virtual inline bool authorizationEnabled() const = 0;
    /**
     * @brief Get the subscription manager
     * @return Reference to the subscription manager
     */
    virtual inline ISubscriptionManager& subscriptionManager() = 0;
    /**
     * @brief Returns the EOPath.
     */
    virtual const std::string& EOPath() = 0;
    /**
     * @brief Regesters a Connect CallData object into the Connection priority queue.
     * @param cd The Connect CallData object to register.
     * @return TRUE if successfully registered, FALSE otherwise.
     */
    virtual bool registerConnection(catena::common::IConnect* cd) = 0;
    /**
     * @brief Deregisters a Connect CallData object into the Connection priority queue.
     * @param cd The Connect CallData object to deregister.
     */
    virtual void deregisterConnection(catena::common::IConnect* cd) = 0;
};

};  // namespace REST
};  // namespace catena
