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

#include <google/protobuf/map.h>
#include <google/protobuf/util/json_util.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

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

    auto &fac = catena::ArrayAccessor::Factory::getInstance();
    if (fac.canMake(v.kind_case())) {
        ArrayAccessor *aa = fac.makeProduct(v.kind_case(), v);
        return aa->operator[](idx);
    } else {
        BAD_STATUS("Not implemented, sorry", grpc::StatusCode::UNIMPLEMENTED);
    }
}

using int_array = catena::ConcreteArrayAccessor<int>;
using float_array = catena::ConcreteArrayAccessor<float>;
template <> bool int_array::_added = int_array::registerWithFactory(Value::KindCase::kInt32ArrayValues);
template <>
bool float_array::_added = float_array ::registerWithFactory(Value::KindCase::kFloat32ArrayValues);

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
