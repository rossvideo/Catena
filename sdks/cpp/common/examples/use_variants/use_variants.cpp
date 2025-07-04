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

/**
 * @file use_variants.cpp
 * @author john.naylor@rossvideo.com
 * @brief Demonstrates the variant param type
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here and use_structs
 * example and builds on that. Less chatty comments.
 * 
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// device model
#include "device.use_variants.json.h" 


// common
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

using namespace catena::common;
using namespace catena::common;
using namespace use_variants;
using catena::common::ParamTag;
#include <iostream>

void printCoordinate(const Coordinates_elem& coord) {
    std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Cartesian>) {
            std::cout << "Cartesian: " << arg.x << ", " << arg.y << ", " << arg.z << std::endl;
        } else if constexpr (std::is_same_v<T, _coordinates::Cylindrical>) {
            std::cout << "Cylindrical: " << arg.rho << ", " << arg.phi << "°, " << arg.z << std::endl;
        } else if constexpr (std::is_same_v<T, _coordinates::Spherical>) {
            std::cout << "Spherical: " << arg.r << ", " << arg.theta << "°, " << arg.phi << "°" << std::endl;
        }
    }, coord);
}

int main() {
    // // lock the model
    std::lock_guard lg(dm.mutex());
    catena::exception_with_status err{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> ip;
    
    // get the number param
    ip = dm.getParam("/number", err);
    if (!ip) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catena::Param numberParam;
    ip->toProto(numberParam, Authorizer::kAuthzDisabled);
    std::cout << numberParam.DebugString() << std::endl;

    use_variants::Number& number = getParamValue<use_variants::Number>(ip.get());
    number = "five";
    ip->toProto(numberParam, Authorizer::kAuthzDisabled);
    std::cout << "Updated Number:\n" << numberParam.DebugString() << std::endl;

    // get the coordinates param
    ip = dm.getParam("/coordinates", err);
    if (!ip) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catena::Param coordinatesParam;
    ip->toProto(coordinatesParam, Authorizer::kAuthzDisabled);
    std::cout << coordinatesParam.DebugString() << std::endl;

    ip = dm.getParam("/coordinates/2", err);
    if (!ip) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    use_variants::Coordinates_elem& coord = getParamValue<use_variants::Coordinates_elem>(ip.get());
    std::cout << "Coordinate/2: ";
    printCoordinate(coord);

    ip = dm.getParam("/cartesian", err);
    if (!ip) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    catena::Value value;
    value.mutable_struct_variant_value()->set_struct_variant_type("cartesian");
    dm.getValue("/cartesian", *value.mutable_struct_variant_value()->mutable_value());
    dm.setValue("/coordinates/2", value);
    std::cout << "Updated Coordinate/2: ";
    printCoordinate(coord);

    std::cout << "\n";


    ip = dm.getParam("/coordinates/0", err);
    if (!ip) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    ip->toProto(value, Authorizer::kAuthzDisabled);
    std::cout << value.DebugString() << std::endl;

    value.set_int32_value(42);
    dm.setValue("/coordinates/0/cartesian/z", value);

    std::cout << "\n";

    ip = dm.getParam("/coordinates/0/cartesian", err);
    if (!ip) {
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
    ip->toProto(value, Authorizer::kAuthzDisabled);
    std::cout << value.DebugString() << std::endl;
    use_variants::Cartesian& cartesian = getParamValue<use_variants::Cartesian>(ip.get());
    std::cout << "Updated Coordinates/0/cartesian: ";
    printCoordinate(cartesian);

    return EXIT_SUCCESS;
}

/* Possible Output
MessageLite at 0x7ffd86515610
Updated Number:
MessageLite at 0x7ffd86515610
MessageLite at 0x7ffd86515700
Coordinate/2: Spherical: 1, 90°, 180°
Updated Coordinate/2: Cartesian: 5, 10, 15

MessageLite at 0x7ffd865155d0

MessageLite at 0x7ffd865155d0
Updated Coordinates/0/cartesian: Cartesian: 1, 2, 42
*/