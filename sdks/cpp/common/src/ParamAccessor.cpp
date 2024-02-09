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

#include <DeviceModel.h>
#include <ParamAccessor.h>
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

using catena::ParamIndex;
using catena::ParamAccessor;
using VP = catena::Value *;  // shared value pointer
using catena::Value;
using catena::Param;
using catena::DeviceModel;
using LockGuard = catena::ParamAccessor::LockGuard;

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
                if(param.constraint().string_choice().strict()) {
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

std::unique_ptr<ParamAccessor> ParamAccessor::subParam(const std::string& fieldName) {
    // apply lock
    LockGuard lock(deviceModel_.get().mutex_);
    // then call the private version of the method
    return subParam_(fieldName);
}

std::unique_ptr<ParamAccessor> ParamAccessor::subParam_(const std::string& fieldName) {
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

const std::unique_ptr<ParamAccessor> ParamAccessor::subParam(const std::string& fieldName) const {
    // apply lock
    LockGuard lock(deviceModel_.get().mutex_);
    // then call the private version of the method
    return subParam_(fieldName);
}

const std::unique_ptr<ParamAccessor> ParamAccessor::subParam_(const std::string& fieldName) const {
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

static bool floatSetter = catena::ParamAccessor::registerSetter(catena::Value::KindCase::kFloat32Value, [](catena::Value *dst, const void *srcAddr) {
    dst->set_float32_value(*reinterpret_cast<const float *>(srcAddr));
});

static bool int32Setter = catena::ParamAccessor::registerSetter(catena::Value::KindCase::kInt32Value, [](catena::Value *dst, const void *srcAddr) {
    dst->set_int32_value(*reinterpret_cast<const int32_t *>(srcAddr));
});

static bool stringSetter = catena::ParamAccessor::registerSetter(catena::Value::KindCase::kStringValue, [](catena::Value *dst, const void *srcAddr) {
    *dst->mutable_string_value() = (*reinterpret_cast<const std::string *>(srcAddr));
});

static bool int32ArraySetter = catena::ParamAccessor::registerSetter(catena::Value::KindCase::kInt32ArrayValues, [](catena::Value *dst, const void *srcAddr) {
    dst->mutable_int32_array_values()->clear_ints();
    auto *src = reinterpret_cast<const std::vector<int32_t>*>(srcAddr);
    for (auto &it : *src) {
        dst->mutable_int32_array_values()->add_ints(it);
    }
});

static bool int32ArrayGetter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kInt32ArrayValues, [](void *dstAddr, const catena::Value *src) {
    auto *dst = reinterpret_cast<std::vector<int32_t>*>(dstAddr);
    dst->clear();
    auto& arr = src->int32_array_values().ints();
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        dst->push_back(*it);
    }
});

static bool int32ArraySetterAt = catena::ParamAccessor::registerSetterAt(catena::Value::KindCase::kInt32ArrayValues, [](catena::Value *dst, const void *srcAddr, catena::ParamIndex idx) {
    auto *src = reinterpret_cast<const int32_t*>(srcAddr);
    if (idx >= dst->mutable_int32_array_values()->ints_size()) {
        // range error
        std::stringstream err;
        err << "array index is out of bounds, " << idx << " >= " << dst->mutable_int32_array_values()->ints_size();
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
    } else {
        // update array element
        dst->mutable_int32_array_values()->set_ints(idx, *src);
    }
});

static bool int32ArrayGetterAt = catena::ParamAccessor::registerGetterAt(catena::Value::KindCase::kInt32ArrayValues, [](void *dstAddr, const catena::Value *src, const catena::ParamIndex idx) {
    auto *dst = reinterpret_cast<int32_t*>(dstAddr);
    if (idx >= src->int32_array_values().ints_size()) {
        // range error
        std::stringstream err;
        err << "array index is out of bounds, " << idx << " >= " << src->int32_array_values().ints_size();
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
    } else {
        // update array element
        *dst = src->int32_array_values().ints(idx);
    }
});

static bool floatGetter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kFloat32Value, [](void *dstAddr, const catena::Value *src) {
    *reinterpret_cast<float *>(dstAddr) = src->float32_value();
});

static bool int32Getter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kInt32Value, [](void *dstAddr, const catena::Value *src) {
    *reinterpret_cast<int32_t *>(dstAddr) = src->int32_value();
});

static bool stringGetter = catena::ParamAccessor::registerGetter(catena::Value::KindCase::kStringValue, [](void *dstAddr, const catena::Value *src) {
    *reinterpret_cast<std::string *>(dstAddr) = src->string_value();
});

template<>
catena::Value::KindCase catena::getKindCase<int32_t>(int32_t& src) {
    return catena::Value::KindCase::kInt32Value;
}

template<>
catena::Value::KindCase catena::getKindCase<float>(float& src) {
    return catena::Value::KindCase::kFloat32Value;
}

template<>
catena::Value::KindCase catena::getKindCase<std::string>(std::string& src) {
    return catena::Value::KindCase::kStringValue;
}

template<>
catena::Value::KindCase catena::getKindCase<std::vector<int32_t>>(std::vector<int32_t>& src) {
    return catena::Value::KindCase::kInt32ArrayValues;
}

template<>
catena::Value::KindCase catena::getKindCase<std::vector<float>>(std::vector<float>& src) {
    return catena::Value::KindCase::kFloat32ArrayValues;
}

template<>
catena::Value::KindCase catena::getKindCase<std::vector<std::string>>(std::vector<std::string>& src) {
    return catena::Value::KindCase::kStringArrayValues;
}
