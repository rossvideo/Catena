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
#include <sstream>
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
template <typename V> catena::Value::KindCase getKindCase(const V& src);

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
          v.has_struct_array_values() || v.has_struct_variant_array_values();
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
     * @brief type alias for the getter function for scalar types
     */
    using Getter = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*>;

    /**
     * @brief type alias for the setter Functory for scalar types
     */
    using Setter = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*>;

    /**
     * @brief type alias for the getter function for vector types
     */
    using GetterAt = catena::patterns::Functory<catena::Value::KindCase, void, void*, const catena::Value*,
                                                const ParamIndex>;

    /**
     * @brief type alias for the setter Functory for vector types
     */
    using SetterAt = catena::patterns::Functory<catena::Value::KindCase, void, catena::Value*, const void*,
                                                const ParamIndex>;

    /**
     * @brief type alias for the function that gets the VariantInfo for a type
     */
    using VariantInfoGetter = catena::patterns::Functory<std::type_index, const VariantInfo&>;

    /**
     * @brief type alias for the function that gets values from the device model for delivery to attached
     * clients
     */
    using ValueGetter =
      catena::patterns::Functory<catena::Value::KindCase, void, Value*, const Value&>;

    /**
     * @brief type alias for the function that sets values in the device model in response to client requests
     */
    using ValueSetter =
      catena::patterns::Functory<catena::Value::KindCase, void, Value&, const Value&>;

    /**
     * @brief type alias for the function that gets values from the device model for delivery to attached
     * clients
     */
    using ValueGetterAt =
      catena::patterns::Functory<catena::Value::KindCase, void, Value*, const Value&, ParamIndex>;

    /**
     * @brief type alias for the function that sets values in the device model in response to client requests
     */
    using ValueSetterAt =
      catena::patterns::Functory<catena::Value::KindCase, void, Value&, const Value&, ParamIndex>;

  public:
    /**
     * @brief Construct a new ParamAccessor object
     *
     * @param dm the parent device model.
     * @param pad the data to initialize the param accessor
     */

    ParamAccessor(DeviceModel& dm, DeviceModel::ParamAccessorData& pad, const std::string& oid, const std::string& scope);

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

    inline virtual ~ParamAccessor() {  // nothing to do
    }


    /**
     * @brief get accessor to the value of the parameter
     */
    // inline Value& value() { 
    //     std::lock_guard<Mutex> lock(deviceModel_.get().mutex_);
    //     return value_.get(); 
    // }

    template <bool Threadsafe = true>
    inline const Value& value() const {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
        return value_.get(); 
    }

    /**
     * @brief get accessor to sub-parameter matching fieldName
     * @param fieldName name of the sub-parameter
     * @return unique_ptr to ParamAccessor interface
     * @throws catena::exception_with_status if fieldName is not a sub-parameter
     * @throws catena::exception_with_status if fieldName is not a struct or variant type
     *
     * Note that the value element of the returned sub-param references the appropriate
     * part of the _parent's_ value element. This is because the value element in the child
     * param descriptor is not part of the active value of the larger object. If even present,
     * it's used as the default value for the matching part of the parent's value object.
     *
     * This method is threadsafe because it asserts the DeviceModel's mutex.
     */
    template <bool Threadsafe> std::unique_ptr<ParamAccessor> subParam(const std::string& fieldName) {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
        Param& parent = param_.get();
        Param& childParam = parent.mutable_params()->at(fieldName);
        Value& value = value_.get();
        std::string scope = childParam.access_scope();
        if (scope == "") {
            scope = parent.access_scope();
        }
        DeviceModel::ParamAccessorData pad{};
        if (value.kind_case() == Value::KindCase::kStructValue) {
            // field is a struct
            if (!value.struct_value().fields().contains(fieldName)) {
                BAD_STATUS("subParam called on non-existent field", catena::StatusCode::INVALID_ARGUMENT);
            }
            Value* v = value.mutable_struct_value()->mutable_fields()->at(fieldName).mutable_value();
            pad = {&childParam, v};
        } else if (value.kind_case() == Value::KindCase::kStructVariantValue) {
            // field is a variant
            Value* v = value.mutable_struct_variant_value()->mutable_value();
            pad = {&childParam, v};
        } else {
            // field is a simple or simple array type
            BAD_STATUS("subParam called on non-struct or variant type", catena::StatusCode::INVALID_ARGUMENT);
        }
        return std::unique_ptr<ParamAccessor>(new ParamAccessor{deviceModel_.get(), pad, oid_ + "/" + fieldName, scope});
    }

    /**
     * @brief get accessor to sub-parameter matching fieldName, const version
     * @param fieldName name of the sub-parameter
     * @return unique_ptr to ParamAccessor interface
     * @throws catena::exception_with_status if fieldName is not a sub-parameter
     * @throws catena::exception_with_status if fieldName is not a struct or variant type
     *
     * Note that the value element of the returned sub-param references the appropriate
     * part of the _parent's_ value element. This is because the value element in the child
     * param descriptor is not part of the active value of the larger object. If even present,
     * it's used as the default value for the matching part of the parent's value object.
     *
     * This method is threadsafe because it asserts the DeviceModel's mutex.
     */
    template <bool Threadsafe>
    const std::unique_ptr<ParamAccessor> subParam(const std::string& fieldName) const {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
        Param& parent = param_.get();
        const Param& childParam = parent.params().at(fieldName);
        const Value& value = value_.get();
        std::string scope = childParam.access_scope();
        if (scope == "") {
            scope = parent.access_scope();
        }
        DeviceModel::ParamAccessorData pad{};
        if (value.kind_case() == Value::KindCase::kStructValue) {
            // field is a struct
            // yes the const_cast is gross, but ok because we re-apply constness on return
            if (!value.struct_value().fields().contains(fieldName)) {
                BAD_STATUS("subParam called on non-existent field", catena::StatusCode::INVALID_ARGUMENT);
            }
            const Value& v = value.struct_value().fields().at(fieldName).value();
            pad = {const_cast<Param*>(&childParam), const_cast<Value*>(&v)};
        } else if (value.kind_case() == Value::KindCase::kStructVariantValue) {
            // field is a variant
            // yes the const_cast is gross, but ok because we re-apply constness on return
            const Value& v = value.struct_variant_value().value();
            pad = {const_cast<Param*>(&childParam), const_cast<Value*>(&v)};
        } else {
            // field is a simple or simple array type
            BAD_STATUS("subParam called on non-struct or variant type", catena::StatusCode::INVALID_ARGUMENT);
        }
        return std::unique_ptr<ParamAccessor>(new ParamAccessor{deviceModel_.get(), pad, oid_ + "/" + fieldName, scope});
    }


    /**
     * @brief Get the value of the stored parameter to which objects of
     * this class provide access.
     *
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for V is not
     * implemented, or with catena::Status::UNKNOWN if an unknown exception is thrown is encountered. 
     * Other catena::exception_with_status exceptions are re-thrown with no change to their status.
     * 
     * @tparam V type of the value stored by the param. This must be a native type.
     * @param dst reference to the destination object written to by this method.
     */
    template <bool Threadsafe = true, typename V> 
    void getValue(V& dst) const {
        try {
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
                const StructVariantValue& src = value_.get().struct_variant_value();
                const std::string& variant = src.struct_variant_type();
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
                } else if (kc == Value::KindCase::kStructVariantValue) {
                    // variant value is a variant
                    BAD_STATUS("Variant of variant not supported", catena::StatusCode::INVALID_ARGUMENT);
                } else {
                    // field is a simple or simple array type
                    getter[kc](reinterpret_cast<char*>(ptr), &src.value());
                }
            } else {
                // dst is a simple type or whole array
                getter[getKindCase<V>(dst)](&dst, &value_.get());
            }
        } catch (const catena::exception_with_status& why) {
            std::stringstream err;
            err << "getValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), why.status);
        } catch (const std::runtime_error& why) {
            // most likely thrown by the getter encountering a missing function
            std::stringstream err;
            err << "getValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNIMPLEMENTED);
        } catch (...) {
            std::stringstream err;
            err << "getValue failed with unknown exception: " << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNKNOWN);
        }
    }

    /**
     * @brief Get the value of the stored parameter to which this object
     * provides access from an array with the specified index.
     * 
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for
     * objects of type V is not supported, or with catena::Status::UNKNOWN if an unknown exception
     * is encountered, or catena::Status::RANGE_ERROR if the index is out of range.
     *
     * @tparam V type of the value stored by the param. This must be a native type.
     * @param dst reference to the destination object written to by this method.
     * @param idx index into the array
     */
    template <bool Threadsafe = true, typename V> 
    void getValue(V& dst, const ParamIndex idx) const {
        try {
            using ElementType = typename std::remove_reference<decltype(dst)>::type;
            using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, catena::FakeLock>;
            LockGuard lock(deviceModel_.get().mutex_);
            auto& getter = GetterAt::getInstance();
            static std::vector<ElementType> x;
            getter[getKindCase(x)](&dst, &value_.get(), idx);
        } catch (const catena::exception_with_status& why) {
            std::stringstream err;
            err << "getValueAt failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), why.status);
        } catch (const std::runtime_error& why) {
            // most likely thrown by the getter encountering a missing function
            std::stringstream err;
            err << "getValueAt failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNIMPLEMENTED);
        } catch (...) {
            std::stringstream err;
            err << "getValueAt failed with unknown exception: " << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNKNOWN);
        }
    }

    /**
     * @brief Set the value of the stored parameter to which this object
     * provides access.
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for V is not implemented, 
     * or with catena::Status::UNKNOWN if an unknown exception is thrown is encountered. 
     * Other catena::exception_with_status exceptions are re-thrown with no change to their status.
     *
     * @tparam V type of the value stored by the param. This must be a native type.
     */
    template <bool Threadsafe = true, typename V> 
    void setValue(const V& src) {
        try {
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
                StructVariantValue* vv = v.mutable_struct_variant_value();
                std::string* currentVariant = vv->mutable_struct_variant_type();
                const std::string& variant = variantInfo.lookup[src.index()];
                const std::unique_ptr<ParamAccessor> sp = subParam<false>(variant);
                if (variant.compare(*currentVariant) != 0) {
                    // we need to change the variant type in the protobuf
                    *currentVariant = variant;
                }
                variantInfo.members.at(variant).wrapSetter(sp.get(), &src);
            } else {
                setter[getKindCase<V>(src)](&value_.get(), &src);
            }
            deviceModel_.get().valueSetByService(*this, kParamEnd);
        } catch (const catena::exception_with_status& why) {
            std::stringstream err;
            err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), why.status);
        } catch (const std::runtime_error& why) {
            // most likely thrown by the setter encountering a missing function
            std::stringstream err;
            err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNIMPLEMENTED);
        } catch (...) {
            std::stringstream err;
            err << "setValue failed with unknown exception: " << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNKNOWN);
        }
    }

    /**
     * @brief Set the value of the stored parameter to which this object
     * provides access.
     * 
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for V is not implemented, 
     * or with catena::Status::UNKNOWN if an unknown exception is thrown is encountered. 
     * Other catena::exception_with_status exceptions are re-thrown with no change to their status.
     * 
     * @tparam V type of the value stored by the param. This must be a native type.
     * @tparam Threadsafe if true, the method will assert the DeviceModel's mutex.
     * @param src the value to set the parameter owned by the ParamAccessor to.
     * @param idx index into the array, if set to kParamEnd, the value is appended to the array
     */
    template <bool Threadsafe = true, typename V> 
    void setValue(const V& src, const ParamIndex idx) {
        try {
            using ElementType = std::remove_const<typename std::remove_reference<decltype(src)>::type>::type;
            using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<Mutex>, catena::FakeLock>;
            LockGuard lock(deviceModel_.get().mutex_);
            auto& setterAt = SetterAt::getInstance();
            static std::vector<ElementType> x;
            setterAt[getKindCase(x)](&value_.get(), &src, idx);
            deviceModel_.get().valueSetByService(*this, idx);
        } catch (const catena::exception_with_status& why) {
            std::stringstream err;
            err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), why.status);
        } catch (const std::runtime_error& why) {
            // most likely thrown by the setter encountering a missing function
            std::stringstream err;
            err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNIMPLEMENTED);
        } catch (...) {
            std::stringstream err;
            err << "setValue failed with unknown exception: " << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNKNOWN);
        }
    }

    /**
     * @brief get the parameter's value packaged as a catena::Value object for
     * sending to a client.
     *
     * @param dst [out] destination for the value
     * @param idx [in] index into the array, if set to kParamEnd, the entire array is returned
     * 
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for
     * the parameter type is not implemented, or with catena::Status::UNKNOWN if an unknown exception
     * is encountered, or with catena::Status::RANGE_ERROR if the index is out of range.
     * 
     * @tparam Threadsafe if true, the method will assert the DeviceModel's mutex. If false,
     * no lock is asserted - use when making recursive calls to avoid deadlock.
     */
    template<bool Threadsafe = true> 
    void getValue(Value* dst) const {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<DeviceModel::Mutex>, FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
        auto& getter = ValueGetter::getInstance();
        try {
            const Value& value = value_.get();
            getter[value.kind_case()](dst, value);
        } catch (const catena::exception_with_status& why) {
            std::stringstream err;
            err << "getValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), why.status);
        } catch (...) {
            std::stringstream err;
            err << "getValue failed for with uknown exception" << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNKNOWN);
        }
    }

    /**
     * @brief get the parameter's value packaged as a catena::Value object for
     * sending to a client.
     *
     * @param dst [out] destination for the value
     * @param idx [in] index into the array, if set to kParamEnd, the entire array is returned
     * @param clientScopes [in] the scopes of the client requesting the value
     * 
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for
     * the parameter type is not implemented, or with catena::Status::UNKNOWN if an unknown exception
     * is encountered, or with catena::Status::RANGE_ERROR if the index is out of range.
     * @throws catena::exception_with_status catena::Status::PERMISSION_DENIED if the client is not authorized
     * 
     * @tparam Threadsafe if true, the method will assert the DeviceModel's mutex. If false,
     * no lock is asserted - use when making recursive calls to avoid deadlock.
     */
    template<bool Threadsafe = true> 
    void getValue(Value* dst, ParamIndex idx, std::vector<std::string> clientScopes) const {
        using LockGuard = std::conditional_t<Threadsafe, std::lock_guard<DeviceModel::Mutex>, FakeLock>;
        LockGuard lock(deviceModel_.get().mutex_);
        try {
            if (std::find(clientScopes.begin(), clientScopes.end(), scope_) == clientScopes.end()) {
                BAD_STATUS("Not authorized to access this parameter", catena::StatusCode::PERMISSION_DENIED);
            }

            const Value& value = value_.get();
            if (isList() && idx != kParamEnd) {
                auto& getterAt = ValueGetterAt::getInstance();
                getterAt[value.kind_case()](dst, value, idx);
            } else {
                // value is a scalar or whole array type
                auto& getter = ValueGetter::getInstance();
                getter[value.kind_case()](dst, value);
            }
        } catch (const catena::exception_with_status& why) {
            std::stringstream err;
            err << "getValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), why.status);
        } catch (...) {
            std::stringstream err;
            err << "getValue failed for with uknown exception" << '\n' << __PRETTY_FUNCTION__ << '\n';
            throw catena::exception_with_status(err.str(), catena::StatusCode::UNKNOWN);
        }
    }

    /**
     * @brief set the parameter's value packaged as a catena::Value object most likely 
     * received from client
     *
     * @param dst [out] destination for the value
     * @param idx [in] index into the array, if set to kParamEnd, the entire array is returned
     * 
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for
     * the parameter type is not implemented, or with catena::Status::UNKNOWN if an unknown exception
     * is encountered, or with catena::Status::RANGE_ERROR if the index is out of range.
     * 
     * Threadsafe - asserts a lock on the DeviceModel's mutex.
     */
    void setValue(const std::string& peer, const Value& src);

    /**
     * @brief set the parameter's value packaged as a catena::Value object most likely 
     * received from client
     *
     * @param dst [out] destination for the value
     * @param idx [in] index into the array, if set to kParamEnd, the entire array is returned
     * @param clientScopes [in] the scopes of the client requesting the value
     * 
     * @throws catena::exception_with_status catena::Status::UNIMPLEMENTED if support for
     * the parameter type is not implemented, or with catena::Status::UNKNOWN if an unknown exception
     * is encountered, or with catena::Status::RANGE_ERROR if the index is out of range.
     * 
     * Threadsafe - asserts a lock on the DeviceModel's mutex.
     */
    void setValue(const std::string& peer, const Value& src, ParamIndex idx, std::vector<std::string>& clientScopes);

    /** 
     * @brief get the parameter's fully qualified object id
    */
    inline const std::string& oid() const {
        return oid_; 
    }

    /**
     * @brief get the parameter's unique id
     */ 
    inline size_t id() const {
        return id_;
    }

    /**
     * @brief comparison operator, returns true if the 2 ids are equal
    */
    inline bool operator==(const ParamAccessor& rhs) const {
        return id_ == rhs.id_;
    }

    /**
     * @brief returns true if the param is an array (list in protobuf) type.
    */
    inline bool isList() const {
        return catena::isList(value_.get());
    }

  private:
    /** @brief a reference to the device model that contains accessed parrameter */
    std::reference_wrapper<catena::DeviceModel> deviceModel_;

    /** @brief a reference to the parameter accessed by this object */
    std::reference_wrapper<catena::Param> param_;

    /** @brief a reference to the accessed parameter's value object */
    std::reference_wrapper<catena::Value> value_;

    /** @brief the accessed parameter's fully qualified object id*/
    std::string oid_;

    /** @brief the accessed parameter's access scope */
    std::string scope_;

    /** 
     * @brief a unique id (probability of non-uniqueness is approx 1:10^19). 
     * Motivation - to allow for fast comparison of ParamAccessor objects and
     * to only compute the hash once.
     * 
    */
    size_t id_;
};
}  // namespace catena
