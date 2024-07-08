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
 * @file use_structs.cpp
 * @author john.naylor@rossvideo.com
 * @brief Steps up the complexity a notch by dealing with structured data
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here example and
 * builds on that. Less chatty comments.
 */


#include "lite/examples/use_structs/device.use_structs.json.h"  // dm
#include <lite/include/Device.h>
#include <lite/include/Param.h>
#include <lite/param.pb.h>

using namespace catena::lite;
using namespace catena::common;
using namespace use_structs;
#include <iostream>
int main() {
    // lock the model
    Device::LockGuard lg(dm);

    auto& locationParam = *dynamic_cast<Param<Location>*>(dm.GetParam("/location"));
    Location& locationValue = locationParam.Get();
    std::cout << "Location from model - latitude: " << locationValue.latitude.degrees
              << ", longitude: " << locationValue.longitude << std::endl;
    // create another Location object, using the default initial value specified in the model
    Location externalToModel;
    std::cout << "Location object external to model - latitude: " << externalToModel.latitude.degrees
              << ", longitude: " << externalToModel.longitude << std::endl;
    locationValue = externalToModel;


    // serialize the location parameter - will be removed from this example
    catena::Value value{};
    locationParam.toProto(value);

    // print the serialized value
    auto& struct_value = value.struct_value();
    auto& fields = struct_value.fields();
    for (const auto& field : fields) {
        std::cout << "Field: " << field.first << std::endl;
        auto& field_value = field.second.value();
        if (field_value.has_float32_value()) {
            std::cout << "float32: " << field_value.float32_value() << std::endl;
        } else {
            std::cout << "Unknown type" << std::endl;
        }
    }


    return EXIT_SUCCESS;
}