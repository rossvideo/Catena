#pragma once

/**
 * @brief Convenient access to Params in the DeviceModel
 * @file ParamAccessor.h
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

#include <constraint.pb.h>
#include <device.pb.h>
#include <param.pb.h>

#include <Fake.h>
#include <Path.h>
#include <Status.h>
#include <Threading.h>
#include <TypeTraits.h>
#include <Functory.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

namespace catena {

/**
 * @brief helper meta function to pass small types by value and complex
 * ones by reference.
*/
template<typename T>
struct PassByValueOrReference {
    using type = std::conditional_t<std::is_scalar<T>::value, T, T&>;
};

/**
 * @brief Value::KindCase looker upper
*/
template<typename V>
catena::Value::KindCase getKindCase([[maybe_unused]] V &src ) {
    return catena::Value::KindCase::KIND_NOT_SET;
}

/**
 * @brief type for indexing into parameters
 *
 */
using ParamIndex = uint32_t;

/**
 * @brief index value used to trigger special behaviors.
 *
 * getValue with this value specified as the index will get all values of an
 * array. setValue will append the value to the array
 *
 */
static constexpr ParamIndex kParamEnd = ParamIndex(-1);

class IParamAccessor {
public:
    /**
     * @brief type alias for the setter Functory for scalar types
    */
    using Setter = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*> ;
    
    /**
     * @brief type alias for the getter function for scalar types
    */
    using Getter = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*>;

    /**
     * @brief type alias for the setter Functory for vector types
    */
    using SetterAt = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*, const ParamIndex> ;

    /**
     * @brief type alias for the getter function for vector types
    */
    using GetterAt = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*, const ParamIndex>;


public:
  virtual ~IParamAccessor() = default;


  static bool inline registerSetter(Value::KindCase kind, Setter::Function func) {
    auto& setter = Setter::getInstance();
    return setter.addFunction(kind, func);
  }

  static bool inline registerGetter(Value::KindCase kind, Getter::Function func) {
    auto& getter = Getter::getInstance();
    return getter.addFunction(kind, func);
  }

  static bool inline registerSetterAt(Value::KindCase kind, SetterAt::Function func) {
    auto& setterAt = SetterAt::getInstance();
    return setterAt.addFunction(kind, func);
  }

  static bool inline registerGetterAt(Value::KindCase kind, GetterAt::Function func) {
    auto& getterAt = GetterAt::getInstance();
    return getterAt.addFunction(kind, func);
  }

};
/**
 * @brief wrapper around a catena::ParamAccessor stored in the device model
 *
 * Provides convenient accessor methods that are made threadsafe using
 * the DeviceModel's mutex.
 *
 * Enables client programs to "cache" a param without having to use a
 * method involving potentially expensive searches.
 *
 */
template <typename DM> class ParamAccessor : public IParamAccessor {
    // prevent use of this class outside of DeviceModel
    static_assert(std::is_same_v<DM, catena::DeviceModel<catena::Threading::kMultiThreaded>> ||
                    std::is_same_v<DM, catena::DeviceModel<catena::Threading::kSingleThreaded>>,
                  "Class Template Parameter must be of type DeviceModel<T>");

    public:
    /**
   * @brief Construct a new ParamAccessor object
   *
   * @param dm the parent device model.
   * @param p the parameter owned by the device model.
   */

    ParamAccessor(DM &dm, typename DM::ParamAccessorData &pad) noexcept
        : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)} {}

    /**
   * @brief ParamAccessor has no default constructor.
   *
   */
    ParamAccessor() = delete;

    /**
   * @brief ParamAccessor has copy semantics.
   *
   * @param other
   */
    ParamAccessor(const ParamAccessor &other) = default;

    /**
   * @brief ParamAccessor has copy semantics.
   *
   * @param rhs
   */
    ParamAccessor &operator=(const ParamAccessor &rhs) = default;

    /**
   * @brief ParamAccessor does not have move semantics.
   *
   * @param other
   */
    ParamAccessor(ParamAccessor &&other) = delete;

    /**
   * @brief ParamAccessor does not have move semantics
   *
   * @param rhs, right hand side of the equals sign
   */
    ParamAccessor &operator=(ParamAccessor &&rhs) = delete;

    inline ~ParamAccessor() {  // nothing to do
    }

    /**
   * @brief Get the value of the param referenced by this object
   *
   * Threadsafe because it asserts the DeviceModel's mutex.
   *
   * @param idx index into the array if parameter is an array type kParamEnd
   * will return all array elements.
   * @tparam V underlying value type of param
   * @return V value of parameter
   * @throws catena::exception_with_status if a type mismatch is detected, or
   * support for the type used has not been implemented.
   */
    template <typename V> V getValue([[maybe_unused]] ParamIndex idx = 0);

    /**
   * @brief Get the value of the array-type param referenced by this object
   *
   * Threadsafe because it asserts the DeviceModel's mutex.
   * @param idx index into the array
   * @throws catena::exception_with_status if idx is out of range, or value
   * referenced by this object is not a list type, or if support for the type
   * has not been implemented
   * @return V catena::Value set to type matching that of the param referenced
   * by this object.
   */
    catena::Value getValueAt(ParamIndex idx);

    /**
   * @brief Set the value of the stored catena::ParamAccessor.
   *
   * Threadsafe because it asserts the DeviceModel's mutex.
   * @param idx index into the array if parameter is an array type.
   * @tparam V type of the value stored by the param.
   * @param v value to set.
   */
    template <typename V> void setValue(V v, [[maybe_unused]] ParamIndex idx = kParamEnd);

    template <typename V> void setValueExperimental(const V &src) {
      typename DM::LockGuard lock(deviceModel_.get().mutex_);
      auto& setter = Setter::getInstance();
      if constexpr(catena::has_getType<V>) {
        const auto& typeInfo = src.getType();
        const char* base = reinterpret_cast<const char*>(&src);
        auto* dstFields = value_.get().mutable_struct_value()->mutable_fields();
        for (auto& field : typeInfo.fields) {
          const char* srcAddr = base + field.offset;
          Value* dstField;
          if (!dstFields->contains(field.name)){
            // dstFields->insert({field.name, StructField{}});
            // dstFields->at(field.name)->set_allocated_value(new Value{});
            // skip missing field for now
            // it's a debate whether fields that are not present should be added
          } else {
            dstField = dstFields->at(field.name).mutable_value();
            setter[dstField->kind_case()](dstField, srcAddr);
          }
        }
      } else {
        typename std::remove_const<typename std::remove_reference<decltype(src)>::type>::type x;
        setter[getKindCase(x)](&value_.get(), &src);
      }
    }

    template <typename V> void setValueAtExperimental(const V &src, const ParamIndex idx) {
      using ElementType = std::remove_const<typename std::remove_reference<decltype(src)>::type>::type;
      typename DM::LockGuard lock(deviceModel_.get().mutex_);
      auto& setter = SetterAt::getInstance();
      static std::vector<ElementType> x;
      setter[getKindCase(x)](&value_.get(), &src, idx);
    }

    template <typename V> void getValueExperimental(V &dst) {
      typename DM::LockGuard lock(deviceModel_.get().mutex_);
      auto& getter = Getter::getInstance();
      if constexpr(catena::has_getType<V>) {
        const auto& typeInfo = dst.getType();
        char* base = reinterpret_cast<char*>(&dst);
        auto* srcFields = value_.get().mutable_struct_value()->mutable_fields();
        for (auto& field : typeInfo.fields) {
          char* dstAddr = base + field.offset;
          if (srcFields->contains(field.name)) {
            Value* srcField = value_.get().mutable_struct_value()->mutable_fields()->at(field.name).mutable_value();
            getter[srcField->kind_case()](dstAddr, srcField);
          }
        }
      } else {
        getter[getKindCase<V>(dst)](&dst, &value_.get());
      }
    }

    template <typename V> void getValueAtExperimental(V &dst, const ParamIndex idx) {
      using ElementType = typename std::remove_reference<decltype(dst)>::type;
      typename DM::LockGuard lock(deviceModel_.get().mutex_);
      auto& getter = GetterAt::getInstance();
      static std::vector<ElementType> x;
      getter[getKindCase(x)](&dst, &value_.get(), idx);
    }
  
  private:
    std::reference_wrapper<DM> deviceModel_;
    std::reference_wrapper<catena::Param> param_;
    std::reference_wrapper<catena::Value> value_;
};

/**
 * @brief true if v is a list type
 *
 * @param v
 * @return true if v is a list type
 * @return false otherwise
 */
inline static bool isList(const catena::Value &v) {
    bool ans = false;
    ans = v.has_float32_array_values() || v.has_int32_array_values() || v.has_string_array_values() ||
          v.has_struct_array_values() || v.has_variant_array_values();
    return ans;
}
}  // namespace catena
