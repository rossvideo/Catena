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

<<<<<<< HEAD
using catena::ParamIndex;
using catena::Threading;
constexpr auto kMulti = catena::Threading::kMultiThreaded;
constexpr auto kSingle = catena::Threading::kSingleThreaded;
using PAM = catena::ParamAccessor<catena::DeviceModel<kMulti>>;
using PAS = catena::ParamAccessor<catena::DeviceModel<kSingle>>;
using VP = catena::Value *;  // shared value pointer

/**
 * @brief set float value override
 *
 * @param param the param descriptor
 * @param dst the param's value object
 * @param src the value to set
 */
void setValueImpl(catena::Param &param, catena::Value &dst, float src, [[maybe_unused]] ParamIndex idx = 0) {
    if (param.has_constraint()) {
        // apply the constraint
        src = std::clamp(src, param.constraint().float_range().min_value(),
                         param.constraint().float_range().max_value());
    }
    dst.set_float32_value(src);
}

int applyIntConstraint(catena::Param &param, int v) {
    if (param.has_constraint()) {
        // apply the constraint
        int constraint_type = param.constraint().type();
        switch (constraint_type) {
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_RANGE:
                v = std::clamp(v, param.constraint().int32_range().min_value(),
                               param.constraint().int32_range().max_value());
                break;
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_CHOICE:
                /** @todo validate that v is one of the choices fallthru is
       * intentional for now */
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_ALARM_TABLE:
                // we can't validate alarm tables easily, so trust the client
                break;
            default:
                std::stringstream err;
                err << "invalid constraint for int32: " << constraint_type << '\n';
                BAD_STATUS((err.str()), grpc::StatusCode::OUT_OF_RANGE);
        }
    }
    return v;
}

std::string applyStringConstraint(catena::Param &param, std::string v) {
    if (param.has_constraint()) {
        // apply the constraint
        int constraint_type = param.constraint().type();

        switch (constraint_type) {
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_STRING_CHOICE:
                /** @todo: validate string choice */
                break;
            case catena::Constraint_ConstraintType::Constraint_ConstraintType_STRING_STRING_CHOICE:
                /** @todo validate that v is string string choice */
                break;
            default:
                std::stringstream err;
                err << "invalid constraint for string: " << constraint_type << '\n';
                BAD_STATUS((err.str()), grpc::StatusCode::OUT_OF_RANGE);
        }
    }
    return v;
}

/**
 * @brief override for int
 *
 * @throws OUT_OF_RANGE if the constraint type isn't valid
 * @tparam
 * @param param the param descriptor
 * @param val the param's value object
 * @param v the value to set
 */
void setValueImpl(catena::Param &param, catena::Value &dst, int src, ParamIndex idx = 0) {
    dst.set_int32_value(applyIntConstraint(param, src));
}

/**
 * @brief override for string
 *
 * @throws OUT_OF_RANGE if the constraint type isn't valid
 * @tparam
 * @param param the param descriptor
 * @param val the param's value object
 * @param v the value to set
 */
void setValueImpl(catena::Param &param, catena::Value &dst, std::string src, ParamIndex idx = 0) {
    dst.set_string_value(applyStringConstraint(param, std::move(src)));
}

/**
 * @brief override for catena::Value
 *
 * @throws INVALID_ARGUMENT if the value type doesn't match the param type.
 * @throws UNIMPLEMENTED if support for param type not implemented.
 * @param param the param descriptor
 * @param val the param's value object
 * @param v the value to set
 * @param idx index into array types
 */
void setValueImpl(catena::Param &p, catena::Value &dst, const catena::Value &src, ParamIndex idx = 0) {
    auto type = p.type().type();
    switch (type) {
        case catena::ParamType_Type_STRING:
            if (!src.has_string_value()) {
                BAD_STATUS("expected string value", grpc::StatusCode::INVALID_ARGUMENT);
            }
            setValueImpl(p, dst, src.string_value());
            break;

        case catena::ParamType_Type_FLOAT32:
            if (!src.has_float32_value()) {
                BAD_STATUS("expected float value", grpc::StatusCode::INVALID_ARGUMENT);
            }
            setValueImpl(p, dst, src.float32_value());
            break;

        case catena::ParamType_Type_INT32:
            if (!src.has_int32_value()) {
                BAD_STATUS("expected int32 value", grpc::StatusCode::INVALID_ARGUMENT);
            }
            setValueImpl(p, dst, src.int32_value());
            break;

        case catena::ParamType_Type_INT32_ARRAY: {
            if (!dst.has_int32_array_values()) {
                BAD_STATUS("expected int32 value", grpc::StatusCode::INVALID_ARGUMENT);
            }
            catena::Int32List *arr = dst.mutable_int32_array_values();
            if (idx == catena::kParamEnd) {
                // special case, add to end of the array
                arr->add_ints(applyIntConstraint(p, src.int32_value()));
            } else if (arr->ints_size() < idx) {
                // range error
                std::stringstream err;
                err << "array index is out of bounds, " << idx << " >= " << arr->ints_size();
                BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
            } else {
                // update array element
                arr->set_ints(idx, applyIntConstraint(p, src.int32_value()));
            }
        } break;
        default: {
            std::stringstream err;
            err << "Unimplemented value type: " << type;
            BAD_STATUS(err.str(), grpc::StatusCode::UNIMPLEMENTED);
        }
    }
}

template <typename DM> template <typename V> void catena::ParamAccessor<DM>::setValue(V v, ParamIndex idx) {
    typename DM::LockGuard lock(deviceModel_.get().mutex_);
    setValueImpl(param_.get(), value_.get(), v, idx);
}

template void PAM::setValue<std::string>(std::string, ParamIndex);
template void PAS::setValue<std::string>(std::string, ParamIndex);
template void PAM::setValue<float>(float, ParamIndex);
template void PAS::setValue<float>(float, ParamIndex);
template void PAM::setValue<int>(int, ParamIndex);
template void PAS::setValue<int>(int, ParamIndex);
template void PAM::setValue<catena::Value>(catena::Value, ParamIndex);
template void PAS::setValue<catena::Value>(catena::Value, ParamIndex);

/**
 * @brief Function template to implement getValue
 *
 * @tparam V
 * @param v
 * @return V
 */
template <typename V> V getValueImpl(const catena::Value &v);

/**
 * @brief specialize for string
 *
 * @tparam
 * @param v
 * @return std::string
 */
template <> std::string getValueImpl<std::string>(const catena::Value &v) { return v.string_value(); }

/**
 * @brief specialize for float
 *
 * @tparam
 * @param v
 * @return float
 */
template <> float getValueImpl<float>(const catena::Value &v) { return v.float32_value(); }

/**
 * @brief specialize for int
 *
 * @tparam
 * @param v
 * @return int
 */
template <> int getValueImpl<int>(const catena::Value &v) { return v.int32_value(); }

/**
 * @brief function template to implement getValue for array types
 *
 * @tparam V
 * @param v
 * @param idx
 * @return V
 */
template <typename V> V getValueImpl(const catena::Value &v, ParamIndex idx);

/**
 * @brief specialize for int
 *
 * @tparam
 * @param v
 * @return int
 */
template <> int getValueImpl<int>(const catena::Value &v, ParamIndex idx) {
    if (!v.has_int32_array_values()) {
        // i.e. we're not calling this on an INT32_ARRAY parameter
        std::stringstream err;
        err << "expected Catena::Value of Int32List";
        BAD_STATUS(err.str(), grpc::StatusCode::FAILED_PRECONDITION);
    }
    auto &arr = v.int32_array_values();
    if (idx >= arr.ints_size()) {
        // range error
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.ints_size();
        BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
    }
    return arr.ints(idx);
}

template <typename DM> template <typename V> V catena::ParamAccessor<DM>::getValue(ParamIndex idx) {
    using W = typename std::remove_reference<V>::type;
    typename DM::LockGuard(deviceModel_.get().mutex_);
    catena::Param &cp = param_.get();
    catena::Value &v = value_.get();

    if constexpr (std::is_same<W, std::string>::value) {
        if (cp.type().type() != catena::ParamType::Type::ParamType_Type_STRING) {
            BAD_STATUS("expected param of string type", grpc::StatusCode::FAILED_PRECONDITION);
        }
        return getValueImpl<std::string>(v);

    } else if constexpr (std::is_same<W, float>::value) {
        if (cp.type().type() != catena::ParamType::Type::ParamType_Type_FLOAT32) {
            BAD_STATUS("expected param of FLOAT32 type", grpc::StatusCode::FAILED_PRECONDITION);
        }
        return getValueImpl<float>(v);

    } else if constexpr (std::is_same<W, int>::value) {
        catena::ParamType_Type t = cp.type().type();
        if (t == catena::ParamType::Type::ParamType_Type_INT32) {
            return getValueImpl<int>(v);
        } else if (t == catena::ParamType::Type::ParamType_Type_INT32_ARRAY) {
            return getValueImpl<int>(v, idx);
        } else {
            BAD_STATUS("expected param of INT32 type", grpc::StatusCode::FAILED_PRECONDITION);
        }

    } else if constexpr (std::is_same<W, catena::Value>::value) {

        if (isList(v) && idx != kParamEnd) {
            return getValueAt(idx);
        } else {
            // return the whole parameter
            return v;
        }

    } else {
        /** @todo complete support for all param types in
     * DeviceModel::Param::getValue */
        BAD_STATUS("Unsupported Param type", grpc::StatusCode::UNIMPLEMENTED);
    }
}

template <typename DM> catena::Value catena::ParamAccessor<DM>::getValueAt(ParamIndex idx) {
    typename DM::LockGuard(deviceModel_.get().mutex_);
    auto v = value_.get();
    if (!isList(v)) {
        std::stringstream err;
        err << "Expected list value";
        BAD_STATUS(err.str(), grpc::StatusCode::FAILED_PRECONDITION);
    }

    /** @todo when we add support for the next array type, use a factory or static
   * map of functions to handle each type */
    if (v.has_int32_array_values()) {
        auto &arr = v.int32_array_values();
        if (arr.ints_size() < idx) {
            // range error
            std::stringstream err;
            err << "Index is out of range: " << idx << " >= " << arr.ints_size();
            BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
        }
        catena::Value ans{};
        ans.set_int32_value(arr.ints(idx));
        return ans;
    } else {
        BAD_STATUS("Not implemented, sorry", grpc::StatusCode::UNIMPLEMENTED);
    }
}

// instantiate all the getValues
template std::string PAM::getValue<std::string>(ParamIndex);
template std::string PAS::getValue<std::string>(ParamIndex);
template float PAM::getValue<float>(ParamIndex);
template float PAS::getValue<float>(ParamIndex);
template int PAM::getValue<int>(ParamIndex);
template int PAS::getValue<int>(ParamIndex);
template catena::Value PAM::getValue<catena::Value>(ParamIndex);
template catena::Value PAS::getValue<catena::Value>(ParamIndex);

// instantiate the 2 ParamAccessors
// instantiate the 2 versions of DeviceModel, and its streaming operator
template class catena::ParamAccessor<catena::DeviceModel<Threading::kMultiThreaded>>;
template class catena::ParamAccessor<catena::DeviceModel<Threading::kSingleThreaded>>;
=======
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


ParamAccessor::ParamAccessor(DeviceModel &dm, DeviceModel::ParamAccessorData &pad, const std::string& oid)
    : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)}, oid_{oid}, id_{std::hash<std::string>{}(oid)} {
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


template <> catena::Value::KindCase catena::getKindCase<int32_t>(int32_t & src) {
    return catena::Value::KindCase::kInt32Value;
}

template <> catena::Value::KindCase catena::getKindCase<float>(float &src) {
    return catena::Value::KindCase::kFloat32Value;
}

template <> catena::Value::KindCase catena::getKindCase<std::string>(std::string & src) {
    return catena::Value::KindCase::kStringValue;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<int32_t>>(std::vector<int32_t> & src) {
    return catena::Value::KindCase::kInt32ArrayValues;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<float>>(std::vector<float> & src) {
    return catena::Value::KindCase::kFloat32ArrayValues;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<std::string>>(std::vector<std::string> & src) {
    return catena::Value::KindCase::kStringArrayValues;
}

void ParamAccessor::setValue(const std::string& peer, const Value &src) {
    std::lock_guard<DeviceModel::Mutex> lock(deviceModel_.get().mutex_);
    try {
        Value &value = value_.get();
        auto& setter = ValueSetter::getInstance();
        setter[value.kind_case()](value, src);
        deviceModel_.get().valueSetByClient.emit(*this, -1, peer);
    } catch (const catena::exception_with_status& why) {
        std::stringstream err;
        err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
        throw catena::exception_with_status(err.str(), why.status);
    } catch (...) {
        throw catena::exception_with_status(__PRETTY_FUNCTION__, catena::StatusCode::UNKNOWN);
    }
}

void ParamAccessor::setValue(const std::string& peer, const Value &src, ParamIndex idx) {
    std::lock_guard<DeviceModel::Mutex> lock(deviceModel_.get().mutex_);
    try {
        Value &value = value_.get();
        if (isList() && idx != kParamEnd) {
            // update array element
            auto& setterAt = ValueSetterAt::getInstance();
            setterAt[value.kind_case()](value, src, idx);
        } else {
            // update scalar value or whole array
            value.CopyFrom(src);
        }
        deviceModel_.get().valueSetByClient.emit(*this, idx, peer);
    } catch (const catena::exception_with_status& why) {
        std::stringstream err;
        err << "setValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
        throw catena::exception_with_status(err.str(), why.status);
    } catch (...) {
        throw catena::exception_with_status(__PRETTY_FUNCTION__, catena::StatusCode::UNKNOWN);
    }
}
>>>>>>> develop
