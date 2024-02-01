// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <typeinfo>

using Index = catena::Path::Index;
using DeviceModel = catena::DeviceModel;
using Param = catena::Param;
using ParamAccessor = catena::ParamAccessor;

struct Wibble {
};

// clang_format push
// clang_format off
// define some structs to test struct support
// note use of REFLECTABLE_STRUCT macro which provides runtime reflection
// and type conversion support used by ParamAccessor's *Native methods
//
struct Coords {
    REFLECTABLE_STRUCT(Coords, (float) x, (float) y, (float) z);
};


// note nested struct
struct Location {
    REFLECTABLE_STRUCT(Location, (Coords) coords, (float) latitude, (float) longitude, (int32_t) altitude, (std::string) name);
};

struct AudioSlot {
    REFLECTABLE_STRUCT(AudioSlot, (std::string) name);
};

struct VideoSlot {
    REFLECTABLE_STRUCT(VideoSlot, (std::string) name);
};

REFLECTABLE_VARIANT(
    SlotVariant,
    (AudioSlot),
    (VideoSlot)
);

static bool floatGetter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kFloat32Value, [](void *dstAddr, const catena::Value *src) {
    *reinterpret_cast<float *>(dstAddr) = src->float32_value();
});

static bool int32Getter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kInt32Value, [](void *dstAddr, const catena::Value *src) {
    *reinterpret_cast<int32_t *>(dstAddr) = src->int32_value();
});

static bool stringGetter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kStringValue, [](void *dstAddr, const catena::Value *src) {
    *reinterpret_cast<std::string *>(dstAddr) = src->string_value();
});

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

        // read & write native struct
        Location loc = {{91.f, 82.f, 73.f}, 10.0f, 20.0f, -30, "Old Trafford" }, loc2;
        ParamAccessor locParam = dm.param("/location");

        locParam.getValueNative(loc2);
        std::cout << "Location: " << loc2.latitude << ", " << loc2.longitude << ", " << loc2.altitude << ", "
                  << loc2.name << ", " << loc2.coords.x << ", " << loc2.coords.y << ", " << loc2.coords.z
                  << '\n';
        locParam.setValueNative(loc);

        // read a variant
        ParamAccessor slotParam = dm.param("/slot");
        SlotVariant slot = {AudioSlot{"audio"}}; // initialize the variant
        slotParam.getValueNative(slot);

        // read & write native int32_t
        ParamAccessor numParam = dm.param("/a_number");
        int32_t num = 0;
        numParam.getValueNative(num);
        std::cout << "Number: " << num << '\n';
        num *= 2;
        numParam.setValueNative(num);

        // read & write native vector of int32_t
        std::vector<int32_t> primes = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
        ParamAccessor primesParam = dm.param("/primes");
        primesParam.setValueNative(primes);

        std::vector<int32_t> squares;
        ParamAccessor squaresParam = dm.param("/squares");
        squaresParam.getValueNative(squares);
        std::cout << "Squares: ";
        for (auto &i : squares) {
            std::cout << i << ' ';
        }
        std::cout << '\n';

        // read & write elements of a native vector of int32_t
        ParamAccessor powersParam = dm.param("/powers_of_two");
        int32_t mistake = 0;
        powersParam.setValueAtNative(mistake, 1);

        int32_t twoCubed = 0;
        powersParam.getValueAtNative(twoCubed, 3);
        std::cout << "2^3: " << twoCubed << '\n';

        // write the device model to stdout
        std::cout << "Updated Device Model: " << dm << '\n';


        // // cache a param and get its value
        // ParamAccessor helloParam = dm.param("/hello");
        // std::cout << "Hello Param: " << helloParam.getValue<float>() << '\n';

        // // access a sub-param directly
        // std::cout << "location.latitude: " << dm.param("/location/latitude").getValue<float>() << '\n';

        // report the wire-size of the device model
        std::string serialized;
        dm.device().SerializeToString(&serialized);
        std::cout << "Device model serializes to " << serialized.size() << " bytes\n";

    } catch (std::exception &why) {
        std::cerr << "Problem: " << why.what() << '\n';
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}