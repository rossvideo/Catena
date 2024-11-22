/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

/**
 * @brief Reads a catena device model from a JSON file and exercises the
 * parameter getValue, setValue methods.
 *
 * @author John R. Naylor (john.naylor@rossvideo.com)
 * @file basic_param_access.cpp
 * @copyright Copyright © 2023, Ross Video Ltd
 */

#include <DeviceModel.h>
#include <ParamAccessor.h>
#include <Path.h>
#include <Reflect.h>
#include <utils.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <typeinfo>
#include <typeindex>
#include <variant>

using Index = catena::Path::Index;
using DeviceModel = catena::DeviceModel;
using Param = catena::Param;
using ParamAccessor = catena::ParamAccessor;


// clang_format push
// clang_format off
// define some structs to test struct support
// note use of REFLECTABLE_STRUCT macro which provides runtime reflection
// and type conversion support used by ParamAccessor's get/set methods
//
struct Coords {
    REFLECTABLE_STRUCT(Coords, (float)x, (float)y, (float)z);
};

// note nested struct
struct Location {
    REFLECTABLE_STRUCT(Location, (Coords)coords, (float)latitude, (float)longitude, (int32_t)altitude,
                       (std::string)name);
};

struct AudioSlot {
    REFLECTABLE_STRUCT(AudioSlot, (std::string)name, (float)gain);
};

struct VideoSlot {
    REFLECTABLE_STRUCT(VideoSlot, (std::string)name);
};

REFLECTABLE_VARIANT(SlotVariant, (AudioSlot), (VideoSlot));


int main(int argc, char **argv) {
    // process command line
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " path/to/input-file.json\n";
        exit(EXIT_SUCCESS);
    }

    try {
        // read a json file into a DeviceModel object
        DeviceModel dm(argv[1]);

        // write the device model to stdout
        std::cout << "Read Device Model: " << dm << '\n';

        // read a variant
        std::unique_ptr<ParamAccessor> slotParam = dm.param("/slot");
        SlotVariant slot = {AudioSlot{"audio", 10.0f}};  // initialize the variant
        slotParam->getValue(slot);
        assert(std::holds_alternative<VideoSlot>(slot));
        slot = AudioSlot{"back to audio", 0.0f};  // change the variant
        slotParam->setValue(slot);

        // read & write native struct
        Location loc = {{91.f, 82.f, 73.f}, 10.0f, 20.0f, -30, "Old Trafford"}, loc2;
        std::unique_ptr<ParamAccessor> locParam = dm.param("/location");

        locParam->getValue(loc2);
        std::cout << "Location: " << loc2.latitude << ", " << loc2.longitude << ", " << loc2.altitude << ", "
                  << loc2.name << ", " << loc2.coords.x << ", " << loc2.coords.y << ", " << loc2.coords.z
                  << '\n';
        locParam->setValue(loc);

        // read & write native int32_t
        std::unique_ptr<ParamAccessor> numParam = dm.param("/a_number");
        int32_t num = 0;
        numParam->getValue(num);
        std::cout << "Number: " << num << '\n';
        num *= 2;
        numParam->setValue(num);

        // read & write native vector of int32_t
        std::vector<int32_t> primes = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
        std::unique_ptr<ParamAccessor> primesParam = dm.param("/primes");
        primesParam->setValue(primes);

        std::vector<int32_t> squares;
        std::unique_ptr<ParamAccessor> squaresParam = dm.param("/squares");
        squaresParam->getValue(squares);
        std::cout << "Squares: ";
        for (auto &i : squares) {
            std::cout << i << ' ';
        }
        std::cout << '\n';

        // read & write elements of a native vector of int32_t
        std::unique_ptr<ParamAccessor> powersParam = dm.param("/powers_of_two");
        int32_t mistake = 0;
        powersParam->setValue(mistake, 1);

        int32_t twoCubed = 0;
        powersParam->getValue(twoCubed, 3);
        std::cout << "2^3: " << twoCubed << '\n';

        // write the device model to stdout
        std::cout << "Updated Device Model: " << dm << '\n';

        std::string serialized;
        dm.device().SerializeToString(&serialized);
        std::cout << "Device model serializes to " << serialized.size() << " bytes\n";

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}