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

#include <google/protobuf/map.h>
#include <google/protobuf/util/json_util.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using catena::Threading;
constexpr auto kMulti = catena::Threading::kMultiThreaded;
constexpr auto kSingle = catena::Threading::kSingleThreaded;

/**
 * @brief internal implementation of the setValue method
 *
 * @tparam T underlying type of param to set value of
 * @param p the param
 * @param v the value
 */
template <typename T>
void setValueImpl(catena::Param &p, catena::Value &val, T v);

template <>
void setValueImpl<float>(catena::Param &param, catena::Value &val, float v) {
  if (param.has_constraint()) {
    // apply the constraint
    v = std::clamp(v, param.constraint().float_range().min_value(),
                   param.constraint().float_range().max_value());
  }
  val.set_float32_value(v);
}

/**
 * @brief specialize for int
 *
 * @throws std::range_error if the constraint type isn't valid
 * @tparam
 * @param param
 * @param v
 */
template <>
void setValueImpl<int>(catena::Param &param, catena::Value &val, int v) {
  if (param.has_constraint()) {
    // apply the constraint
    int constraint_type = param.constraint().type();
    switch (constraint_type) {
    case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_RANGE:
      v = std::clamp(v, param.constraint().int32_range().min_value(),
                     param.constraint().int32_range().max_value());
      break;
    case catena::Constraint_ConstraintType::
        Constraint_ConstraintType_INT_CHOICE:
      /** @todo validate that v is one of the choices fallthru is
       * intentional for now */
    case catena::Constraint_ConstraintType::
        Constraint_ConstraintType_ALARM_TABLE:
      // we can't validate alarm tables easily, so trust the client
      break;
    default:
      std::stringstream err;
      err << "invalid constraint for int32: " << constraint_type << '\n';
      BAD_STATUS((err.str()), grpc::StatusCode::OUT_OF_RANGE);
    }
  }
  val.set_int32_value(v);
}

template <>
void setValueImpl<catena::Value const *>(catena::Param &p, catena::Value &val,
                                         catena::Value const *v) {
  auto type = p.type().param_type();
  switch (type) {
  case catena::ParamType_ParamTypes_FLOAT32:
    if (!v->has_float32_value()) {
      BAD_STATUS("expected float value", grpc::StatusCode::INVALID_ARGUMENT);
    }
    setValueImpl<float>(p, val, v->float32_value());
    break;

  case catena::ParamType_ParamTypes_INT32:
    if (!v->has_int32_value()) {
      BAD_STATUS("expected int32 value", grpc::StatusCode::INVALID_ARGUMENT);
    }
    setValueImpl<int>(p, val, v->int32_value());
    break;

  default: {
    std::stringstream err;
    err << "Unimplemented value type: " << type;
    BAD_STATUS(err.str(), grpc::StatusCode::UNIMPLEMENTED);
  }
  }
}

template <typename DM>
template <typename V>
void catena::ParamAccessor<DM>::setValue(V v) {
  typename DM::LockGuard lock(deviceModel_.get().mutex_);
  setValueImpl(param_.get(), value_.get(), v);
}

template void
catena::ParamAccessor<catena::DeviceModel<kMulti>>::setValue<float>(float);
template void
catena::ParamAccessor<catena::DeviceModel<kSingle>>::setValue<float>(float);
template void
catena::ParamAccessor<catena::DeviceModel<kMulti>>::setValue<int>(int);
template void
catena::ParamAccessor<catena::DeviceModel<kSingle>>::setValue<int>(int);
template void catena::ParamAccessor<catena::DeviceModel<kMulti>>::setValue<
    catena::Value const *>(catena::Value const *);
template void catena::ParamAccessor<catena::DeviceModel<kSingle>>::setValue<
    catena::Value const *>(catena::Value const *);

template <typename DM>
template <typename V>
void catena::ParamAccessor<DM>::setValueAt(V v, uint32_t idx) {
  /// @todo implement DeviceModel::Param::setValueAt
  BAD_STATUS("not implemented, sorry", grpc::StatusCode::UNIMPLEMENTED);
}

template void catena::ParamAccessor<catena::DeviceModel<kMulti>>::setValueAt<
    catena::Value const *>(catena::Value const *v, uint32_t idx);
template void catena::ParamAccessor<catena::DeviceModel<kSingle>>::setValueAt<
    catena::Value const *>(catena::Value const *v, uint32_t idx);

template <typename V> V getValueImpl(const catena::Value &v);

template <> float getValueImpl<float>(const catena::Value &v) {
  return v.float32_value();
}

template <> int getValueImpl<int>(const catena::Value &v) {
  return v.int32_value();
}

template <typename DM>
template <typename V>
V catena::ParamAccessor<DM>::getValue() {
  using W = typename std::remove_reference<V>::type;
  typename DM::LockGuard(deviceModel_.get().mutex_);
  catena::Param &cp = param_.get();
  catena::Value &v = value_.get();

  if constexpr (std::is_same<W, float>::value) {
    if (cp.type().param_type() !=
        catena::ParamType::ParamTypes::ParamType_ParamTypes_FLOAT32) {
      BAD_STATUS("expected param of FLOAT32 type",
                 grpc::StatusCode::FAILED_PRECONDITION);
    }
    return getValueImpl<float>(v);

  } else if constexpr (std::is_same<W, int>::value) {
    if (cp.type().param_type() !=
        catena::ParamType::ParamTypes::ParamType_ParamTypes_INT32) {
      BAD_STATUS("expected param of INT32 type",
                 grpc::StatusCode::FAILED_PRECONDITION);
    }
    return getValueImpl<int>(v);

  } else if constexpr (std::is_same<W, catena::Value *>::value) {
    return &v;

  } else {
    /** @todo complete support for all param types in
     * DeviceModel::Param::getValue */
    BAD_STATUS("Unsupported Param type", grpc::StatusCode::UNIMPLEMENTED);
  }
}

// instantiate all the getValues
template float
catena::ParamAccessor<catena::DeviceModel<kMulti>>::getValue<float>();
template float
catena::ParamAccessor<catena::DeviceModel<kSingle>>::getValue<float>();
template int
catena::ParamAccessor<catena::DeviceModel<kMulti>>::getValue<int>();
template int
catena::ParamAccessor<catena::DeviceModel<kSingle>>::getValue<int>();
template catena::Value *
catena::ParamAccessor<catena::DeviceModel<kMulti>>::getValue<catena::Value *>();
template catena::Value *catena::ParamAccessor<
    catena::DeviceModel<kSingle>>::getValue<catena::Value *>();

// instantiate the 2 ParamAccessors
// instantiate the 2 versions of DeviceModel, and its streaming operator
template class catena::ParamAccessor<
    catena::DeviceModel<Threading::kMultiThreaded>>;
template class catena::ParamAccessor<
    catena::DeviceModel<Threading::kSingleThreaded>>;