#pragma once
/**
 * @brief Param Interface analog to catena::Param
 * @file IParam.h
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

#include <GenericFactory.h>




namespace catena {
namespace sdk {

/**
 * Parameter interface definition
 */
class IParam {
  public:
    /**
     * @brief Factory to make concrete objects derived from IParam
     */
    using Factory = catena::patterns::GenericFactory<IParam, catena::Value::KindCase, const catena::Param&>;

    using SharedIParam = std::shared_ptr<IParam>;
    using ParamsMap = std::unordered_map<std::string, SharedIParam>;

  public:
    /**
     * @brief Default destructor
     */
    virtual ~IParam() = default;

    /**
     * @brief get the parameter's value as a catena::Value (protobuf)
     */
    virtual catena::Value& getValue(catena::Value& dst) const = 0;

    /**
     * @brief set the parameter's value from a catena::Value (protobuf)
     */
    virtual void setValue(const catena::Value& src) = 0;

    virtual void getValue(void* dst) const = 0;

    /**
     * @brief true if parameter needs to be included
     */
    virtual bool include() const = 0;

    /**
     * @brief marks the param as having been included.
     *
     * Subsequent calls to include() will return false.
     */
    virtual void included() = 0;

    virtual bool hasSubparams() const = 0;

    virtual ParamsMap& subparams() = 0;

    virtual IParam& operator=(const catena::Param& src) = 0;
};



}  // namespace sdk
}  // namespace catena