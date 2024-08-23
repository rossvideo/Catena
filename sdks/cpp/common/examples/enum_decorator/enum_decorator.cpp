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
 * @brief Example program to demonstrate ENUMDECORATOR macros
 * @file enum_decorator.cpp
 * @copyright Copyright © 2024 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// common
#include <patterns/EnumDecorator.h>

#include <iostream>

// example using the ENUMDECORATOR macros
// DRY by defining a macro as an argument for the ENUMDECORATOR macros
// which take identical arguments.
// give the enum a name, underlying type and list of value/string pairs
// clang-format push
// clang-format off
#define COUNTER ( \
    Counter, \
    int32_t, \
    (kDefault) "default", \
    (kOne) "one", \
    (kTwo) "two" \
)
// clang-format pop

// declare the enum
// this would normally be in a header file
INSTANTIATE_ENUM(ENUMDECORATOR_DECLARATION, COUNTER);

using Counter = catena::patterns::EnumDecorator<Counter_e>;

// define the enum, initializes the forward map
// this would normally be in a source file
INSTANTIATE_ENUM(ENUMDECORATOR_DEFINITION, COUNTER);



// example of enum with non sequential values
// we have to do this by hand instead of using the macros

// first declare the enum
enum class Primes_e : uint16_t {
    kUndefined = 0, 
    kTwo = 2,
    kThree = 3,
    kFive = 5,
    kSeven = 7,
    kEleven = 11
};

// then declare the EnumDecorator
using Primes = catena::patterns::EnumDecorator<Primes_e>;

// then define the forward map
template <>
const Primes::FwdMap Primes::fwdMap_ = {
    {Primes_e::kUndefined, "undefined"},
    {Primes_e::kTwo, "two"},
    {Primes_e::kThree, "three"},
    {Primes_e::kFive, "five"},
    {Primes_e::kSeven, "seven"},
    {Primes_e::kEleven, "eleven"}
};

int main() {

    // default constructor, sets value to zero
    Counter c0;
    std::cout << "c0: " << c0.toString() << " has value: " << Counter::utype(c0) << std::endl;

    // construct from enum value
    Counter c1(Counter_e::kOne);
    std::cout << "c1: " << c1.toString() << std::endl;

    // construct from string
    Counter c2("two");
    std::cout << "c2: " << Counter::utype(c2) << std::endl;

    // construct from integer
    Counter c3(2);
    std::cout << "c3: " << c3.toString() << " has value: " << Counter::utype(c3) << std::endl;

    // construct from invalid string
    Counter c4("three");
    std::cout << "c4: " << c4.toString() << " has value: " << Counter::utype(c4) << std::endl;

    // construct from invalid integer
    Counter c5(-1);
    std::cout << "c5: " << c5.toString() << " has value: " << Counter::utype(c5) << std::endl;

    // inequality
    std::cout << std::boolalpha << "c1 == c2: " << (c1 == c2) << std::endl;

    // equality
    std::cout << std::boolalpha << "c2 == c3: " << (c2 == c3) << std::endl;

    // cast to string
    std::cout << "c3: " << std::string(c3) << std::endl;

    // Using the hand-built EnumDecorator for Primes
    // default constructor, demonstrates that the default value is zero
    // so it's recommended to use this behaviour to flag an undefined or invalid EnumDecorator
    Primes p0; 
    std::cout << "p0: " << p0.toString() << " = " << Primes::utype(p0) << std::endl;

    Primes p5(Primes_e::kFive);
    std::cout << "p5: " << p5.toString() << " = " << Primes::utype(p5) << std::endl;
}
