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

#include <string>

namespace catena {
namespace sdk {

/**
 * Parameter interface definition
 */
class IParam {
  public:
    /**
     * @brief Default destructor
     */
    virtual ~IParam() = default;

    /**
     * @brief Construct from a catena::Param (protobuf)
     */
    virtual IParam(const std::string& oid, const catena::Param& src) = 0;
};

class ParamCommon : public IParam {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     */
    ParamCommon(const std::string& oid, const catena::Param& src) : oid_{oid}, type_{src.type()} override {
        oid_ = oid;
        *this << src;
    }

  protected:
    std::string oid_;
    catena::ParamType type_;
};

template <typename VT> class Param : public ParamCommon {
  public:
    /**
     * @brief Construct from a catena::Param (protobuf)
     */
    Param(const std::string& oid, const catena::Param& src) override : ParamCommon(oid, src) {
        auto& getter = catena::ParamAccessor::Getter::getInstance();
        getter[src_.kind_case()](&value_, src_);
    }

  private:
    VT value_;
};

}  // namespace sdk
}  // namespace catena