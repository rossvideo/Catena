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

#include "absl/flags/flag.h"

#include <iostream>
#include <regex>

using catena::API;

void API::route(std::string& method, std::string& request, std::string& jsonPayload, Tcp::socket& socket, catena::common::Authorizer* authz) {
    if (method == "GET") {          // GET methods.
        if (request.starts_with("/v1/DeviceRequest")) {
            deviceRequest(request, socket, authz);
        } else if (request.starts_with("/v1/GetPopulatedSlots")) {
            getPopulatedSlots(socket);
        } else if (request.starts_with("/v1/GetValue")) {
            getValue(request, socket, authz);
        } else {
            throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
    } else if (method == "POST") {  // POST methods.
        throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
    } else if (method == "PUT") {   // PUT methods.
        if (request.starts_with("/v1/SetValue")) {
            setValue(jsonPayload, socket, authz);
        } else if (request.starts_with("/v1/MultiSetValue")) {
            multiSetValue(jsonPayload, socket, authz);
        } else {
            throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
        }
    } else {
        throw catena::exception_with_status("Request does not exist", catena::StatusCode::INVALID_ARGUMENT);
    }
}