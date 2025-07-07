/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Example program to demonstrate ENUMDECORATOR macros
 * @file enum_decorator.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
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

/* Possible Output
c0: default has value: 0
c1: one
c2: 2
c3: two has value: 2
c4: default has value: 0
c5: default has value: 0
c1 == c2: false
c2 == c3: true
c3: two
p0: undefined = 0
p5: five = 5
*/