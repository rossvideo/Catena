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
#include <typeinfo>
#include <typeindex>

namespace catena {

/**
 * @brief helper meta function to pass small types by value and complex
 * ones by reference.
*/
template <typename T> struct PassByValueOrReference {
    using type = std::conditional_t<std::is_scalar<T>::value, T, T&>;
};

/**
 * @brief Value::KindCase looker upper
*/
template <typename V> catena::Value::KindCase getKindCase([[maybe_unused]] V& src) {
    if constexpr (has_getStructInfo<V>) {
        return catena::Value::KindCase::kStructValue;
    }
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

class ParamAccessor {
  public:
    /**
   * @brief Mutexlock type
   *
   */
    using Mutex = std::recursive_mutex;

    /**
   * @brief Lock Guard type
   *
   */
    using LockGuard = std::lock_guard<Mutex>;
    /**
     * @brief type alias for the setter Functory for scalar types
    */
    using Setter = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*>;

    /**
     * @brief type alias for the getter function for scalar types
    */
    using Getter = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*>;

    /**
     * @brief type alias for the setter Functory for vector types
    */
    using SetterAt = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*,
                                                const ParamIndex>;

    /**
     * @brief type alias for the getter function for vector types
    */
    using GetterAt = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*,
                                                const ParamIndex>;

    /**
     * @brief type alias for the function that gets the VariantInfo for a type
    */
    using VariantInfoGetter = catena::patterns::Functory<std::type_index, const VariantInfo&>;


  public:
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

    static bool inline registerVariantGetter(const std::type_index ti, VariantInfoGetter::Function func) {
        auto& getter = VariantInfoGetter::getInstance();
        return getter.addFunction(ti, func);
    }

  public:
    /**
   * @brief Construct a new ParamAccessor object
   *
   * @param dm the parent device model.
   * @param pad the data to initialize the param accessor
   */

    ParamAccessor(DeviceModel& dm, DeviceModel::ParamAccessorData& pad)
        : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)} {}

    /**
   * @brief ParamAccessor has no default constructor.
   *
   */
    ParamAccessor() = delete;

    /**
   * @brief ParamAccessor does not have copy semantics.
   *
   * @param other
   */
    ParamAccessor(const ParamAccessor& other) = delete;

    /**
   * @brief ParamAccessor does not have copy semantics.
   *
   * @param rhs
   */
    ParamAccessor& operator=(const ParamAccessor& rhs) = delete;

    /**
   * @brief ParamAccessor does not have move semantics.
   *
   * @param other
   */
    ParamAccessor(ParamAccessor&& other) = delete;

    /**
   * @brief ParamAccessor does not have move semantics
   *
   * @param rhs, right hand side of the equals sign
   */
    ParamAccessor& operator=(ParamAccessor&& rhs) = delete;

    inline ~ParamAccessor() {  // nothing to do
    }


    /**
    * @brief get accessor to the value of the parameter
   */
    inline Value& value() { return value_.get(); }
    inline const Value& value() const { return value_.get(); }

    /**
   *  @brief get accessor to sub-parameter matching fieldName
   *  @param fieldName name of the sub-parameter
   *  @return unique_ptr to ParamAccessor interface
   * 
   * Note that the value element of the returned sub-param references the appropriate
   * part of the _parent's_ value element. This is because the value element in the child
   * param descriptor is not part of the active value of the larger object. If even present,
   * it's used as the default value for the matching part of the parent's value object.
   */
    std::unique_ptr<ParamAccessor> subParam(const std::string& fieldName);
    

    /**
     * @brief get const accessor to sub-parameter matching fieldName
     * @param fieldName name of the sub-parameter
     * @return unique_ptr to ParamAccessor interface
     * 
     * See notes on value element of returned object in non-const version of this method.
    */
    const std::unique_ptr<ParamAccessor> subParam(const std::string& fieldName) const;

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

    /**
     * @brief Set the value of the stored parameter to which this object
     * provides access.
     * 
     * @tparam V type of the value stored by the param. This must be a native type.
    */
    template <typename V> void setValueNative(const V& src) {
        LockGuard lock(mutex_);
        auto& setter = Setter::getInstance();
        if constexpr (catena::has_getStructInfo<V>) {
            const auto& structInfo = src.getStructInfo();
            const char* base = reinterpret_cast<const char*>(&src);
            auto* dstFields = value_.get().mutable_struct_value()->mutable_fields();
            for (auto& field : structInfo.fields) {
                const char* srcAddr = base + field.offset;      
                if (!dstFields->contains(field.name)) {
                    dstFields->insert({field.name, StructField{}});
                    *dstFields->at(field.name).mutable_value() = Value{};
                    std::unique_ptr<ParamAccessor> sp = subParam(field.name);
                    field.wrapSetter(sp.get(), srcAddr);
                } else {
                    Value* dstField = dstFields->at(field.name).mutable_value();
                    if (dstField->kind_case() == Value::KindCase::kStructValue) {
                        // field is a struct
                        std::unique_ptr<ParamAccessor> sp = subParam(field.name);
                        field.wrapSetter(sp.get(), srcAddr);
                    } else {
                        // field is a simple or simple array type
                        
                        setter[dstField->kind_case()](dstField, srcAddr);
                    }
                }
            }
        } else if constexpr (catena::meta::is_variant<V>::value) {
            auto& variantInfoFunctory = catena::ParamAccessor::VariantInfoGetter::getInstance();
            const catena::VariantInfo& variantInfo = variantInfoFunctory[std::type_index(typeid(V))]();
            Value& v = value_.get();
            VariantValue* vv = v.mutable_variant_value();   
            std::string* currentVariant = vv->mutable_variant_type();
            const std::string& variant = variantInfo.lookup[src.index()];
            const std::unique_ptr<ParamAccessor> sp = subParam(variant);
            if (variant.compare(*currentVariant) != 0) {
                // we need to change the variant type in the protobuf
                *currentVariant = variant;
            } 
            variantInfo.members.at(variant).wrapSetter(sp.get(), &src);          
        } else {
            typename std::remove_const<typename std::remove_reference<decltype(src)>::type>::type x;
            setter[getKindCase(x)](&value_.get(), &src);
        }
    }

    /**
     * @brief Set the value of the stored parameter to which this object
     * provides access.
     * 
     * @tparam V type of the value stored by the param. This must be a native type.
    */
    template <typename V> void setValueAtNative(const V& src, const ParamIndex idx) {
        using ElementType = std::remove_const<typename std::remove_reference<decltype(src)>::type>::type;
        LockGuard lock(mutex_);
        auto& setter = SetterAt::getInstance();
        static std::vector<ElementType> x;
        setter[getKindCase(x)](&value_.get(), &src, idx);
    }

    /**
     * @brief Get the value of the stored parameter to which objects of
     * this class provide access.
     * 
     * @tparam V type of the value stored by the param. This must be a native type.
     * @param dst reference to the destination object written to by this method.
    */
    template <typename V> void getValueNative(V& dst) const {
        LockGuard lock(mutex_);
        auto& getter = Getter::getInstance();
        if constexpr (catena::has_getStructInfo<V>) {
            // dst is a struct
            const auto& structInfo = dst.getStructInfo();
            char* base = reinterpret_cast<char*>(&dst);
            auto* srcFields = value_.get().mutable_struct_value()->mutable_fields();
            for (auto& field : structInfo.fields) {
                char* dstAddr = base + field.offset;
                const Value& srcField = srcFields->at(field.name).value();
                if (srcFields->contains(field.name)) {
                    const catena::Value::KindCase kc = srcField.kind_case();
                    if (kc == Value::KindCase::kStructValue) {
                        // field is a struct
                        const std::unique_ptr<ParamAccessor> sp = subParam(field.name);
                        field.wrapGetter(dstAddr, sp.get());
                    } else {
                        // field is a simple or simple array type
                        getter[kc](dstAddr, &srcField);
                    }
                }
            }
        } else if constexpr (catena::meta::is_variant<V>::value) {
            // dst is a variant gather information about the protobuf source
            const VariantValue& src = value_.get().variant_value();
            const std::string& variant = src.variant_type();
            Value::KindCase kc = src.value().kind_case();

            // gather info about the native destination
            auto& variantInfoFunctory = catena::ParamAccessor::VariantInfoGetter::getInstance();
            const catena::VariantInfo& variantInfo = variantInfoFunctory[std::type_index(typeid(V))]();
            const catena::VariantMemberInfo vmi = variantInfo.members.at(variant);

            // set the variant to the correct type, and return a pointer to it
            void* ptr = vmi.set(&dst);
            if (kc == Value::KindCase::kStructValue) {
                // variant value is a struct
                const std::unique_ptr<ParamAccessor> sp = subParam(variant);
                vmi.wrapGetter(ptr, sp.get());
            } else if (kc == Value::KindCase::kVariantValue) {
                // variant value is a variant
                BAD_STATUS("Variant of variant not supported", catena::StatusCode::INVALID_ARGUMENT);
            } else {
                // field is a simple or simple array type
                getter[kc](reinterpret_cast<char*>(ptr), &src.value());
            }
        } else {
            // dst is a simple type
            getter[getKindCase<V>(dst)](&dst, &value_.get());
        }
    }

    /**
     * @brief Get the value of the stored parameter to which this object
     * provides access from an array with the specified index.
     * 
     * @tparam V type of the value stored by the param. This must be a native type.
     * @param dst reference to the destination object written to by this method.
     * @param idx index into the array
    */
    template <typename V> void getValueAtNative(V& dst, const ParamIndex idx) {
        using ElementType = typename std::remove_reference<decltype(dst)>::type;
        LockGuard lock(mutex_);
        auto& getter = GetterAt::getInstance();
        static std::vector<ElementType> x;
        getter[getKindCase(x)](&dst, &value_.get(), idx);
    }

  private:
    std::reference_wrapper<catena::DeviceModel> deviceModel_;
    std::reference_wrapper<catena::Param> param_;
    std::reference_wrapper<catena::Value> value_;
    mutable Mutex mutex_;
};

/**
 * @brief true if v is a list type
 *
 * @param v
 * @return true if v is a list type
 * @return false otherwise
 */
inline static bool isList(const catena::Value& v) {
    bool ans = false;
    ans = v.has_float32_array_values() || v.has_int32_array_values() || v.has_string_array_values() ||
          v.has_struct_array_values() || v.has_variant_array_values();
    return ans;
}
}  // namespace catena
