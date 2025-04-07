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
 * @file IServiceImpl.h
 * @brief Implements REST API
 * @author Benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2024 Ross Video Ltd
 */

#pragma once

// common
#include <Status.h>
#include <vdk/signals.h>
#include <IParam.h>
#include <Device.h>

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// std
#include <string>
#include <iostream>
#include <regex>

namespace catena {
namespace REST {

/**
 * @brief Interface class for REST ServiceImpl
 */
class IServiceImpl {

  // Specifying which Device and IParam to use (defaults to catena::...)
  using Device = catena::common::Device;
  using IParam = catena::common::IParam;

  public:
    IServiceImpl() = default;
    IServiceImpl(const IServiceImpl&) = delete;
    IServiceImpl& operator=(const IServiceImpl&) = delete;
    IServiceImpl(IServiceImpl&&) = delete;
    IServiceImpl& operator=(IServiceImpl&&) = delete;
    virtual ~IServiceImpl() = default;

    /**
     * @brief Returns the API's version.
     */
    virtual std::string version() const = 0;
    /**
     * @brief Starts the API.
     */
    virtual void run() = 0;
    /**
     * @brief Shuts down an running API. This should only be called sometime
     * after a call to run().
     */
    virtual void Shutdown() = 0;
    /**
     * @brief Returns true if authorization is enabled.
     */
    virtual bool authorizationEnabled() = 0;
    
  private:
    /**
     * @brief Routes a request to the appropriate controller.
     * @param socket The socket to communicate with the client with.
     * @returns Nothing, communicated through the socket, at which point
     * process ends.
     */
    virtual void route(tcp::socket& socket) = 0;
    /**
     * @brief Returns true if port_ is already in use.
     * 
     * Currently unused.
     */
    virtual bool is_port_in_use_() const = 0;
};

};  // namespace REST
};  // namespace catena
