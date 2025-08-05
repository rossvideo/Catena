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
 * @file use_structs.cpp
 * @author john.naylor@rossvideo.com
 * @brief Steps up the complexity a notch by dealing with structured data
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here example and
 * builds on that. Less chatty comments.
 * 
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// device model
#include "device.use_structs.json.h" 


// common
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>
#include <Authorizer.h>

// protobuf interface
#include <interface/param.pb.h>

#include <iostream>
#include <Logger.h>

using namespace catena::common;
using namespace use_structs;
using catena::common::ParamTag;

int main (int argc, char** argv) {
    Logger::StartLogging(argc, argv);

    // lock the model
    std::lock_guard lg(dm.mutex());
    catena::exception_with_status err{"", catena::StatusCode::OK};

    std::unique_ptr<IParam> ip = dm.getParam("/location", err);
    if (ip == nullptr){
        LOG(ERROR) << "Error: " << err.what();
        return EXIT_FAILURE;
    }
    auto& locationParam = *dynamic_cast<ParamWithValue<Location>*>(ip.get());
    Location& loc = locationParam.get();

    DEBUG_LOG << "Location: lat(" << loc.latitude.degrees << "˚ " << loc.latitude.minutes << "' "
              << loc.latitude.seconds << "\") lon(" << loc.longitude.degrees << "˚ " << loc.longitude.minutes
              << "' " << loc.longitude.seconds << "\")";

    catena::Value value;
    ip->toProto(value, Authorizer::kAuthzDisabled);
    DEBUG_LOG << "Location: " << value.DebugString();

    // this line is for demonstrating the fromProto method
    // this should never be done in a real device
    value.mutable_struct_value()->mutable_fields()->at("latitude").mutable_struct_value()->mutable_fields()->at("degrees").set_float32_value(100);
    ip->fromProto(value, Authorizer::kAuthzDisabled);

    DEBUG_LOG << "New Location: lat(" << loc.latitude.degrees << "˚ " << loc.latitude.minutes << "' "
              << loc.latitude.seconds << "\") lon(" << loc.longitude.degrees << "˚ " << loc.longitude.minutes
              << "' " << loc.longitude.seconds << "\")";

    ip = dm.getParam("/location/latitude", err);
    if (ip == nullptr){
        LOG(ERROR) << "Error: " << err.what();
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, Authorizer::kAuthzDisabled);
    DEBUG_LOG << "Latitude: " << value.DebugString();

    ip = dm.getParam("/location/latitude/degrees", err);
    if (ip == nullptr){
        LOG(ERROR) << "Error: " << err.what();
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, Authorizer::kAuthzDisabled);
    DEBUG_LOG << "Latitude degrees: " << value.DebugString();

    ip = dm.getParam("/location/longitude/seconds", err);
    if (ip == nullptr){
        LOG(ERROR) << "Error: " << err.what();
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, Authorizer::kAuthzDisabled);
    DEBUG_LOG << "Longitude seconds: " << value.DebugString();

    return EXIT_SUCCESS;
}

/* Possible Output
Location: lat(1˚ 2' 3") lon(4˚ 5' 6")
Location: MessageLite at 0x7ffce3575850
New Location: lat(1˚ 2' 3") lon(4˚ 5' 6")
Latitude: MessageLite at 0x7ffce3575850
Latitude degrees: MessageLite at 0x7ffce3575850
Longitude seconds: MessageLite at 0x7ffce3575850
*/