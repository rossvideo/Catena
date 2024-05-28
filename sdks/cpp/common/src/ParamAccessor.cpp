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

#include <ParamAccessor.h>
#include <DeviceModel.h>
#include <Path.h>
#include <utils.h>
#include <ArrayAccessor.h>
#include <TypeTraits.h>

#include <google/protobuf/map.h>
#include <google/protobuf/util/json_util.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

using catena::ParamAccessor;
using catena::ParamIndex;
using VP = catena::Value *;  // shared value pointer
using catena::DeviceModel;
using catena::Param;
using catena::Value;
using KindCase = Value::KindCase;

/**
 * @brief A macro used to add getter functions for simple data types
*/
#define REGISTER_GETTER(kind_case, get_method, cast) \
getter.addFunction( kind_case, [](void *dstAddr, const Value *src) { \
    *reinterpret_cast<cast *>(dstAddr) = src->get_method(); \
})

/**
 * @brief A macro used to add setter functions for simple data types
*/
#define REGISTER_SETTER(kind_case, set_method, cast) \
setter.addFunction( kind_case, [](Value *dst, const void *srcAddr) { \
    dst->set_method(*reinterpret_cast<const cast*>(srcAddr));  \
})

/**
 * @brief A macro used to add getter functions for array of simple data types
*/
#define REGISTER_ARRAY_GETTER(kind_case, cast, array_values_method, access_method) \
getter.addFunction(kind_case, [](void *dstAddr, const Value *src) { \
    auto *dst = reinterpret_cast<std::vector<cast>*>(dstAddr); \
    dst->clear(); \
    auto &arr = src->array_values_method().access_method(); \
    for (auto it = arr.begin(); it != arr.end(); ++it) { \
        dst->push_back(*it); \
    } \
})

/**
 * @brief A macro used to add setter functions for array of simple data types
*/
#define REGISTER_ARRAY_SETTER(kind_case, mutable_array_method, clear_method, cast, add_method) \
setter.addFunction(kind_case, [](catena::Value *dst, const void *srcAddr) { \
    dst->mutable_array_method()->clear_method(); \
    auto *src = reinterpret_cast<const std::vector<cast> *>(srcAddr); \
    for (auto &it : *src) { \
        dst->mutable_array_method()->add_method(it); \
    } \
})

/**
 * @brief A macro used to add getter functions for element of array of simple data types
*/
#define REGISTER_ARRAY_GETTER_AT(kind_case, cast, array_values_method, size_method, access_method) \
getterAt.addFunction(kind_case, \
    [](void *dstAddr, const Value *src, const catena::ParamIndex idx) { \
        auto *dst = reinterpret_cast<cast *>(dstAddr); \
        if (idx >= src->array_values_method().size_method()) { \
            /* range error  */ \
            std::stringstream err; \
            err << "array index is out of bounds, " << idx \
                << " >= " << src->array_values_method().size_method(); \
            BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
        } else { \
            /* update array element */ \
            *dst = src->array_values_method().access_method(idx); \
        } \
    } \
)

/**
 * @brief A macro used to add setter functions for element of array of simple data types
*/
#define REGISTER_ARRAY_SETTER_AT(kind_case, cast, mutable_array_values_method, size_method, set_method) \
setterAt.addFunction(kind_case, \
    [](Value *dst, const void *srcAddr, catena::ParamIndex idx) { \
        auto *src = reinterpret_cast<const cast *>(srcAddr); \
        if (idx >= dst->mutable_array_values_method()->size_method()) { \
            /* range error */ \
            std::stringstream err; \
            err << "array index is out of bounds, " << idx \
                << " >= " << dst->mutable_array_values_method()->size_method(); \
            BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
        } else { \
            /* update array element */ \
            dst->mutable_array_values_method()->set_method(idx, *src); \
        } \
    } \
)



/**
 * @brief A macro used to add getter functions accessible by clients
*/
#define REGISTER_VALUE_GETTER(kind_case) \
valueGetter.addFunction(kind_case, [](Value* dst, const Value &src) -> void { \
    dst->CopyFrom(src); \
})

/**
 * @brief A macro used to add setter functions accessible by clients
*/
#define REGISTER_VALUE_SETTER(kind_case, dst_method, src_method) \
valueSetter.addFunction(kind_case, [](Value &dst, const Value &src) -> void { \
    dst.CopyFrom(src); \
})

/**
 * @brief A macro used to add getter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_GETTER(kind_case) \
valueGetter.addFunction(kind_case, [](Value* dst, const Value &src) -> void { \
    dst->CopyFrom(src); \
})

/**
 * @brief A macro used to add setter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_SETTER(kind_case) \
valueSetter.addFunction(kind_case, [](Value &dst, const Value &src) -> void { \
    dst.CopyFrom(src); \
})

/**
 * @brief A macro used to add getter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_GETTER_AT(kind_case, array_val_method, size_method, set_value_method, index_method) \
valueGetterAt.addFunction(kind_case, [](Value* dst, const Value &val, ParamIndex idx) -> void { \
    auto size = val.array_val_method().size_method(); \
    if (idx >= size) { \
        std::stringstream err; \
        err << "array index is out of bounds, " << idx \
            << " >= " << size; \
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
    } \
    dst->set_value_method(val.array_val_method().index_method(idx)); \
})

/**
 * @brief A macro used to add setter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_SETTER_AT(kind_case, array_val_method, size_method, mutable_array_val_method, mutable_method, value_method) \
valueSetterAt.addFunction(kind_case, [](Value &dst, const Value &src, ParamIndex idx) -> void { \
    auto size = dst.array_val_method().size_method(); \
    if (idx >= size) { \
        std::stringstream err; \
        err << "array index is out of bounds, " << idx \
            << " >= " << size; \
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
    } \
    dst.mutable_array_val_method()->mutable_method()->at(idx) = src.value_method(); \
})

// ParamAccessor::Setter::Protector setterProtector;
// static ParamAccessor::Setter setter(setterProtector);
// ParamAccessor::Getter::Protector getterProtector;
// static ParamAccessor::Getter getter(getterProtector);

int applyIntConstraint(catena::Param &param, int v) {
    /// @todo: add warning log for invalid constraint
    if (param.has_constraint()) {
        // apply the constraint
        int constraint_type = param.constraint().type();

        switch (constraint_type) {
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_RANGE:
                v = std::clamp(v, param.constraint().int32_range().min_value(),
                               param.constraint().int32_range().max_value());
                break;
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_CHOICE:
                if (std::find_if(param.constraint().int32_choice().choices().begin(),
                                 param.constraint().int32_choice().choices().end(),
                                 [&](catena::Int32ChoiceConstraint_IntChoice const &c) {
                                     return c.value() == v;
                                 }) == param.constraint().int32_choice().choices().end()) {

                    // if value is not in constraint, choose first item in list
                    v = param.constraint().int32_choice().choices(0).value();
                }
                break;
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_ALARM_TABLE: {
                // e.g. for bit_value of 1 and 3, bit_location = 1010
                int bit_location = 0;
                for (const auto &it : param.constraint().alarm_table().alarms()) {
                    bit_location |= (1 << it.bit_value());
                }

                /*
                 * assume you have three binary numbers, x=1010, y=0111 and t=1010
                 * if the corresponding bit in t is 1 then take the corresponding bit in x,
                 * if the corresponding bit in t is 0, then take the corresponding bit in y
                 *
                 * so result = 0010
                 */

                v = (bit_location & v) | ((~bit_location) & param.value().int32_value());

            }; break;
            default:
                std::stringstream err;
                err << "invalid constraint for int32: " << constraint_type << '\n';
                BAD_STATUS((err.str()), catena::StatusCode::OUT_OF_RANGE);
        }
    }
    return v;
}

std::string applyStringConstraint(catena::Param &param, std::string v) {
    /// @todo: add warning log for invalid constraint
    if (param.has_constraint()) {
        // apply the constraint
        int constraint_type = param.constraint().type();

        switch (constraint_type) {
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_STRING_CHOICE:
                if (param.constraint().string_choice().strict()) {
                    if (std::find_if(param.constraint().string_choice().choices().begin(),
                                     param.constraint().string_choice().choices().end(),
                                     [&](std::string const &c) { return c == v; }) ==
                        param.constraint().string_choice().choices().end()) {

                        // if value is not in constraint, choose first item in list
                        v = param.constraint().string_choice().choices(0);
                    }
                }
                break;
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_STRING_STRING_CHOICE:
                if (param.constraint().string_string_choice().strict()) {
                    if (std::find_if(param.constraint().string_string_choice().choices().begin(),
                                     param.constraint().string_string_choice().choices().end(),
                                     [&](catena::StringStringChoiceConstraint_StringStringChoice const &c) {
                                         return c.value() == v;
                                     }) == param.constraint().string_string_choice().choices().end()) {
                        // if value is not in constraint, choose first item in list
                        v = param.constraint().string_string_choice().choices(0).value();
                    }
                }
                break;
            default:
                std::stringstream err;
                err << "invalid constraint for string: " << constraint_type << '\n';
                BAD_STATUS((err.str()), catena::StatusCode::OUT_OF_RANGE);
        }
    }
    return v;
}


ParamAccessor::ParamAccessor(DeviceModel &dm, DeviceModel::ParamAccessorData &pad, const std::string& oid, const std::string& scope)
    : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)}, oid_{oid}, id_{std::hash<std::string>{}(oid)}, scope_{scope} {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;  // so we only do this once

        // get our functories
        auto &getter = Getter::getInstance();
        auto &setter = Setter::getInstance();
        auto &getterAt = GetterAt::getInstance();
        auto &setterAt = SetterAt::getInstance();
        auto &variantGetter = VariantInfoGetter::getInstance();
        auto &valueGetter = ValueGetter::getInstance();
        auto &valueSetter = ValueSetter::getInstance();
        auto &valueGetterAt = ValueGetterAt::getInstance();
        auto &valueSetterAt = ValueSetterAt::getInstance();

        // register int getter
        REGISTER_GETTER(KindCase::kInt32Value, int32_value, int32_t);

        // register float getter
        REGISTER_GETTER(KindCase::kFloat32Value, float32_value, float);

        // register string getter
        REGISTER_GETTER(KindCase::kStringValue, string_value, std::string);

        // register int32 setter
        REGISTER_SETTER(KindCase::kInt32Value, set_int32_value, int32_t);

        // register float32 setter
        REGISTER_SETTER(KindCase::kFloat32Value, set_float32_value, float);

        // register string setter
        REGISTER_SETTER(KindCase::kStringValue, set_string_value, std::string);

        // register array of int getter
        REGISTER_ARRAY_GETTER(KindCase::kInt32ArrayValues, int32_t, int32_array_values, ints);

        // register array of float getter
        REGISTER_ARRAY_GETTER(KindCase::kFloat32ArrayValues, float, float32_array_values, floats);

        // register array of string getter
        REGISTER_ARRAY_GETTER(KindCase::kStringArrayValues, std::string, string_array_values, strings);

        // register array of int setter
        REGISTER_ARRAY_SETTER(KindCase::kInt32ArrayValues, mutable_int32_array_values, clear_ints, int32_t, add_ints);

        //register array of float setter
        REGISTER_ARRAY_SETTER(KindCase::kFloat32ArrayValues, mutable_float32_array_values, clear_floats, float, add_floats);

        //register array of string setter
        REGISTER_ARRAY_SETTER(KindCase::kStringArrayValues, mutable_string_array_values, clear_strings, std::string, add_strings);
    
        // register element of array of int getter
        REGISTER_ARRAY_GETTER_AT(KindCase::kInt32ArrayValues, int32_t, int32_array_values, ints_size, ints);

        // register element of array of float getter
        REGISTER_ARRAY_GETTER_AT(KindCase::kFloat32ArrayValues, float, float32_array_values, floats_size, floats);

        // register element of array of string getter
        REGISTER_ARRAY_GETTER_AT(KindCase::kStringArrayValues, std::string, string_array_values, strings_size, strings);

        // register element of array of int setter
        REGISTER_ARRAY_SETTER_AT(KindCase::kInt32ArrayValues, int32_t, mutable_int32_array_values, ints_size, set_ints);

        // register element of array of float setter
        REGISTER_ARRAY_SETTER_AT(KindCase::kFloat32ArrayValues, float, mutable_float32_array_values, floats_size, set_floats);

        // register element of array of string setter
        REGISTER_ARRAY_SETTER_AT(KindCase::kStringArrayValues, std::string, mutable_string_array_values, strings_size, set_strings);
        
        
        /** Register Value Getters/Setters */


        //register value getter for int32
        REGISTER_VALUE_GETTER(KindCase::kInt32Value);

        //register value getter for float32
        REGISTER_VALUE_GETTER(KindCase::kFloat32Value);

        //register value getter for string
        REGISTER_VALUE_GETTER(KindCase::kStringValue);

        //register value getter for struct
        REGISTER_VALUE_GETTER(KindCase::kStructValue);

        // register value setter for int32
        REGISTER_VALUE_SETTER(KindCase::kInt32Value, set_int32_value, int32_value);

        // register value setter for float32
        REGISTER_VALUE_SETTER(KindCase::kFloat32Value, set_float32_value, float32_value);

        // register value setter for string
        REGISTER_VALUE_SETTER(KindCase::kStringValue, set_string_value, string_value);

        // register value getter for int32 array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kInt32ArrayValues);

        // register value getter for float32 array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kFloat32ArrayValues);

        // register value getter for string array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kStringArrayValues);

        // register value getter for struct array
        REGISTER_ARRAY_VALUE_GETTER(KindCase::kStructArrayValues);

        // register value setter for int32 array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kInt32ArrayValues);

        // register value setter for float32 array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kFloat32ArrayValues);

        // register value setter for string array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kStringArrayValues);

        // register value setter for struct array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kStructArrayValues);

        // register value setter for variant array
        REGISTER_ARRAY_VALUE_SETTER(KindCase::kStructVariantArrayValues);

        // register value getter for element of int32 array
        REGISTER_ARRAY_VALUE_GETTER_AT(KindCase::kInt32ArrayValues, int32_array_values, ints_size, set_int32_value, ints);

        // register value getter for element of float array
        REGISTER_ARRAY_VALUE_GETTER_AT(KindCase::kFloat32ArrayValues, float32_array_values, floats_size, set_float32_value, floats);

        // register value getter for element of string array
        REGISTER_ARRAY_VALUE_GETTER_AT(KindCase::kStringArrayValues, string_array_values, strings_size, set_string_value, strings);

        // register value getter for element of struct array
        valueGetterAt.addFunction(KindCase::kStructArrayValues, [](Value* dst, const Value &val, ParamIndex idx) -> void {
            auto size = val.struct_array_values().struct_values_size();
            if (idx >= size) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << size;
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            *dst->mutable_struct_value() = val.struct_array_values().struct_values(idx);
        });

        // register value getter for element of variant array
        valueGetterAt.addFunction(KindCase::kStructVariantArrayValues, [](Value* dst, const Value &val, ParamIndex idx) -> void {
            auto size = val.struct_variant_array_values().struct_variants_size();
            if (idx >= size) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << size;
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            *dst->mutable_struct_variant_value() = val.struct_variant_array_values().struct_variants(idx);
        });
        
        // register value setter for element of int32 array
        REGISTER_ARRAY_VALUE_SETTER_AT(KindCase::kInt32ArrayValues, int32_array_values, ints_size, mutable_int32_array_values, mutable_ints, int32_value);     

        // register value setter for element of float array
        REGISTER_ARRAY_VALUE_SETTER_AT(KindCase::kFloat32ArrayValues, float32_array_values, floats_size, mutable_float32_array_values, mutable_floats, float32_value); 

        // register value setter for element of string array
        REGISTER_ARRAY_VALUE_SETTER_AT(KindCase::kStringArrayValues, string_array_values, strings_size, mutable_string_array_values, mutable_strings, string_value);  
    }
}


template <> catena::Value::KindCase catena::getKindCase<int32_t>(const int32_t & src) {
    return catena::Value::KindCase::kInt32Value;
}

template <> catena::Value::KindCase catena::getKindCase<float>(const float &src) {
    return catena::Value::KindCase::kFloat32Value;
}

template <> catena::Value::KindCase catena::getKindCase<std::string>(const std::string & src) {
    return catena::Value::KindCase::kStringValue;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<int32_t>>(const std::vector<int32_t> & src) {
    return catena::Value::KindCase::kInt32ArrayValues;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<float>>(const std::vector<float> & src) {
    return catena::Value::KindCase::kFloat32ArrayValues;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<std::string>>(const std::vector<std::string> & src) {
    return catena::Value::KindCase::kStringArrayValues;
}

template <typename V> catena::Value::KindCase getKindCase(const V& src) {
    if constexpr (catena::has_getStructInfo<V>) {
        return catena::Value::KindCase::kStructValue;
    }
    return catena::Value::KindCase::KIND_NOT_SET;
}

void ParamAccessor::setValue(const std::string& peer, const Value &src) {
    std::lock_guard<DeviceModel::Mutex> lock(deviceModel_.get().mutex_);
    try {
        Value &value = value_.get();
        auto& setter = ValueSetter::getInstance();
        setter[value.kind_case()](value, src);
        deviceModel_.get().valueSetByClient.emit(*this, -1, peer);
        deviceModel_.get().pushUpdates.emit(*this, -1);
    } catch (const catena::exception_with_status& why) {
        std::stringstream err;
        err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
        throw catena::exception_with_status(err.str(), why.status);
    } catch (...) {
        throw catena::exception_with_status(__PRETTY_FUNCTION__, catena::StatusCode::UNKNOWN);
    }
}

void ParamAccessor::setValue(const std::string& peer, const Value &src, ParamIndex idx, std::vector<std::string>& clientScopes) {
    std::lock_guard<DeviceModel::Mutex> lock(deviceModel_.get().mutex_);
    try {  
        if (clientScopes[0] != catena::kAuthzDisabled) {
            if (std::find(clientScopes.begin(), clientScopes.end(), scope_.append(":w")) == clientScopes.end()) {
                BAD_STATUS("Not authorized to access this parameter", catena::StatusCode::PERMISSION_DENIED);
            }
        }

        Value &value = value_.get();
        if (!sameKind(src, idx)) {
            BAD_STATUS("Value type mismatch", catena::StatusCode::INVALID_ARGUMENT);
        }

        
        if (isList() && idx != kParamEnd) {
            // update array element
            auto& setterAt = ValueSetterAt::getInstance();
            setterAt[value.kind_case()](value, src, idx);
        } else {
            // update scalar value or whole array
            value.CopyFrom(src);
        }
        deviceModel_.get().valueSetByClient.emit(*this, idx, peer);
        deviceModel_.get().pushUpdates.emit(*this, idx);
    } catch (const catena::exception_with_status& why) {
        std::stringstream err;
        err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
        throw catena::exception_with_status(err.str(), why.status);
    } catch (...) {
        throw catena::exception_with_status(__PRETTY_FUNCTION__, catena::StatusCode::UNKNOWN);
    }
}

bool ParamAccessor::sameKind(const Value &src, const ParamIndex idx) const {
    if (!isList() || idx == kParamEnd) {
        return src.kind_case() == value_.get().kind_case();
    } else {
        switch (value_.get().kind_case()) {
            case catena::Value::KindCase::kInt32ArrayValues:
                return src.kind_case() == catena::Value::KindCase::kInt32Value;
            case catena::Value::KindCase::kFloat32ArrayValues:
                return src.kind_case() == catena::Value::KindCase::kFloat32Value;
            case catena::Value::KindCase::kStringArrayValues:
                return src.kind_case() == catena::Value::KindCase::kStringValue;
            default:
                return false;
        }
    }
}