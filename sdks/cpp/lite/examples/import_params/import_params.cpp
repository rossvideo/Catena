/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
 following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.
*/
//

/**
 * @file import_params.cpp
 * @author john.naylor@rossvideo.com
 * @brief Demo's a device model that uses imported params
 *
 * The buisness logic here is the same as use_structs.cpp, but the json device model uses imported params.
 *
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// device model
#include "device.tree.json.h"


// lite
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

using namespace catena::lite;
using namespace catena::common;
using namespace use_structs;
using catena::common::ParamTag;
#include <iostream>
int main() {
    // lock the model
    Device::LockGuard lg(dm);
    catena::exception_with_status err{"", catena::StatusCode::OK};

    std::unique_ptr<IParam> ip = dm.getParam("/location", err);
    if (ip == nullptr) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    auto& locationParam = *dynamic_cast<ParamWithValue<Location>*>(ip.get());
    Location& loc = locationParam.get();

    std::cout << "Location: lat(" << loc.latitude.degrees << "˚ " << loc.latitude.minutes << "' "
              << loc.latitude.seconds << "\") lon(" << loc.longitude.degrees << "˚ " << loc.longitude.minutes
              << "' " << loc.longitude.seconds << "\")" << std::endl;

    catena::Value value;
    std::string clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "Location: " << value.DebugString() << std::endl;

    // this line is for demonstrating the fromProto method
    // this should never be done in a real device
    value.mutable_struct_value()
      ->mutable_fields()
      ->at("latitude")
      .mutable_value()
      ->mutable_struct_value()
      ->mutable_fields()
      ->at("degrees")
      .mutable_value()
      ->set_float32_value(100);
    ip->fromProto(value, clientScope);

    std::cout << "New Location: lat(" << loc.latitude.degrees << "˚ " << loc.latitude.minutes << "' "
              << loc.latitude.seconds << "\") lon(" << loc.longitude.degrees << "˚ " << loc.longitude.minutes
              << "' " << loc.longitude.seconds << "\")" << std::endl;

    ip = dm.getParam("/location/latitude", err);
    if (ip == nullptr) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, clientScope);
    std::cout << "Latitude: " << value.DebugString() << std::endl;

    ip = dm.getParam("/location/latitude/degrees", err);
    if (ip == nullptr) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, clientScope);
    std::cout << "Latitude degrees: " << value.DebugString() << std::endl;

    ip = dm.getParam("/location/longitude/seconds", err);
    if (ip == nullptr) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    value.Clear();
    ip->toProto(value, clientScope);
    std::cout << "Longitude seconds: " << value.DebugString() << std::endl;

    return EXIT_SUCCESS;
}