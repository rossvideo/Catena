#pragma once

/**
 * @brief API for using the Catena device model.
 * @file DeviceModel.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

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

 #include <device.pb.h>

 #include <iostream>

 namespace catena {

/**
* @brief Provide access to the Catena data model that's similar to the 
* ogscript API in DashBoard.
* 
*/
class DeviceModel {
public:
   /**
   * @brief default constructor, creates an empty model.
   * 
   */
   DeviceModel();

   /**
    * @brief Copy constructor, makes deep copy of other's device model.
    * 
    * @param other the DeviceModel to copy.
    */
   DeviceModel(const DeviceModel& other);

   /**
    * @brief Assignment operator, makes deep copy of rhs's device model.
    * @param rhs, the right hand side of the = operator
    * 
    */
   DeviceModel& operator=(const DeviceModel& rhs);

   /**
    * @brief Move constructor, takes possession of other's state
    * @param other, the donor object
    */
   DeviceModel(DeviceModel&& other);

   /**
   * @brief Move assignment, takes possession of rhs's state
   * 
   * @param rhs the right hand side of the = operator
   * @return DeviceModel 
   */
   DeviceModel operator=(DeviceModel&& rhs);

   /**
   * @brief construct from a catena protobuf Device object.
   *
   */
  explicit DeviceModel(catena::Device& pbDevice);

   /**
   * @brief Construct a new Device Model from a json file
   * 
   * @param filename 
   */
   explicit DeviceModel(const std::string& filename);

   /**
    * @brief read access to the protobuf Device
    * 
    * @return const catena::Device& 
    */
   const catena::Device& device() const;

private:
   catena::Device device_; /**< the protobuf device model */
};

} // catena namespace

/**
 * @brief operator << for DeviceModel
 * 
 * @relates catena::DeviceModel
 * @param os the ostream to stream to
 * @param dm the device model to stream
 * @return updated os
 */
std::ostream& operator<<(std::ostream& os, const catena::DeviceModel& dm);
