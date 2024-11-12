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



#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>

#include <api.h>

using catena::API;

API::API() : version_{"1.0.0"} {
    CROW_ROUTE(app_, "/v1/PopulatedSlots")
    ([]() {
        ::catena::SlotList slotList;
        slotList.add_slots(1);
        slotList.add_slots(42);
        slotList.add_slots(65535);

        // Convert the SlotList message to JSON
        std::string json_output;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        auto status = MessageToJsonString(slotList, &json_output, options);

        // Check if the conversion was successful
        if (!status.ok()) {
            return crow::response(500, "Failed to convert protobuf to JSON");
        }

        // Create a Crow response with JSON content type
        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.write(json_output);
        return res;
    });
}

std::string API::version() const {
  return version_;
}

void API::run() {
    app_.port(8080).run();
}