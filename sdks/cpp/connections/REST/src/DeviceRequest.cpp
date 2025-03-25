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

void API::deviceRequest(std::string& request, Tcp::socket& socket, catena::common::Authorizer* authz) {
    // Creating a SocketWriter.
    ChunkedWriter writer(socket);
    try {        
        // Parsing request for fields.
        std::string slotStr = getField(request, "slot");
        uint32_t slot = slotStr.empty() ? 0 : std::stoi(slotStr);
        std::string language = getField(request, "language");
        std::string detailLevel = getField(request, "detail_level");
        // TODO subscribed oids

        // controls whether shallow copy or deep copy is used
        bool shallowCopy = true;
        // Getting the component serializer.
        auto serializer = dm_.getComponentSerializer(*authz, shallowCopy);
        // Write the HTTP response headers to stream.
        catena::exception_with_status status{"OK", catena::StatusCode::OK};
        writer.writeHeaders(status);
        // Getting each component ans writing to the stream.
        while (serializer.hasMore()) {
            catena::DeviceComponent component{};
            {
            Device::LockGuard lg(dm_);
            component = serializer.getNext();
            }
            writer.write(component);
        }
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer.write(err);
    } catch (...) {
        catena::exception_with_status err{"Unknown errror", catena::StatusCode::UNKNOWN};
        writer.write(err);
    }
    /* 
     * POSTMAN does not like this line as they do not support chunked encoding.
     * CURL yells at you if you don't have it.
     */
    // writer.finish();
}
