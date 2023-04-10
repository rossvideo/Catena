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
 #include <param.pb.h>

 #include <Fake.h>

 #include <iostream>
 #include <string>
 #include <type_traits>


 namespace catena {

/**
* @brief Provide access to the Catena data model that's similar to the 
* ogscript API in DashBoard.
* 
* @tparam THREADSAFE controls whether the class asserts locks when being
* accessed
* 
*/
template <bool THREADSAFE = true>
class DeviceModel {
public:
   /**
    * @brief Choose the mutex type depending on the value of the
    * THREADSAFE tparam
    * 
    */
   using Mutex_t = typename std::conditional<
      THREADSAFE, 
      std::recursive_mutex,
      FakeMutex
   >::type;

   /**
    * @brief Choose the lock guard type depending on the value of the
    * THREADSAFE tparam
    * 
    */
   using LockGuard_t = typename std::conditional<
      THREADSAFE, 
      std::lock_guard<Mutex_t>,
      FakeLockGuard<Mutex_t>
   >::type;

   /**
   * @brief Param Descriptor type
   *
   */
   using PDesc_t = google::protobuf::RepeatedPtrField<catena::Param>;

   /**
   * @brief default constructor, creates an empty model.
   * 
   */
   DeviceModel() {}

   /**
    * @brief Copy constructor, makes deep copy of other's device model.
    * @todo implementation
    * @param other the DeviceModel to copy.
    */
   DeviceModel(const DeviceModel& other) {}

   /**
    * @brief Assignment operator, makes deep copy of rhs's device model.
    * @param rhs the right hand side of the = operator
    * @todo implementation
    */
   DeviceModel& operator=(const DeviceModel& rhs) {
      LockGuard_t lock(mutex_);
      return *this;
   }

   /**
    * @brief Move constructor, takes possession of other's state
    * @param other the donor object
    * @todo implementation
    */
   DeviceModel(DeviceModel&& other){}

   /**
   * @brief Move assignment, takes possession of rhs's state
   * @todo implementation
   * @param rhs the right hand side of the = operator
   * @return DeviceModel 
   */
   DeviceModel operator=(DeviceModel&& rhs) {
      return *this;
   }

   /**
   * @brief construct from a catena protobuf Device object.
   * @todo implementation
   */
   explicit DeviceModel(catena::Device& pbDevice) {}

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

   /**
    * @brief Get the Param object at path
    * 
    * @param path uniquely locates the parameter
    * @return catena::Param& 
    */
   catena::Param& getParam(const std::string& path);

   /**
    * @brief Get the value of the parameter indicated by path
    * 
    * @tparam T type of the value to be retrieved
    * @param ans [out] value retreived
    * @param path unique oid to get
    * @return catena::Param& at the path
    */
   template<typename T>
   catena::Param& getValue(T& ans, const std::string& path);


   /**
    * @brief Set value of param identified by path
    * 
    * @tparam T underlying value type of param
    * @param path unique oid of param
    * @param v value to set, passed by value for fundamental types, reference for others
    */
   template<typename T>
   catena::Param& setValue(const std::string& path, T v);

/**
 * @brief Get the param's oid
 * 
 * @param param from which to retrieve the oid
 * @return const std::string& oid
 */
const std::string& getOid(const catena::Param& param);

/**
 * @brief Set the param's value
 * 
 * @tparam T  value type of param
 * @param param 
 * @param v value to set, passed by value for fundamental types, reference for others
 */

template<typename T>
void setValue(catena::Param& param, T v);

/**
 * @brief get the param's value
 * 
 * @tparam T type of parameter's value
 * @param param 
 * @return value of param
 */
template<typename T>
void getValue(T& ans, const catena::Param& param);


private:
   catena::Device device_; /**< the protobuf device model */
   mutable Mutex_t mutex_; /**< used to mediate access */
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
template<bool THREADSAFE = true>
std::ostream& operator<<(std::ostream& os, const catena::DeviceModel<THREADSAFE>& dm);
