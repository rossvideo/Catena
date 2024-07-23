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
#include <lite/include/Device.h> // catena::lite::Device, LockGuard
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
    // for the shortest possible time - production code wouldn't output
    // to std::out with the lock asserted, c) avoid deadlock by nesting calls
    // to Device methods that try to lock the mutex.
    Device::LockGuard lg(dm); 

    // because we designed the device model, we know it contains a parameter, hello
    // and we also know its value type - std::string
    // so we can get a reference to its Param object by casting it from 
    // an IParam* supplied by the Device's look up method
    auto& helloParam = *dynamic_cast<Param<std::string>*>(dm.getItem("/hello", Device::ParamTag()));

    // With the Param object we can get a reference to the parameter's value object
    std::string& helloValue = helloParam.get();

    // and then read it, directly - which is simple and should be performant
    // The assertion of the lock, above is essential to thread safety because
    // asynchronous access by connected client will also change the Device Model's 
    // state.
    std::cout << helloValue << std::endl;

    // and we can change the value
    helloValue = "Goodbye, Cruel World!";
    std::cout << helloValue << std::endl;

    // Example with a parameter of type int
    auto& countParam = *dynamic_cast<Param<int>*>(dm.getItem("/count", Device::ParamTag()));
    int32_t& countValue = countParam.get();
    std::cout << "counter initial value: " << countValue << std::endl;
    countValue++;
    std::cout << "counter incremented value: " << countValue << std::endl;

    // Example with a parameter of type float
    auto& gainParam = *dynamic_cast<Param<float>*>(dm.getItem("/gain", Device::ParamTag()));
    float& gainValue = gainParam.get();
    std::cout << "gain initial value: " << gainValue << std::endl;
    gainValue *= gainValue;
    std::cout << "gain squared value: " << gainValue << std::endl;

    // Example with array of strings
    auto& phonetic_alphabetParam = *dynamic_cast<Param<std::vector<std::string>>*>(dm.getItem("/phonetic_alphabet", Device::ParamTag()));
    std::vector<std::string>& phonetic_alphabetValue = phonetic_alphabetParam.get();
    std::cout << "phonetic alphabet initial value: ";
    for (const auto& s : phonetic_alphabetValue) {
        std::cout << s << " ";
    }
    std::cout << std::endl;
    // note we change the length of the vector - decrease by one
    phonetic_alphabetValue = {"Whiskey", "Yankee", "Zulu"};
    std::cout << "phonetic alphabet new value: ";
    for (const auto& s : phonetic_alphabetValue) {
        std::cout << s << " ";
    }
    std::cout << std::endl;

    // Example with array of integers
    auto& primesParam = *dynamic_cast<Param<std::vector<int>>*>(dm.getItem("/primes", Device::ParamTag()));
    std::vector<int>& primesValue = primesParam.get();
    std::cout << "primes initial value: ";
    for (const auto& i : primesValue) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    // note we change the length of the vector - increase by one
    primesValue = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
    std::cout << "primes new value: ";
    for (const auto& i : primesValue) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    // example with array of floats that's initially empty
    auto& physical_constantsParam = *dynamic_cast<Param<std::vector<float>>*>(dm.getItem("/physical_constants", Device::ParamTag()));
    std::vector<float>& physical_constantsValue = physical_constantsParam.get();
    std::cout << "physical constants " << (physical_constantsValue.size() == 0 ? "is empty" : "is not empty") << std::endl;
    physical_constantsValue.push_back(3.14159);
    physical_constantsValue.push_back(2.71828);
    physical_constantsValue.push_back(1.61803);
    std::cout << "physical constants new value: ";
    for (const auto& f : physical_constantsValue) {
        std::cout << f << " ";
    }
    std::cout << std::endl;

    return EXIT_SUCCESS;

    // The Device::LockGuard object will automatically release the lock
    // upon its destruction, when it goes out of scope here.
}