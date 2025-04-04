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

// REST
#include <api.h>

// RPCs
#include <Connect.h>
#include <MultiSetValue.h>
#include <SetValue.h>
#include <DeviceRequest.h>
#include <GetValue.h>
#include <GetPopulatedSlots.h>

using catena::API;

void API::route(tcp::socket& socket) {
    if (!shutdown_) {
        try {
            // Reading from the socket.
            SocketReader context;
            context.read(socket, authorizationEnabled_);
            // Routing to RPC.
            if (context.method() == "GET") {
                if (context.rpc() == "/v1/Connect") {
                    Connect rpc(socket, context, dm_);
                }
                else if (context.rpc() == "/v1/DeviceRequest") {
                    DeviceRequest rpc(socket, context, dm_);
                }
                else if (context.rpc() == "/v1/GetPopulatedSlots") {
                    GetPopulatedSlots rpc(socket, context, dm_);
                }
                else if (context.rpc() == "/v1/GetValue") {
                    GetValue rpc(socket, context, dm_);
                }
            }
            else if (context.method() == "PUT") {
                if (context.rpc() == "/v1/MultiSetValue") {
                    MultiSetValue rpc(socket, context, dm_);
                }
                else if (context.rpc() == "/v1/SetValue") {
                    SetValue rpc(socket, context, dm_);
                }
            }
            else {
                throw catena::exception_with_status("Method " + context.method() + " does not exist", catena::StatusCode::INVALID_ARGUMENT);
            }
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
}
