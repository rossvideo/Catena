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
 * 
 * It does not support any connections so is not a complete example
 * of a working device.
 */

// this include header was generated from the json device model
// it's in the BINARY folder structure, not SOURCE.
#include "lite/examples/start_here/device.start_here.json.h" // dm

// these includes are from the LITE SDK, they're in the SOURCE
// folder structure.
#include <lite/include/DeviceModel.h> // catena::lite::DeviceModel, LockGuard
#include <lite/include/Param.h> // catena::lite::Param

// this include header was generated from the protobuf definition
// it's in the BINARY folder structure, not SOURCE.
#include <lite/param.pb.h> // catena::Value

using namespace catena::lite;
using namespace catena::common;
#include <iostream>
int main () {
    // The client code, below, directly accesses parts of the device model
    // so we assert a lock on the device model's mutex to ensure threadsafe 
    // behaviour. This is a simple example, and in a real-world application
    // client code should strive to a) use locks like this, b) assert them
    // for the shortest possible time, c) avoid deadlock using DeviceModel 
    // methods that try to assert the its mutex (deadlock).
    DeviceModel::LockGuard lg(dm); 

    // because we designed the device model, we know it contains a parameter, hello
    // and we also know its value type - std::string
    // so we can get a reference to its Param object
    Path helloPath("/hello");
    auto& helloParam = *dynamic_cast<Param<std::string>*>(dm.GetParam(helloPath));

    // With the Param object we can get a reference to the parameter's value object
    std::string& helloValue = helloParam.Get();

    // and then read it, directly - which is simple and should be performant
    // The assertion of the lock, above is essential to thread safety because
    // asynchronous access by connected client will also change the Device Model's 
    // state.
    std::cout << helloValue << std::endl;

    // and we can change the value
    helloValue = "Goodbye, Cruel World!";
    std::cout << helloValue << std::endl;

    return EXIT_SUCCESS;

    // The DeviceModel::LockGuard object will automatically release the lock
    // upon its destruction, when it goes out of scope here.
}