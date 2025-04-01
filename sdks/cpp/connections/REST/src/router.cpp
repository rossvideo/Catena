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

// common
#include <Tags.h>

#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
#include <utils.h>

#include <api.h>
#include <Connect.h>
#include <MultiSetValue.h>
#include <SetValue.h>
#include <DeviceRequest.h>
#include <GetValue.h>
#include <GetPopulatedSlots.h>

#include "absl/flags/flag.h"

#include <iostream>
#include <regex>

using catena::API;

void API::route(tcp::socket& socket, SocketReader& context, catena::common::Authorizer* authz) {
    // This is a temporary fix.
    std::string request = std::string(context.req());
    if (context.method() == "GET") {          // GET methods.
        if (context.rpc().starts_with("/v1/DeviceRequest")) {
            DeviceRequest deviceRequest(request, socket, dm_, authz);
        } else if (context.rpc().starts_with("/v1/GetPopulatedSlots")) {
            GetPopulatedSlots getPopulatedSlots(socket, dm_);
        } else if (context.rpc().starts_with("/v1/GetValue")) {
            GetValue getValue(request, socket, dm_, authz);
        } else if (context.rpc().starts_with("/v1/Connect")) {
            Connect connect(request, socket, dm_, authz);
        } else {
            throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
    } else if (context.method() == "POST") {  // POST methods.
        throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
    } else if (context.method() == "PUT") {   // PUT methods.
        if (context.rpc().starts_with("/v1/SetValue")) {
            SetValue(context.jsonBody(), socket, dm_, authz);
        } else if (context.rpc().starts_with("/v1/MultiSetValue")) {
            MultiSetValue(context.jsonBody(), socket, dm_, authz);
        } else {
            throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
    } else {
        throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
    }
}

void API::route(tcp::socket& socket) {
    // Incrementing activeRPCs.
    {
    std::lock_guard<std::mutex> lock(activeRpcMutex_);
    activeRpcs_ += 1;
    }
    if (!shutdown_) {
        try {
            // Reading from the socket.
            SocketReader context(socket, authorizationEnabled_);
            // Setting up authorizer
            std::shared_ptr<catena::common::Authorizer> sharedAuthz;
            catena::common::Authorizer* authz = nullptr;
            if (authorizationEnabled_) {
                sharedAuthz = std::make_shared<catena::common::Authorizer>(context.jwsToken());
                authz = sharedAuthz.get();
            } else {
                authz = &catena::common::Authorizer::kAuthzDisabled;
            }
            // Routing the request based on the name.
            route(socket, context, authz);
        // Catching errors.
        } catch (catena::exception_with_status& err) {
            SocketWriter writer(socket);
            writer.write(err);
        } catch (...) {
            SocketWriter writer(socket);
            catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
            writer.write(err);
        }
    }
    // Decrementing activeRPCs.
    {
    std::lock_guard<std::mutex> lock(activeRpcMutex_);
    activeRpcs_ -= 1;
    std::cout << "Active RPCs remaining: " << activeRpcs_ << '\n';
    }
}
