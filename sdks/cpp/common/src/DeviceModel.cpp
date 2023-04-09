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

/**
 * @brief default constructor, creates an empty model.
 *
 */
catena::DeviceModel::DeviceModel() : device_{} {}

/**
 * @brief copy constructor, makes deep copy of other's device model.
 */
catena::DeviceModel::DeviceModel(const DeviceModel& other) {
  // not implemented
}

/**
 * @brief assignment operator, makes deep copy of rhs's device model.
 * @todo implementation
 */
catena::DeviceModel& catena::DeviceModel::operator=(
    const catena::DeviceModel& rhs) {
  return *this;
}

/**
 * @brief move constructor, takes possession of other's state.
 * @todo implementation
 */
catena::DeviceModel::DeviceModel(DeviceModel&& other) {}

/**
 * @brief move assignment, moves rhs's state into self.
 * @todo implementation
 */
catena::DeviceModel catena::DeviceModel::operator=(
    DeviceModel&& rhs) {
  return *this;
}

/**
 * @brief construct from a catena protobuf Device object.
 *
 *
 */
catena::DeviceModel::DeviceModel(catena::Device& pbDevice)
    : device_(pbDevice) {}

/**
 * @brief construct from a JSON file.
 * @todo implementation
 */
catena::DeviceModel::DeviceModel(const std::string& filename)
    : device_{} {
  auto jpopts = google::protobuf::util::JsonParseOptions{};
  std::string file = catena::readFile(filename);
  google::protobuf::util::JsonStringToMessage(file, &device_, jpopts);
}

const catena::Device& catena::DeviceModel::device() const {return device_;}

catena::Param& catena::DeviceModel::getParam(const std::string& path) {
  // simple implementation for now, only handles flat params
  catena::Path path_(path);
  if (path_.size() != 1) {
    std::stringstream why;
    why << __PRETTY_FUNCTION__ << " implementation limit, can only handle"
      "flat parameter structures at this time.";
    throw std::runtime_error(why.str());
  }

  // get our oid and look for it in the array of params
  std::string oid = path_.pop_front();
  int nParams = device_.mutable_params()->descriptors_size();
  PDesc_t* descs = device_.mutable_params()->mutable_descriptors();
  bool found = false;
  int pdx = 0; // param index
  while (pdx < nParams && !found) {
    std::string match = descs->Get(pdx).basic_param_info().oid();
    if (oid.compare(match) == 0) {
      found = true;
    } else {
      ++pdx;
    }
  }
  if (!found) {
    std::stringstream why;
    why << __PRETTY_FUNCTION__;
    why << "Failed to find oid '" << oid << "'\n";
    throw std::runtime_error(why.str());
  }
  return descs->at(pdx);
}

template<>
catena::Param& catena::DeviceModel::getValue<float>(float& ans, const std::string& path) {
  catena::Param& param = getParam(path);
  ans = param.value().float32_value();
  return param;
}

template<>
void catena::DeviceModel::setValue<float>(catena::Param& param, float v) {
  param.mutable_value()->set_float32_value(v);
}

template<>
catena::Param& catena::DeviceModel::setValue<float>(const std::string& path, float v) {
  catena::Param& param = getParam(path);
  setValue(param, v);
  return param;
}

std::ostream& operator<<(std::ostream& os, const catena::DeviceModel& dm) {
   os << printJSON(dm.device());
   return os;
}