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
 * @file use_templates.cpp
 * @author john.danen@rossvideo.com
 * @brief Steps up the complexity a notch by dealing with structured data
 *
 * It does not support any connections so is not a complete example
 * of a working device.
 *
 * It presumes the reader has understood the start_here example and
 * builds on that. Less chatty comments.
 */

// device model
#include "device.use_templates.json.h"  

// lite
#include <Device.h>
#include <ParamWithValue.h>
#include <ParamDescriptor.h>
#include <PolyglotText.h>

// protobuf interface
#include <interface/param.pb.h>

using namespace catena::lite;
using namespace catena::common;
using namespace use_templates;
#include <iostream>
int main() {
    // lock the model
    Device::LockGuard lg(dm);

    IParam* ip = dm.getItem<ParamTag>("ottawa");
    assert(ip != nullptr);
    auto& canadasCapital = *dynamic_cast<ParamWithValue<City>*>(ip);
    City& city = canadasCapital.get();
    std::cout << "Canada's capital city is " << city.city_name
              << " at latitude " << city.latitude
              << " and longitude " << city.longitude
              << " with a population of " << city.population
              << std::endl;

    ip = dm.getItem<ParamTag>("toronto");
    auto& ontariosCapital = *dynamic_cast<ParamWithValue<City>*>(ip);
    City& city2 = ontariosCapital.get();
    std::cout << "Ontario's capital city is " << city2.city_name
              << " at latitude " << city2.latitude
              << " and longitude " << city2.longitude
              << " with a population of " << city2.population
              << std::endl;
   
    return EXIT_SUCCESS;
}