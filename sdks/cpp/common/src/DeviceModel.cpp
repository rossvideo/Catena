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
#include <utils.h>
#include <Path.h>

#include <google/protobuf/map.h>
#include <google/protobuf/util/json_util.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

using catena::Threading;
const auto kMulti = catena::Threading::kMultiThreaded;
const auto kSingle = catena::Threading::kSingleThreaded;


template<enum Threading T>
catena::DeviceModel<T>::DeviceModel(const std::string& filename)
    : device_{} {
  auto jpopts = google::protobuf::util::JsonParseOptions{};
  std::string file = catena::readFile(filename);
  google::protobuf::util::JsonStringToMessage(file, &device_, jpopts);
}

template<enum Threading T>
const catena::Device& catena::DeviceModel<T>::device() const {
  LockGuard lock(mutex_);
  return device_;
}

template<enum Threading T>
typename catena::DeviceModel<T>::CachedParam
catena::DeviceModel<T>::getParam(const std::string& path) {
  // simple implementation for now, only handles flat params
  LockGuard lock(mutex_);
  catena::Path path_(path);
  if (path_.size() != 1) {
    std::stringstream why;
    why << __PRETTY_FUNCTION__ << " implementation limit, can only handle"
      "flat parameter structures at this time.";
    throw std::runtime_error(why.str());
  }

  // get our oid and look for it in the array of params
  catena::Path::Segment segment = path_.pop_front();

  if (!std::holds_alternative<std::string>(segment)) {
    std::stringstream why;
    why << __PRETTY_FUNCTION__;
    why << "expected oid, got an index";
    throw std::runtime_error(why.str());
  }
  std::string oid(std::get<std::string>(segment));

  if (!device_.mutable_params()->contains(oid)){
    std::stringstream why;
    why << __PRETTY_FUNCTION__ << ", " << __FILE__ << ':' << __LINE__;
    why << "param " << std::quoted(oid) << " not found";
    throw std::runtime_error(why.str());
  }

  return CachedParam(device_.mutable_params()->at(oid));
}

template<enum Threading T>
template<typename V>
typename catena::DeviceModel<T>::CachedParam
catena::DeviceModel<T>::getValue(V& ans, const std::string& path) {
  LockGuard lock(mutex_);
  CachedParam param = getParam(path);

  // N.B. function templates that are members of class templates
  // cannot be specialized, so we have to use conditional compilation based
  // on the tparam instead

  // specialize for float
  if constexpr(std::is_same<float, V>::value) {
    ans = param.theItem_.value().float32_value();
  }

  // specialize for int
  if constexpr(std::is_same<int, V>::value) {
    ans = param.theItem_.value().int32_value();
  }

  return param;
}

// instantiate the versions of getValue that have been implemented
template catena::DeviceModel<kMulti>::CachedParam catena::DeviceModel<kMulti>::getValue<float>(float& ans, const std::string& path);
template catena::DeviceModel<kSingle>::CachedParam catena::DeviceModel<kSingle>::getValue<float>(float& ans, const std::string& path);
template catena::DeviceModel<kMulti>::CachedParam catena::DeviceModel<kMulti>::getValue<int>(int& ans, const std::string& path);
template catena::DeviceModel<kSingle>::CachedParam catena::DeviceModel<kSingle>::getValue<int>(int& ans, const std::string& path);



template<enum Threading T>
template<typename V>
void catena::DeviceModel<T>::getValue(V& ans, const CachedParam& cp) {
  LockGuard lock(mutex_);
  
  // N.B. function templates that are members of class templates
  // cannot be specialized, so we have to use conditional compilation based
  // on the tparam instead

  // specialize for float
  if constexpr(std::is_same<float, V>::value) {
    ans = cp.theItem_.value().float32_value();
  }
}

/**
 * @brief internal implementation of the setValue method
 * 
 * @tparam T underlying type of param to set value of
 * @param p the param
 * @param v the value
 */
template<typename T>
void setValueImpl(catena::Param& p, T v);


template<>
void setValueImpl<float>(catena::Param& param, float v) {
  if (param.has_constraint()){
    // apply the constraint
    v = std::clamp(v,
      param.constraint().float_range().min_value(),
      param.constraint().float_range().max_value()
    );
  }
  param.mutable_value()->set_float32_value(v);
}

/**
 * @brief specialize for int
 * 
 * @throws std::range_error if the constraint type isn't valid
 * @tparam  
 * @param param 
 * @param v 
 */
template<>
void setValueImpl<int>(catena::Param& param, int v) {
  if (param.has_constraint()){
    // apply the constraint
    int constraint_type = param.constraint().type();
    switch (constraint_type) {
      case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_RANGE:
        v = std::clamp(v, 
          param.constraint().int32_range().min_value(),
          param.constraint().int32_range().max_value()
        );
        break;
      case catena::Constraint_ConstraintType::Constraint_ConstraintType_INT_CHOICE:
        // todo validate that v is one of the choices
        // fallthru is purposeful for now
      case catena::Constraint_ConstraintType::Constraint_ConstraintType_ALARM_TABLE:
        // we can't validate alarm tables easily, so trust the client
        break;
      default:
        std::stringstream why;
        why << __PRETTY_FUNCTION__;
        why << "invalid constraint for int32: " << constraint_type << '\n';
        throw std::range_error(why.str());
    }
  }
  param.mutable_value()->set_int32_value(v);  
}

template<enum Threading T>
template<typename V>
void catena::DeviceModel<T>::setValue(CachedParam& cp, V v) {
  LockGuard lock(mutex_);
  catena::Param& param{cp.theItem_};
  setValueImpl(param, v);
}

template<enum Threading T>
template<typename V>
typename catena::DeviceModel<T>::CachedParam
catena::DeviceModel<T>::setValue(const std::string& path, V v) {
  LockGuard lock(mutex_);
  CachedParam param = getParam(path);
  setValue(param, v);
  return param;
}

// instantiate the versions of setValue that have been implemented
template catena::DeviceModel<kMulti>::CachedParam catena::DeviceModel<kMulti>::setValue<float>(const std::string& path, float v);
template catena::DeviceModel<kSingle>::CachedParam catena::DeviceModel<kSingle>::setValue<float>(const std::string& path, float v);
template catena::DeviceModel<kMulti>::CachedParam catena::DeviceModel<kMulti>::setValue<int>(const std::string& path, int v);
template catena::DeviceModel<kSingle>::CachedParam catena::DeviceModel<kSingle>::setValue<int>(const std::string& path, int v);

template<enum Threading T>
std::ostream& operator<<(std::ostream& os, const catena::DeviceModel<T>& dm) {
  os << printJSON(dm.device());
  return os;
}

/**
 * @brief convenience wrapper around catena::Param
 * 
 * @param param from which to retreive the oid
 * @return param's object id
 */
template<enum Threading T>
const std::string& catena::DeviceModel<T>::getOid(const CachedParam& param) {
  LockGuard lock(mutex_);
  return param.theItem_.basic_param_info().oid();
}

// instantiate the 2 versions of DeviceModel, and its streaming operator
template class catena::DeviceModel<Threading::kMultiThreaded>;
template class catena::DeviceModel<Threading::kSingleThreaded>;
template std::ostream& operator<<(std::ostream& os, const catena::DeviceModel<Threading::kMultiThreaded>& dm);
template std::ostream& operator<<(std::ostream& os, const catena::DeviceModel<Threading::kSingleThreaded>& dm);