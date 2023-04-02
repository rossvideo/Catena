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

#include <google/protobuf/map.h>
#include <google/protobuf/util/json_util.h>

#include <fstream>
#include <iostream>
#include <sstream>

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

std::ostream& operator<<(std::ostream& os, const catena::DeviceModel& dm) {
   os << printJSON(dm.device());
   return os;
}