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
using catena::ParamIndex;
constexpr auto kMulti = catena::Threading::kMultiThreaded;
constexpr auto kSingle = catena::Threading::kSingleThreaded;
using PAM = catena::ParamAccessor<catena::DeviceModel<kMulti>>;
using PAS = catena::ParamAccessor<catena::DeviceModel<kSingle>>;
using SVP = std::shared_ptr<catena::Value>; // shared value pointer

/**
 * @brief internal implementation of the setValue method
 *
 * @tparam T underlying type of param to set value of
 * @param p the param descriptor
 * @param val the param's value object
 * @param v the value
 */
template <typename T>
void setValueImpl(catena::Param &p, catena::Value &val, T v);

/**
 * @brief specialize for float
 *
 * @tparam
 * @param param the param descriptor
 * @param val the param's value object
 * @param v the value to set
 */
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
 * @throws OUT_OF_RANGE if the constraint type isn't valid
 * @tparam
 * @param param the param descriptor
 * @param val the param's value object
 * @param v the value to set
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

/**
 * @brief specialize for catena::Param
 *
 * @throws INVALID_ARGUMENT if the value type doesn't match the param type.
 * @throws UNIMPLEMENTED if support for param type not implemented.
 * @tparam
 * @param param the param descriptor
 * @param val the param's value object
 * @param v the value to set
 */
template <>
void setValueImpl<catena::Value const *>(catena::Param &p, catena::Value &val,
                                         catena::Value const *v) {
  auto type = p.type().type();
  switch (type) {
  case catena::ParamType_Type_FLOAT32:
    if (!v->has_float32_value()) {
      BAD_STATUS("expected float value", grpc::StatusCode::INVALID_ARGUMENT);
    }
    setValueImpl<float>(p, val, v->float32_value());
    break;

  case catena::ParamType_Type_INT32:
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
void catena::ParamAccessor<DM>::setValue(V v, ParamIndex idx) {
  typename DM::LockGuard lock(deviceModel_.get().mutex_);
  setValueImpl(param_.get(), value_.get(), v);
}

template void PAM::setValue<float>(float, ParamIndex);
template void PAS::setValue<float>(float, ParamIndex);
template void PAM::setValue<int>(int, ParamIndex);
template void PAS::setValue<int>(int, ParamIndex);
template void PAM::setValue<catena::Value const *>(catena::Value const *,
                                                   ParamIndex);
template void PAS::setValue<catena::Value const *>(catena::Value const *,
                                                   ParamIndex);

/**
 * @brief Function template to implement getValue
 * 
 * @tparam V 
 * @param v 
 * @return V 
 */
template <typename V> V getValueImpl(const catena::Value &v);

/**
 * @brief specialize for float
 * 
 * @tparam  
 * @param v 
 * @return float 
 */
template <> float getValueImpl<float>(const catena::Value &v) {
  return v.float32_value();
}

/**
 * @brief specialize for int
 * 
 * @tparam  
 * @param v 
 * @return int 
 */
template <> int getValueImpl<int>(const catena::Value &v) {
  return v.int32_value();
}


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
  auto& arr = v.int32_array_values();
  if (idx >= arr.ints_size()) {
    // range error
    std::stringstream err;
    err << "Index is out of range: " << idx << " >= " << arr.ints_size();
    BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
  }
  return arr.ints(idx);
}


template <typename DM>
template <typename V>
V catena::ParamAccessor<DM>::getValue(ParamIndex idx) {
  using W = typename std::remove_reference<V>::type;
  typename DM::LockGuard(deviceModel_.get().mutex_);
  catena::Param &cp = param_.get();
  catena::Value &v = value_.get();

  if constexpr (std::is_same<W, float>::value) {
    if (cp.type().type() != catena::ParamType::Type::ParamType_Type_FLOAT32) {
      BAD_STATUS("expected param of FLOAT32 type",
                 grpc::StatusCode::FAILED_PRECONDITION);
    }
    return getValueImpl<float>(v);

  } else if constexpr (std::is_same<W, int>::value) {
    catena::ParamType_Type t = cp.type().type();
    if (t == catena::ParamType::Type::ParamType_Type_INT32) {
      return getValueImpl<int>(v);
    } else if (t == catena::ParamType::Type::ParamType_Type_INT32_ARRAY) {
      return getValueImpl<int>(v, idx);
    } else {
      BAD_STATUS("expected param of INT32 type",
                 grpc::StatusCode::FAILED_PRECONDITION);
    }

  } else if constexpr (std::is_same<W, std::shared_ptr<catena::Value>>::value) {

    if (isList(v) && idx != kParamEnd) {
      return getValueAt(v, idx);
    } else {
      // return the whole parameter
      std::shared_ptr<catena::Value> ans;
      ans.reset(&v);
      return ans;
    }
    

  } else {
    /** @todo complete support for all param types in
     * DeviceModel::Param::getValue */
    BAD_STATUS("Unsupported Param type", grpc::StatusCode::UNIMPLEMENTED);
  }
}

template<typename DM>
SVP catena::ParamAccessor<DM>::getValueAt(catena::Value& v, ParamIndex idx) {
  if (!isList(v)) {
    std::stringstream err;
    err << "Expected list value";
    BAD_STATUS(err.str(), grpc::StatusCode::FAILED_PRECONDITION);
  }
  if (v.has_int32_array_values()) {
    auto& arr = v.int32_array_values();
    if (arr.ints_size() < idx) {
      // range error
      std::stringstream err;
      err << "Index is out of range: " << idx << " >= " << arr.ints_size();
      BAD_STATUS(err.str(), grpc::StatusCode::OUT_OF_RANGE);
    }
    auto ans = std::make_shared<catena::Value>(catena::Value{});
    ans->set_int32_value(arr.ints(idx));
    return ans;
  } else {
    BAD_STATUS("Not implemented, sorry", grpc::StatusCode::UNIMPLEMENTED);
  }
}

// instantiate all the getValues
template float PAM::getValue<float>(ParamIndex);
template float PAS::getValue<float>(ParamIndex);
template int PAM::getValue<int>(ParamIndex);
template int PAS::getValue<int>(ParamIndex);
template SVP PAM::getValue<SVP>(ParamIndex);
template SVP PAS::getValue<SVP>(ParamIndex);

// instantiate the 2 ParamAccessors
// instantiate the 2 versions of DeviceModel, and its streaming operator
template class catena::ParamAccessor<
    catena::DeviceModel<Threading::kMultiThreaded>>;
template class catena::ParamAccessor<
    catena::DeviceModel<Threading::kSingleThreaded>>;