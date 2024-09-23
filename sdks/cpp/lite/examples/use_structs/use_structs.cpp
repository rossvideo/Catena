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
 * 
 * @copyright Copyright © 2024 Ross Video Ltd
 */

// device model
#include "device.use_structs.json.h" 


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

    std::unique_ptr<IParam> ip = dm.getParam("/location");
    assert(ip != nullptr);
    auto& locationParam = *dynamic_cast<ParamWithValue<Location>*>(ip.get());
    Location& loc = locationParam.get();

    std::cout << "Location: lat(" << loc.latitude.degrees << "˚ " << loc.latitude.minutes << "' "
              << loc.latitude.seconds << "\") lon(" << loc.longitude.degrees << "˚ " << loc.longitude.minutes
              << "' " << loc.longitude.seconds << "\")" << std::endl;

    assert(ip != nullptr);
    catena::Value value;
    std::string clientScope = "operate";
    ip->toProto(value, clientScope);
    std::cout << "Location: " << value.DebugString() << std::endl;


    ip = dm.getParam("/location/latitude");
    assert(ip != nullptr);
    value.Clear();
    ip->toProto(value, clientScope);
    std::cout << "Latitude: " << value.DebugString() << std::endl;

    ip = dm.getParam("/location/latitude/degrees");
    assert(ip != nullptr);
    value.Clear();
    ip->toProto(value, clientScope);
    std::cout << "Latitude degrees: " << value.DebugString() << std::endl;

    ip = dm.getParam("/location/longitude/seconds");
    assert(ip != nullptr);
    value.Clear();
    ip->toProto(value, clientScope);
    std::cout << "Longitude seconds: " << value.DebugString() << std::endl;

    return EXIT_SUCCESS;
}