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
#include <Path.h>
#include <Threading.h>
#include <Status.h>
#include <mutex>
#include <memory>

#include <iostream>
#include <string>
#include <type_traits>
#include <functional>

namespace catena {

/**
 * @brief Provide access to the Catena data model that's similar to the
 * ogscript API in DashBoard.
 *
 * @tparam T controls whether the class asserts locks when being
 * accessed
 *
 * The data model access methods all begin with code to assert a lock guard.
 * When threadsafe mode is selected, this is of type
 * std::lock_guard<std::recursive_mutex>.
 * When false, it is of type catena::FakeLockGuard<FakeMutex> which is an empty,
 * do-nothing struct that is completely optimized out of the
 * implementation by asserting the -O1 (or higher) compiler option. 
 * Design motivation - small, single threaded devices shouldn't
 * be asserting locks pointlessly, they're resource bound enough.
 *
 */
template <enum Threading T = Threading::kMultiThreaded>
class DeviceModel {
public:
  /**
   * @brief which threading model is active.
   *
   */
  const catena::Threading kThreading = T;

  /**
   * @brief Choose the mutex type
   *
   */
  using Mutex =
      typename std::conditional<T == catena::Threading::kMultiThreaded,
                                std::recursive_mutex, FakeMutex>::type;

  /**
   * @brief Choose the lock guard type
   *
   */
  using LockGuard =
      typename std::conditional<T == catena::Threading::kMultiThreaded,
                                std::lock_guard<Mutex>,
                                FakeLockGuard<Mutex>>::type;

  /**
   * @brief Param Descriptor type
   *
   */
  using PDesc = google::protobuf::RepeatedPtrField<catena::Param>;

  // /**
  //  * @brief Provides a way of sharing the device model's internal state
  //  * with client programs without letting them directly access it.
  //  *
  //  * Design Motivation - it can be expensive to retreive items from the
  //  * data model. So, once acquired it's nice for a client program to
  //  * hold onto them yet still only use the device model interface to
  //  * manipulate the state. N.B. the device model interface provides thread
  //  * safety by locking every access to its state. Providing direct access
  //  * of state items to clients would put the onus of guaranteeing thread
  //  * safety on the client which isn't a great idea.
  //  *
  //  * This is achieved by CachedItem holding a private reference to the item
  //  * of state that only DeviceModel can access by dint of its friendship with
  //  * this class template.
  //  *
  //  * @tparam T the type of the cached object
  //  */
  // template <typename I> class CachedItem {
  // public:
  //   /**
  //    * @brief provide DeviceModel access to the private attribute.
  //    *
  //    */
  //   friend DeviceModel<Threading::kSingleThreaded>;

  //   /**
  //    * @brief provide DeviceModel access to the private attribute.
  //    *
  //    */
  //   friend DeviceModel<Threading::kMultiThreaded>;

  //   /**
  //    * @brief CachedItems cannot be default constructed.
  //    *
  //    */
  //   CachedItem() = delete;

  //   /**
  //    * @brief CachedItems cannot be copy constructed.
  //    *
  //    */
  //   CachedItem(const CachedItem &) = delete;

  //   /**
  //    * @brief CachedItems cannot be copy assigned.
  //    *
  //    * @return CachedItem&
  //    */
  //   CachedItem &operator=(const CachedItem &) = delete;

  //   /**
  //    * @brief CachedItems have move semantics.
  //    *
  //    * @param other
  //    */
  //   CachedItem(CachedItem &&other) : theItem_{other.theItem_} {}

  //   /**
  //    * @brief CachedItems have move semantics.
  //    *
  //    * @param rhs right hand side of the = sign
  //    * @return CachedItem&
  //    */
  //   CachedItem &operator=(CachedItem &&rhs) {
  //     theItem_ = std::move(rhs.theItem_);
  //     return *this;
  //   };
  //
  //   /**
  //    * @brief Main constructor
  //    *
  //    * @param item reference to the item to cache
  //    */
  //   CachedItem(I &item) : theItem_{item} {};

  // private:
  //   I &theItem_;
  // };

  /**
   * @brief wrapper around a catena::Param stored in the device model
   *
   * Provides convenient accessor methods that are made threadsafe using
   * the DeviceModel's mutex.
   * 
   * Enables client programs to "cache" a param without having to use a 
   * method involving potentially expensive searches.
   *
   */
  class Param {
    friend DeviceModel; /**< access to private state is useful */

  public:
    /**
     * @brief Construct a new Param object
     *
     * @param dm the parent device model.
     * @param p the parameter owned by the device model.
     */
    Param(DeviceModel &dm, catena::Param &p) noexcept
        : deviceModel_{dm}, param_{p} {}

    /**
     * @brief Param has no default constructor.
     *
     */
    Param() = delete;

    /**
     * @brief Param has copy semantics.
     *
     * @param other
     */
    Param(const Param &other) = default;

    /**
     * @brief Param has copy semantics.
     *
     * @param rhs
     */
    Param &operator=(const Param &rhs) = default;

    /**
     * @brief Param does not have move semantics.
     *
     * @param other
     */
    Param(Param &&other) = delete;

    /**
     * @brief Param does not have move semantics
     *
     * @param rhs, right hand side of the equals sign
     */
    Param &operator=(Param &&rhs) = delete;

    inline ~Param() { // nothing to do
    }

    /**
     * @brief Get the value stored by the catena::Param
     *
     * Threadsafe because it asserts the DeviceModel's mutex.
     *
     * @tparam V value type of param
     * @return V value of parameter
     */
    template <typename V> V getValue();

    /**
     * @brief Set the value of the stored catena::Param.
     *
     * Threadsafe because it asserts the DeviceModel's mutex.
     *
     * @tparam V type of the value stored by the param.
     * @param v value to set.
     */
    template <typename V> void setValue(V v);

    /**
     * @brief Set value of the stored catena::Param at index idx.
     * 
     * @tparam V type of the value stored
     * @param v value to set
     * @param idx index into the array
     */
    template <typename V> void setValueAt(V v, size_t idx);

  private:
    std::reference_wrapper<DeviceModel> deviceModel_;
    std::reference_wrapper<catena::Param> param_;
  };

  friend Param; /**< used for accessing mutex_ attribute.*/

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
  DeviceModel(const DeviceModel &other) {}

  /**
   * @brief Assignment operator, makes deep copy of rhs's device model.
   * @param rhs the right hand side of the = operator
   * @todo implementation
   */
  DeviceModel &operator=(const DeviceModel &rhs) {
    LockGuard lock(mutex_);
    // not implemented
    return *this;
  }

  /**
   * @brief Move constructor, takes possession of other's state
   * @param other the donor object
   * @todo implementation
   */
  DeviceModel(DeviceModel &&other) {
    // not implemented
  }

  /**
   * @brief Move assignment, takes possession of rhs's state
   * @todo implementation
   * @param rhs the right hand side of the = operator
   * @return DeviceModel
   */
  DeviceModel operator=(DeviceModel &&rhs) {
    // not implemented
    return *this;
  }

  /**
   * @brief construct from a catena protobuf Device object.
   * @todo implementation
   */
  explicit DeviceModel(catena::Device &pbDevice) {
    // not implemented
  }

  /**
   * @brief Construct a new Device Model from a json file
   *
   * @param filename
   */
  explicit DeviceModel(const std::string &filename);

  /**
   * @brief read access to the protobuf Device
   *
   * @return const catena::Device&
   */
  const catena::Device &device() const;

  /**
   * @brief Get the Param object at path
   *
   * @param path uniquely locates the parameter
   * @throws catena::exception_with_status if the code to navigate
   * to the requested fully qualified oid has not been implemented.
   * @throws catena::exception_with_status if the requested oid is not present in the
   * device model
   * @return DeviceModel Param
   */
  Param param(const std::string &path);

  /**
   * @brief Get the value of the parameter indicated by path
   *
   * @tparam V type of the value to be retrieved
   * @param ans [out] value retreived
   * @param path unique oid to get
   * @return param at the path because finding params can be
   * expensive and the client is likely to want to access the param
   * again after getting its value
   */
  template <typename V> void getValue(V &ans, const std::string &path);

  /**
   * @brief Set value of param identified by path
   *
   * @tparam V underlying value type of param
   * @param path unique oid of param
   * @param v value to set
   * @return the param indicated by path because finding params can be
   * expensive and the client is likely to want to access the param
   * again after setting its value
   */
  template <typename V> void setValue(const std::string &path, V v);

  /**
   * @brief Get the param's oid
   *
   * @param param from which to retrieve the oid
   * @return const std::string& oid
   */
  const std::string &getOid(const Param &param);

  /**
   * @brief moves the param into the device model
   *
   * @param jptr - json pointer to the place to insert the param, must be
   * escaped.
   * @param param the param to be added to the device model
   * @returns cached version of param which client can use
   * for ongoing access to the param in a threadsafe way
   */
  Param addParam(const std::string &jptr, catena::Param &&param);

private:
  catena::Device device_; /**< the protobuf device model */
  mutable Mutex mutex_;   /**< used to mediate access */

  /**
   * @brief Get the parent's sub-param at front of path
   *
   * @param path [in|out] path to the sub-param, the first segment will be
   * consumed.
   * @param parent param
   * @return child param indicated by front of path
   * @throws catena::exception_with_status if is STRUCT_ARRAY
   * @throws catena::exception_with_status if parent is not a sub-param supporting param
   * @throws catena::exception_with_status if param doesn't have a values object
   * type
   *
   */
  catena::Param *getSubparam_(catena::Path &path, catena::Param &parent);
};

} // namespace catena

/**
 * @brief operator << for DeviceModel
 *
 * @relates catena::DeviceModel
 * @param os the ostream to stream to
 * @param dm the device model to stream
 * @return updated os
 */
template <enum catena::Threading T = catena::Threading::kMultiThreaded>
std::ostream &operator<<(std::ostream &os, const catena::DeviceModel<T> &dm);
