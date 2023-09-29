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
    virtual catena::Value operator[](std::size_t idx) = 0;

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
        * @brief override array accessor operator
        * @param idx index of array
        * @erturn catena value
        */
    catena::Value operator[](std::size_t idx) override;


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

// float implementation
template <> catena::Value ConcreteArrayAccessor<float>::operator[](std::size_t idx) {
    auto &arr = _in.get().float32_array_values();
    if (arr.floats_size() >= idx) {
        catena::Value ans{};
        ans.set_float32_value(arr.floats(idx));
        return ans;
    } else {
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.floats_size();
        BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
    }
}

// int implementation
template <> catena::Value ConcreteArrayAccessor<int>::operator[](std::size_t idx) {
    auto &arr = _in.get().int32_array_values();
    if (arr.ints_size() >= idx) {
        catena::Value ans{};
        ans.set_int32_value(arr.ints(idx));
        return ans;
    } else {
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.ints_size();
        BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
    }
}

// string implementation
template <> catena::Value ConcreteArrayAccessor<std::string>::operator[](std::size_t idx) {
    auto &arr = _in.get().string_array_values();
    if (arr.strings_size() >= idx) {
        catena::Value ans{};
        ans.set_string_value(arr.strings(idx));
        return ans;
    } else {
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.strings_size();
        BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
    }
}

// struct implementation
template <> catena::Value ConcreteArrayAccessor<catena::StructList>::operator[](std::size_t idx) {
    auto &arr = _in.get().struct_array_values();

    if (arr.struct_values_size() >= idx) {
        auto &sv = arr.struct_values(idx);

        catena::StructValue *out{};
        catena::Value ans{};

        out->mutable_fields()->insert(sv.fields().begin(), sv.fields().end());
        ans.set_allocated_struct_value(out);

        return ans;
    } else {
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.struct_values_size();
        BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
    }
}

}  // namespace catena
