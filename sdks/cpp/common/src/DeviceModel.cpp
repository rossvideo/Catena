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
using google::protobuf::Map;

const auto kMulti = catena::Threading::kMultiThreaded;
const auto kSingle = catena::Threading::kSingleThreaded;

template <enum Threading T>
catena::DeviceModel<T>::DeviceModel(const std::string &filename) : device_{} {
  auto jpopts = google::protobuf::util::JsonParseOptions{};
  // read in the top level file
  {
    std::string file = catena::readFile(filename);
    google::protobuf::util::JsonStringToMessage(file, &device_, jpopts);
  }

  // read in imported params
  // the top-level ones will come from path/to/device/params
  //
  std::filesystem::path current_folder(filename);
  current_folder = current_folder.remove_filename() /= "params";

  for (auto it = device_.mutable_params()->begin();
       it != device_.mutable_params()->end(); ++it) {
    std::cout << it->first << '\n';
    catena::ParamDescriptor &pdesc = device_.mutable_params()->at(it->first);
    if (pdesc.has_import()) {
      std::string imported;
      if (pdesc.import().url().compare("") != 0) {
        /** @todo implement url imports*/
        EXCEPTION("Cannot (yet) import from urls, sorry.",
                  catena::not_implemented);
      } else {
        // there's no url specified, so do a local import using the oid
        // to create the filename
        std::filesystem::path to_import(current_folder);
        std::stringstream fn;
        fn << "param." << it->first << ".json";
        to_import /= fn.str();
        std::cout << "importing: " << to_import << '\n';

        // import the file
        imported = readFile(to_import);
      }

      // clear the "import" typing of the current param
      // and overwrite with what we just imported
      pdesc.clear_import();
      google::protobuf::util::JsonStringToMessage(
          imported, pdesc.mutable_param(), jpopts);
    }
  }
}

template <enum Threading T>
const catena::Device &catena::DeviceModel<T>::device() const {
  LockGuard lock(mutex_);
  return device_;
}

template <enum Threading T>
typename catena::DeviceModel<T>::Param
catena::DeviceModel<T>::getParam(const std::string &jptr) {
  // simple implementation for now, only handles flat params
  LockGuard lock(mutex_);
  catena::Path path_(jptr);

  // get our oid and look for it in the array of params
  catena::Path::Segment segment = path_.pop_front();

  if (!std::holds_alternative<std::string>(segment)) {
    EXCEPTION("expected oid, got an index", std::runtime_error);
  }
  std::string oid(std::get<std::string>(segment));

  if (!device_.mutable_params()->contains(oid)) {
    std::stringstream msg;
    msg << "param " << std::quoted(oid) << " not found";
    EXCEPTION((msg.str()), std::runtime_error);
  }

  catena::Param *ans = device_.mutable_params()->at(oid).mutable_param();
  while (path_.size()) {
    ans = getSubparam_(path_, *ans);
  }

  return Param(*this, *ans);
}

template <enum Threading T>
catena::Param *catena::DeviceModel<T>::getSubparam_(catena::Path &path,
                                                    catena::Param &parent) {

  // validate the param type
  catena::ParamType_ParamTypes type =
      parent.basic_param_info().type().param_type();
  switch (type) {
  case catena::ParamType_ParamTypes::ParamType_ParamTypes_STRUCT:
    // this is ok
    break;
  case catena::ParamType_ParamTypes::ParamType_ParamTypes_STRUCT_ARRAY:
    /** @todo implement sub-param navigation for STRUCT_ARRAY */
    EXCEPTION("sub-param navigation for STRUCT_ARRAY not implemented, sorry",
              catena::not_implemented);
    break;
  default:
    std::stringstream err;
    err << "cannot sub-param param of type: " << type;
    EXCEPTION((err.str()), std::invalid_argument);
  }

  // validate path argument and the parameter

  // is there a value field? It's optional, after all.
  if (!parent.has_value()) {
    EXCEPTION("value field is missing", std::runtime_error);
  }

  // is there a struct-value field?
  if (!parent.value().has_struct_value()) {
    EXCEPTION("struct_value field is missing", catena::schema_error);
  }

  // is our oid a string?
  auto seg = path.pop_front();
  if (!std::holds_alternative<std::string>(seg)) {
    EXCEPTION("expected oid, got index", std::invalid_argument);
  }

  // fourth, is the oid in the value.struct_value.fields object?
  std::string oid(std::get<std::string>(seg));
  if (!parent.value().struct_value().fields().contains(oid)) {
    std::stringstream err;
    err << oid << " not found";
    EXCEPTION((err.str()), catena::schema_error);
  }

  return parent.mutable_value()
      ->mutable_struct_value()
      ->mutable_fields()
      ->at(oid)
      .mutable_param();
}

template <enum Threading T>
template <typename V>
void catena::DeviceModel<T>::getValue(V &ans, const std::string &path) {
  LockGuard lock(mutex_);
  catena::Param& param_ = getParam(path).param_;

  // N.B. function templates that are members of class templates
  // cannot be specialized, so we have to use conditional compilation based
  // on the tparam instead

  // specialize for float
  if constexpr (std::is_same<float, V>::value) {
    ans = param_.value().float32_value();
  }

  // specialize for int
  if constexpr (std::is_same<int, V>::value) {
    ans = param_.value().int32_value();
  }
}

// instantiate the versions of getValue that have been implemented
template void
catena::DeviceModel<kMulti>::getValue<float>(float &ans,
                                             const std::string &path);
template void
catena::DeviceModel<kSingle>::getValue<float>(float &ans,
                                              const std::string &path);
template void
catena::DeviceModel<kMulti>::getValue<int>(int &ans, const std::string &path);
template void
catena::DeviceModel<kSingle>::getValue<int>(int &ans, const std::string &path);

template <enum Threading T>
template <typename V>
void catena::DeviceModel<T>::getValue(V &ans, const Param &cp) {
  LockGuard lock(mutex_);

  // N.B. function templates that are members of class templates
  // cannot be specialized, so we have to use conditional compilation based
  // on the tparam instead

  // specialize for float
  if constexpr (std::is_same<float, V>::value) {
    ans = cp.param_.value().float32_value();
  }
}

/**
 * @brief internal implementation of the setValue method
 *
 * @tparam T underlying type of param to set value of
 * @param p the param
 * @param v the value
 */
template <typename T> void setValueImpl(catena::Param &p, T v);

template <> void setValueImpl<float>(catena::Param &param, float v) {
  if (param.has_constraint()) {
    // apply the constraint
    v = std::clamp(v, param.constraint().float_range().min_value(),
                   param.constraint().float_range().max_value());
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
template <> void setValueImpl<int>(catena::Param &param, int v) {
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
      // todo validate that v is one of the choices
      // fallthru is purposeful for now
    case catena::Constraint_ConstraintType::
        Constraint_ConstraintType_ALARM_TABLE:
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

template <enum Threading T>
template <typename V>
void catena::DeviceModel<T>::setValue(Param &cp, V v) {
  LockGuard lock(mutex_);
  catena::Param &param{cp.param_};
  setValueImpl(param, v);
}

template <enum Threading T>
template <typename V>
void
catena::DeviceModel<T>::setValue(const std::string &path, V v) {
  LockGuard lock(mutex_);
  Param param = getParam(path);
  setValue(param, v);
}

// instantiate the versions of setValue that have been implemented
template void
catena::DeviceModel<kMulti>::setValue<float>(const std::string &path, float v);
template void
catena::DeviceModel<kSingle>::setValue<float>(const std::string &path, float v);
template void
catena::DeviceModel<kMulti>::setValue<int>(const std::string &path, int v);
template void
catena::DeviceModel<kSingle>::setValue<int>(const std::string &path, int v);

template <enum Threading T>
std::ostream &operator<<(std::ostream &os, const catena::DeviceModel<T> &dm) {
  os << printJSON(dm.device());
  return os;
}

template <enum Threading T>
const std::string &catena::DeviceModel<T>::getOid(const Param &param) {
  LockGuard lock(mutex_);
  return param.param_.basic_param_info().oid();
}

template <enum Threading T>
typename catena::DeviceModel<T>::Param
catena::DeviceModel<T>::addParam(const std::string &jptr,
                                 catena::Param &&param) {
  catena::Path path(jptr);
  LockGuard lock(mutex_);
  if (path.size() > 1) {
    /** @todo implement ability to add parameters below /params */
    std::stringstream why;
    why << __FILE__ << ":" << __LINE__ << "\n" << __PRETTY_FUNCTION__ << '\n';
    why << "implementation only supports adding params to top level";
    throw catena::not_implemented(why.str());
  }
  if (path.size() == 0) {
    std::stringstream why;
    why << __FILE__ << ":" << __LINE__ << "\n" << __PRETTY_FUNCTION__ << '\n';
    why << "empty path is invalid in this context";
    throw std::invalid_argument(why.str());
  }
  catena::Param p(param);
  auto seg = path.pop_front();
  if (!std::holds_alternative<std::string>(seg)) {
    std::stringstream why;
    why << __FILE__ << ":" << __LINE__ << "\n" << __PRETTY_FUNCTION__ << '\n';
    why << "invalid path: " << std::quoted(jptr);
    throw std::invalid_argument(why.str());
  }
  std::string oid(std::get<std::string>(seg));

  *(*device_.mutable_params())[oid].mutable_param() = p;
  return Param(*this, *device_.mutable_params()->at(oid).mutable_param());
}

template <typename V> V getValueImpl(const catena::Param &p);

template <> float getValueImpl<float>(const catena::Param &p) {
  return p.value().float32_value();
}

template <> int getValueImpl<int>(const catena::Param &p) {
  return p.value().int32_value();
}

template <enum Threading T>
template <typename V>
V catena::DeviceModel<T>::Param::getValue() {
  using W = typename std::remove_reference<V>::type;
  DeviceModel::LockGuard(deviceModel_.mutex_);
  if constexpr (std::is_same<W, float>::value) {
    return getValueImpl<float>(param_);
  }

  if constexpr (std::is_same<W, int>::value) {
    return getValueImpl<int>(param_);
  }
}

template float catena::DeviceModel<kMulti>::Param::getValue<float>();
template float catena::DeviceModel<kSingle>::Param::getValue<float>();
template int catena::DeviceModel<kMulti>::Param::getValue<int>();
template int catena::DeviceModel<kSingle>::Param::getValue<int>();

template <enum Threading T>
template <typename V>
void catena::DeviceModel<T>::Param::setValue(V v) {
  LockGuard lock(deviceModel_.mutex_);
  setValueImpl(param_, v);
}


template void catena::DeviceModel<kMulti>::Param::setValue<float>(float);
template void catena::DeviceModel<kSingle>::Param::setValue<float>(float);
template void catena::DeviceModel<kMulti>::Param::setValue<int>(int);
template void catena::DeviceModel<kSingle>::Param::setValue<int>(int);

// instantiate the 2 versions of DeviceModel, and its streaming operator
template class catena::DeviceModel<Threading::kMultiThreaded>;
template class catena::DeviceModel<Threading::kSingleThreaded>;
template std::ostream &
operator<<(std::ostream &os,
           const catena::DeviceModel<Threading::kMultiThreaded> &dm);
template std::ostream &
operator<<(std::ostream &os,
           const catena::DeviceModel<Threading::kSingleThreaded> &dm);