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

#include <Functory.h>
#include <DeviceModel.h>
#include <Path.h>
#include <Status.h>
#include <Threading.h>
#include <TypeTraits.h>



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

class ParamAccessor {
  public:
    /**
     * @brief Mutexlock type
     *
     */
    using Mutex = DeviceModel::Mutex;

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

    /**
     * @brief type alias for the function that gets values from the device model for delivery to attached clients
     */
    using ValueGetter = catena::patterns::Functory<catena::Value::KindCase, void, Value*, const Value&, ParamIndex>;

    /**
     * @brief type alias for the function that sets values in the device model in response to client requests
     */
    using ValueSetter = catena::patterns::Functory<catena::Value::KindCase, void, Value&, const Value&, ParamIndex>;
    
  public:
    /**
     * @brief Construct a new ParamAccessor object
     *
     * @param dm the parent device model.
     * @param pad the data to initialize the param accessor
     */

    ParamAccessor(DeviceModel& dm, DeviceModel::ParamAccessorData& pad);

    /**
     * @brief ParamAccessor has no default constructor.
     *
     */
    ParamAccessor() = delete;

    /**
     * @brief ParamAccessor copy constructor
     *
     * @param other
     */
    ParamAccessor(const ParamAccessor& other) = default;

    /**
     * @brief ParamAccessor copy assignment
     *
     * @param rhs
     */
    ParamAccessor& operator=(const ParamAccessor& rhs) = default;

    /**
     * @brief ParamAccessor move constructor.
     *
     * @param other
     */
    ParamAccessor(ParamAccessor&& other) = default;

    /**
     * @brief ParamAccessor move assignment
     *
     * @param rhs, right hand side of the equals sign
     */
    ParamAccessor& operator=(ParamAccessor&& rhs) = default;

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
     * 
     * This method is threadsafe because it asserts the DeviceModel's mutex.
     */
template <bool Threadsafe>
std::unique_ptr<ParamAccessor> subParam(const std::string& fieldName) {
    using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, FakeLock>;
    LockGuard lock(deviceModel_.get().mutex_);
    Param& parent = param_.get();
    Param& childParam = parent.mutable_params()->at(fieldName);
    Value& value = value_.get();
    DeviceModel::ParamAccessorData pad{};
    if (value.kind_case() == Value::KindCase::kStructValue) {
        // field is a struct
        Value* v = value.mutable_struct_value()->mutable_fields()->at(fieldName).mutable_value();
        pad = {&childParam, v};
    } else if (value.kind_case() == Value::KindCase::kVariantValue) {
        // field is a variant
        Value* v = value.mutable_variant_value()->mutable_value();
        pad = {&childParam, v};
    } else {
        // field is a simple or simple array type
        BAD_STATUS("subParam called on non-struct or variant type", catena::StatusCode::INVALID_ARGUMENT);
    }
    return std::unique_ptr<ParamAccessor>(new ParamAccessor{deviceModel_.get(), pad});
}


template <bool Threadsafe>
const std::unique_ptr<ParamAccessor> subParam(const std::string& fieldName) const {
    using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, FakeLock>;
    LockGuard lock(deviceModel_.get().mutex_);
    Param& parent = param_.get();
    const Param& childParam = parent.params().at(fieldName);
    const Value& value = value_.get();
    DeviceModel::ParamAccessorData pad{};
    if (value.kind_case() == Value::KindCase::kStructValue) {
        // field is a struct
        // yes the const_cast is gross, but ok because we re-apply constness on return
        const Value& v = value.struct_value().fields().at(fieldName).value();
        pad = {const_cast<Param*>(&childParam), const_cast<Value*>(&v)};
    } else if (value.kind_case() == Value::KindCase::kVariantValue) {
        // field is a variant
        // yes the const_cast is gross, but ok because we re-apply constness on return
        const Value& v = value.variant_value().value();
        pad = {const_cast<Param*>(&childParam), const_cast<Value*>(&v)};
    } else {
        // field is a simple or simple array type
        BAD_STATUS("subParam called on non-struct or variant type", catena::StatusCode::INVALID_ARGUMENT);
    }
    return std::unique_ptr<ParamAccessor>(new ParamAccessor{deviceModel_.get(), pad});
}

/**
 * @brief make a ParamAccessor with a value set to the index indicated
 * @tparam Threadsafe, whether or not the device model's lock will be asserted
 * @param idx index into the array
 * @throws catena::exception_with_status::INVALID_ARGUMENT if the param is not an array
 * @throws catena::exception_with_status::OUT_OF_RANGE if the index isn't valid
*/
// template <bool Threadsafe = true>
// std::unique_ptr<ParamAccessor> at(const ParamIndex idx) {
//     using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, FakeLock>;
//     LockGuard lock(deviceModel_.get().mutex_);
//     Param& parent = param_.get();
//     Value& value = value_.get();
//     DeviceModel::ParamAccessorData pad{};
//     if (catena::isList(value)) {
//         // field is a list
//         Value* v = nullptr;
//         switch (value.kind_case()) {
//         case Value::KindCase::kFloat32ArrayValues:
//             BAD_STATUS("Float array does not have sub-params.", catena::StatusCode::INVALID_ARGUMENT);
//             break;
//         case Value::KindCase::kInt32ArrayValues:
//             BAD_STATUS("Int32 array does not have sub-params.", catena::StatusCode::INVALID_ARGUMENT);
//             break;
//         case Value::KindCase::kStringArrayValues:
//             BAD_STATUS("String Array does not have sub-params.", catena::StatusCode::INVALID_ARGUMENT);
//             break;
//         case Value::KindCase::kStructArrayValues:
//             if (idx >= value.struct_array_values().struct_values_size()) {
//                 BAD_STATUS("index out of range", catena::StatusCode::OUT_OF_RANGE);
//             }
//             Value newValue{};
//             newValue.set_allocated_struct_value
//             v = value.mutable_struct_array_values()->mutable_struct_values();
//             break;
//         case Value::KindCase::kVariantArrayValues:
//             if (idx >= value.variant_array_values().size()) {
//                 BAD_STATUS("index out of range", catena::StatusCode::OUT_OF_RANGE);
//             }
//             v = value.mutable_variant_array_values()->mutable_values()->at(idx)
//             break;
//         default:
//             BAD_STATUS("at called on non-list type", catena::StatusCode::INVALID_ARGUMENT);
//         }
//         pad = {&parent, v};
//     } else {
//         BAD_STATUS("at called on non-list type", catena::StatusCode::INVALID_ARGUMENT);
//     }
//     return std::unique_ptr<ParamAccessor>(new ParamAccessor{deviceModel_.get(), pad});
// }

    /**
     * @brief Set the value of the stored parameter to which this object
     * provides access.
     *
     * @tparam V type of the value stored by the param. This must be a native type.
     */
    template <bool Threadsafe = true, typename V> void setValue(const V& src) {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, catena::FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
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
                    std::unique_ptr<ParamAccessor> sp = subParam<false>(field.name);
                    field.wrapSetter(sp.get(), srcAddr);
                } else {
                    Value* dstField = dstFields->at(field.name).mutable_value();
                    if (dstField->kind_case() == Value::KindCase::kStructValue) {
                        // field is a struct
                        std::unique_ptr<ParamAccessor> sp = subParam<false>(field.name);
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
            const std::unique_ptr<ParamAccessor> sp = subParam<false>(variant);
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
    template <bool Threadsafe = true, typename V> void setValueAt(const V& src, const ParamIndex idx) {
        using ElementType = std::remove_const<typename std::remove_reference<decltype(src)>::type>::type;
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, catena::FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
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
    template <bool Threadsafe = true, typename V> void getValue(V& dst) const {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, catena::FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
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
                        const std::unique_ptr<ParamAccessor> sp = subParam<false>(field.name);
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
                const std::unique_ptr<ParamAccessor> sp = subParam<false>(variant);
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
    template <bool Threadsafe = true, typename V> void getValueAt(V& dst, const ParamIndex idx) {
        using ElementType = typename std::remove_reference<decltype(dst)>::type;
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, catena::FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
        auto& getter = GetterAt::getInstance();
        static std::vector<ElementType> x;
        getter[getKindCase(x)](&dst, &value_.get(), idx);
    }

    /**
     * @brief get the parameter's value packaged as a catena::Value object for
     * sending to a client.
     * 
     * @param dst [out] destination for the value
     * @param idx [in] index into the array, if set to kParamEnd, the entire array is returned
     */
    void getValue(Value *dst, [[maybe_unused]] ParamIndex idx) const; 

    void setValue(const Value& src, [[maybe_unused]] ParamIndex idx);

  private:
    std::reference_wrapper<catena::DeviceModel> deviceModel_;
    std::reference_wrapper<catena::Param> param_;
    std::reference_wrapper<catena::Value> value_;
};


}  // namespace catena
