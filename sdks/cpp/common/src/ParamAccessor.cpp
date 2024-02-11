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


ParamAccessor::ParamAccessor(DeviceModel &dm, DeviceModel::ParamAccessorData &pad)
    : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)} {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;  // so we only do this once

        // get our functories
        auto &setter = Setter::getInstance();
        auto &getter = Getter::getInstance();
        auto &setterAt = SetterAt::getInstance();
        auto &getterAt = GetterAt::getInstance();
        auto &variantGetter = VariantInfoGetter::getInstance();
        auto &variantSetter = VariantInfoGetter::getInstance();
        auto &valueGetter = ValueGetter::getInstance();

        // register int32 setter
        setter.addFunction(KindCase::kInt32Value, [](Value *dst, const void *srcAddr) {
            dst->set_int32_value(*reinterpret_cast<const int32_t *>(srcAddr));
        });

        // register float32 setter
        setter.addFunction(KindCase::kFloat32Value, [](Value *dst, const void *srcAddr) {
            dst->set_float32_value(*reinterpret_cast<const float *>(srcAddr));
        });

        // register string setter
        setter.addFunction(KindCase::kStringValue, [](Value *dst, const void *srcAddr) {
            *dst->mutable_string_value() = (*reinterpret_cast<const std::string *>(srcAddr));
        });

        // register array of int setter
        setter.addFunction(KindCase::kInt32ArrayValues, [](catena::Value *dst, const void *srcAddr) {
            dst->mutable_int32_array_values()->clear_ints();
            auto *src = reinterpret_cast<const std::vector<int32_t> *>(srcAddr);
            for (auto &it : *src) {
                dst->mutable_int32_array_values()->add_ints(it);
            }
        });
        
        // register array of int getter
        getter.addFunction(KindCase::kInt32ArrayValues, [](void *dstAddr, const Value *src) {
            auto *dst = reinterpret_cast<std::vector<int32_t>*>(dstAddr);
            dst->clear();
            auto &arr = src->int32_array_values().ints();
            for (auto it = arr.begin(); it != arr.end(); ++it) {
                dst->push_back(*it);
            }
        });

        // register element of array of int setter
        setterAt.addFunction(KindCase::kInt32ArrayValues, 
            [](Value *dst, const void *srcAddr, catena::ParamIndex idx) {
                auto *src = reinterpret_cast<const int32_t *>(srcAddr);
                if (idx >= dst->mutable_int32_array_values()->ints_size()) {
                    // range error
                    std::stringstream err;
                    err << "array index is out of bounds, " << idx
                        << " >= " << dst->mutable_int32_array_values()->ints_size();
                    BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
                } else {
                    // update array element
                    dst->mutable_int32_array_values()->set_ints(idx, *src);
                }
            }
        );

        // register element of array of int getter
        getterAt.addFunction(KindCase::kInt32ArrayValues,
            [](void *dstAddr, const Value *src, const catena::ParamIndex idx) {
                auto *dst = reinterpret_cast<int32_t *>(dstAddr);
                if (idx >= src->int32_array_values().ints_size()) {
                    // range error
                    std::stringstream err;
                    err << "array index is out of bounds, " << idx
                        << " >= " << src->int32_array_values().ints_size();
                    BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
                } else {
                    // update array element
                    *dst = src->int32_array_values().ints(idx);
                }
            }
        );
        
        // register float getter
        getter.addFunction(KindCase::kFloat32Value, [](void *dstAddr, const Value *src) {
            *reinterpret_cast<float *>(dstAddr) = src->float32_value();
        });

        // register int getter
        getter.addFunction(KindCase::kInt32Value, [](void *dstAddr, const Value *src) {
            *reinterpret_cast<int32_t *>(dstAddr) = src->int32_value();
        });

        // register string getter
        getter.addFunction(KindCase::kStringValue, [](void *dstAddr, const Value *src) {
            *reinterpret_cast<std::string *>(dstAddr) = src->string_value();
        });

        // register value getter for int32 array
        valueGetter.addFunction(KindCase::kInt32ArrayValues, [](Value* dst, const Value &val, ParamIndex idx) -> void {
            if (idx >= val.int32_array_values().ints_size()) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << val.int32_array_values().ints_size();
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            dst->set_int32_value(val.int32_array_values().ints(idx));
        });

        // register value getter for float array
        valueGetter.addFunction(KindCase::kFloat32ArrayValues, [](Value* dst, const Value &val, ParamIndex idx) -> void {
            if (idx >= val.float32_array_values().floats_size()) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << val.float32_array_values().floats_size();
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            dst->set_float32_value(val.float32_array_values().floats(idx));
        });

        // register value getter for string array
        valueGetter.addFunction(KindCase::kStringArrayValues, [](Value* dst, const Value &val, ParamIndex idx) -> void {
            auto size = val.string_array_values().strings_size();
            if (idx >= size) {
                std::stringstream err;
                err << "array index is out of bounds, " << idx
                    << " >= " << size;
                BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
            }
            dst->set_string_value(val.string_array_values().strings(idx));
        });
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

template <>
catena::Value::KindCase catena::getKindCase<std::vector<int32_t>>(std::vector<int32_t> & src) {
    return catena::Value::KindCase::kInt32ArrayValues;
}

template <> catena::Value::KindCase catena::getKindCase<std::vector<float>>(std::vector<float> & src) {
    return catena::Value::KindCase::kFloat32ArrayValues;
}

template <>
catena::Value::KindCase catena::getKindCase<std::vector<std::string>>(std::vector<std::string> & src) {
    return catena::Value::KindCase::kStringArrayValues;
}

void ParamAccessor::getValue(Value *dst, ParamIndex idx) const {
    std::lock_guard<DeviceModel::Mutex> lock(deviceModel_.get().mutex_);
    const Value& value = value_.get();
    if (isList(value) && idx != kParamEnd) {
        auto& getter = ValueGetter::getInstance();
        getter[value.kind_case()](dst, value, idx);
    } else {
        // value is a scalar type
        dst->CopyFrom(value);
    }
}