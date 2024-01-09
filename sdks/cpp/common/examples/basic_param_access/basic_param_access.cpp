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
using DeviceModel = catena::DeviceModel<catena::Threading::kSingleThreaded>;
using Param = catena::Param;
using ParamAccessor = catena::ParamAccessor<DeviceModel>;

struct Location  {
REFLECTABLE(
    Location, 
    (float) latitude, 
    (float) longitude
);
};
template void catena::ParamAccessor<DeviceModel>::setValueExperimental<Location>(Location const&);

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

        Location loc = {10.0f,20.0f};
        ParamAccessor locParam = dm.param("/location");

        if constexpr (catena::has_getType<Location>) {
            locParam.setValueExperimental(loc);
        } 

        // write the device model to stdout
        std::cout << "Updated Device Model: " << dm << '\n';


        // cache a param and get its value
        ParamAccessor helloParam = dm.param("/hello");
        std::cout << "Hello Param: " << helloParam.getValue<float>() << '\n';

        // cache a array param and get its value using two methods
        ParamAccessor strArrayParam= dm.param("/primes_str");
        std::cout << "Prime String Param using getValueAt: " << strArrayParam.getValueAt(0).string_value() << '\n';
        std::cout << "Prime String Param using getValue: " << strArrayParam.getValue<std::string>() << '\n';

        // access a param directly
        std::cout << "location.latitude: " << dm.param("/location/latitude").getValue<float>() << '\n';

        // set some values in the device model using a cached param
        // and a path. N.B. types can be inferred
        std::cout << "setting values to something different\n";
        helloParam.setValue(3.142f);     // example using cached param
        dm.param("/world").setValue(3);  // example using chaining

        std::cout << "cached param value: " << helloParam.getValue<float>() << '\n';

        // demo caching a sub-param
        ParamAccessor longitudeParam = dm.param("/location/longitude");
        std::cout << "Longitude: " << longitudeParam.getValue<float>() << '\n';
        longitudeParam.setValue(30.0f);
        std::cout << "Updated Longitude: " << longitudeParam.getValue<float>() << '\n';


        // add a struct param the hard way
        // catena::Param sparam{};
        // *(sparam.mutable_name()->mutable_monoglot()) = "struct param";
        // *(sparam.mutable_fqoid()) = "sparam";
        // sparam.mutable_type()->set_param_type(
        //     catena::ParamType_ParamTypes::ParamType_ParamTypes_STRUCT);

        // catena::Value fval;
        // fval.set_float32_value(1.23);
        // catena::Value struct_val;
        // (*(*struct_val.mutable_struct_value()->mutable_fields())["float_field"]
        //       .mutable_value()) = fval;
        // (*sparam.mutable_value()) = struct_val;
        // dm.addParam("/sparam", std::move(sparam));

        // write out the updated device model
        // std::cout << "Updated Device Model: " << dm << '\n';

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