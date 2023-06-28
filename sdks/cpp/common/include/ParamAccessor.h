#pragma once

/**
 * @brief Convenient access to Params in the DeviceModel
 * @file ParamAccessor.h
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

#include <constraint.pb.h>
#include <device.pb.h>
#include <param.pb.h>

#include <Fake.h>
#include <Path.h>
#include <Status.h>
#include <Threading.h>

#include <functional>
#include <iostream>
#include <string>
#include <type_traits>

namespace catena {

/**
 * @brief wrapper around a catena::ParamAccessor stored in the device model
 *
 * Provides convenient accessor methods that are made threadsafe using
 * the DeviceModel's mutex.
 *
 * Enables client programs to "cache" a param without having to use a
 * method involving potentially expensive searches.
 *
 */
template <typename DM> class ParamAccessor {

  // prevent use of this class outside of DeviceModel
  static_assert(
      std::is_same_v<DM,
                     catena::DeviceModel<catena::Threading::kMultiThreaded>> ||
          std::is_same_v<
              DM, catena::DeviceModel<catena::Threading::kSingleThreaded>>,
      "Class Template Parameter must be of type DeviceModel<T>");

public:
  /**
   * @brief Construct a new ParamAccessor object
   *
   * @param dm the parent device model.
   * @param p the parameter owned by the device model.
   */

  ParamAccessor(DM &dm, typename DM::ParamAccessorData &pad) noexcept
      : deviceModel_{dm}, param_{*std::get<0>(pad)}, value_{*std::get<1>(pad)} {
  }

  /**
   * @brief ParamAccessor has no default constructor.
   *
   */
  ParamAccessor() = delete;

  /**
   * @brief ParamAccessor has copy semantics.
   *
   * @param other
   */
  ParamAccessor(const ParamAccessor &other) = default;

  /**
   * @brief ParamAccessor has copy semantics.
   *
   * @param rhs
   */
  ParamAccessor &operator=(const ParamAccessor &rhs) = default;

  /**
   * @brief ParamAccessor does not have move semantics.
   *
   * @param other
   */
  ParamAccessor(ParamAccessor &&other) = delete;

  /**
   * @brief ParamAccessor does not have move semantics
   *
   * @param rhs, right hand side of the equals sign
   */
  ParamAccessor &operator=(ParamAccessor &&rhs) = delete;

  inline ~ParamAccessor() { // nothing to do
  }

  /**
   * @brief Get the value stored by the catena::ParamAccessor
   *
   * Threadsafe because it asserts the DeviceModel's mutex.
   *
   * @tparam V value type of param
   * @return V value of parameter
   */
  template <typename V> V getValue();

  /**
   * @brief Set the value of the stored catena::ParamAccessor.
   *
   * Threadsafe because it asserts the DeviceModel's mutex.
   *
   * @tparam V type of the value stored by the param.
   * @param v value to set.
   */
  template <typename V> void setValue(V v);

  /**
   * @brief Set value of the stored catena::ParamAccessor at index idx.
   *
   * @tparam V type of the value stored
   * @param v value to set
   * @param idx index into the array
   */
  template <typename V> void setValueAt(V v, uint32_t idx);

private:
  std::reference_wrapper<DM> deviceModel_;
  std::reference_wrapper<catena::Param> param_;
  std::reference_wrapper<catena::Value> value_;
};
} // namespace catena