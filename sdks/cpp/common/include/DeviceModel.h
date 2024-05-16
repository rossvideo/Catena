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
#include <service.grpc.pb.h>

#include <signals.h>

#include <Path.h>
#include <Status.h>

#include <mutex>
#include <memory>
#include <iostream>
#include <string>
#include <type_traits>
#include <functional>
#include <fstream>
#include <filesystem>

using grpc::ServerWriter;

namespace catena {

/**
 * a fake lock for use in recursive function calls
 */
struct FakeLock {
    FakeLock(std::mutex &) {}
};

class ParamAccessor;  // forward reference
class DeviceStream;   // forward reference

/**
 * @brief type for indexing into parameters
 *
 */
using ParamIndex = uint32_t;

/**
 * @brief Provide access to the Catena data model that's similar to the
 * ogscript API in DashBoard.
 *
 * The data model access methods all begin with code to assert a lock guard.
 *
 */
class DeviceModel {
    friend ParamAccessor; /**< so this class can access the DeviceModel's mutex */
    friend DeviceStream;

  public:
    /**
     * @brief Mutex type
     *
     */
    using Mutex = std::mutex;

    /**
     * @brief Param Accessor Data
     *
     * Yes, this could be a std::pair, but I've a hunch that we'll need to add
     * a pointer to catena::Constraint too in the near future because constraints
     * can be referenced and not only defined in-line.
     */
    using ParamAccessorData = std::tuple<catena::Param *, catena::Value *>;

    /**
     * @brief Params Map
     */
    using ParamsMap = google::protobuf::Map<std::string, ::catena::Param>;

    /**
     * @brief default constructor, creates an empty model.
     * @todo implement DeviceModel::DeviceModel()
     */
    DeviceModel();

    /**
     * @brief Copy constructor, makes deep copy of other's device model.
     * @todo implement DeviceModel::DeviceModel(const DeviceModel &other)
     * @param other the DeviceModel to copy.
     */
    DeviceModel(const DeviceModel &other);

    /**
     * @brief Assignment operator, makes deep copy of rhs's device model.
     * @param rhs the right hand side of the = operator
     * @todo implement DeviceModel::operator=(const DeviceModel &rhs)
     */
    DeviceModel &operator=(const DeviceModel &rhs);

    /**
     * @brief Move constructor, takes possession of other's state
     * @param other the donor object
     * @todo implement DeviceModel::DeviceModel(DeviceModel &&other)
     */
    DeviceModel(DeviceModel &&other);

    /**
     * @brief Move assignment, takes possession of rhs's state
     * @todo implement DeviceModel::operator=(DeviceModel &&rhs)
     * @param rhs the right hand side of the = operator
     * @return DeviceModel
     */
    DeviceModel operator=(DeviceModel &&rhs);

    /**
     * @brief construct from a catena protobuf Device object.
     * @todo implement DeviceModel::DeviceModel(Device &&pbDevice)
     */
    explicit DeviceModel(catena::Device &pbDevice);

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

     * @brief sends device info to client via writer
     * @param writer the writer to send the device info to
     * @param tag the tag to associate with the stream
     * @return true if the device model was sent, false is there's more to come.
     */
    bool streamDevice(grpc::ServerAsyncWriter<::catena::DeviceComponent> *writer, void* tag);

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
    std::unique_ptr<ParamAccessor> param(const std::string &path);

    /**
     * @brief moves the param into the device model
     *
     * @param jptr - json pointer to the place to insert the param, must be
     * escaped.
     * @param param the param to be added to the device model
     * @returns cached version of param which client can use
     * for ongoing access to the param in a threadsafe way
     * @todo implement DeviceModel::addParam(const std::string &jptr, Param &&param)
     */
    Param addParam(const std::string &jptr, Param &&param);

  private:
    /**
     * @brief import sub-params from a folder
     *
     * @param current_folder the folder to import from
     * @param params the params to import into
     */
    void importSubParams_(std::filesystem::path &current_folder, ParamsMap &params);

  private:
    catena::Device device_;        /**< the protobuf device model */
    mutable Mutex mutex_;          /**< used to mediate access */
    static catena::Value noValue_; /**< to flag undefined values */

  public:

   /**
    *  signal to share value changes by clients 
    * 
    */
    vdk::signal<void(const ParamAccessor&, ParamIndex idx, const std::string&)> valueSetByClient; 

    /**
    *  signal to share value changes to all connected clients with proper authorization
    */
    vdk::signal<void(const ParamAccessor&, ParamIndex idx)> pushUpdates;
};

class DeviceStream{
  public:
    DeviceStream(DeviceModel &dm);

    ~DeviceStream();

    const catena::DeviceComponent& next();

    bool hasNext();

    

  private:
    catena::DeviceComponent& basicDeviceInfo();

    enum class ComponentType {
      BASIC_DEVICE_INFO,
      PARAM,
      CONSTRAINT,
      MENU,
      COMMAND,
      LANGUAGE_PACK,
      FINISHED
    };
    
    std::reference_wrapper<DeviceModel> deviceModel_;
    ComponentType nextType_;
    DeviceComponent component_;
};

}  // namespace catena

/**
 * @brief operator << for DeviceModel
 *
 * @relates catena::DeviceModel
 * @param os the ostream to stream to
 * @param dm the device model to stream
 * @return updated os
 */
std::ostream &operator<<(std::ostream &os, const catena::DeviceModel &dm);
