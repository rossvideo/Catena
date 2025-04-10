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
 * @file ICallData.h
 * @brief Interface class for REST RPCs.
 * @author benjamin.whitten@rossvideo.com
 * @copyright Copyright © 2025 Ross Video Ltd
 */

#pragma once

// common
#include <Enums.h>
#include <patterns/EnumDecorator.h>
#include <Device.h>

using DetailLevel = catena::patterns::EnumDecorator<catena::Device_DetailLevel>;

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
using boost::asio::ip::tcp;

// REST
#include "SocketReader.h"
#include "SocketWriter.h"

namespace catena {
namespace REST {

/**
 * @brief CallData states for status messages.
 */
enum class CallStatus { kCreate, kProcess, kRead, kWrite, kPostWrite, kFinish };

/**
 * @brief Interface class for the REST API RPCs.
 */
class ICallData {
	public:
		virtual ~ICallData() = default;
		/**
		 * @brief The RPC's main process.
		 */
		virtual void proceed() = 0;
		/**
		 * @brief Finishes the RPC.
		 */
		virtual void finish() = 0;

	protected:
		/**
		 * @brief Helper function to write status messages to the API console.
		 * 
		 * @param status The current state of the RPC (kCreate, kFinish, etc.)
		 * @param ok The status of the RPC (open or closed).
		 */
		virtual inline void writeConsole(CallStatus status, bool ok) const = 0;
};

};
};
