#pragma once

/**
 * @brief API for accessing arrays
 * @file ArrayAccessor.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

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

#include <device.pb.h>
#include <param.pb.h>
#include <GenericFactory.h>
#include <Status.h>

namespace catena {

class ArrayAccessor {
  public:
    /**
        * @brief define factory for ArrayAccessor (int for key type)
        *
        */
    using Factory = rv::patterns::GenericFactory<ArrayAccessor, int, catena::Value &>;

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

        if (Value::KindCase::kUndefinedValue < key && key < Value::KindCase::kDataPayload) {
            std::cout << key << std::endl;
            return fac.addProduct(key, makeOne);
        } else {
            return false;
        }
    }
};


}  // namespace catena
