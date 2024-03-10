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
#include <ValueAccessors.h>
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
using catena::ValueIndex;
using VP = catena::Value *;  // shared value pointer
using catena::DeviceModel;
using catena::Param;
using catena::Value;
using KindCase = Value::KindCase;
using catena::meta::TypeTag;

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
    [](void *dstAddr, const Value *src, const catena::ValueIndex idx) { \
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
    [](Value *dst, const void *srcAddr, catena::ValueIndex idx) { \
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
#define REGISTER_VALUE_GETTER(kind_case, dst_method, src_method) \
valueGetter.addFunction(kind_case, [](Value* dst, const Value &src, ValueIndex idx) -> void { \
    dst->dst_method(src.src_method());\
})

/**
 * @brief A macro used to add setter functions accessible by clients
*/
#define REGISTER_VALUE_SETTER(kind_case, dst_method, src_method) \
valueSetter.addFunction(kind_case, [](Value &dst, const Value &src, ValueIndex idx) -> void { \
    dst.dst_method(src.src_method()); \
})

/**
 * @brief A macro used to add getter functions for array of simple data types accessible by clients
*/
#define REGISTER_ARRAY_VALUE_GETTER(kind_case, array_val_method, size_method, set_value_method, index_method) \
valueGetter.addFunction(kind_case, [](Value* dst, const Value &val, ValueIndex idx) -> void { \
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
#define REGISTER_ARRAY_VALUE_SETTER(kind_case, array_val_method, size_method, mutable_array_val_method, mutable_method, value_method) \
valueSetter.addFunction(kind_case, [](Value &dst, const Value &src, ValueIndex idx) -> void { \
    auto size = src.array_val_method().size_method(); \
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
    : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)}, oid_{oid}, id_{std::hash<std::string>{}(oid)} {}

void ParamAccessor::setValue(const std::string& peer, const Value &src, ValueIndex idx) {
    std::lock_guard<DeviceModel::Mutex> lock(deviceModel_.get().mutex_);
    try {
        Value &value = value_.get();
        if (isList() && idx != kValueEnd) {
            // update array element
            auto& setter = ValueSetter::getInstance();
            setter[value.kind_case()](value, src, idx);
        } else {
            // update scalar value
            value.CopyFrom(src);
        }
        deviceModel_.get().valueSetByClient.emit(*this, idx, peer);
    } catch (const catena::exception_with_status& why) {
        std::stringstream err;
        err << "getValue failed: " << why.what() << '\n' << __PRETTY_FUNCTION__ << '\n';
        throw catena::exception_with_status(err.str(), why.status);
    } catch (...) {
        throw catena::exception_with_status(__PRETTY_FUNCTION__, catena::StatusCode::UNKNOWN);
    }
}