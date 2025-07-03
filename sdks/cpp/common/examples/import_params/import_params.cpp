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
 * @file import_params.cpp
 * @author john.naylor@rossvideo.com
 * @brief Demo's a device model that uses imported params
 *
 * On the C++ side, imported params are identical to params that are defined in the device file.
 * 
 * This example aims to give more detail on how default values can be set for subparams of a struct.
 *
 * @copyright Copyright © 2025 Ross Video Ltd
 */

// device model
#include "device.import_params.json.h"


// common
#include <Device.h>
#include <ParamWithValue.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

#include <iostream>
#include "Logger.h"

using namespace catena::common;
using namespace import_params;
using catena::common::ParamTag;

std::string locationToString(const City::Location& location) {
    return "Lat:" +
           std::to_string(location.latitude.degrees) + "°" +
           std::to_string(location.latitude.minutes) + "'" +
           std::to_string(location.latitude.seconds) + "''" +
              ", Long:" +
           std::to_string(location.longitude.degrees) + "°" +
           std::to_string(location.longitude.minutes) + "'" +
           std::to_string(location.longitude.seconds) + "''" +
           "";
}

int main (int argc, char** argv) {
    Logger::StartLogging(argc, argv);

    // lock the model
    std::lock_guard lg(dm.mutex());
    catena::exception_with_status err{"", catena::StatusCode::OK};

    /**
     * In the device model, plane_ticket has a value so it was added as a param to the device. The departure
     * subparam of plane_ticket was left undefined in the plane_ticket value so it was initialized with the value
     * defined within the departure subparam (defined in "params/param.ottawa.json")
     */
    std::unique_ptr<catena::common::IParam> p = dm.getParam("/plane_ticket", err);
    if (p == nullptr){
        LOG(ERROR) << "Error: " << err.what();
        return EXIT_FAILURE;
    }
    Plane_ticket& plane_ticket = getParamValue<Plane_ticket>(p.get());
    DEBUG_LOG << "Departure: " << plane_ticket.departure.name << " (" << locationToString(plane_ticket.departure.location) << ")";
    DEBUG_LOG << "Destination: " << plane_ticket.destination.name << " (" << locationToString(plane_ticket.destination.location) << ")";

    /**
     * When a new Plane_ticket is created, the departure subparam is initialized with the value defined in the 
     * departure param. Destination does not have a default value so its name is left empty and its location is
     * the default value defined in the location param.
     */
    Plane_ticket new_plane_ticket;
    DEBUG_LOG << "Departure: " << new_plane_ticket.departure.name << " (" << locationToString(new_plane_ticket.departure.location) << ")";
    DEBUG_LOG << "Destination: " << new_plane_ticket.destination.name << " (" << locationToString(new_plane_ticket.destination.location) << ")";

    return EXIT_SUCCESS;
}

/* Possible Output
Departure:  (Lat:1°2'3.000000'', Long:4°5'6.000000'')
Destination: Paris (Lat:48°43'49.099998'', Long:2°22'22.100000'')

Departure: Ottawa (Lat:45°19'4.900000'', Long:75°39'56.700001'')
Destination:  (Lat:1°2'3.000000'', Long:4°5'6.000000'')
*/