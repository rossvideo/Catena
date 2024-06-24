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
 * @file start_here.cpp
 * @author john.naylor@rossvideo.com
 * @brief Demonstrates how to create a trivially simple device model
 * and access a parameter within it from your business logic.
 */

#include "build/lite/examples/start_here/device.start_here.json.h"
#include <lite/include/DeviceModel.h>
#include <lite/include/Param.h>

using namespace catena::lite;
#include <iostream>
int main () {
    // because we designed the device model, we know it contains a parameter, hello
    // and we also know its type - std::string
    // so we can get a reference to it
    auto& helloParam = *dynamic_cast<Param<std::string>*>(dm.GetParam("hello"));

    // and we can get a reference to the parameter's value
    std::string& helloValue = helloParam.Get();

    // and then read it, directly - which is nice and simple but prone to race conditions
    // if more than one thread is accessing the value
    std::cout << helloValue << std::endl;

    // and we can change the value - with the same caveat as race conditions
    helloValue = "Goodbye, Cruel World!";
    std::cout << helloValue << std::endl;

    return EXIT_SUCCESS;
}