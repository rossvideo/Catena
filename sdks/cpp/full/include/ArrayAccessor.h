#pragma once

/**
 * @brief API for accessing arrays
 * @file ArrayAccessor.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

#include <interface/device.pb.h>
#include <interface/param.pb.h>
#include <patterns/GenericFactory.h>
#include <Status.h>

namespace catena {
namespace full {

class ArrayAccessor {
  public:
    /**
     * @brief define factory for ArrayAccessor (int for key type)
     *
     */
    using Factory = catena::patterns::GenericFactory<ArrayAccessor, int, catena::Value &>;

    /**
     * @brief override array accessor operator
     * @param idx index of array
     * @return catena value
     */
    virtual catena::Value operator[](std::size_t idx) const = 0;

    /**
     * @brief virtual destructor
     */
    virtual ~ArrayAccessor() = default;
};

template <typename T> class ConcreteArrayAccessor : public ArrayAccessor {
  private:
    std::reference_wrapper<catena::Value> _in;

    /**
     * @brief create a concrete array accessor using a value
     * @param v the catena value
     * @return ConcreteArrayAccessor
     */
    static ArrayAccessor *makeOne(catena::Value &v) { return new ConcreteArrayAccessor(v); }

    /*
     * This is the key attribute that Classes to be created via a
     * GenericFactory must declare & define.
     */
    static bool _added;

  public:
    /**
     * @brief constructor for the concrete array accessor
     * @param in catena value
     */
    ConcreteArrayAccessor(catena::Value &in) : _in{in} {};

    /**
     * @brief destructor
     *
     */
    ~ConcreteArrayAccessor() = default;

    /**
     * @brief read an array element
     * @param idx index of array
     * @return value of element at idx, packaged as a catena::Value
     */
    catena::Value operator[](std::size_t idx) const override;


    /**
     * @brief register a product
     * @param key key of product
     * @return true if product was able to be made
     */
    static bool registerWithFactory(int key) {
        Factory &fac = Factory::getInstance();

        if (key > Value::KindCase::kUndefinedValue < key) {
            std::cout << key << std::endl;
            return fac.addProduct(key, makeOne);
        } else {
            return false;
        }
    }
};
}  // namespace full
}  // namespace catena
