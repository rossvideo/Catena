#pragma once
/**
 * @brief Param analog to catena::Param
 * @file Param.h
 * @copyright Copyright © 2024 Ross Video Ltd
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

#include <constraint.pb.h>
#include <device.pb.h>
#include <param.pb.h>

#include <Functory.h>
#include <Path.h>
#include <Status.h>
#include <TypeTraits.h>
#include <Param.h>

#include <string>
#include <assert.h>
#include <unordered_map>
#include <filesystem>


namespace catena {
namespace sdk {


class Device {
  public:
    using ParamsMap = IParam::ParamsMap;
    
  public:
    /**
     * @brief default constructor
     */
    Device() = default;

    /**
     * @brief virtual destructor
     */
    virtual ~Device() = default;

    /**
     * @brief Construct from file
     */
    Device(const std::string& filename);

    /**
     * @brief assign from a protobuf Device message
     */
    Device& operator=(const catena::Device& pbufDevice);

    /**
     * @brief add a parameter to the device
     */
    void addParam(const std::string& oid, const catena::Param& param);

    /**
     * @brief get a parameter by oid
     */
    catena::sdk::IParam* param(const std::string& oid);

  private:
    void importSubParams_(std::filesystem::path& current_folder, ParamsMap& params);

  private:
    ParamsMap params_;
};

}  // namespace sdk
}  // namespace catena
